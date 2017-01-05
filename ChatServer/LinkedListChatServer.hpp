#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <string>
#include <iostream>
#include "NodeChatServer.hpp"

class LinkedList
{
    public:
        LinkedList();
        ~LinkedList();
        Node* getHead();
	int getNumPeople();
	void insertPerson(std::string name);
	std::string deletePerson(std::string name);
	std::string list();
	Node* search(std::string name);

    private:
	int numPeople;
	Node* head;
	void setHead(Node* n);
	void setNumPeople(int n);
};

#endif
