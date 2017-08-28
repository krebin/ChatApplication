#include <iostream>
#include <functional>
#include <memory>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <grpc++/grpc++.h>
#include "chatserver.grpc.pb.h"
#include "ChatServerGlobal.h"

#include <ncurses.h>
#include <form.h>
#include <menu.h>

#define KEY_ENTER_ALT 10
#define KEY_BACKSPACE_ALT 127
#define X_RPC_RECEIVE 7
#define Y_RPC_RECEIVE 0
#define X_USER_INPUT 7
#define Y_USER_INPUT 0
#define NUM_FIELDS 1
#define X_START_TYPE 5
#define Y_START_TYPE 1

enum color {RED = 1, GREEN, WHITE};

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

/** ChatServer Client
 * @param channel: channel connecting the client and server
 */
class ChatServerClient
{
    public:
	    explicit ChatServerClient(std::shared_ptr<Channel> channel)
		    		  : stub_(ChatServer::NewStub(channel)) {}



        /** Method to create a chat message
         * @param message: message to send
         * @param user: user sending the message
         * @return ChatMessage: created chat message
         */
        ChatMessage createChatMessage(std::string message, std::string user) 
        {
            ChatMessage chatMessage;
            chatMessage.set_messages(message);
            chatMessage.set_user(user);
            return chatMessage; 
        } 

