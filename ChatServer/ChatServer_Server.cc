#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unistd.h>

#include <grpc++/grpc++.h>
#include "chatserver.grpc.pb.h"
#include "UserNode.hpp"

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

typedef std::unordered_map<std::string, UserNode*> user_map;
typedef std::pair<std::string, UserNode*> user_pair;

enum serviceTypes {LOGIN = 0, LOGOUT, SENDM, RECEIVEM, LIST};

class ServerImpl final
{
    public:
	ServerImpl(){}

	~ServerImpl()
	{
	    server_->Shutdown();
	    cq_->Shutdown();
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

    private:

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
                              ServerCompletionQueue* cq, user_map& users)
			: service_(service), cq_(cq), responder_(&ctx_), 
                          status_(CREATE)
		{
		    setType(LOGIN);
		    Proceed(users);
		}

		void Proceed(user_map& users)
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
			new CallDataLogIn(service_, cq_, users);
                        std::string conformation;
                        std::string name = request_.user();

                        // iterator to user
                        auto user = users.find(name);

                        if(user != users.end())
                        {
                            users.insert(user_pair(name, new UserNode(name)));
                            conformation = "Logged in as new user: ";
                        }
                        else
                        {
                            // user already logged in
                            if(user->second->getStatus()) 
                            {
                                conformation = "User already logged in.";
                            }
                            else
                            {
                                // online_ to true
                                user->second->setStatus(true);
                                conformation = "Logged in as: ";
                            }
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
                                       user_map& users)
			: service_(service), cq_(cq), 
                          responder_(&ctx_), status_(CREATE)
		{
		    setType(RECEIVEM);
		    Proceed(users);
		}

		void Proceed(user_map& users)
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
			new CallDataReceiveMessage(service_, cq_, users);
                        auto user = users.find(request_.user());             
 
                        // .5 second delay
                        usleep(.750);
			// get messages from queue in node
                        auto messages = user->second->getMessages();

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
                                    user_map& users)
			: service_(service), cq_(cq), 
                          responder_(&ctx_), status_(CREATE)
		{
		    setType(SENDM);
		    Proceed(users);
		}


		void Proceed(std::unordered_map<std::string, UserNode*>& users)
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
			new CallDataSendMessage(service_, cq_, users);
                        std::string state;
                        auto name = request_.user();
                        auto message = request_.message();
                        auto recipient = users.find(request_.recipient());

                        // .75 second delay
                        usleep(750);

                        // Put user's name before message
			std::string newMessage = "Message from " + 
                                                 name + 
                                                 ":" + 
                                                 message;

                        // user exists
                        if(recipient != users.end())
                        {
                            // push message
                            recipient->second->addMessage(message);
                            // confirm sent
                            state = "Message sent to " + request_.recipient();
                        }
                        else
                        {
                            // confirm sent
                            state = "The user " + 
                                    request_.recipient() +
                                    " does not exist.";                                                   
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
                             user_map& users)
			: service_(service), cq_(cq), 
                          responder_(&ctx_), status_(CREATE)
		{
		    setType(LIST);
		    Proceed(users);
		}

		void Proceed(std::unordered_map<std::string, UserNode*>& users)
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
			new CallDataList(service_, cq_, users);
                        std::string list = "List of users: ";
                        auto begin = users.begin();

                        for(int i = 0; i < users.size() - 1; i++)
                        {
                            // append users, iterate
                            list+=(begin->second->getName());
                            list+=(", ");
                            begin++;
                        }
                        list+=(begin->second->getName());

                        // Set list of users
			reply_.set_list(list);

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
                               ServerCompletionQueue* cq, user_map& users)
			: service_(service), cq_(cq), 
                          responder_(&ctx_), status_(CREATE)
		{
		    setType(LOGOUT);
		    Proceed(users);
		}

		void Proceed(std::unordered_map<std::string, UserNode*>& users)
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
			new CallDataLogOut(service_, cq_, users);
                        auto name = request_.user();
                        auto user = users.find(name);
                        // set online_ to false
                        user->second->setStatus(false);

                        // Set conformation
			reply_.set_conformation("Logged out: " + name);
                        
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
	    new CallDataLogIn(&service_, cq_.get(), users_);
	    new CallDataList(&service_, cq_.get(), users_);
	    new CallDataLogOut(&service_, cq_.get(), users_);
	    new CallDataSendMessage(&service_, cq_.get(), users_);
	    new CallDataReceiveMessage(&service_, cq_.get(), users_);
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
                                    Proceed(users_);
			break;
		    case LIST:
			static_cast<CallDataList*>(tag)->
                                    Proceed(users_);
			break;
		    case LOGOUT:
			static_cast<CallDataLogOut*>(tag)->
                                    Proceed(users_);
			break;
		    case SENDM:
			static_cast<CallDataSendMessage*>(tag)->
                                    Proceed(users_);
			break;
		    case RECEIVEM:
			static_cast<CallDataReceiveMessage*>(tag)->
                                    Proceed(users_);
			break;
		    default:
			break;
		}
	    }
 	}

    	std::unique_ptr<ServerCompletionQueue> cq_;
    	ChatServer::AsyncService service_;
    	std::unique_ptr<Server> server_;

        user_map users_;
};

int main(int argc, char** argv)
{
    ServerImpl server;
    server.Run();
    return 0;
}
