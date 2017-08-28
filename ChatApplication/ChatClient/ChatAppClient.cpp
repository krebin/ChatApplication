#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <utility>
#include <grpc++/grpc++.h>


#include "chatserver.grpc.pb.h"
#include "ChatServerGlobal.h"
#include "ChatAppClient.hpp"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
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

/** ChatServer Client
 * @param channel: channel connecting the client and server
 */
ChatServerClient::ChatServerClient(std::shared_ptr<Channel> channel)
                                 : _stub(ChatServer::NewStub(channel))
                                 , _rpcInProgress(0)
{
    _signalSender = std::make_shared<SignalSender>();
    _logInWindow = std::make_shared<LogInWindow>(this);
    _mainWindow = std::make_shared<MainWindow>(this);


    _logInWindow->setMainWindow(_mainWindow);
    _mainWindow->setLogInWindow(_logInWindow);

}

/** Show the log in window
 */
void ChatServerClient::start()
{
    _logInWindow->show();
}

/** Unary RPC to log out
 * @param user: user to log out
 */
void ChatServerClient::LogOut()
{
    ClientContext context;
    CompletionQueue cq;
    Status status;
    LogOutRequest logOutRequest;
    LogOutReply logOutReply;

    // set user
    logOutRequest.set_user(_user);
    std::unique_ptr<ClientAsyncResponseReader<LogOutReply>>
    rpc(_stub->AsyncLogOut(&context, logOutRequest, &cq));
    rpc->Finish(&logOutReply, &status, (void*)1);

    void* got_tag;
    bool ok = false;
    GPR_ASSERT(cq.Next(&got_tag, &ok));
    GPR_ASSERT(got_tag == (void*)1);
    GPR_ASSERT(ok);

    if(status.ok())
        _logInWindow->setLabelText("Log out successful");
    else
        _logInWindow->setLabelText("Something went wrong logging out");
}

/** Server streaming RPC to receive messages sent to user
 * @param user: Name of user
 */
void ChatServerClient::ReceiveMessage()
{
    _rpcInProgress = true;

    ReceiveMessageRequest request;
    ReceiveMessageReply reply;

    // Set current user to get messages for
    request.set_user(_user);

    ClientContext context;
    // Start server streaming RPC
    // also sends first request
    std::shared_ptr<ClientReader<ReceiveMessageReply>>
    reader(_stub->ReceiveMessage(&context, request));

    // Read reply
    reader->Read(&reply);

    // If the message queue was empty to begin with
    if(reply.queuestate() == chatserver::ReceiveMessageReply::EMPTY)
    {
        _mainWindow->appendMessage("No new messages");
    }
    // Else run as normal
    else
    {
        // Will terminate once there are no more messages to read
        while(reader->Read(&reply))
        {
            _mainWindow->appendMessage(reply.messages());
        }
    }

    Status status;
    status = reader->Finish();

    // Check if RPC finished successfully
    if(status.ok())
        _mainWindow->setRpcStateLabelText("Messages received");
    else
        _mainWindow->setRpcStateLabelText("Something went wrong");

    _rpcInProgress = false;
}

/** Bidirectional RPC to privately message another user
 */
void ChatServerClient::SendMessage()
{
    _rpcInProgress = true;

    ClientContext context;
    // Start bidirectional RPC
    std::shared_ptr<ClientReaderWriter<SendMessageRequest, SendMessageReply>>
    stream(_stub->SendMessage(&context));

    // Ask for recipient
    _mainWindow->appendMessage("Send messages to who? Type in input box");

    std::string recipient = _mainWindow->waitForMessageBoxInput();

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
        _mainWindow->appendMessage("Now sending messages");
        std::string message;

        // Will break when some sort of quit signal comes from _mainWindow
        while(true)
        {
            message = _mainWindow->waitForMessageBoxInput();
                    
            // Set request parameters
            request.set_user(_user);
            request.set_recipient(recipient);
            request.set_messages(message);
            request.set_requeststate(chatserver::SendMessageRequest::PROCESSING);                     

            if(!(_mainWindow->getRpcQuitRequest())
            && !(_mainWindow->getAppQuitRequest()))
            {
                stream->Write(request);
                stream->Read(&reply);
                _mainWindow->appendMessage("Message sent");
            }
            else
            {
                // Declare writes done so server can finish RPC
                stream->WritesDone();
                // Reset quit flags
                _mainWindow->setRpcQuitRequest(false);
                _mainWindow->setAppQuitRequest(false);
                break;
            }
        }
    }
    else
    {
        // Declare writes done so server can finish RPC
        stream->WritesDone();
    }

    _mainWindow->appendMessage("Done sending messages");
    Status status = stream->Finish();

    // Check that RPC finished successfully
    if(!status.ok())
        _mainWindow->setRpcStateLabelText("Something went wrong");
    else
        _mainWindow->setRpcStateLabelText("Messages sent successfully");

    _rpcInProgress = false;
}

/** Bidirectional RPC for user to chat real time with other users
 * @param user: name of user
 */
