#include "QMessageInputBox.h"

QMessageInputBox::QMessageInputBox(QWidget* parent): QTextEdit(parent){}

QMessageInputBox::~QMessageInputBox(){}

void QMessageInputBox::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Return)
    {
        _mainWindow->processMessage();
    }
    else
    {
        QTextEdit::keyPressEvent(event);
    }
}

void QMessageInputBox::setMainWindow(MainWindow *mainWindow)
{
    _mainWindow = mainWindow;
}