        /** Unary RPC to log out
         * @param user: user to log out
         */
        void LogOut()
        {

            ClientContext context;
            CompletionQueue cq;
            Status status;

            LogOutRequest logOutRequest;
            LogOutReply logOutReply;

            // set user
            logOutRequest.set_user(user_);
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

        /** Server streaming RPC to receive messages sent to user
         * @param user: Name of user
         */
        void ReceiveMessage()
        {
            ReceiveMessageRequest request;
            ReceiveMessageReply reply;

            // Set current user to get messages for
            request.set_user(user_); 

            ClientContext context;
            // Start server streaming RPC
            // also sends first request
            std::shared_ptr<ClientReader<ReceiveMessageReply>>
            reader(stub_->ReceiveMessage(&context, request));

            // Read reply
            reader->Read(&reply);

            // If the message queue was empty to begin with
            if(reply.queuestate() == chatserver::ReceiveMessageReply::EMPTY)
            {
                std::cout << RECEIVE_MESSAGE_NONE;
            }
            // Else run as normal
            else
            {
                // Will terminate once there are no more messages to read
                while(reader->Read(&reply))
                {
                    std::cout << reply.messages() << std::endl;
                }
            }

            Status status;
            status = reader->Finish();

            // Check if RPC finished successfully
            if(status.ok())
                std::cout << RECEIVE_MESSAGE_DONE;
            else
                std::cout << RECEIVE_MESSAGE_FAIL;

        }

        /** Bidirectional RPC to privately message another user
         * @param user: name of sender
         */
        void SendMessage()
        {
            ClientContext context;
            // Start bidirectional RPC
            std::shared_ptr<ClientReaderWriter<SendMessageRequest, SendMessageReply>>
            stream(stub_->SendMessage(&context));


            // Ask for recipient
            std::cout << SEND_MESSAGES_WHO;
            std::string recipient;
            getline(std::cin, recipient);

            SendMessageReply reply;
            SendMessageRequest request;
            request.set_recipient(recipient);

            // Initial state because first request send to server
            // will be to check if the user exists
            request.set_requeststate(chatserver::SendMessageRequest::INITIAL);

            // Send request
            stream->Write(request);
            // Read reply
            stream->Read(&reply);
     
            // Server sets status to 1 if user exists
            // Need status member to see if user exists
            // in the first place
            if(reply.recipientstate() == chatserver::SendMessageReply::EXIST)
            {
                std::cout << SEND_MESSAGE_PROMPT; 
                std::string message;

                // Will break when the message is "#done"
                while(true)
                {
                    getline(std::cin, message);
                    
                    // Set request parameters
                    request.set_user(user_);
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
                // Declare writes done so server can finish RPC
                stream->WritesDone(); 
            }        

            std::cout << reply.confirmation();      
     
            Status status = stream->Finish();

            // Check that RPC finished successfully
            if(!status.ok())
                std::cout << SEND_MESSAGE_FAIL;
            else
                std::cout << SEND_MESSAGE_DONE;
        }

        /** Bidirectional RPC for user to chat real time with other users
         * @param user: name of user
         */
        void Chat()
        {
            std::cout << CHAT_PROMPT;

            // Start bidirectional RPC
            ClientContext context;
            std::shared_ptr<ClientReaderWriter<ChatMessage, ChatMessage>>
            stream(stub_->Chat(&context));

            // lambda function to read new messages repeatedly
            // started in another thread as to not block the
            // main thread from reading in messages
            std::thread reader([stream]()
            {
                    ChatMessage server_note;
                    // While loop will terminate once the stream
                    // returns termination signal, either when
                    // this method reaches stream->WritesDone()
                    // or stream->Finish()
                    while(stream->Read(&server_note))
                    {
                        std::cout << "["
                                  << server_note.user()
                                  << "]: "
                                  << server_note.messages()
                                  << std::endl;
                    }
            });

            // Until the user sends the break string
            while(true)
            {
                // Read in message
                std::string message;
                getline(std::cin, message);

                // String not "#done"
                if(message != DONE)
                {
                    // Send to server
                    stream->Write(createChatMessage(message, user_));
                }
                // String is "#done"
                else
                {
                    // Declare writes done so server can finish RPC
                    stream->WritesDone();
                    break;
                }
            }

            // Wait for lamba thread to finish
            reader.join();
            Status status = stream->Finish();

            // Check is RPC finished successfully
            if(!status.ok())
                std::cout << CHAT_FAIL;
            else
                std::cout << CHAT_DONE;

                                
        }

        /** RPC to log in to server
         * @return int: 0 if failed to log in, 1 if successful
         */
        int LogIn()
        {
            std::cout << LOG_IN_PROMPT;
            ClientContext context;

            // Start bidirectional RPC
            std::shared_ptr<ClientReaderWriter<LogInRequest, LogInReply>>
            stream(stub_->LogIn(&context));
            int success = 1;

            LogInRequest request;
            LogInReply reply;

            // Desired name
            std::string user;
            getline(std::cin, user);

            request.set_user(user);

            // Send request
            stream->Write(request);
            // Read reply
            stream->Read(&reply);

            // If the Write/Read did nothing, then the server is offline
            // and the default enum value was not changed
            if(reply.loginstate() == chatserver::LogInReply::SERVER_OFF)
            {
                std::cout << SERVER_OFFLINE;
                success = 0;
            }
            // Otherwise, there is an error if this block was not skipped
            else if(reply.loginstate() != chatserver::LogInReply::SUCCESS)
            {
                // As long as the user is not picking a valid name
                while(reply.loginstate() != chatserver::LogInReply::SUCCESS)
                {
                    // If the reason is because the name is not completely
                    // alphabetical
                    if(reply.loginstate() == chatserver::LogInReply::INVALID)
                    {
                        std::cout << LOG_IN_INVALID;
                    }
                    // The other reasion is because the name desired is
                    // already being used
                    else
                    {
                        std::cout << LOG_IN_ALREADY;
                    } 
                    // Choose another name
                    std::cout << LOG_IN_CHOOSE_AGAIN;

                    // Again, desired name
                    std::string user;
                    getline(std::cin, user);
                    request.set_user(user);

                    // Send request
                    stream->Write(request);
                    // Read reply
                    stream->Read(&reply);
                }
                // Name is successfully chosen once out of loop
                success = 1;
            }
            // Declare done writing so server can finish RPC
            stream->WritesDone();

            Status status = stream->Finish();

            // Check if RPC completed successfully
            if(status.ok())
                std::cout << LOG_IN_DONE;
            else
                std::cout << LOG_IN_FAIL;

            user_ = user;
            return success;
        }

        /** RPC to get list of users online
         */
        void List()
        {
            ClientContext context;
            CompletionQueue cq;
            Status status;

            ListRequest listRequest;
            ListReply listReply;

            // Send RPC
            std::unique_ptr<ClientAsyncResponseReader<ListReply>> 
            rpc(stub_->AsyncList(&context, listRequest, &cq));
            rpc->Finish(&listReply, &status, (void*)1);

            void* got_tag;
            bool ok = false;
            GPR_ASSERT(cq.Next(&got_tag, &ok));
            GPR_ASSERT(got_tag == (void*)1);
            GPR_ASSERT(ok);

            // Output list
            std::cout << listReply.list();

            // Check if RPC completed successfully
            if(status.ok())
                std::cout << LIST_DONE;
            else
                std::cout << LIST_FAIL;

        }

        
        /** Method to display menu of RPC's
         * @param Chatter: Pointer to address of the client
         * @return int: 1 if user wants to continue, 0 if not
         */
        int displayMenu()
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
                this->LogOut();
                return 0;
            }

            // convert to int
            std::stringstream format(input);
            format >> choice;

            switch(choice)
            {
                // User wants to log out
                case 1:
                    this->LogOut();
                    return 0;
                // User wants to send messages to someone
                case 2:
                    this->SendMessage();    
                    return 1;
                // User wants to receieve messages
                case 3:
                    this->ReceiveMessage();
                    return 1;
                // User wants a list of people on server
                case 4:
                    this->List();
                    return 1;
                case 5: 
                // User wants to chat real time
                    this->Chat();
                    return 1;
                default:
                    std::cout << INVALID_RPC;
                    return 1;
            }
        }

