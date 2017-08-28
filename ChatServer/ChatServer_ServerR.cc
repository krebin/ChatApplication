#include <memory>
#include <mutex>
#include <iostream>
#include <string>
#include <thread>
#include <ctime>
#include <vector>
#include <unordered_map>
#include <functional>
#include <unordered_set>
#include <unistd.h>
#include <atomic>

#include <grpc++/grpc++.h>
#include "chatserver.grpc.pb.h"
#include "UserNode.hpp"
#include "ChatServerGlobal.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerAsyncWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using grpc::ServerAsyncReaderWriter;
using grpc::CompletionQueue;

using chatserver::LogInRequest;
using chatserver::LogInReply;
using chatserver::LogOutRequest;
using chatserver::LogOutReply;
using chatserver::SendMessageRequest;
using chatserver::SendMessageReply;
using chatserver::ReceiveMessageRequest;
using chatserver::ReceiveMessageReply;
using chatserver::ListRequest;
using chatserver::ListReply;
using chatserver::ChatMessage;
using chatserver::ChatServer;

// Forward declaration
class ServerImpl;
static ServerImpl* gServerImpl;


bool isValid(std::string name);

/** Function to check whether or not name chosen is valid name
 * Name must contain only alphanumeric and no spaces
 * @param std::string name: string to be checked
 * @return bool: true if valid, false if not
 */
bool isValid(std::string name)
{
    for(int i = 0; i < name.length(); i++)
    {
        // letter must be A through z
        if(name[i] < ALPHA_START || name[i] > ALPHA_END)
            return false;
    }
    return true;
}

// Globals to analyze the results and make sure we are not leaking any rpcs
static std::atomic_int32_t gUnaryRpcCounter(0);
static std::atomic_int32_t gServerStreamingRpcCounter(0);
static std::atomic_int32_t gClientStreamingRpcCounter(0);
static std::atomic_int32_t gBidirectionalStreamingRpcCounter(0);


// We add a 'TagProcessor' to the completion queue for each event. This way, each tag knows how to process itself. 
using TagProcessor = std::function<void(bool)>;
struct TagInfo
{
    TagProcessor* tagProcessor; // The function to be called to process incoming event
    bool ok; // The result of tag processing as indicated by gRPC library. Calling it 'ok' to be in sync with other gRPC examples.
};

using TagList = std::list<TagInfo>;

// As the tags become available from completion queue thread, we put them in a queue in order to process them on our application thread. 
static TagList gIncomingTags;
std::mutex gIncomingTagsMutex;
// A base class for various rpc types. With gRPC, it is necessary to keep track of pending async operations.
// Only 1 async operation can be pending at a time with an exception that both async read and write can be pending at the same time.
class RpcJob
{
public:
    enum AsyncOpType
    {
        ASYNC_OP_TYPE_INVALID,
        ASYNC_OP_TYPE_QUEUED_REQUEST,
        ASYNC_OP_TYPE_READ,
        ASYNC_OP_TYPE_WRITE,
        ASYNC_OP_TYPE_FINISH
    };

    RpcJob()
        : mAsyncOpCounter(0)
        , mAsyncReadInProgress(false)
        , mAsyncWriteInProgress(false)
        , mOnDoneCalled(false)
    {

    }

    virtual ~RpcJob() {};

    void AsyncOpStarted(AsyncOpType opType)
    {
        ++mAsyncOpCounter;

        switch (opType)
        {
        case ASYNC_OP_TYPE_READ:
            mAsyncReadInProgress = true;
            break;
        case ASYNC_OP_TYPE_WRITE:
            mAsyncWriteInProgress = true;
        default: //Don't care about other ops
            break;
        }
    }

    // returns true if the rpc processing should keep going. false otherwise.
    bool AsyncOpFinished(AsyncOpType opType)
    {
        --mAsyncOpCounter;

        switch (opType)
        {
        case ASYNC_OP_TYPE_READ:
            mAsyncReadInProgress = false;
            break;
        case ASYNC_OP_TYPE_WRITE:
            mAsyncWriteInProgress = false;
        default: //Don't care about other ops
            break;
        }

        // No async operations are pending and gRPC library notified as earlier that it is done with the rpc.
        // Finish the rpc. 
        if (mAsyncOpCounter == 0 && mOnDoneCalled)
        {
            Done();
            return false;
        }

        return true;
    }

    bool AsyncOpInProgress() const
    {
        return mAsyncOpCounter != 0;
    }

    bool AsyncReadInProgress() const
    {
        return mAsyncReadInProgress;
    }

    bool AsyncWriteInProgress() const
    {
        return mAsyncWriteInProgress;
    }

    // Tag processor for the 'done' event of this rpc from gRPC library
    void OnDone(bool /*ok*/)
    {
        mOnDoneCalled = true;
        if (mAsyncOpCounter == 0)
            Done();
    }

    // Each different rpc type need to implement the specialization of action when this rpc is done.
    virtual void Done() = 0;
private:
    int32_t mAsyncOpCounter;
    bool mAsyncReadInProgress;
    bool mAsyncWriteInProgress;

    // In case of an abrupt rpc ending (for example, client process exit), gRPC calls OnDone prematurely even while an async operation is in progress
    // and would be notified later. An example sequence would be
    // 1. The client issues an rpc request. 
    // 2. The server handles the rpc and calls Finish with response. At this point, ServerContext::IsCancelled is NOT true.
    // 3. The client process abruptly exits. 
    // 4. The completion queue dispatches an OnDone tag followed by the OnFinish tag. If the application cleans up the state in OnDone, OnFinish invocation would result in undefined behavior. 
    // This actually feels like a pretty odd behavior of the gRPC library (it is most likely a result of our multi-threaded usage) so we account for that by keeping track of whether the OnDone was called earlier. 
    // As far as the application is considered, the rpc is only 'done' when no asyn Ops are pending. 
    bool mOnDoneCalled;
};

// The application code communicates with our utility classes using these handlers. 
template<typename ServiceType, typename RequestType, typename ResponseType>
struct RpcJobHandlers
{
public:
    // typedefs. See the comments below. 
    using ProcessRequestHandler = std::function<void(ServiceType*, RpcJob*, const RequestType*)>;
    using CreateRpcJobHandler = std::function<void()>;
    using RpcJobDoneHandler = std::function<void(ServiceType*, RpcJob*, bool)>;

