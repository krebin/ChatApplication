#include "QMessageReceiverPane.h"
#include <QScrollBar>

QMessageReceiverPane::QMessageReceiverPane(QWidget* parent)
                    : QPlainTextEdit(parent)
{
    this->setReadOnly(true);
}

QMessageReceiverPane::~QMessageReceiverPane(){}
