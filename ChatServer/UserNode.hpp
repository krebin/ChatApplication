#ifndef NODE_H
#define NODE_H

#include <string>
#include <iostream>
#include <queue>

class UserNode
{
    public:
        UserNode(std::string name);
        std::string getName() const;
        bool getOnline() const;
        void setOnline(bool online);
        std::string getMessages();
        void addMessage(std::string message);


    private:
        bool online_;
    	std::string name_;
	    std::queue<std::string> messages_;
};

#endif
