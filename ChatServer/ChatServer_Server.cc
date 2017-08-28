#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <ctime>
#include <vector>
#include <unordered_map>
#include <functional>
#include <unordered_set>
#include <unistd.h>

#include <grpc++/grpc++.h>
#include "chatserver.grpc.pb.h"
#include "UserNode.hpp"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerAsyncWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using grpc::ServerAsyncReaderWriter;

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
using chatserver::ChatMessage;

enum serviceTypes {LOGIN = 0, LOGOUT, SENDM, RECEIVEM, LIST};
#define ALPHA_START 65
#define ALPHA_END 122

/** A function class to use as Compare class in unordered_set<UserNode*>
 */
struct UserNodeComp
{
    public:
        /** Overloaded operator
         * @return bool: true if same, false if not
         * @param UserNode*& lhs: first UserNode pointer to compare
         * @param UserNode*& rhs: second UserNode pointer to compare
         */
        bool operator()(UserNode* const& lhs, UserNode* const& rhs) const
        {
            // dereference pointers to compare
            return *(lhs) == *(rhs);
        }
};

/** A function class to use as a Hash class in unordered_set<UserNode*>
 */
struct UserNodeHash 
{
    public:
        /** Overloaded operator
         * @return size_t hash value
         * @param UserNode*& node: UserNode pointer to hash
         */
        size_t operator()(UserNode* const& node) const
        {
           // standard library hash function
           return hash_(node->getName());
        }

    private:
        // hash element
        std::hash<std::string> hash_;
};


bool isValid(std::string name);

