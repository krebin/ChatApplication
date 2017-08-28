#ifndef CHATSERVER_CLIENT_HPP
#define CHATSERVER_CLIENT_HPP

#include <grpc++/grpc++.h>
#include "chatserver.grpc.pb.h"
#include "LogInWindow.h"
#include "MainWindow.h"
#include "SignalSender.h"

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

// forward declaration
class LogInWindow;
class MainWindow;
class ChatServerClient
{
    public:
        explicit ChatServerClient(std::shared_ptr<Channel> channel);

        std::shared_ptr<LogInWindow> getLogInWindow() const;
        std::shared_ptr<MainWindow> getMainWindow() const;

        std::string getUser() const;
        void start();
        void List();
        void LogOut();
        void SendMessage();
        void ReceiveMessage();
        void Chat();
        bool LogIn();
        bool rpcInProgress() const;
        std::shared_ptr<SignalSender> getSignalSender() const;



    private:
        std::shared_ptr<LogInWindow> _logInWindow;
        std::shared_ptr<MainWindow> _mainWindow;
        std::shared_ptr<SignalSender> _signalSender;
        std::unique_ptr<ChatServer::Stub> _stub;

        CompletionQueue cq_;
        std::string _user;
        bool _rpcInProgress;
};

#endif // CHATSERVER_CLIENT_HPP
