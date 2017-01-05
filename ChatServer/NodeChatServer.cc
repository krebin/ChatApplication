#include <iostream>
#include <string>
#include "NodeChatServer.hpp"
#include "QueueChatServer.hpp"

Node::Node(std::string name)
{
    setName(name);
    setNext(NULL);
    setPrev(NULL);
    allocateQueue();
}

Node::~Node()
{
    delete messages;
}

std::string Node::getName()
{
    return name;
}

Queue* Node::getMessages()
{
    return messages;
}

int Node::getNumMessages()
{
    return numMessages;
}

Node* Node::getNext()
{
    return next;
}

Node* Node::getPrev()
{
    return prev;
}

void Node::setPrev(Node* n)
{
    prev = n;
}

void Node::setNext(Node* n)
{
    next = n;
}

void Node::setName(std::string n)
{
    name = n;
}

void Node::setNumMessages(int num)
{
    numMessages = num;
}

void Node::allocateQueue()
{
    messages = new Queue();
}
