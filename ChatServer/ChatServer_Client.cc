#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <grpc++/grpc++.h>
#include "chatserver.grpc.pb.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientAsyncReaderInterface;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using grpc::ClientReaderWriter;

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
using chatserver::ChatMessage;
using chatserver::HelloRequest;
using chatserver::HelloReply;

enum serviceTypes {LOGIN = 0, LOGOUT, SENDM, RECEIVEM, LIST, CHAT};

template<typename RequestType, typename ReplyType>
struct UnaryServiceType
{
    RequestType request;
    ReplyType reply;
    std::unique_ptr<ClientAsyncResponseReader<ReplyType>> rpc;
};


class ChatServerClient
{
    public:
	explicit ChatServerClient(std::shared_ptr<Channel> channel)
				  : stub_(ChatServer::NewStub(channel)) {}

	std::string createMessage()
	{
	    std::cout << "Enter your message. "
                  << "Enter your last message as \"end\" "
                  << "to stop sending messages.\n\n";

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
	    return message;
	}



    ChatMessage createChatMessage(std::string message, std::string user) 
    {
        ChatMessage chatMessage;
        chatMessage.set_messages(message);
        chatMessage.set_user(user);
        return chatMessage; 
    } 


    void Chat(std::string user)
    {
        std::cout << "Now chatting, type anything. "
                  << "Send \"#done\" to end the Chat.\n\n";

        ClientContext context;
        std::shared_ptr<ClientReaderWriter<ChatMessage, ChatMessage>>
        stream(stub_->Chat(&context));

        // lambda function to read new messages repeatedly
        std::thread reader([stream]()
        {
                ChatMessage server_note;
                while(stream->Read(&server_note))
                {
                    // server will set note's user as done
                    if(server_note.user() == "#done")
                        break;

                    std::cout << "["
                              << server_note.user()
                              << "]: "
                              << server_note.messages()
                              << std::endl;
                }
        });

        // Repeat input of messages
        // "#done" is message to end chat
        std::string message;
        do
        {   
    		getline(std::cin, message);
            stream->Write(createChatMessage(message, user));
        }
        while(message != "#done");

        // Signal done writing messages to stream
        stream->WritesDone();
        Status status = stream->Finish();

        // wait for thread to finish
        reader.join();

        if(!status.ok())
            std::cout << "Chat RPC failed.\n\n";
        else
            std::cout << "Chat finished.\n\n";

                            
    }

    void processLogIn()
    {
    }

	std::string sendService(std::string& user 
                          , int type
                          , const std::string& recipient
                          , std::string message)
	{
	
	    ClientContext context;
	    CompletionQueue cq;
        Status status;

	    // grpc generated classes to send services
        LogInRequest logInRequest;
	    LogInReply logInReply;
	    LogOutRequest logOutRequest;
	    LogOutReply logOutReply;
	    ListRequest listrequest;
	    ListReply listReply;
	    ReceiveMessageRequest receiveMessageRequest;
	    ReceiveMessageReply receiveMessageReply;
	    SendMessageRequest sendMessageRequest;
	    SendMessageReply sendMessageReply;

	    std::string conformation;

	    // service is LogIn
	    if(type == LOGIN)
	    {
		// set user
	        logInRequest.set_user(user);
            std::unique_ptr<ClientAsyncResponseReader<LogInReply>>
            rpc(stub_->AsyncLogIn(&context, logInRequest, &cq));
		    rpc->Finish(&logInReply, &status, (void*)1);
	    }
	    // service is LogOut
	    else if(type == LOGOUT)
	    {
		// set user
		    logOutRequest.set_user(user);
		    std::unique_ptr<ClientAsyncResponseReader<LogOutReply>> 
            rpc(stub_->AsyncLogOut(&context, logOutRequest, &cq));

		    rpc->Finish(&logOutReply, &status, (void*)1);
	    }
	    // service is SendMessage
            else if(type == SENDM)
        {
		// set user, message, recipient
	        sendMessageRequest.set_user(user);
	        sendMessageRequest.set_message(message);
	        sendMessageRequest.set_recipient(recipient);
		    std::unique_ptr<ClientAsyncResponseReader<SendMessageReply>> 
            rpc(stub_->AsyncSendMessage(&context,sendMessageRequest, &cq));

		    rpc->Finish(&sendMessageReply, &status, (void*)1);
	    }
	    // service is ReceiveMessage
	    else if(type == RECEIVEM)
	    {
		// set user
	        receiveMessageRequest.set_user(user);
		    std::unique_ptr<ClientAsyncResponseReader<ReceiveMessageReply>> 
            rpc(stub_->AsyncReceiveMessage(&context, receiveMessageRequest,
                                               &cq));
		    rpc->Finish(&receiveMessageReply,  &status, (void*)1);
	    }
            // service is List
	    else if(type == LIST)
        {
	    	std::unique_ptr<ClientAsyncResponseReader<ListReply>> 
            rpc(stub_->AsyncList(&context, listrequest, &cq));
            rpc->Finish(&listReply, &status, (void*)1);
	    }
	    else
	    {
		    return "No such service\n\n";
	    }

	    void* got_tag;
	    bool ok = false;
	    GPR_ASSERT(cq.Next(&got_tag, &ok));
	    GPR_ASSERT(got_tag == (void*)1);
	    GPR_ASSERT(ok);

	    // Get responses from replies depending on which service
	    // was called.
	    if(type == LIST)
		conformation = listReply.list();
	    else if(type == LOGIN)
        {
		    conformation = logInReply.conformation();
            // In case name was changed
            user = logInReply.user();
        }
	    else if(type == LOGOUT)
		    conformation = logOutReply.conformation();
	    else if(type == RECEIVEM)
		    conformation = receiveMessageReply.conformation();
	    else if(type == SENDM)
		    conformation = sendMessageReply.conformation();

	    if(status.ok())
	    {
		    return conformation;
	    }
	    else
	    {
		    return "RPC failed";
	    }
	}