        /** Accessor for name of user
         * @return std::string: name of user
         */
        std::string getUser()
        {
            return user_;
        }    

        /** Initializes all the windows, fields, and items needed for display
         * Heavily reliant on ncurses library
         */
        void initCurses()
        {
            // Pushed back specifically in this order, so no magic numbers
            // needed when assigning function binding to user pointer
            _forwarders.push_back(std::bind(&ChatServerClient::LogOutN, this));
            _forwarders.push_back(std::bind(&ChatServerClient::SendMessageN, this));
            _forwarders.push_back(std::bind(&ChatServerClient::ReceiveMessageN, this));
            _forwarders.push_back(std::bind(&ChatServerClient::ListN, this));
            _forwarders.push_back(std::bind(&ChatServerClient::ChatN, this));
            
           
            //void (*func)() = _forwarder.target<void()>();

            //int (*test)() = _forwarder.target<int*>();
            //_forwarderChat = std::bind(&ChatServerClient::func, this);

            // Names for menu items
            _choices.push_back("Log Out");
            _choices.push_back("Send Messages");
            _choices.push_back("Receive Messages");
            _choices.push_back("List of online users");
            _choices.push_back("Chat");
            _choices.push_back((char*)NULL);

            // initialize ncurses settings
            initscr();
            curs_set(0);
            noecho();
            start_color();
            cbreak();
            keypad(stdscr, TRUE);
            start_color();
            init_pair(RED, COLOR_RED, COLOR_BLACK);
            init_pair(GREEN, COLOR_GREEN, COLOR_BLACK);
            init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);

            // Initialize windows
            _menuWindow = newwin(30, 40, 2, 81);
            _userInputWindow = newwin(10, 80, 22, 1);
            _rpcReceiveWindow = newwin(20, 80, 2, 1);

            
            // Begin FIELD initialization, FIELD(s) are where
            // inputs go within a FORM

            int rows, cols;

            // Initialize fields settings
            // Field dimensions and properties

            _fields[0] = new_field(8, 70,  0, 4, 0, 0);
            field_opts_off(_fields[0], O_AUTOSKIP);
            set_field_back(_fields[0], A_UNDERLINE);

            // The last position in the index must be NULL
            _fields[1] = NULL;

            // Forms for user input
            _form = new_form(_fields);
            scale_form(_form, &rows, &cols);

            keypad(_userInputWindow, TRUE);
            keypad(_menuWindow, TRUE);

            // Connect _form and _userInputWindow window
            set_form_win(_form, _userInputWindow);
            set_form_sub(_form, derwin(_userInputWindow, rows, cols, 1, 1));

            // Begin MENU creation
            // Create and initialize ITEM(s) to exist within a MENU
            _items = (ITEM**)calloc(_choices.size() + 1, sizeof(ITEM *));
            for(int i = 0; i < _choices.size() - 1; i++)
            {
                _items[i] = new_item(_choices[i], NULL);
                std::function<int()>& forwarder = _forwarders[i];
                set_item_userptr(_items[i], &forwarder);
            }
            _items[_choices.size()] = (ITEM *)NULL;

            _menu = new_menu((ITEM **)_items);

            set_menu_win(_menu, _menuWindow);
            set_menu_sub(_menu, derwin(_menuWindow, 6, 38, 3, 1)); 
            set_menu_mark(_menu, " * ");
            box(_menuWindow, 0, 0);
            mvwaddch(_menuWindow, 2, 0, ACS_LTEE);
            mvwhline(_menuWindow, 2, 1, ACS_HLINE, 38);
            mvwaddch(_menuWindow, 2, 50, ACS_RTEE);
            
            // Post FORM on window
            post_form(_form);

            // Post MENU on window
            post_menu(_menu);

            // Draw outline
            box(_rpcReceiveWindow, 0, 0);
            box(_menuWindow, 0, 0);
            box(_userInputWindow, 0, 0);

