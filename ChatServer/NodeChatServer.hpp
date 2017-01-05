#ifndef NODE_H
#define NODE_H

#include <string>
#include <iostream>
#include "QueueChatServer.hpp"

class Node
{
    public:
        Node(std::string name);
        ~Node();
        std::string getName();
	Queue* getMessages();
	int getNumMessages();
	Node* getPrev();
	Node* getNext();
	void setNext(Node* n);
	void setPrev(Node* n);

    private:
	std::string name;
	Queue* messages;
	int numMessages;
	Node* next;
	Node* prev;

	void setName(std::string n);
	void setNumMessages(int num);
	void allocateQueue();
};

#endif