    private:
	    std::unique_ptr<ChatServer::Stub> stub_;
        CompletionQueue cq_;
                   
};

int displayMenu(ChatServerClient* Chatter, std::string& user);

int displayMenu(ChatServerClient* Chatter, std::string& user)
{
    std::cout << "Enter the number corresponding to the command.\n"     
              << "1: Log Out\n"
	          << "2: Send Messages\n"
	          << "3: Receive Messages\n"
	          << "4: List of people on server\n"
              << "5: Chat\n\n";
    int choice;
    std::string input, rec;
    std::getline(std::cin, input);

    // check for EOF, default behavior will log out
    if(std::cin.eof())
    {
        std::cout << Chatter->sendService(user, LOGOUT, "", "");
        return 0;
    }

    std::stringstream format(input);
    format >> choice;

	switch(choice)
	{
	    // User wants to log out
	    case 1:
		std::cout << Chatter->sendService(user, LOGOUT, "", "");
	        return 0;
	    // User wants to send messages to someone
	    case 2:
		    std::cout << "Who would you like to send messages to?\n";
		    std::cin >> rec;
		    std::cout << "Sending messages to " << rec << std::endl;
		    std::cout << Chatter->sendService(user, SENDM, rec, 
                         Chatter->createMessage());            
		return 1;
	    // User wants to receieve messages
	    case 3:
	        std::cout << "Receiving messages.\n\n"
                      << Chatter->sendService(user, RECEIVEM, "", "")
                      << "All messages recieved.\n\n";
		return 1;
	    // User wants a list of people on server
	    case 4:
            std::cout << Chatter->sendService(user, LIST, "", "");
	        return 1;
        case 5: 
            Chatter->Chat(user);
            return 1;
	    default:
	        std::cout << "Invalid Choice.\n\n";
	        return 1;
	}
}

int main(int argc, char** argv)
{
    ChatServerClient chatter(grpc::CreateChannel("localhost:50051", 
                             grpc::InsecureChannelCredentials()));
    
    // name of user
    std::string user;
    // string to store conformation of log in service
    std::string conformation;

    std::cout << "Log In As: ";
    std::getline(std::cin, user);
   
    while((conformation = chatter.sendService(user, LOGIN, "", "")) == "+\n"
         ||conformation == "-\n")
    {
       // Marker for user already logged in
       if(conformation == "+\n")
       {
           std::cout << "The user "
                     << user
                     << " is already logged in.\n\n";
       }
       // - is marker for invalid name
       else
       {
           std::cout << "The name "
                     << user
                     << " is not alphanumeric or contains spaces.\n\n";
       }
       std::cout << "Log in as: ";

       std::getline(std::cin, user);
    }

    // log in did not go through
    if(conformation == "RPC failed")
    {
	std::cout << "Cannot connect to server\n";
        return -1;
    }

    // Log In successful
    std::cout << conformation << std::endl;
    

    

    // run displayMenu until user wants to log out
    // or chooses an invalid service
    int choice;

    do
    {
	    choice = displayMenu(&chatter, user);
    }
    while(choice);
    return 0;
}