            // Flavor text
            print(_userInputWindow, Y_RPC_RECEIVE, X_RPC_RECEIVE
                , COLOR_PAIR(RED), "User Input"); 

            print(_rpcReceiveWindow, 0, 7, COLOR_PAIR(RED), "Choose RPC");

            print(_menuWindow, 1, (40 - strlen("RPC Menu"))/2
                , COLOR_PAIR(GREEN), "RPC Menu");

            // Refresh windows
            wrefresh(_userInputWindow);
            wrefresh(_rpcReceiveWindow);
            wrefresh(_menuWindow);
        }

        void takeInput()
        {
            int c;
            while((c = wgetch(_menuWindow)) != KEY_F(1))
            {
                switch(c)
                {
                    case KEY_DOWN:
                        menu_driver(_menu, REQ_DOWN_ITEM);
                        break;

                    case KEY_UP:
                        menu_driver(_menu, REQ_UP_ITEM);
                        break;

                    case KEY_ENTER_ALT:
                    case KEY_ENTER:
                    {
                        // Obtain pointer stored within the ITEM, recast from (void*)
                        auto func = (std::function<int()>*)item_userptr(current_item(_menu));
                        (*func)();
                        break;
                    }
                    default:
                        break;
                }
            }        
        }

    private:

        void print(WINDOW *win, int y, int x, chtype color, const char *string)
        {
            wattron(win, color);
            mvwprintw(win, y, x, "%s", string);
            wattroff(win, color);
        }

        int ChatN()
        {
            // Blinking cursor
            curs_set(1);

            // Recolor text
            print(_userInputWindow, Y_USER_INPUT
                , X_USER_INPUT, COLOR_PAIR(GREEN)
                , "User Input - Type #done to exit");

            wmove(_rpcReceiveWindow, 0, 0);
            wclrtoeol(_rpcReceiveWindow);
            box(_rpcReceiveWindow, 0 , 0);


            print(_rpcReceiveWindow, Y_RPC_RECEIVE
                , X_RPC_RECEIVE, COLOR_PAIR(GREEN), "Chat");

            wrefresh(_rpcReceiveWindow);
            // Move cursor to starting position
            wmove(_userInputWindow, Y_START_TYPE, X_START_TYPE);
            wrefresh(_userInputWindow);

            int ch;
            char *input = "";
            while(ch = wgetch(_userInputWindow))
            {
                switch(ch)
                {
                    case KEY_ENTER:
                    case KEY_ENTER_ALT:
                    {
                        form_driver(_form, REQ_NEXT_FIELD);
                        form_driver(_form, REQ_PREV_FIELD);

                        for(int i = 0; _fields[i]; i++)
                        {
                            print(_rpcReceiveWindow, 2+i, 4, COLOR_PAIR(WHITE), (field_buffer(_fields[i], 0)));
                            input = field_buffer(_fields[i], 0);

                            set_field_buffer(_fields[i], 0, "");
                            //if(!strcmp(input, "#done"))
                            //{
                              //  break;
                            //}
                        }
    
                        wrefresh(_rpcReceiveWindow);
                        pos_form_cursor(_form);
                        break;
                    }
                    case KEY_BACKSPACE:
                    case KEY_BACKSPACE_ALT:
                    {
                        form_driver(_form, REQ_DEL_PREV);
                        break;
                    }
                    default:
                    {
                        // Print characters
                        form_driver(_form, ch);
                        break;
                    }
                }
            }

        }

        int ListN()
        {
        }

        int ReceiveMessageN()
        {
        }

        int SendMessageN()
        {
        }

        int LogInN()
        {
        }

        int LogOutN()
        {
        }

	    std::unique_ptr<ChatServer::Stub> stub_;
        CompletionQueue cq_;
        std::string user_;

		// ncurses
		ITEM **_items;
		MENU *_menu;
		WINDOW *_menuWindow, *_userInputWindow, *_rpcReceiveWindow;
		FIELD *_fields[NUM_FIELDS + 1];
		FORM *_form;
		std::vector<const char *> _choices;

        // Vector to store binding RPC methods
        std::vector<std::function<int()>> _forwarders;
		
};



/** main driver for client
 * @param argc: number of command line args
 * @param argv: array of command line args
 */
int main(int argc, char** argv)
{
    ChatServerClient chatter(grpc::CreateChannel("localhost:50051", 
                             grpc::InsecureChannelCredentials()));
    
    // 1 if successful, 0 if not    
    //int choice = chatter.LogIn();

	//if(choice)
	//{
		chatter.initCurses();
        chatter.takeInput();
	//}
    endwin(); 
 
    return 0;
}

