#include "SignalSender.h"

SignalSender::SignalSender(QObject *parent) : QObject(parent){}

void SignalSender::setCurrentMessage(std::string currentMessage)
{
    _currentMessage = currentMessage;
}

std::string SignalSender::getCurrentMessage() const
{
    return _currentMessage;
}

void SignalSender::emitMessageReceived()
{
    emit messageReceived();
}