    using SendResponseHandler = std::function<bool(const ResponseType*)>; // GRPC_TODO - change to a unique_ptr instead to avoid internal copying.
    using RpcJobContextHandler = std::function<void(ServiceType*, RpcJob*, grpc::ServerContext*, SendResponseHandler)>;

    // Job to Application code handlers/callbacks
    ProcessRequestHandler processRequestHandler; // RpcJob calls this to inform the application of a new request to be processed. 
    CreateRpcJobHandler createRpcJobHandler; // RpcJob calls this to inform the application to create a new RpcJob of this type.
    RpcJobDoneHandler rpcJobDoneHandler; // RpcJob calls this to inform the application that this job is done now. 

    // Application code to job
    RpcJobContextHandler rpcJobContextHandler; // RpcJob calls this to inform the application of the entities it can use to respond to the rpc request.
};

// Each rpc type specializes RpcJobHandlers by deriving from it as each of them have a different responder to talk back to gRPC library.
template<typename ServiceType, typename RequestType, typename ResponseType>
struct UnaryRpcJobHandlers : public RpcJobHandlers<ServiceType, RequestType, ResponseType>
{
public:
    using GRPCResponder = grpc::ServerAsyncResponseWriter<ResponseType>;
    using QueueRequestHandler = std::function<void(ServiceType*, grpc::ServerContext*, RequestType*, GRPCResponder*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void *)>;

    // Job to Application code handlers/callbacks
    QueueRequestHandler queueRequestHandler; // RpcJob calls this to inform the application to queue up a request for enabling rpc handling.
};

template<typename ServiceType, typename RequestType, typename ResponseType>
struct ServerStreamingRpcJobHandlers : public RpcJobHandlers<ServiceType, RequestType, ResponseType>
{
public:
    using GRPCResponder = grpc::ServerAsyncWriter<ResponseType>;
    using QueueRequestHandler = std::function<void(ServiceType*, grpc::ServerContext*, RequestType*, GRPCResponder*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void *)>;

    // Job to Application code handlers/callbacks
    QueueRequestHandler queueRequestHandler; // RpcJob calls this to inform the application to queue up a request for enabling rpc handling.
};

template<typename ServiceType, typename RequestType, typename ResponseType>
struct ClientStreamingRpcJobHandlers : public RpcJobHandlers<ServiceType, RequestType, ResponseType>
{
public:
    using GRPCResponder = grpc::ServerAsyncReader<ResponseType, RequestType>;
    using QueueRequestHandler = std::function<void(ServiceType*, grpc::ServerContext*, GRPCResponder*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void *)>;

    // Job to Application code handlers/callbacks
    QueueRequestHandler queueRequestHandler; // RpcJob calls this to inform the application to queue up a request for enabling rpc handling.
};

template<typename ServiceType, typename RequestType, typename ResponseType>
struct BidirectionalStreamingRpcJobHandlers : public RpcJobHandlers<ServiceType, RequestType, ResponseType>
{
public:
    using GRPCResponder = grpc::ServerAsyncReaderWriter<ResponseType, RequestType>;
    using QueueRequestHandler = std::function<void(ServiceType*, grpc::ServerContext*, GRPCResponder*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void *)>;

    // Job to Application code handlers/callbacks
    QueueRequestHandler queueRequestHandler; // RpcJob calls this to inform the application to queue up a request for enabling rpc handling.
};


/*
We implement UnaryRpcJob, ServerStreamingRpcJob, ClientStreamingRpcJob and BidirectionalStreamingRpcJob. The application deals with these classes.

As a convention, we always send grpc::Status::OK and add any error info in the google.rpc.Status member of the ResponseType field. In streaming scenarios, this allows us to indicate error
in a request to a client without completion of the rpc (and allow for more requests on same rpc). We do, however, allow server side cancellation of the rpc.
*/

template<typename ServiceType, typename RequestType, typename ResponseType>
class UnaryRpcJob : public RpcJob
{
    using ThisRpcTypeJobHandlers = UnaryRpcJobHandlers<ServiceType, RequestType, ResponseType>;

public:
    UnaryRpcJob(ServiceType* service, grpc::ServerCompletionQueue* cq, ThisRpcTypeJobHandlers jobHandlers)
        : mService(service)
        , mCQ(cq)
        , mResponder(&mServerContext)
        , mHandlers(jobHandlers)
    {
        ++gUnaryRpcCounter;

        // create TagProcessors that we'll use to interact with gRPC CompletionQueue
        mOnRead = std::bind(&UnaryRpcJob::OnRead, this, std::placeholders::_1);
        mOnFinish = std::bind(&UnaryRpcJob::OnFinish, this, std::placeholders::_1);
        mOnDone = std::bind(&RpcJob::OnDone, this, std::placeholders::_1);

        // set up the completion queue to inform us when gRPC is done with this rpc.
        mServerContext.AsyncNotifyWhenDone(&mOnDone);

        // inform the application of the entities it can use to respond to the rpc
        mSendResponse = std::bind(&UnaryRpcJob::SendResponse, this, std::placeholders::_1);
        jobHandlers.rpcJobContextHandler(mService, this, &mServerContext, mSendResponse);

        // finally, issue the async request needed by gRPC to start handling this rpc.
        AsyncOpStarted(RpcJob::ASYNC_OP_TYPE_QUEUED_REQUEST);
        mHandlers.queueRequestHandler(mService, &mServerContext, &mRequest, &mResponder, mCQ, mCQ, &mOnRead);
    }

private:

    bool SendResponse(const ResponseType* response)
    {
        // We always expect a valid response for Unary rpc. If no response is available, use ServerContext::TryCancel.
        GPR_ASSERT(response);
        if (response == nullptr)
            return false;

        mResponse = *response;

        AsyncOpStarted(RpcJob::ASYNC_OP_TYPE_FINISH);
        mResponder.Finish(mResponse, grpc::Status::OK, &mOnFinish);

        return true;
    }

    void OnRead(bool ok)
    {
        // A request has come on the service which can now be handled. Create a new rpc of this type to allow the server to handle next request.
        mHandlers.createRpcJobHandler();

        if (AsyncOpFinished(RpcJob::ASYNC_OP_TYPE_QUEUED_REQUEST))
        {
            if (ok)
            {
                // We have a request that can be responded to now. So process it. 
                mHandlers.processRequestHandler(mService, this, &mRequest);
            }
            else
            {
                GPR_ASSERT(ok);
            }
        }
    }

