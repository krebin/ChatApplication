#include <iostream>
#include <string>
#include "QueueChatServer.hpp"
#define MAX_MESSAGE 10

Queue::Queue()
{
    allocateMessages();
    setFront(0);
    setRear(0);
    setNumMessages(0);
}

Queue::~Queue()
{
    delete[] messages_;
}

int Queue::enqueue(std::string message)
{
    if(getNumMessages() == MAX_MESSAGE)
	return 1;
    
    getMessages()[getRear()] = message;
    setRear(getRear() + 1);
    setNumMessages(getNumMessages() + 1);
	return 0;
}

std::string Queue::dequeueAll()
{
    std::string messages;
    if(getNumMessages() == 0)
        messages = "You have no messages.";
    else
        messages = "";
    
    // dqeuue all messages
    while(getNumMessages() != 0)
        messages+=(dequeue());
    
    return messages;
}

std::string* Queue::getMessages()
{
    return messages_;
}

int Queue::getFront()
{
    return front_;
}

int Queue::getRear()
{
    return rear_;
}

int Queue::getNumMessages()
{
    return numMessages_;
}

void Queue::allocateMessages()
{
    messages_ = new std::string[MAX_MESSAGE];
}

void Queue::setFront(int n)
{
    front_ = n % MAX_MESSAGE;
}

void Queue::setRear(int n)
{
    rear_ = n % MAX_MESSAGE;
}

std::string Queue::dequeue()
{
    std::string message = getMessages()[getFront()];

    setFront(getFront() + 1);
    setNumMessages(getNumMessages() - 1);
    return message;
}

void Queue::setNumMessages(int n)
{
    numMessages_ = n;
}


