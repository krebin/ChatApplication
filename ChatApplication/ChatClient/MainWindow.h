#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "LogInWindow.h"
#include "SignalSender.h"
#include "ChatAppClient.hpp"

static QString SERVICE_IN_PROGRESS = "A service is currently in\nprogress";

// Forward declaration
class ChatServerClient;

namespace Ui
{
    class MainWindow;
}

class LogInWindow;
class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0);
        MainWindow(ChatServerClient *client);
        ~MainWindow();

        void setClient(ChatServerClient *client);
        void setLogInWindow(std::shared_ptr<LogInWindow> logInWindow);
        void setRpcStateLabelText(QString text);

        std::string waitForMessageBoxInput();
        void processMessage();
        void appendMessage(std::string message);

        bool getRpcQuitRequest() const;
        bool getAppQuitRequest() const;

        void setRpcQuitRequest(bool rpcQuitRequest);
        void setAppQuitRequest(bool appQuitRequest);

        signals:
            void loggingOff();
            void showLogInWindow();
            void messageProcessed();
            void appQuitButtonPressed();
            void rpcQuitButtonPressed();

    private slots:
        void on_logOffButton_clicked();
        void on_chatButton_clicked();

        void on_doneButton_clicked();

        void on_listButton_clicked();

        void on_receiveMessageButton_clicked();

        void on_sendMessageButton_clicked();

    private:
            std::shared_ptr<LogInWindow> _logInWindow;
            std::shared_ptr<SignalSender> _signalSender;
            Ui::MainWindow *ui;
            ChatServerClient *_client;

            void closeEvent(QCloseEvent *event);
            std::string _currentMessage;

            bool _rpcQuitRequest, _appQuitRequest;
};

#endif // MAINWINDOW_H