    void OnFinish(bool ok)
    {
        AsyncOpFinished(RpcJob::ASYNC_OP_TYPE_FINISH);
    }

    void Done() override
    {
        mHandlers.rpcJobDoneHandler(mService, this, mServerContext.IsCancelled());

        --gUnaryRpcCounter;
    }

private:

    ServiceType* mService;
    grpc::ServerCompletionQueue* mCQ;
    typename ThisRpcTypeJobHandlers::GRPCResponder mResponder;
    grpc::ServerContext mServerContext;

    RequestType mRequest;
    ResponseType mResponse;

    ThisRpcTypeJobHandlers mHandlers;

    typename ThisRpcTypeJobHandlers::SendResponseHandler mSendResponse;

    TagProcessor mOnRead;
    TagProcessor mOnFinish;
    TagProcessor mOnDone;
};



template<typename ServiceType, typename RequestType, typename ResponseType>
class ServerStreamingRpcJob : public RpcJob
{
    using ThisRpcTypeJobHandlers = ServerStreamingRpcJobHandlers<ServiceType, RequestType, ResponseType>;

public:
    ServerStreamingRpcJob(ServiceType* service, grpc::ServerCompletionQueue* cq, ThisRpcTypeJobHandlers jobHandlers)
        : mService(service)
        , mCQ(cq)
        , mResponder(&mServerContext)
        , mHandlers(jobHandlers)
        , mServerStreamingDone(false)
    {
        ++gServerStreamingRpcCounter;

        // create TagProcessors that we'll use to interact with gRPC CompletionQueue
        mOnRead = std::bind(&ServerStreamingRpcJob::OnRead, this, std::placeholders::_1);
        mOnWrite = std::bind(&ServerStreamingRpcJob::OnWrite, this, std::placeholders::_1);
        mOnFinish = std::bind(&ServerStreamingRpcJob::OnFinish, this, std::placeholders::_1);
        mOnDone = std::bind(&RpcJob::OnDone, this, std::placeholders::_1);

        // set up the completion queue to inform us when gRPC is done with this rpc.
        mServerContext.AsyncNotifyWhenDone(&mOnDone);

        //inform the application of the entities it can use to respond to the rpc
        mSendResponse = std::bind(&ServerStreamingRpcJob::SendResponse, this, std::placeholders::_1);
        jobHandlers.rpcJobContextHandler(mService, this, &mServerContext, mSendResponse);

        // finally, issue the async request needed by gRPC to start handling this rpc.
        AsyncOpStarted(RpcJob::ASYNC_OP_TYPE_QUEUED_REQUEST);
        mHandlers.queueRequestHandler(mService, &mServerContext, &mRequest, &mResponder, mCQ, mCQ, &mOnRead);
    }

private:

    // gRPC can only do one async write at a time but that is very inconvenient from the application point of view.
    // So we buffer the response below in a queue if gRPC lib is not ready for it. 
    // The application can send a null response in order to indicate the completion of server side streaming. 
    bool SendResponse(const ResponseType* response)
    {
        if (response != nullptr)
        {
            mResponseQueue.push_back(*response);

            if (!AsyncWriteInProgress())
            {
                doSendResponse();
            }
        }
        else
        {
            mServerStreamingDone = true;

            if (!AsyncWriteInProgress())
            {
                doFinish();
            }
        }

        return true;
    }

    void doSendResponse()
    {
        AsyncOpStarted(RpcJob::ASYNC_OP_TYPE_WRITE);
        mResponder.Write(mResponseQueue.front(), &mOnWrite);
    }

    void doFinish()
    {
        AsyncOpStarted(RpcJob::ASYNC_OP_TYPE_FINISH);
        mResponder.Finish(grpc::Status::OK, &mOnFinish);
    }

    void OnRead(bool ok)
    {
        mHandlers.createRpcJobHandler();

        if (AsyncOpFinished(RpcJob::ASYNC_OP_TYPE_QUEUED_REQUEST))
        {
            if (ok)
            {
                mHandlers.processRequestHandler(mService, this, &mRequest);
            }
        }
    }

    void OnWrite(bool ok)
    {
        if (AsyncOpFinished(RpcJob::ASYNC_OP_TYPE_WRITE))
        {
            // Get rid of the message that just finished.
            mResponseQueue.pop_front();

            if (ok)
            {
                if (!mResponseQueue.empty()) // If we have more messages waiting to be sent, send them.
                {
                    doSendResponse();
                }
                else if (mServerStreamingDone) // Previous write completed and we did not have any pending write. If the application has finished streaming responses, finish the rpc processing.
                {
                    doFinish();
                }
            }
        }
    }

    void OnFinish(bool ok)
    {
        AsyncOpFinished(RpcJob::ASYNC_OP_TYPE_FINISH);
    }

    void Done() override
    {
        mHandlers.rpcJobDoneHandler(mService, this, mServerContext.IsCancelled());

        --gServerStreamingRpcCounter;
    }

private:

    ServiceType* mService;
    grpc::ServerCompletionQueue* mCQ;
    typename ThisRpcTypeJobHandlers::GRPCResponder mResponder;
    grpc::ServerContext mServerContext;

    RequestType mRequest;
    
    ThisRpcTypeJobHandlers mHandlers;

    typename ThisRpcTypeJobHandlers::SendResponseHandler mSendResponse;

    TagProcessor mOnRead;
    TagProcessor mOnWrite;
    TagProcessor mOnFinish;
    TagProcessor mOnDone;

    std::list<ResponseType> mResponseQueue;
    bool mServerStreamingDone;
};