void ChatServerClient::Chat()
{
    _rpcInProgress = true;

    // Start bidirectional RPC
    ClientContext context;
    std::shared_ptr<ClientReaderWriter<ChatMessage, ChatMessage>>
    stream(_stub->Chat(&context));
    auto mainWindow = _mainWindow;
    auto signalSender = _signalSender;

    // lambda function to read new messages repeatedly
    // started in another thread as to not block the
    // main thread from reading in messages
    std::thread reader([stream, mainWindow, signalSender]()
    {
            ChatMessage server_note;
            // While loop will terminate once the stream
            // returns termination signal, either when
            // this method reaches stream->WritesDone()
            // or stream->Finish()
            while(stream->Read(&server_note))
            {
                std::string string = "[" + server_note.user() + "]: "
                                   + server_note.messages();

                signalSender->setCurrentMessage(string);
                signalSender->emitMessageReceived();
            }
    });

    // Until the user sends the break string
    while(true)
    {
        std::string message = _mainWindow->waitForMessageBoxInput();

        if(!(_mainWindow->getRpcQuitRequest())
        && !(_mainWindow->getAppQuitRequest()))
        {
            _mainWindow->appendMessage("[" + _user + "]: " + message);
            stream->Write(createChatMessage(message, _user));
        }
        else
        {
            // Declare writes done so server can finish RPC
            stream->WritesDone();

            // Reset quit flags
            _mainWindow->setRpcQuitRequest(false);
            _mainWindow->setAppQuitRequest(false);
            break;
        }
    }

    // Wait for lamba thread to finish
    reader.join();
    Status status = stream->Finish();

    // Check is RPC finished successfully
    if(status.ok())
        _mainWindow->setRpcStateLabelText("Chat finished");

    else
        _mainWindow->setRpcStateLabelText("Something went wrong");

    _rpcInProgress = false;
}

/** RPC to log in to server
 * @return int: 0 if failed to log in, 1 if successful
 */
bool ChatServerClient::LogIn()
{
    // Wait for LogIn button to be pressed so there's a name to check
    std::string user = (_logInWindow->getUser()).toStdString();

    // If the quit button was clicked, then end the method early
    if(_logInWindow->quitAttempted())
        return false;

    ClientContext context;

    // Start bidirectional RPC
    std::shared_ptr<ClientReaderWriter<LogInRequest, LogInReply>>
    stream(_stub->LogIn(&context));
    bool success = true;

    LogInRequest request;
    LogInReply reply;

    request.set_user(user);
    // Send request
    stream->Write(request);
    // Read reply
    stream->Read(&reply);

    // If the Write/Read did nothing, then the server is offline
    // and the default enum value was not changed
    if(reply.loginstate() == chatserver::LogInReply::SERVER_OFF)
    {
        success = false;
        _logInWindow->setInitialLogIn(true);
    }
    else if(reply.loginstate() == chatserver::LogInReply::SUCCESS)
    {
        _logInWindow->setLabelText("No errors");
        _logInWindow->logInSuccessful();
        success = true;
    }
    // Otherwise, there is an error if this block was not skipped
    else
    {
        // As long as the user is not picking a valid name
        while(reply.loginstate() != chatserver::LogInReply::SUCCESS)
        {
            // If the reason is because the name is not completely
            // alphabetical
            if(reply.loginstate() == chatserver::LogInReply::INVALID)
            {
                _logInWindow->setLabelText("Invalid name");
            }
            // The other reasion is because the name desired is
            // already being used
            else
            {
                _logInWindow->setLabelText("User already logged in");
            }
            // Again, desired name
            user = (_logInWindow->waitForLogInButtonPress())
                   .toStdString();

            // If the quit button was clicked, then end the method early
            if(_logInWindow->quitAttempted())
                return false;

            request.set_user(user);
            // Send request
            stream->Write(request);
            // Read reply
            stream->Read(&reply);
        }
        // Name is successfully chosen once out of loop
        _logInWindow->setLabelText("No errors");
        _logInWindow->logInSuccessful();
        success = true;
    }
    // Declare done writing so server can finish RPC
    stream->WritesDone();
    Status status = stream->Finish();

    // Check if RPC completed successfully
    if(status.ok())
        _mainWindow->setRpcStateLabelText("Log in successful");
    else
        _logInWindow->setLabelText("Something went wrong logging in");

    _user = user;
    return success;
}

/** RPC to get list of users online
 */
void ChatServerClient::List()
{
    _rpcInProgress = true;

    ClientContext context;
    CompletionQueue cq;
    Status status;

    ListRequest listRequest;
    ListReply listReply;

    // Send RPC
    std::unique_ptr<ClientAsyncResponseReader<ListReply>>
    rpc(_stub->AsyncList(&context, listRequest, &cq));
    rpc->Finish(&listReply, &status, (void*)1);

    void* got_tag;
    bool ok = false;
    GPR_ASSERT(cq.Next(&got_tag, &ok));
    GPR_ASSERT(got_tag == (void*)1);
    GPR_ASSERT(ok);

    // Output list
    _mainWindow->appendMessage(listReply.list());

    // Check if RPC completed successfully
    if(status.ok())
        _mainWindow->setRpcStateLabelText("List posted");
    else
        _mainWindow->setRpcStateLabelText("Something went wrong listing");

    _rpcInProgress = false;
}

/** Accessor for _user member
 * @return std::string: name of user
 */
std::string ChatServerClient::getUser() const
{
    return _user;
}

/** Accessor for shared_ptr of LogInWindow
 * @return shared_ptr<LogInWindow>: shared ptr to LogInWindow object
 */
std::shared_ptr<LogInWindow> ChatServerClient::getLogInWindow() const
{
    return _logInWindow;
}

/** Accessor for shared_ptr of MainWindow
 * @return shared_ptr<LogInWindow>: shared ptr to MainWindow object
 */
std::shared_ptr<MainWindow> ChatServerClient::getMainWindow() const
{
    return _mainWindow;
}

/** Accessor for _signalSender member
 * @return shared_ptr<SignalSender>: shared ptr to SignalSender object
 */
std::shared_ptr<SignalSender> ChatServerClient::getSignalSender() const
{
    return _signalSender;
}

/** Accessor for _rpcInProgress member
 * @return bool: true if rpc in progress, false if not
 */
bool ChatServerClient::rpcInProgress() const
{
    return _rpcInProgress;
}
