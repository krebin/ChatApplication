#ifndef QUEUE_H
#define QUEUE_H

#include <iostream>
#include <string>

class Queue
{
    public:
	Queue();
	~Queue();
	int enqueue(std::string);
	std::string dequeueAll();
	std::string* getMessages();
	int getFront();
	int getRear();
	int getNumMessages();
    
    private:
	std::string* messages_;
	int front_;
	int rear_;
	int numMessages_;

	void allocateMessages();
	void setFront(int n);
	void setRear(int n);
	std::string dequeue();
	void setNumMessages(int n);
};
#endif