template<typename ServiceType, typename RequestType, typename ResponseType>
class ClientStreamingRpcJob : public RpcJob
{
    using ThisRpcTypeJobHandlers = ClientStreamingRpcJobHandlers<ServiceType, RequestType, ResponseType>;

public:
    ClientStreamingRpcJob(ServiceType* service, grpc::ServerCompletionQueue* cq, ThisRpcTypeJobHandlers jobHandlers)
        : mService(service)
        , mCQ(cq)
        , mResponder(&mServerContext)
        , mHandlers(jobHandlers)
        , mClientStreamingDone(false)
    {
        ++gClientStreamingRpcCounter;

        // create TagProcessors that we'll use to interact with gRPC CompletionQueue
        mOnInit = std::bind(&ClientStreamingRpcJob::OnInit, this, std::placeholders::_1);
        mOnRead = std::bind(&ClientStreamingRpcJob::OnRead, this, std::placeholders::_1);
        mOnFinish = std::bind(&ClientStreamingRpcJob::OnFinish, this, std::placeholders::_1);
        mOnDone = std::bind(&RpcJob::OnDone, this, std::placeholders::_1);

        // set up the completion queue to inform us when gRPC is done with this rpc.
        mServerContext.AsyncNotifyWhenDone(&mOnDone);

        //inform the application of the entities it can use to respond to the rpc
        mSendResponse = std::bind(&ClientStreamingRpcJob::SendResponse, this, std::placeholders::_1);
        jobHandlers.rpcJobContextHandler(mService, this, &mServerContext, mSendResponse);

        // finally, issue the async request needed by gRPC to start handling this rpc.
        AsyncOpStarted(RpcJob::ASYNC_OP_TYPE_QUEUED_REQUEST);
        mHandlers.queueRequestHandler(mService, &mServerContext, &mResponder, mCQ, mCQ, &mOnInit);
    }

private:

    bool SendResponse(const ResponseType* response)
    {
        // We always expect a valid response for client streaming rpc. If no response is available, use ServerContext::TryCancel.
        GPR_ASSERT(response);
        if (response == nullptr)
            return false;

        if (!mClientStreamingDone)
        {
            // It does not make sense to send a response before client has streamed all the requests. Supporting this behavior could lead to writing error-prone
            // code so it is specifically disallowed. If you need to error out before client can stream all the requests, use ServerContext::TryCancel.
            GPR_ASSERT(false);
            return false;
        }

        mResponse = *response;

        AsyncOpStarted(RpcJob::ASYNC_OP_TYPE_FINISH);
        mResponder.Finish(mResponse, grpc::Status::OK, &mOnFinish);

        return true;
    }

    void OnInit(bool ok)
    {
        mHandlers.createRpcJobHandler();

        if (AsyncOpFinished(RpcJob::ASYNC_OP_TYPE_QUEUED_REQUEST))
        {
            if (ok)
            {
                AsyncOpStarted(RpcJob::ASYNC_OP_TYPE_READ);
                mResponder.Read(&mRequest, &mOnRead);
            }
        }
    }

    void OnRead(bool ok)
    {
        if (AsyncOpFinished(RpcJob::ASYNC_OP_TYPE_READ))
        {
            if (ok)
            {
                // inform application that a new request has come in
                mHandlers.processRequestHandler(mService, this, &mRequest);

                // queue up another read operation for this rpc
                AsyncOpStarted(RpcJob::ASYNC_OP_TYPE_READ);
                mResponder.Read(&mRequest, &mOnRead);
            }
            else
            {
                mClientStreamingDone = true;
                mHandlers.processRequestHandler(mService, this, nullptr);
            }
        }
    }

    void OnFinish(bool ok)
    {
        AsyncOpFinished(RpcJob::ASYNC_OP_TYPE_FINISH);
    }

    void Done() override
    {
        mHandlers.rpcJobDoneHandler(mService, this, mServerContext.IsCancelled());

        --gClientStreamingRpcCounter;
    }

private:

    ServiceType* mService;
    grpc::ServerCompletionQueue* mCQ;
    typename ThisRpcTypeJobHandlers::GRPCResponder mResponder;
    grpc::ServerContext mServerContext;

    RequestType mRequest;
    ResponseType mResponse;

    ThisRpcTypeJobHandlers mHandlers;

    typename ThisRpcTypeJobHandlers::SendResponseHandler mSendResponse;

    TagProcessor mOnInit;
    TagProcessor mOnRead;
    TagProcessor mOnFinish;
    TagProcessor mOnDone;

    bool mClientStreamingDone;
};



template<typename ServiceType, typename RequestType, typename ResponseType>
class BidirectionalStreamingRpcJob : public RpcJob
{
    using ThisRpcTypeJobHandlers = BidirectionalStreamingRpcJobHandlers<ServiceType, RequestType, ResponseType>;

public:
    BidirectionalStreamingRpcJob(ServiceType* service, grpc::ServerCompletionQueue* cq, ThisRpcTypeJobHandlers jobHandlers)
        : mService(service)
        , mCQ(cq)
        , mResponder(&mServerContext)
        , mHandlers(jobHandlers)
        , mServerStreamingDone(false)
        , mClientStreamingDone(false)
    {
        ++gBidirectionalStreamingRpcCounter;

        // create TagProcessors that we'll use to interact with gRPC CompletionQueue
        mOnInit = std::bind(&BidirectionalStreamingRpcJob::OnInit, this, std::placeholders::_1);
        mOnRead = std::bind(&BidirectionalStreamingRpcJob::OnRead, this, std::placeholders::_1);
        mOnWrite = std::bind(&BidirectionalStreamingRpcJob::OnWrite, this, std::placeholders::_1);
        mOnFinish = std::bind(&BidirectionalStreamingRpcJob::OnFinish, this, std::placeholders::_1);
        mOnDone = std::bind(&RpcJob::OnDone, this, std::placeholders::_1);

        // set up the completion queue to inform us when gRPC is done with this rpc.
        mServerContext.AsyncNotifyWhenDone(&mOnDone);

        //inform the application of the entities it can use to respond to the rpc
        mSendResponse = std::bind(&BidirectionalStreamingRpcJob::SendResponse, this, std::placeholders::_1);
        jobHandlers.rpcJobContextHandler(mService, this, &mServerContext, mSendResponse);

        // finally, issue the async request needed by gRPC to start handling this rpc.
        AsyncOpStarted(RpcJob::ASYNC_OP_TYPE_QUEUED_REQUEST);
        mHandlers.queueRequestHandler(mService, &mServerContext, &mResponder, mCQ, mCQ, &mOnInit);
    }

private:

    bool SendResponse(const ResponseType* response)
    {
        if (response == nullptr && !mClientStreamingDone)
        {
            // Wait for client to finish the all the requests. If you want to cancel, use ServerContext::TryCancel. 
            GPR_ASSERT(false);
            return false;
        }

        if (response != nullptr)
        {
            mResponseQueue.push_back(*response); // We need to make a copy of the response because we need to maintain it until we get a completion notification. 

            if (!AsyncWriteInProgress())
            {
                doSendResponse();
            }
        }
        else
        {
            std::cout << "Server streaming done\n";
            mServerStreamingDone = true;

            if (!AsyncWriteInProgress()) // Kick the async op if our state machine is not going to be kicked from the completion queue
            {
                doFinish();
            }
        }

        return true;
    }

