#include "LogInWindow.h"
#include "ui_loginwindow.h"
#include "MainWindow.h"
#include <vector>
#include <grpc++/grpc++.h>
#include <QCloseEvent>

LogInWindow::LogInWindow(QWidget *parent)
                      : QMainWindow(parent)
                      , ui(new Ui::LogInWindow)
                      , _quit(false)
                      , _initialLogIn(true)
{
    ui->setupUi(this);
}

LogInWindow::LogInWindow(ChatServerClient *client)
                      : _client(client)
                      , ui(new Ui::LogInWindow)
                      , _quit(false)
                      , _initialLogIn(true)
{
    ui->setupUi(this);
    this->setFixedSize(400, 242);
    this->setWindowTitle(" ");
}

LogInWindow::~LogInWindow()
{
    delete ui;
}

void LogInWindow::setClient(ChatServerClient *client)
{
    _client = client;
}

void LogInWindow::on_pushButtonLogIn_clicked()
{
    _user = ui->lineEditUserName->text();
    ui->lineEditUserName->clear();

    if(_initialLogIn)
    {
        _initialLogIn = 0;
        _client->LogIn();
    }
    else
    {
        emit logInButtonPressed();
    }
}

void LogInWindow::logInSuccessful()
{
    this->hide();
    _mainWindow->show();
}

void LogInWindow::onMainWindowLogOff()
{
    // Reset log in state
    _initialLogIn = 1;
}

void LogInWindow::setLabelText(QString text)
{
    ui->stateLabel->setText(text);
}

void LogInWindow::setMainWindow(std::shared_ptr<MainWindow> mainWindow)
{
    _mainWindow = mainWindow;
}

QString LogInWindow::waitForLogInButtonPress()
{
    QEventLoop loop;
    // Only exit the loop once a name is available to be checked
    // Or user wants to quit the app
    connect(this, SIGNAL(logInButtonPressed()), &loop, SLOT(quit()));
    connect(this, SIGNAL(quitButtonPressed()), &loop, SLOT(quit()));
    // Run loop
    loop.exec();
    disconnect(this, SIGNAL(logInButtonPressed()), &loop, SLOT(quit()));
    disconnect(this, SIGNAL(quitButtonPressed()), &loop, SLOT(quit()));

    return _user;
}

void LogInWindow::closeEvent(QCloseEvent *event)
{
    // Ignore normal close
    event->ignore();

    // Signal that user wants to quit
    _quit = 1;
    emit quitButtonPressed();

    // Manual quit
    QApplication::quit();
}

bool LogInWindow::quitAttempted() const
{
    return _quit;
}

QString LogInWindow::getUser() const
{
    return _user;
}

void LogInWindow::setInitialLogIn(bool initialLogIn)
{
    _initialLogIn = initialLogIn;
}
