#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>
#include <MainWindow.h>
#include "ChatAppClient.hpp"

// forward declaration
class MainWindow;
class ChatServerClient;

namespace Ui
{
    class LogInWindow;
}

class LogInWindow : public QMainWindow
{
    Q_OBJECT

    public:
        explicit LogInWindow(QWidget *parent = 0);
        LogInWindow(ChatServerClient *client);
        ~LogInWindow();

        void setInitialLogIn(bool initialLogIn);
        void setClient(ChatServerClient *client);
        void setLabelText(QString text);
        void logInSuccessful();
        void setMainWindow(std::shared_ptr<MainWindow> mainWindow);

        QString waitForLogInButtonPress();   
        QString getUser() const;
        bool quitAttempted() const;


    signals:
        void logInButtonPressed();
        void quitButtonPressed();

    private slots:
        void on_pushButtonLogIn_clicked();
        void onMainWindowLogOff();

    private:
        bool _quit;
        bool _initialLogIn;

        void closeEvent(QCloseEvent *event);
        QString _user;
        ChatServerClient *_client;
        Ui::LogInWindow *ui;

        std::shared_ptr<MainWindow> _mainWindow;
};

#endif // LOGINWINDOW_H