    void OnInit(bool ok)
    {
        mHandlers.createRpcJobHandler();

        if (AsyncOpFinished(RpcJob::ASYNC_OP_TYPE_QUEUED_REQUEST))
        {
            if (ok)
            {
                AsyncOpStarted(RpcJob::ASYNC_OP_TYPE_READ);
                mResponder.Read(&mRequest, &mOnRead);
            }
        }
    }

    void OnRead(bool ok)
    {
        if (AsyncOpFinished(RpcJob::ASYNC_OP_TYPE_READ))
        {
            std::cout << "On read\n";
            if (ok)
            {
                std::cout << "On read ok\n";
                mHandlers.processRequestHandler(mService, this, &mRequest);
                // queue up another read operation for this rpc
                AsyncOpStarted(RpcJob::ASYNC_OP_TYPE_READ);
                mResponder.Read(&mRequest, &mOnRead);
            }
            else
            {
                std::cout << "On read not ok\n";
                std::cout << "Client Streaming Done\n";
                mClientStreamingDone = true;
                mHandlers.processRequestHandler(mService, this, nullptr);
            }
        }
    }

    void OnWrite(bool ok)
    {
        if (AsyncOpFinished(RpcJob::ASYNC_OP_TYPE_WRITE))
        {
            // Get rid of the message that just finished. 
            mResponseQueue.pop_front();
            if (ok)
            {
                if (!mResponseQueue.empty()) // If we have more messages waiting to be sent, send them.
                {
                    doSendResponse();
                }
                else if (mServerStreamingDone) // Previous write completed and we did not have any pending write. If the application indicated a done operation, finish the rpc processing.
                {
                    doFinish();
                }
            }
        }
    }

    void OnFinish(bool ok)
    {
        AsyncOpFinished(RpcJob::ASYNC_OP_TYPE_FINISH);
    }

    void Done() override
    {
        mHandlers.rpcJobDoneHandler(mService, this, mServerContext.IsCancelled());

        --gBidirectionalStreamingRpcCounter;
    }

    void doSendResponse()
    {
        AsyncOpStarted(RpcJob::ASYNC_OP_TYPE_WRITE);
        mResponder.Write(mResponseQueue.front(), &mOnWrite);
    }

    void doFinish()
    {
        AsyncOpStarted(RpcJob::ASYNC_OP_TYPE_FINISH);
        mResponder.Finish(grpc::Status::OK, &mOnFinish);
    }

private:

    ServiceType* mService;
    grpc::ServerCompletionQueue* mCQ;
    typename ThisRpcTypeJobHandlers::GRPCResponder mResponder;
    grpc::ServerContext mServerContext;

    RequestType mRequest;

    ThisRpcTypeJobHandlers mHandlers;

    typename ThisRpcTypeJobHandlers::SendResponseHandler mSendResponse;

    TagProcessor mOnInit;
    TagProcessor mOnRead;
    TagProcessor mOnWrite;
    TagProcessor mOnFinish;
    TagProcessor mOnDone;


    std::list<ResponseType> mResponseQueue;
    bool mServerStreamingDone;
    bool mClientStreamingDone;
};





class ServerImpl final : public ChatServer::AsyncService
{
    public:
    	ServerImpl(){}

	    ~ServerImpl()
	    {
	        mServer->Shutdown();
	        mCQ->Shutdown();
	    }


        // There is no shutdown handling in this code.
        void Run() 
        {
            std::string server_address("0.0.0.0:50051");

            grpc::ServerBuilder builder;
            // Listen on the given address without any authentication mechanism.
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            // Register "service_" as the instance through which we'll communicate with
            // clients. In this case it corresponds to an *asynchronous* service.
            builder.RegisterService(&mChatServerService);
            // Get hold of the completion queue used for the asynchronous communication
            // with the gRPC runtime.
            mCQ = builder.AddCompletionQueue();
            // Finally assemble the server.
            mServer = builder.BuildAndStart();
            std::cout << "Server listening on " << server_address << std::endl;

            // Proceed to the server's main loop.
            HandleRpcs();
        }


    private:

        void createLogOutRpc()
        {
            UnaryRpcJobHandlers<chatserver::ChatServer::AsyncService, LogOutRequest, LogOutReply> jobHandlers;
            jobHandlers.rpcJobContextHandler = &LogOutContextSetterImpl;
            jobHandlers.rpcJobDoneHandler = &LogOutDone;
            jobHandlers.createRpcJobHandler = std::bind(&ServerImpl::createLogOutRpc, this);
            jobHandlers.queueRequestHandler = &chatserver::ChatServer::AsyncService::RequestLogOut;
            jobHandlers.processRequestHandler = &LogOutProcessor;

            new UnaryRpcJob<chatserver::ChatServer::AsyncService, LogOutRequest, LogOutReply>(&mChatServerService, mCQ.get(), jobHandlers);
        }
    
        struct LogOutResponder
        {
            std::function<bool(chatserver::LogOutReply*)> sendFunc;
            grpc::ServerContext* serverContext;
        };

        std::unordered_map<RpcJob*, LogOutResponder> mLogOutResponders;
        static void LogOutContextSetterImpl(chatserver::ChatServer::AsyncService* service, RpcJob* job, ServerContext* serverContext, std::function<bool(LogOutReply*)> sendResponse)
        {
            LogOutResponder responder;
            responder.sendFunc = sendResponse;
            responder.serverContext = serverContext;

            gServerImpl->mLogOutResponders[job] = responder;
        }

        static void LogOutProcessor(chatserver::ChatServer::AsyncService* service, RpcJob* job, const chatserver::LogOutRequest* request)
        {
            // Get user's name
            std::string name = request->user();
            LogOutReply reply;
            reply.set_confirmation(LOG_OUT_CONFIRM);

            // Set UserNode's online status to false
            gServerImpl->users_[name]->setOnline(false);
            // Send back reply
            gServerImpl->mLogOutResponders[job].sendFunc(&reply);
        }