/** Function to check whether or not name chosen is valid name
 * Name must contain only alphanumeric and no spaces
 * @param std::string name: string to be checked
 * @return bool valid: true if valid, false if not
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


class ServerImpl final : public ChatServer::AsyncService
{


    using user_set = std::unordered_set<UserNode*, UserNodeHash, UserNodeComp>;

    using chat_responder = ServerAsyncReaderWriter<ChatMessage, ChatMessage>;

    using responder_set = std::unordered_map<void*, ServerAsyncReaderWriter<ChatMessage, ChatMessage>>;

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

        /** Base class for all CallData
         */
	class CallData
	{
	    public:
                /** Must override this method, proceed with RPC
                 * @param user_set& users: ref to unordered_set of UserNode*
                 */
                virtual void Proceed(user_set& users) = 0;
	
	};

        /** Class for Log In RPC service
         */
        class CallDataLogIn final: public CallData
	{
	    public:
		CallDataLogIn(ChatServer::AsyncService* service, 
                              ServerCompletionQueue* cq, user_set& users)
			: service_(service), cq_(cq), responder_(&ctx_), 
                          status_(CREATE)
		{
		    Proceed(users);
		}

		void Proceed(user_set& users)
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
                        std::string confirmation;
                        std::string name = request_.user();
                        bool valid = isValid(name);

                        // UserNode on stack for find
                        UserNode tempUser(name);

                        // iterator to user
                        auto user = (users.find(&tempUser));

                        // if name was valid
                        if(valid)
                        {
                            if(user == users.end())
                            {
                                users.insert(new UserNode(name));
                                confirmation = "Logged in as new user: " + name;
                            }
                            else
                            {
                                // user already logged in
                                if((*user)->getStatus()) 
                                {
                                    confirmation = "+";
                                }
                                else
                                {
                                    // online_ to true
                                    (*user)->setStatus(true);
                                    confirmation = "Logged in as: " + name;
                                }
                            }
                        }
                        else
                        {
                            confirmation = "-";
                        }

                        // update user
                        reply_.set_user(request_.user());

	 	// service finished
                        reply_.set_confirmation(confirmation + "\n");
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

	class CallDataReceiveMessage final: public CallData
	{
	    public:
		CallDataReceiveMessage(ChatServer::AsyncService* service, 
                                       ServerCompletionQueue* cq, 
                                       user_set& users)
			: service_(service), cq_(cq), 
                          responder_(&ctx_), status_(CREATE)
		{
		    Proceed(users);
		}

		void Proceed(user_set& users)
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

                        // user on stack for find
                        UserNode tempUser(request_.user());
                        auto user = users.find(&tempUser);             
 
                        // .5 second delay
                        usleep(.750);
			// get messages from queue in node
                        auto messages = (*user)->getMessages();

			// set confirmation
			reply_.set_confirmation(messages);

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

    class CallDataChat final: public CallData
	{
	    public:
		CallDataChat(ChatServer::AsyncService* service
                   , ServerCompletionQueue* cq 
                   , user_set& users
                   , std::vector<CallDataChat*>& chatters)

			       : service_(service)
                   , cq_(cq) 
                   , responder_(&ctx_)
                   , status_(CREATE)
                   , chatters_(chatters)
		{
		    Proceed(users);
		}

		ServerAsyncReaderWriter<ChatMessage, ChatMessage> responder_;
        std::vector<CallDataChat*> chatters_;
        std::vector<std::string> receivedMessages_;

		void Proceed(user_set& users)
		{
		    if(status_ == CREATE)
		    {
			    status_ = PROCESS;
			    service_->RequestChat(&ctx_,
                                      &responder_, 
                                      cq_, cq_, this);


                //service_->RequestSendMessage(&ctx_, &request_, 
                //                             &responder_, 
                 //                            cq_, cq_, this);

		    }
		    else if(status_ == PROCESS)
		    {
			    new CallDataChat(service_, cq_, users, chatters_);

                responder_.Read(&request_, this);
                status_ = PROCESSING;
                ChatMessage test;
                test.set_user("Test");
                test.set_messages("Test Message");

                for(int i = 0; i < chatters_.size(); i++)
                {
                    auto responder = chatters_[i];
                    if(responder != this)
                    {
                        responder->responder_.Write(test, this);
                    } 
                }

                std::cout<<"test5\n";
                chatters_.push_back(this);
                
		    }

		    else if(status_ == PROCESSING)
		    {
                responder_.Read(&request_, this);
                ChatMessage test;
                test.set_user("Test");
                test.set_messages("Test Message");

                for(int i = 0; i < chatters_.size(); i++)
                {
                    auto responder = chatters_[i];
                    if(responder != this)
                    {
                        responder->responder_.Write(test, this);
                    } 
                }



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
            responder_set responders_;
		    ChatMessage request_;
		
		    enum CallStatus {CREATE, PROCESS, PROCESSING, FINISH};
		    CallStatus status_;
	};



    class CallDataSendMessage final: public CallData
	{
	    public:
		CallDataSendMessage(ChatServer::AsyncService* service, 
                                    ServerCompletionQueue* cq, 
                                    user_set& users)
			: service_(service), cq_(cq), 
                          responder_(&ctx_), status_(CREATE)
		{
		    Proceed(users);
		}


		void Proceed(user_set& users)
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
                        
                        // User on stack for find
                        UserNode tempUser(request_.recipient());
                        auto recipient = users.find(&tempUser);

                        // Time and Date
                        time_t now = time(0);
                        tm *gmtm = gmtime(&now);
                        char* dt = asctime(gmtm);

                        // .75 second delay
                        usleep(750);
                        // Put user's name before message
			std::string newMessage = "UTC: " +
                                                 std::string(dt) +
                                                 "Message from " + 
                                                 name + 
                                                 ":\n" +
                                                 message + "\n";

                        // User exists
                        if(recipient != users.end())
                        {
                            // push message
                            (*recipient)->addMessage(newMessage);
                            // confirm sent
                            state = "Message sent to " + 
                                    request_.recipient() +
                                    "\n";
                        }
                        // User does not exist
                        else
                        {
                            state = "The user " + 
                                    request_.recipient() +
                                    " does not exist.\n";                                                   
                        }
 
                        // Send back success/failure
                        reply_.set_confirmation(state + "\n");

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

        class CallDataList final: public CallData
	{
	    public:
		CallDataList(ChatServer::AsyncService* service, 
                             ServerCompletionQueue* cq, 
                             user_set& users)
			: service_(service), cq_(cq), 
                          responder_(&ctx_), status_(CREATE)
		{
		    Proceed(users);
		}

		void Proceed(user_set& users)
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

                        while(begin != users.end())
                        {
                            // append users, iterate
                            if((*begin)->getStatus())
                            {
                                list+="[";
                                list+=((*begin)->getName());
                                list+="] ";
                            }
                            begin++;
                        }

                        // Set list of users
			reply_.set_list(list + "\n");

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

	class CallDataLogOut final: public CallData
	{
	    public:
		CallDataLogOut(ChatServer::AsyncService* service, 
                               ServerCompletionQueue* cq, user_set& users)
			: service_(service), cq_(cq), 
                          responder_(&ctx_), status_(CREATE)
		{
		    Proceed(users);
		}

		void Proceed(user_set& users)
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
                        // User on stack for find
                        UserNode tempUser(name);
                        auto user = users.find(&tempUser);
                        // set online_ to false
                        (*user)->setStatus(false);
                        std::cout << name;

                        // Set confirmation
			reply_.set_confirmation("Logged out: " + name + "\n");
                        
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
        new CallDataChat(&service_, cq_.get(), users_, chatters_);

	    void* tag;
	    bool ok;
	    while(true)
	    {
		    GPR_ASSERT(cq_->Next(&tag, &ok));
		    GPR_ASSERT(ok);
            static_cast<CallData*>(tag)->Proceed(users_);
	    }
 	}

    	std::unique_ptr<ServerCompletionQueue> cq_;
    	ChatServer::AsyncService service_;
    	std::unique_ptr<Server> server_;
		//std::shared_ptr<ServerAsyncReaderWriter<ChatMessage, ChatMessage>> stream_;

        // Hash table that uses user names as keys
        user_set users_;
        // Hash table that uses user names to identify Chat RPC
        //responder_set responders_;
        std::vector<CallDataChat*> chatters_;
};

int main(int argc, char** argv)
{
    ServerImpl server;
    server.Run();
    return 0;
}
