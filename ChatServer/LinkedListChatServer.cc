#include <iostream>
#include <string>
#include "LinkedListChatServer.hpp"
#include "NodeChatServer.hpp"
#define MAX_MESSAGE 10

/*
 * LinkedList
 * Basic data structure to link users
 */
LinkedList::LinkedList()
{
    setNumPeople(0);
}

LinkedList::~LinkedList()
{
// delete every node in linked list chain
    Node* curNode = getHead();
    Node* nextNode;
    while(curNode!=NULL)
    {
        nextNode = curNode->getNext();
        delete curNode;
        curNode = nextNode;
    }
}

Node* LinkedList::getHead()
{
    return head;
}

int LinkedList::getNumPeople()
{
    return numPeople;
}

void LinkedList::insertPerson(std::string name)
{
    Node* addNode = new Node(name);
	if(!getNumPeople())
	    setHead(addNode);
	else
        {
	    Node* curNode = getHead();
	    while(curNode->getNext() != NULL)
		curNode = curNode->getNext();

            curNode->setNext(addNode);
	    addNode->setPrev(curNode);
	}
        setNumPeople(getNumPeople() + 1);
}

std::string LinkedList::deletePerson(std::string name)
{
    std::string n;
    if((getNumPeople() == 1) && (getHead()->getName() == name))
    {
        n = getHead()->getName();
        delete getHead();
	setHead(NULL);
	setNumPeople(getNumPeople() - 1);
	return n;
    }

     Node* curNode = getHead();
	   
     while(curNode->getName() != name)
	curNode = curNode->getNext();
     Node* deleteNode = curNode; 
	    
    n = curNode->getName();
    if(curNode->getPrev() != NULL && curNode->getNext() != NULL)
    {
        curNode->getPrev()->setNext(curNode->getNext());
        curNode->getNext()->setPrev(curNode->getPrev());
    }
    else if(curNode->getNext() == NULL)
    {
        curNode->getPrev()->setNext(NULL);
    }
    else if(curNode->getPrev() == NULL)
    {
        if(curNode == getHead())
            setHead(curNode->getNext());
	curNode->getNext()->setPrev(NULL);
    }

    setNumPeople(getNumPeople() - 1);
    delete deleteNode;
    return n;
}

std::string LinkedList::list()
{
    std::string people;
    Node* curNode = getHead();
    while(curNode->getNext())
    {
        people+=(curNode->getName() + ", ");
	curNode = curNode->getNext();
    }

    people+=(curNode->getName());
    return people;
}
	
Node* LinkedList::search(std::string name)
{
    Node* curNode = getHead();
    while(curNode)
    {
        if(curNode->getName() == name)
        {
	    return curNode;
        }
	curNode = curNode->getNext();
    }
    return NULL;
}

void LinkedList::setHead(Node* n)
{
    head = n;
}

void LinkedList::setNumPeople(int n)
{
    numPeople = n;
}


