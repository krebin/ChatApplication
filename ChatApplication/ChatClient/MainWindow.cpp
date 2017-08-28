#include "MainWindow.h"
#include "ui_mainwindow.h"
#include <QCloseEvent>
#include <grpc++/grpc++.h>

/** Auto-generated constructor
 * @param QWidget *parent: Parent pointer if needed, constructor is explicit
 *                         so there need not be a parent
 */
MainWindow::MainWindow(QWidget *parent)
                     : QMainWindow(parent)
                     , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

/** Constructor with chat client parameter
 *  @param ChatServerClient *client: pointer to client to be used
 */
MainWindow::MainWindow(ChatServerClient *client)
                       : _client(client)
                       , ui(new Ui::MainWindow)
                       , _appQuitRequest(0)
                       , _rpcQuitRequest(0)
{
    ui->setupUi(this);
    ui->messageInputBox->setMainWindow(this);
    _signalSender = _client->getSignalSender();

    connect(_signalSender.get(), &SignalSender::messageReceived
          , this, [this]{appendMessage(_signalSender->getCurrentMessage());});

    this->setFixedSize(639, 338);
}

/** Destructor
 */
MainWindow::~MainWindow()
{
    delete ui;
}

/** Wait for an input to be sent to server
 * @return QString: string to be sent
 */
std::string MainWindow::waitForMessageBoxInput()
{
    QEventLoop loop;

    // Only exit the loop once a message is able to be sent
    // Or user wants to quit the app
    connect(this, SIGNAL(messageProcessed()), &loop, SLOT(quit()));
    connect(this, SIGNAL(appQuitButtonPressed()), &loop, SLOT(quit()));
    connect(this, SIGNAL(rpcQuitButtonPressed()), &loop, SLOT(quit()));
    // Run loop
    loop.exec();
    disconnect(this, SIGNAL(messageProcessed()), &loop, SLOT(quit()));
    disconnect(this, SIGNAL(appQuitButtonPressed()), &loop, SLOT(quit()));
    disconnect(this, SIGNAL(rpcQuitButtonPressed()), &loop, SLOT(quit()));

    return _currentMessage;
}

/** Changes what happens when clicking "x" button in top right
 *  to what happens when the log off button is clicked
 *  @param QCloseEvent *event: Pointer to closing event
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    // Ignore normal close
    event->ignore();

    // Signal that user wants to quit
    if(_client->rpcInProgress())
    {
        _appQuitRequest = true;
        emit appQuitButtonPressed();
    }

    // Manual quit
    // Reset all text
    ui->messageInputBox->clear();
    ui->messageReceiverPane->clear();
    ui->rpcStateLabel->setText("");
    this->setRpcStateLabelText("");

    // Hide main window, show log in window
    this->hide();
    _logInWindow->show();
    _logInWindow->setInitialLogIn(1);

    // LogOut RPC
    _client->LogOut();

}

/** Change the text of the RPC label in the main window
 * @param QString text: The text to be changed to
 */
void MainWindow::setRpcStateLabelText(QString text)
{
    ui->rpcStateLabel->setText(text);
}

/** Mutator method for _logInWindow member
 * @param std::shared_ptr<LogInWindow> logInWindow: shared ptr to a LogInWindow
 */
void MainWindow::setLogInWindow(std::shared_ptr<LogInWindow> logInWindow)
{
    _logInWindow = logInWindow;
}

/** Slot to take text from message box
 */
void MainWindow::processMessage()
{
    // Convert to QString to std::string
    std::string text = ui->messageInputBox->toPlainText().toStdString();
    // Clear the box
    ui->messageInputBox->clear();
    std::cout << text << std::endl;
    _currentMessage = text;
    emit messageProcessed();
}

/** Method to append message to messageReceiver
 */
void MainWindow::appendMessage(std::string message)
{
    // Convert to QString
    QString qString = QString::fromStdString(message);
    // Append to pane
    ui->messageReceiverPane->appendPlainText(qString);
}

/** Accessor method for _rpcQuitRequest member
 * @return int: 0 if none was requested, 1 if it was requested
 */
bool MainWindow::getRpcQuitRequest() const
{
    return _rpcQuitRequest;
}

/** Accessor method for _appQuitRequest member
 * @return int: 0 if none was requested, 1 if it was requested
 */
bool MainWindow::getAppQuitRequest() const
{
    return _appQuitRequest;
}

/** Mutator method for _rpcQuitRequest member
 * @param int: Value to set it to
 */
void MainWindow::setRpcQuitRequest(bool rpcQuitRequest)
{
    _rpcQuitRequest = rpcQuitRequest;
}

/** Mutator method for _appQuitRequest member
 * @param int: Value to set it to
 */
void MainWindow::setAppQuitRequest(bool appQuitRequest)
{
    _appQuitRequest = appQuitRequest;
}

void MainWindow::on_doneButton_clicked()
{
    if(_client->rpcInProgress())
    {
        emit rpcQuitButtonPressed();
        ui->messageReceiverPane->clear();
        _rpcQuitRequest = true;
    }
}

/** Slot to start List RPC which lists people online
 */
void MainWindow::on_listButton_clicked()
{
    if(!(_client->rpcInProgress()))
    {
        _client->List();
    }
    else
    {
        ui->rpcStateLabel->setText(SERVICE_IN_PROGRESS);
    }
}

/** Resets main window features, logs off user, shows LogInWindow
 */
void MainWindow::on_logOffButton_clicked()
{
    if(!(_client->rpcInProgress()))
    {
        // Reset window features
        ui->messageInputBox->clear();
        ui->messageReceiverPane->clear();
        ui->rpcStateLabel->setText("");
        this->setRpcStateLabelText("");
        this->hide();

        // LogOut RPC
        _client->LogOut();
        _logInWindow->show();
        _logInWindow->setInitialLogIn(1);
    }
    else
    {
        ui->rpcStateLabel->setText(SERVICE_IN_PROGRESS);
    }
}

/** Slot to start Chat RPC from client
 */
void MainWindow::on_chatButton_clicked()
{
    if(!(_client->rpcInProgress()))
    {
        this->ui->rpcStateLabel->setText("Chatting\nPress the 'X' to stop");
        this->ui->messageReceiverPane->clear();
        this->ui->messageReceiverPane->appendPlainText("Chat room entered\n");
        _client->Chat();
    }
    else
    {
        this->ui->rpcStateLabel->setText(SERVICE_IN_PROGRESS);
    }
}

void MainWindow::on_receiveMessageButton_clicked()
{
    if(!(_client->rpcInProgress()))
    {
        this->ui->rpcStateLabel->setText("Receiving messages");
        this->ui->messageReceiverPane->clear();
        this->ui->messageReceiverPane->appendPlainText("Receiving messages\n");
        _client->ReceiveMessage();
    }
    else
    {
        this->ui->rpcStateLabel->setText(SERVICE_IN_PROGRESS);
    }
}

void MainWindow::on_sendMessageButton_clicked()
{
    if(!(_client->rpcInProgress()))
    {
        this->ui->rpcStateLabel->setText("Sending messages");
        this->ui->messageReceiverPane->clear();
        _client->SendMessage();
    }
    else
    {
        this->ui->rpcStateLabel->setText(SERVICE_IN_PROGRESS);
    }
}
