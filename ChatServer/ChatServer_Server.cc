#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

#include <grpc++/grpc++.h>
#include "chatserver.grpc.pb.h"
#include "LinkedListChatServer.hpp"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;

using chatserver::ChatServer;

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

enum serviceTypes {LOGIN = 0, LOGOUT, SENDM, RECEIVEM, LIST};

class ServerImpl final
{
    public:
	ServerImpl()
        {
            allocateList();
        }

	~ServerImpl()
	{
	    server_->Shutdown();
	    cq_->Shutdown();
            delete list;
	}

        void Run()
        {
	    std::string server_address("0.0.0.0:50051");

	    ServerBuilder builder;
	    builder.AddListeningPort(server_address, 
                                     grpc::InsecureServerCredentials());
	    builder.RegisterService(&service_);
	    cq_ = builder.AddCompletionQueue();
	    server_ = builder.BuildAndStart();
	    std::cout << "Server Listening On " << server_address << std::endl;
	    HandleRpcs();
        }

	LinkedList* getList()
	{
	    return list;
	}

    private:

	void allocateList()
	{
	    list = new LinkedList();
	}

	class CallData
	{
	    public:
		
		int getType()
		{
		    return type;
		}

		void setType(int t)
		{
		    type = t;
		}

	    private:
		int type;
	};

        class CallDataLogIn: public CallData
	{
	    public:
		CallDataLogIn(ChatServer::AsyncService* service, 
                              ServerCompletionQueue* cq, LinkedList* plist)
			: service_(service), cq_(cq), responder_(&ctx_), 
                          status_(CREATE)
		{
		    setType(LOGIN);
		    Proceed(plist);
		}

		void Proceed(LinkedList* plist)
		{
		    if(status_ == CREATE)
		    {
			status_ = PROCESS;
			service_->RequestLogIn(&ctx_, &request_, 
                                               &responder_, 
                                               cq_, cq_, this);
		    }
		    else if(status_ == PROCESS)
		    {
			new CallDataLogIn(service_, cq_, plist);
                        std::string conformation;

                        // Search is name can be already found
                        // in list
                        if(plist->search(request_.user()))
                        {
                            // add 1 to the end of name
                            // if already taken
                            int count = 1;
                            // string conversion to concatenate
                            std::string scount = std::to_string(count);
                            // keep incrementig by one until open name
                            // is found
                            while(plist->search(request_.user() + scount))
                            {
                                scount = std::to_string(count++);
                            }
                            // insert generated name
                            conformation = request_.user() + " taken; " +
                                           "logged in as: " +
                                           request_.user() + scount;
                            plist->insertPerson(request_.user() + scount);
                            // update name for client
                            reply_.set_user(request_.user() + scount);
                        }
                        else
                        {
                            conformation = "Logged in as: " + request_.user();
                            // insert person into list
                            plist->insertPerson(request_.user());
                            // update name for client
                            reply_.set_user(request_.user());
                        }

			// service finished
                        reply_.set_conformation(conformation);
			status_ = FINISH;
			responder_.Finish(reply_, Status::OK, this);
		    }
		    else
		    {
			GPR_ASSERT(status_ == FINISH);
			delete this;
		    }
		}

	    private:
		ChatServer::AsyncService* service_;
		ServerCompletionQueue* cq_;
		ServerContext ctx_;
		LogInRequest request_;
		LogInReply reply_;
		ServerAsyncResponseWriter<LogInReply> responder_;
		
		enum CallStatus {CREATE, PROCESS, FINISH};
		CallStatus status_;
	};

	class CallDataReceiveMessage: public CallData
	{
	    public:
		CallDataReceiveMessage(ChatServer::AsyncService* service, 
                                       ServerCompletionQueue* cq, 
                                       LinkedList* plist)
			: service_(service), cq_(cq), 
                          responder_(&ctx_), status_(CREATE)
		{
		    setType(RECEIVEM);
		    Proceed(plist);
		}

