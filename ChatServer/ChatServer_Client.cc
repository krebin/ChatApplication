#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <grpc++/grpc++.h>
#include "chatserver.grpc.pb.h"
#include "ChatServerGlobal.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientAsyncReaderInterface;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using grpc::ClientReaderWriter;
using grpc::ClientReader;

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


    ChatMessage createChatMessage(std::string message, std::string user) 
    {
        ChatMessage chatMessage;
        chatMessage.set_messages(message);
        chatMessage.set_user(user);
        return chatMessage; 
    } 

    void LogOut(std::string user)
    {

        ClientContext context;
        CompletionQueue cq;
        Status status;

	    LogOutRequest logOutRequest;
	    LogOutReply logOutReply;

		// set user
		logOutRequest.set_user(user);
        std::unique_ptr<ClientAsyncResponseReader<LogOutReply>> 
        rpc(stub_->AsyncLogOut(&context, logOutRequest, &cq));
		rpc->Finish(&logOutReply, &status, (void*)1);

	    void* got_tag;
	    bool ok = false;
	    GPR_ASSERT(cq.Next(&got_tag, &ok));
	    GPR_ASSERT(got_tag == (void*)1);
	    GPR_ASSERT(ok);

        if(status.ok())
            std::cout << logOutReply.confirmation();
        else
            std::cout << LOG_OUT_FAIL;
    }

    /** Receive user's messages
     * @param user: Name of user
     */
    void ReceiveMessage(std::string user)
    {
        ReceiveMessageRequest request;
        ReceiveMessageReply reply;

        // Set current user to get messages for
        request.set_user(user); 

        ClientContext context;
        std::shared_ptr<ClientReader<ReceiveMessageReply>>
        reader(stub_->ReceiveMessage(&context, request));

        reader->Read(&reply);

        // If the message queue was empty to begin with
        if(reply.queuestate() == chatserver::ReceiveMessageReply::EMPTY)
        {
            std::cout << RECEIVE_MESSAGE_NONE;
        }
        // Else run as normal
        else
        {
            while(reader->Read(&reply))
            {
                std::cout << reply.messages() << std::endl;
            }
        }

    }

    void SendMessage(std::string user)
    {
        ClientContext context;
        std::shared_ptr<ClientReaderWriter<SendMessageRequest, SendMessageReply>>
        stream(stub_->SendMessage(&context));


        // Ask for recipient
        std::cout << SEND_MESSAGES_WHO;
        std::string recipient;
        getline(std::cin, recipient);

        // Server checks if user exists
        SendMessageReply reply;
        SendMessageRequest request;
        request.set_recipient(recipient);

        // Initial state because first request send to server
        // will be to check if the user exists
        request.set_requeststate(chatserver::SendMessageRequest::INITIAL);

        stream->Write(request);
        stream->Read(&reply);
 
        // Server sets status to 1 if user exists
        // Need status member to see if user exists
        // in the first place
        if(reply.recipientstate() == chatserver::SendMessageReply::EXIST)
        {
            std::cout << SEND_MESSAGE_PROMPT; 
            std::string message;

            while(true)
            {
                getline(std::cin, message);
                
                // Set request parameters
                request.set_user(user);
                request.set_recipient(recipient);
                request.set_messages(message);
                request.set_requeststate(chatserver::SendMessageRequest::PROCESSING);

                // Send request to server
                if(message == DONE)
                {
            
                    stream->WritesDone(); 
                    break;
                }
                else
                {
                    stream->Write(request);
                    stream->Read(&reply);
                }
            }
        }
        else
        {
            stream->WritesDone(); 
        }        

        std::cout << reply.confirmation();      
 
        Status status = stream->Finish();

        if(!status.ok())
            std::cout << SEND_MESSAGE_FAIL;
        else
            std::cout << SEND_MESSAGE_DONE;
    }

    void Chat(std::string user)
    {
        std::cout << CHAT_PROMPT;

        ClientContext context;
        std::shared_ptr<ClientReaderWriter<ChatMessage, ChatMessage>>
        stream(stub_->Chat(&context));

        // lambda function to read new messages repeatedly
        std::thread reader([stream]()
        {
                ChatMessage server_note;
                while(stream->Read(&server_note))
                {
                    std::cout << "["
                              << server_note.user()
                              << "]: "
                              << server_note.messages()
                              << std::endl;
                }
        });

        while(true)
        {
            std::string message;
            getline(std::cin, message);
            if(message != DONE)
            {
                stream->Write(createChatMessage(message, user));
            }
            else
            {
                stream->WritesDone();
                break;
            }
        }

        reader.join();
        Status status = stream->Finish();

        if(!status.ok())
            std::cout << CHAT_FAIL;
        else
            std::cout << CHAT_DONE;

                            
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

	    std::string confirmation;

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
		confirmation = listReply.list();
	    else if(type == LOGIN)
        {
		    confirmation = logInReply.confirmation();
            // In case name was changed
            user = logInReply.user();
        }
	    else if(type == LOGOUT)
		    confirmation = logOutReply.confirmation();
	    if(status.ok())
	    {
		    return confirmation;
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
            Chatter->LogOut(user);
	        return 0;
	    // User wants to send messages to someone
	    case 2:
		    Chatter->SendMessage(user);    
		    return 1;
	    // User wants to receieve messages
	    case 3:
            Chatter->ReceiveMessage(user);
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
    // string to store confirmation of log in service
    std::string confirmation;

    std::cout << "Log In As: ";
    std::getline(std::cin, user);
   
    while((confirmation = chatter.sendService(user, LOGIN, "", "")) == "+\n"
         ||confirmation == "-\n")
    {
       // Marker for user already logged in
       if(confirmation == "+\n")
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
    if(confirmation == "RPC failed")
    {
	std::cout << "Cannot connect to server\n";
        return -1;
    }

    // Log In successful
    std::cout << confirmation << std::endl;
    

    

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