        static void LogOutDone(chatserver::ChatServer::AsyncService* service, RpcJob* job, bool rpcCancelled)
        {
            gServerImpl->mLogOutResponders.erase(job);
            delete job;
        }

        void createListRpc()
        {
            UnaryRpcJobHandlers<chatserver::ChatServer::AsyncService, ListRequest, ListReply> jobHandlers;
            jobHandlers.rpcJobContextHandler = &ListContextSetterImpl;
            jobHandlers.rpcJobDoneHandler = &ListDone;
            jobHandlers.createRpcJobHandler = std::bind(&ServerImpl::createListRpc, this);
            jobHandlers.queueRequestHandler = &chatserver::ChatServer::AsyncService::RequestList;
            jobHandlers.processRequestHandler = &ListProcessor;

            new UnaryRpcJob<chatserver::ChatServer::AsyncService, ListRequest, ListReply>(&mChatServerService, mCQ.get(), jobHandlers);
        }
    
        struct ListResponder
        {
            std::function<bool(chatserver::ListReply*)> sendFunc;
            grpc::ServerContext* serverContext;
        };

        std::unordered_map<RpcJob*, ListResponder> mListResponders;
        static void ListContextSetterImpl(chatserver::ChatServer::AsyncService* service, RpcJob* job, ServerContext* serverContext, std::function<bool(ListReply*)> sendResponse)
        {
            ListResponder responder;
            responder.sendFunc = sendResponse;
            responder.serverContext = serverContext;

            gServerImpl->mListResponders[job] = responder;
        }

        static void ListProcessor(chatserver::ChatServer::AsyncService* service, RpcJob* job, const chatserver::ListRequest* request)
        {
            std::string list;
            ListReply reply;
            auto it = gServerImpl->users_.begin();

            // Iterate through all existing users
            for(auto it = gServerImpl->users_.begin()
              ; it != gServerImpl->users_.end()
              ; it++)
            {
                // If user currently online
                if(it->second->getOnline())
                {
                    // Format list of users
                    list += ("[" + it->second->getName() + "] ");
                }
                
            }
            list += "\n\n";
            reply.set_list(list);

            gServerImpl->mListResponders[job].sendFunc(&reply);
        }

        static void ListDone(chatserver::ChatServer::AsyncService* service, RpcJob* job, bool rpcCancelled)
        {
            gServerImpl->mListResponders.erase(job);
            delete job;
        }

        /** Update job handlers for ReceiveMessageRPC
         */
        void createReceiveMessageRpc()
        {
            ServerStreamingRpcJobHandlers<chatserver::ChatServer::AsyncService, ReceiveMessageRequest, ReceiveMessageReply> jobHandlers;
            jobHandlers.rpcJobContextHandler = &ReceiveMessageContextSetterImpl;
            jobHandlers.rpcJobDoneHandler = &ReceiveMessageDone;
            jobHandlers.createRpcJobHandler = std::bind(&ServerImpl::createReceiveMessageRpc, this);
            jobHandlers.queueRequestHandler = &chatserver::ChatServer::AsyncService::RequestReceiveMessage;
            jobHandlers.processRequestHandler = &ReceiveMessageProcessor;

            // Server sends multiple messages back, server streaming
            new ServerStreamingRpcJob<chatserver::ChatServer::AsyncService, ReceiveMessageRequest, ReceiveMessageReply>(&mChatServerService, mCQ.get(), jobHandlers);
        }

        struct ReceiveMessageResponder
        {
            std::function<bool(ReceiveMessageReply*)> sendFunc;
            grpc::ServerContext* serverContext;
        };

        std::unordered_map<RpcJob*, ReceiveMessageResponder> mReceiveMessageResponders;
        static void ReceiveMessageContextSetterImpl(chatserver::ChatServer::AsyncService* service, RpcJob* job, grpc::ServerContext* serverContext, std::function<bool(ReceiveMessageReply*)> sendResponse)
        {
            ReceiveMessageResponder responder;
            responder.sendFunc = sendResponse;
            responder.serverContext = serverContext;

            gServerImpl->mReceiveMessageResponders[job] = responder;
        }

        static void ReceiveMessageProcessor(chatserver::ChatServer::AsyncService* service, RpcJob* job, const ReceiveMessageRequest *request)
        {
            ReceiveMessageReply reply;
            if(request)
            {
                
                // Obtain user's name
                std::string name = request->user();
                // Access UserNode from map
                UserNode* user = gServerImpl->users_[name];
                // Get pair of message queue state and message
                auto pair = user->getMessage();               

                // Update proto fields depending on state of queue
                if(pair.first == UserNode::QUEUE_STATE::EMPTY) 
                {
                    reply.set_queuestate(chatserver::ReceiveMessageReply::EMPTY);
                    gServerImpl->mReceiveMessageResponders[job].sendFunc(nullptr);
                }
                else if(pair.first == UserNode::QUEUE_STATE::NON_EMPTY)
                {
                    reply.set_queuestate(chatserver::ReceiveMessageReply::NON_EMPTY);
                    reply.set_messages(pair.second); 
                    gServerImpl->mReceiveMessageResponders[job].sendFunc(&reply);
                }


                while(reply.queuestate() == chatserver::ReceiveMessageReply::NON_EMPTY)
                {
                    pair = user->getMessage();
                    if(pair.first == UserNode::QUEUE_STATE::EMPTY) 
                    {
                        reply.set_queuestate(chatserver::ReceiveMessageReply::EMPTY);
                        reply.set_messages(pair.second);            
                        gServerImpl->mReceiveMessageResponders[job].sendFunc(&reply);
                        gServerImpl->mReceiveMessageResponders[job].sendFunc(nullptr);
                    }
                    else if(pair.first == UserNode::QUEUE_STATE::NON_EMPTY)
                    {
                        reply.set_queuestate(chatserver::ReceiveMessageReply::NON_EMPTY);
                        reply.set_messages(pair.second);            
                        gServerImpl->mReceiveMessageResponders[job].sendFunc(&reply);

                    }         
                }
            }
            else
            {
                gServerImpl->mReceiveMessageResponders[job].sendFunc(nullptr);
            }
        }

        static void ReceiveMessageDone(chatserver::ChatServer::AsyncService* service, RpcJob* job, bool rpcCancelled)
        {
            gServerImpl->mReceiveMessageResponders.erase(job);
            delete job;
        }