		void Proceed(LinkedList* plist)
		{
		    if(status_ == CREATE)
		    {
			status_ = PROCESS;
			service_->RequestReceiveMessage(&ctx_, &request_, 
                                                        &responder_, 
                                                        cq_, cq_, this);
		    }
		    else if(status_ == PROCESS)
		    {
			new CallDataReceiveMessage(service_, cq_, plist);
                        
                        // .5 second delay
                        usleep(.750);
			// get messages from queue in node
			std::string messages = plist->
                             search(request_.user())->
                                       getMessages()->
                                       dequeueAll();

			// set conformation
			reply_.set_conformation(messages);

		        // service finished
			status_ = FINISH;
			responder_.Finish(reply_, Status::OK, this);
		    }
		    else
		    {
			GPR_ASSERT(status_ == FINISH);
			delete this;
		    }
		}

	    private:
		ChatServer::AsyncService* service_;
		ServerCompletionQueue* cq_;
		ServerContext ctx_;
		ReceiveMessageRequest request_;
		ReceiveMessageReply reply_;
		ServerAsyncResponseWriter<ReceiveMessageReply> responder_;
		
		enum CallStatus {CREATE, PROCESS, FINISH};
		CallStatus status_;
	};

        class CallDataSendMessage: public CallData
	{
	    public:
		CallDataSendMessage(ChatServer::AsyncService* service, 
                                    ServerCompletionQueue* cq, 
                                    LinkedList* plist)
			: service_(service), cq_(cq), 
                          responder_(&ctx_), status_(CREATE)
		{
		    setType(SENDM);
		    Proceed(plist);
		}

		void Proceed(LinkedList* plist)
		{
		    if(status_ == CREATE)
		    {
			status_ = PROCESS;
			service_->RequestSendMessage(&ctx_, &request_, 
                                                     &responder_, 
                                                     cq_, cq_, this);
		    }

		    else if(status_ == PROCESS)
		    {
			new CallDataSendMessage(service_, cq_, plist);
                        std::string state;

                        // .75 second delay
                        usleep(750);

                        // Put user's name before message
			std::string message = "Message from " + 
                                              request_.user() + 
                                                          ":" + 
                                              request_.message();
                        Node* person;
                        // Search to see if recipient is logged on
                        if(!(person = plist->search(request_.recipient())))
                            state = request_.recipient() + "is not logged " +
                                                           "on right now.";
                        else
                        {                        
                            // Add message to queue
                            // max 10 messages per person
			    if(plist->search(request_.recipient())->
                                                     getMessages()->
                                                     enqueue(message + "\n"))
                                state = request_.recipient() + "'s messages "
                                                             + "are full "
                                                             + "try agin "
                                                             + "later.\n";
                            // Messages sent
                            else
                                state = "Messages sent to " +
                                        plist->search(request_.recipient())->
                                                      getName() + "\n";
                        }

                        // Send back success/failure
                        reply_.set_conformation(state);


			status_ = FINISH;
			responder_.Finish(reply_, Status::OK, this);
		    }

		    else
		    {
			GPR_ASSERT(status_ == FINISH);
			delete this;
		    }
		}

	    private:
		ChatServer::AsyncService* service_;
		ServerCompletionQueue* cq_;
		ServerContext ctx_;
		SendMessageRequest request_;
		SendMessageReply reply_;
		ServerAsyncResponseWriter<SendMessageReply> responder_;
		
		enum CallStatus {CREATE, PROCESS, FINISH};
		CallStatus status_;
	};

        class CallDataList: public CallData
	{
	    public:
		CallDataList(ChatServer::AsyncService* service, 
                             ServerCompletionQueue* cq, 
                             LinkedList* plist)
			: service_(service), cq_(cq), 
                          responder_(&ctx_), status_(CREATE)
		{
		    setType(LIST);
		    Proceed(plist);
		}

