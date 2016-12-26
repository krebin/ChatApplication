#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include "chatserver.grpc.pb.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using chatserver::ChatServer;
using chatserver::LogInRequest;
using chatserver::LogInReply;
using chatserver::LogOutRequest;
using chatserver::LogOutReply;
using chatserver::SendMessageReply;
using chatserver::SendMessageRequest;
using chatserver::ReceiveMessageReply;
using chatserver::ReceiveMessageRequest;
using chatserver::ListReply;
using chatserver::ListRequest;



class ChatServerClient
{
    public:
	explicit ChatServerClient(std::shared_ptr<Channel> channel)
				  : stub_(ChatServer::NewStub(channel)) {}

	std::string LogIn(const std::string& user)
        {
	    LogInRequest request;
	    request.set_user(user);

	    LogInReply reply;
	    ClientContext context;
	    CompletionQueue cq;
	    Status status;

	    std::unique_ptr<ClientAsyncResponseReader<LogInReply> > rpc(
	        stub_->AsyncLogIn(&context, request, &cq));

	    rpc->Finish(&reply, &status, (void*)1);
	    void* got_tag;
	    bool ok = false;
	    GPR_ASSERT(cq.Next(&got_tag, &ok));
	    GPR_ASSERT(got_tag == (void*)1);
	    GPR_ASSERT(ok);

	    if(status.ok())
	    {
		return reply.conformation() + "\n";
	    }
	    else
	    {
	        return "RPC failed";
	    }
	}

	std::string list()
        {
	    ListRequest request;
	    request.set_list("");

	    ListReply reply;
	    ClientContext context;
	    CompletionQueue cq;
	    Status status;

	    std::unique_ptr<ClientAsyncResponseReader<ListReply> > rpc(
		stub_->AsyncList(&context, request, &cq));

	    rpc->Finish(&reply, &status, (void*)1);
	    void* got_tag;
	    bool ok = false;
	    GPR_ASSERT(cq.Next(&got_tag, &ok));
	    GPR_ASSERT(got_tag == (void*)1);
	    GPR_ASSERT(ok);

	    if(status.ok())
	    {
		return "List: " + reply.list() + "\n";
	    }
	    else
	    {
	        return "RPC failed";
	    }
        }

	std::string receivemessage(std::string user)
        {
	    ReceiveMessageRequest request;
	    request.set_user(user);

	    ReceiveMessageReply reply;
	    ClientContext context;
	    CompletionQueue cq;
	    Status status;

	    std::unique_ptr<ClientAsyncResponseReader<ReceiveMessageReply> > rpc(
		stub_->AsyncReceiveMessage(&context, request, &cq));

	    rpc->Finish(&reply, &status, (void*)1);
	    void* got_tag;
	    bool ok = false;
	    GPR_ASSERT(cq.Next(&got_tag, &ok));
	    GPR_ASSERT(got_tag == (void*)1);
	    GPR_ASSERT(ok);

	    if(status.ok())
	    {
		return reply.conformation();
	    }
	    else
	    {
	        return "RPC failed";
	    }
        }



	std::string logout(std::string user)
	{
	    LogOutRequest request;
	    request.set_user(user);

	    LogOutReply reply;
	    ClientContext context;
	    CompletionQueue cq;
	    Status status;

	    std::unique_ptr<ClientAsyncResponseReader<LogOutReply> > rpc(
		stub_->AsyncLogOut(&context, request, &cq));

	    rpc->Finish(&reply, &status, (void*)1);
	    void* got_tag;
	    bool ok = false;
	    GPR_ASSERT(cq.Next(&got_tag, &ok));
	    GPR_ASSERT(got_tag == (void*)1);
	    GPR_ASSERT(ok);

	    if(status.ok())
	    {
		return "Logged Out: " + reply.conformation() + "\n";
	    }
	    else
	    {
	        return "RPC failed";
	    }
        }

	std::string sendmessage(std::string user, std::string recipient)
	{
	    std::cout << "Enter your message. Enter your last message as \"end\" to stop sending messages.\n\n";
	    std::string message = "";
	    std::string appendMessage;

            do
	    {
		getline(std::cin, appendMessage);
		if(appendMessage == "end")
		    break;
		message+=(appendMessage + "\n");
	    }
            while(1);

	    SendMessageRequest request;
	    request.set_user(user);
	    request.set_recipient(recipient);
	    request.set_message(message);

	    SendMessageReply reply;
	    ClientContext context;
	    CompletionQueue cq;
	    Status status;

	    std::unique_ptr<ClientAsyncResponseReader<SendMessageReply> > rpc(
		stub_->AsyncSendMessage(&context, request, &cq));

	    rpc->Finish(&reply, &status, (void*)1);
	    void* got_tag;
	    bool ok = false;
	    GPR_ASSERT(cq.Next(&got_tag, &ok));
	    GPR_ASSERT(got_tag == (void*)1);
	    GPR_ASSERT(ok);

	    if(status.ok())
	    {
		return "Messages sent to " + reply.conformation() + "\n\n";
	    }
	    else
	    {
	        return "RPC failed";
	    } 
	}


   

    private:
	std::unique_ptr<ChatServer::Stub> stub_;
};

int displayMenu(ChatServerClient* Chatter, std::string user);

int displayMenu(ChatServerClient* Chatter, std::string user)
{
        std::cout << "Enter the number corresponding to the command.\n";
        std::cout << "1: Log Out\n";
	std::cout << "2: Send Messages\n";
	std::cout << "3: Receive Messages\n";
	std::cout << "4: List of people on server\n\n";

        int choice;
        std::string rec;
        std::cin >> choice;
        

	switch(choice)
	{
	    case 1:
		std::cout << Chatter->logout(user);
	        return 0;
	    case 2:
		std::cout << "Who would you like to send messages to?\n";
		std::cin >> rec;
		std::cout << "Sending messages to " << rec << std::endl;
		std::cout << Chatter->sendmessage(user, rec);	
		return 1;
	    case 3:
		std::cout << "Receiving messages.\n";
		std::cout << Chatter->receivemessage(user) << std::endl << "All messages recieved.\n" << std::endl;
		return 1;
	    case 4:
                std::cout << Chatter->list();
		return 1;
	    default:
	        std::cout << "Invalid Choice. Exiting Client.\n";
	        return 0;
	}
}

int main(int argc, char** argv)
{
    ChatServerClient chatter(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
    
    ChatServerClient* Chatter = &chatter;
 
    std::string user;
    std::string conformation;

    std::cout << "Log In As: ";
    std::cin >> user;

    if((conformation = chatter.LogIn(user)) == "RPC failed")
    {
	std::cout << "Unable to connect to server. Exiting Client.\n";
	return -1;
    }

    std::cout << "Logged In As: " << conformation << std::endl;

    int choice;

    do
    {
	choice = displayMenu(Chatter, user);
    }
    while(choice);
    
    return 0;
}