        void createSendMessageRpc()
        {

            BidirectionalStreamingRpcJobHandlers<chatserver::ChatServer::AsyncService, SendMessageRequest, SendMessageReply> jobHandlers;
            jobHandlers.rpcJobContextHandler = &SendMessageContextSetterImpl;
            jobHandlers.rpcJobDoneHandler = &SendMessageDone;
            jobHandlers.createRpcJobHandler = std::bind(&ServerImpl::createSendMessageRpc, this);
            jobHandlers.queueRequestHandler = &chatserver::ChatServer::AsyncService::RequestSendMessage;
            jobHandlers.processRequestHandler = &SendMessageProcessor;

            new BidirectionalStreamingRpcJob<chatserver::ChatServer::AsyncService, SendMessageRequest, SendMessageReply>(&mChatServerService, mCQ.get(), jobHandlers);
        }
    
        struct SendMessageResponder
        {
            std::function<bool(chatserver::SendMessageReply*)> sendFunc;
            grpc::ServerContext* serverContext;
        };

        std::unordered_map<RpcJob*, SendMessageResponder> mSendMessageResponders;
        static void SendMessageContextSetterImpl(chatserver::ChatServer::AsyncService* service, RpcJob* job, ServerContext* serverContext, std::function<bool(SendMessageReply*)> sendResponse)
        {
            SendMessageResponder responder;
            responder.sendFunc = sendResponse;
            responder.serverContext = serverContext;

            gServerImpl->mSendMessageResponders[job] = responder;
        }

        static void SendMessageProcessor(chatserver::ChatServer::AsyncService* service, RpcJob* job, const chatserver::SendMessageRequest* request)
        {
            chatserver::SendMessageReply reply;
            if(request)
            {
                if(request->requeststate() == chatserver::SendMessageRequest::INITIAL)
                {
                    auto recipientIterator = gServerImpl->users_.find(request->recipient());
                    // Check for existing user
                    if(recipientIterator != gServerImpl->users_.end())
                    {
                        reply.set_recipientstate(chatserver::SendMessageReply::EXIST);
                    }
                    else
                    {
                        reply.set_recipientstate(chatserver::SendMessageReply::NO_EXIST);
                        reply.set_confirmation(SEND_MESSAGE_NO_EXIST);
                    }

                }
                else
                {
                    // Access recipient UserNode from map
                    auto recipientIterator = gServerImpl->users_.find(request->recipient());
                    // Check for existing user
                    if(recipientIterator != gServerImpl->users_.end())
                    {
                        auto recipient = recipientIterator->second;
                        auto name = request->user();
                        auto message = request->messages();

                        time_t now = time(0);
                        tm *gmtm = gmtime(&now);
                        char* dt = asctime(gmtm);

                        // Queue message
                        recipientIterator->second->addMessage("Message from " + name + ": " + message);

                        // Set fields
                        reply.set_confirmation(SEND_MESSAGE_CONFIRM
                                             + request->recipient()
                                             + "\n\n");
                    }
                }
                gServerImpl->mSendMessageResponders[job].sendFunc(&reply);
            }
            else
            {
                gServerImpl->mSendMessageResponders[job].sendFunc(nullptr);
            }
        }

        static void SendMessageDone(chatserver::ChatServer::AsyncService* service, RpcJob* job, bool rpcCancelled)
        {
            gServerImpl->mLogInResponders.erase(job);
            delete job;
        }

        /** Create a BidirectionStreamingRpcJob with Chat RPC specifications
         */
        void createChatRpc()
        {
            BidirectionalStreamingRpcJobHandlers<chatserver::ChatServer::AsyncService, ChatMessage, ChatMessage> jobHandlers;
            jobHandlers.rpcJobContextHandler = &ChatContextSetterImpl;
            jobHandlers.rpcJobDoneHandler = &ChatDone;
            jobHandlers.createRpcJobHandler = std::bind(&ServerImpl::createChatRpc, this);
            jobHandlers.queueRequestHandler = &chatserver::ChatServer::AsyncService::RequestChat;
            jobHandlers.processRequestHandler = &ChatProcessor;

            // Spawn the job to be used later
            new BidirectionalStreamingRpcJob<chatserver::ChatServer::AsyncService, ChatMessage, ChatMessage>(&mChatServerService, mCQ.get(), jobHandlers);
        }

        /** Struct to hold sending function
         */
        struct ChatResponder
        {
            std::function<bool(chatserver::ChatMessage*)> sendFunc;
            grpc::ServerContext* serverContext;
        };

        // Usage shown in ChatProcessor();
        ChatResponder *newResponder;
        // Map to responders
        std::unordered_map<RpcJob*, ChatResponder*> mChatResponders;

        /** Sets up responders for Chat RPC
         * @param AsyncService* service:
         * @param RpcJob* job: current RPC
         * @param sendResponse: Function that defines how to send the message
         */
        static void ChatContextSetterImpl(chatserver::ChatServer::AsyncService* service, RpcJob* job
                                        , ServerContext* serverContext
                                        , std::function<bool(ChatMessage*)> 
                                                             sendResponse)
        {
            // Responder object
            ChatResponder *responder = new ChatResponder();
            // Assign function
            responder->sendFunc = sendResponse;
            // Assign context
            responder->serverContext = serverContext;

            // New responder is not assigned to any client
            // usage of this shown in ChatProcessor()
            gServerImpl->newResponder = responder;
            gServerImpl->mChatResponders[job] = responder;
        }


    
        /** Processor for Chat RPC
         * @param AsyncService* service:
         * @param RpcJob* job: current rpc request is coming from
         * @param const ChatMessage* note: pointer to message that must be sent
         */
        static void ChatProcessor(chatserver::ChatServer::AsyncService* service, RpcJob* job, const ChatMessage* note)
        {
            if (note)
            {
                // Copy note
                ChatMessage responseNote(*note);
                auto responders = gServerImpl->mChatResponders;
                // Single new responder that is spawned every time CHAT
                // RPC is request by client
                auto unassignedResponder = gServerImpl->newResponder;

                if(responseNote.messages() != DONE)
                {
              
                    // Iterate through every responder currently on the chat 
                    for(auto it = responders.begin()
                      ; it != responders.end()
                      ; it++)
                    {
                        auto currJobResponder = it->second;

                        // Ignore the unassigned responder, will segfault
                        // Ignore the current job responder because don't need
                        // to send self messages
                        if((currJobResponder != unassignedResponder) 
                        &&  currJobResponder != responders[job])
                        {
                            // Send note
                            it->second->sendFunc(&responseNote);
                        }
                    }
                }
            }
            else
            {
                gServerImpl->mChatResponders[job]->sendFunc(nullptr);
            }
        }
   