		void Proceed(LinkedList* plist)
		{
		    if(status_ == CREATE)
		    {
			status_ = PROCESS;
			service_->RequestList(&ctx_, &request_, 
                                              &responder_, 
                                              cq_, cq_, this);
		    }
		    else if(status_ == PROCESS)
		    {
			new CallDataList(service_, cq_, plist);
                        // Set list of users
			reply_.set_list("List of users: " + 
                                         plist->list() + 
                                         "\n");

                        // Service finished
			status_ = FINISH;
			responder_.Finish(reply_, Status::OK, this);
		    }
		    else
		    {
			GPR_ASSERT(status_ == FINISH);
			delete this;
		    }
		}

	    private:
		ChatServer::AsyncService* service_;
		ServerCompletionQueue* cq_;
		ServerContext ctx_;
		ListRequest request_;
		ListReply reply_;
		ServerAsyncResponseWriter<ListReply> responder_;
		
		enum CallStatus {CREATE, PROCESS, FINISH};
		CallStatus status_;
	};

	class CallDataLogOut: public CallData
	{
	    public:
		CallDataLogOut(ChatServer::AsyncService* service, 
                               ServerCompletionQueue* cq, LinkedList* plist)
			: service_(service), cq_(cq), 
                          responder_(&ctx_), status_(CREATE)
		{
		    setType(LOGOUT);
		    Proceed(plist);
		}

		void Proceed(LinkedList* plist)
		{
		    if(status_ == CREATE)
		    {
			status_ = PROCESS;
			service_->RequestLogOut(&ctx_, &request_, 
                                                &responder_, 
                                                cq_, cq_, this);
		    }
		    else if(status_ == PROCESS)
		    {
			new CallDataLogOut(service_, cq_, plist);
                        // Set conformation
			reply_.set_conformation("Logged out: " + 
                        plist->deletePerson(request_.user()));

                        // Service finished
			status_ = FINISH;
			responder_.Finish(reply_, Status::OK, this);
		    }
		    else
		    {
			GPR_ASSERT(status_ == FINISH);
			delete this;
		    }
		}

	    private:
		ChatServer::AsyncService* service_;
		ServerCompletionQueue* cq_;
		ServerContext ctx_;
		LogOutRequest request_;
		LogOutReply reply_;
		ServerAsyncResponseWriter<LogOutReply> responder_;
		
		enum CallStatus {CREATE, PROCESS, FINISH};
		CallStatus status_;
	};

	

	void HandleRpcs()
	{
	    new CallDataLogIn(&service_, cq_.get(), getList());
	    new CallDataList(&service_, cq_.get(), getList());
	    new CallDataLogOut(&service_, cq_.get(), getList());
	    new CallDataSendMessage(&service_, cq_.get(), getList());
	    new CallDataReceiveMessage(&service_, cq_.get(), getList());
	    void* tag;
	    bool ok;
	    while(true)
	    {
		GPR_ASSERT(cq_->Next(&tag, &ok));
		GPR_ASSERT(ok);
		// check for type of service in superclass
		switch(static_cast<CallData*>(tag)->getType())
		{
		    case LOGIN:
			static_cast<CallDataLogIn*>(tag)->
                                    Proceed(getList());
			break;
		    case LIST:
			static_cast<CallDataList*>(tag)->
                                    Proceed(getList());
			break;
		    case LOGOUT:
			static_cast<CallDataLogOut*>(tag)->
                                    Proceed(getList());
			break;
		    case SENDM:
			static_cast<CallDataSendMessage*>(tag)->
                                    Proceed(getList());
			break;
		    case RECEIVEM:
			static_cast<CallDataReceiveMessage*>(tag)->
                                    Proceed(getList());
			break;
		    default:
			break;
		}
	    }
 	}

    	std::unique_ptr<ServerCompletionQueue> cq_;
    	ChatServer::AsyncService service_;
    	std::unique_ptr<Server> server_;
	// list to store users+messages
    	LinkedList* list;
};

int main(int argc, char** argv)
{
    ServerImpl server;
    server.Run();
    return 0;
}