        /** Deallocate memory taken by Chat RPC instances
         */
        static void ChatDone(chatserver::ChatServer::AsyncService* service, RpcJob* job, bool rpcCancelled)
        {
            // Deallocate dynamic responder
            delete gServerImpl->mChatResponders[job];
            // Remove responder from map
            gServerImpl->mChatResponders.erase(job);
            // Delete rpc instance
            delete job;
        }

        /** Create a BidirectionStreamingRpcJob with LogIn RPC specifications
         */
        void createLogInRpc()
        {
            BidirectionalStreamingRpcJobHandlers<chatserver::ChatServer::AsyncService, LogInRequest, LogInReply> jobHandlers;
            jobHandlers.rpcJobContextHandler = &LogInContextSetterImpl;
            jobHandlers.rpcJobDoneHandler = &LogInDone;
            jobHandlers.createRpcJobHandler = std::bind(&ServerImpl::createLogInRpc, this);
            jobHandlers.queueRequestHandler = &chatserver::ChatServer::AsyncService::RequestLogIn;
            jobHandlers.processRequestHandler = &LogInProcessor;

            // Spawn the job to be used later
            new BidirectionalStreamingRpcJob<chatserver::ChatServer::AsyncService, LogInRequest, LogInReply>(&mChatServerService, mCQ.get(), jobHandlers);
        }

        /** Struct to hold sending function
         */
        struct LogInResponder
        {
            std::function<bool(chatserver::LogInReply*)> sendFunc;
            grpc::ServerContext* serverContext;
        };

        // Map to responders
        std::unordered_map<RpcJob*, LogInResponder> mLogInResponders;

        /** Sets up responders for LogIn RPC
         * @param AsyncService* service:
         * @param RpcJob* job: current RPC
         * @param sendResponse: Function that defines how to send the message
         */
        static void LogInContextSetterImpl(chatserver::ChatServer::AsyncService* service, RpcJob* job
                                        , ServerContext* serverContext
                                        , std::function<bool(LogInReply*)> 
                                                             sendResponse)
        {
            // Responder object
            LogInResponder responder;
            // Assign function
            responder.sendFunc = sendResponse;
            // Assign context
            responder.serverContext = serverContext;

            gServerImpl->mLogInResponders[job] = responder;
        }
    
        /** Processor for Chat RPC
         * @param AsyncService* service:
         * @param RpcJob* job: current rpc request is coming from
         * @param const ChatMessage* note: pointer to message that must be sent
         */
        static void LogInProcessor(chatserver::ChatServer::AsyncService* service, RpcJob* job, const LogInRequest* request)
        {
            LogInReply reply;
            if(request)
            {
                auto user = request->user();
                // User's desired name is not valid
                if(!isValid(user))
                {
                    std::cout << "INVALID\n";
                    reply.set_loginstate(chatserver::LogInReply::INVALID);
                }
                // User's name has been used before
                else if(gServerImpl->users_.find(user) != gServerImpl->users_.end())
                {
                    // Someone is currently online with that name
                    if(gServerImpl->users_[user]->getOnline())
                    {
                        std::cout << "ALREADY\n";
                        reply.set_loginstate(chatserver::LogInReply::ALREADY);                        
                    }
                    // No one is currently online with that name
                    else
                    {
                        std::cout << "SUCCESS\n";
                        reply.set_loginstate(chatserver::LogInReply::SUCCESS);               
                    }
                }
                // User's desired name has never before been used
                else
                {
                    std::cout <<"SUCCESS NEW\n";
                    // Create new user
                    auto newNode = new UserNode(user);
                    newNode->setOnline(true);
                    gServerImpl->users_[user] = newNode;
                    reply.set_loginstate(chatserver::LogInReply::SUCCESS);
                }
                gServerImpl->mLogInResponders[job].sendFunc(&reply);
            }
            else
            {
                gServerImpl->mLogInResponders[job].sendFunc(nullptr);
            }
        }
   

        /** Deallocate memory taken by Chat RPC instances
         */
        static void LogInDone(chatserver::ChatServer::AsyncService* service, RpcJob* job, bool rpcCancelled)
        {
            // Deallocate dynamic responder
            delete gServerImpl->mChatResponders[job];
            // Remove responder from map
            gServerImpl->mChatResponders.erase(job);
            // Delete rpc instance
            delete job;
        }




        /** Initialize RPC services, push RPC's for processRpcs() to handle
         */
        void HandleRpcs() 
        {

            createSendMessageRpc();
            createReceiveMessageRpc();
            createChatRpc();
            createLogInRpc();
            createLogOutRpc();
            createListRpc();

            TagInfo tagInfo;
            while (true) 
            {
                // Block waiting to read the next event from the completion queue. The
                // event is uniquely identified by its the memory address
                // The return value of Next should always be checked. This return value
                // tells us whether there is any kind of event or cq_ is shutting down.
                GPR_ASSERT(mCQ->Next((void**)&tagInfo.tagProcessor, &tagInfo.ok)); //GRPC_TODO - Handle returned value
            
                gIncomingTagsMutex.lock();
                gIncomingTags.push_back(tagInfo);
                gIncomingTagsMutex.unlock();
            }
        }

        std::unique_ptr<ServerCompletionQueue> mCQ;
        chatserver::ChatServer::AsyncService mChatServerService;
        std::unique_ptr<Server> mServer;
        std::unordered_map<std::string, UserNode*> users_; 

};

/** Loop to process tags, should be run in separate thread
 */
static void processRpcs()
{
    // Implement a busy-wait loop. Not the most efficient thing in the world but but would do for this example
    while (true)
    {
        gIncomingTagsMutex.lock();
        TagList tags = std::move(gIncomingTags);
        gIncomingTagsMutex.unlock();

        while (!tags.empty())
        {
            TagInfo tagInfo = tags.front();
            tags.pop_front();
            (*(tagInfo.tagProcessor))(tagInfo.ok);
        };
    }
}


int main(int argc, char** argv) {
  std::thread processorThread(processRpcs);

  ServerImpl server;
  gServerImpl = &server;
  server.Run();

  return 0;
}


