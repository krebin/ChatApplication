#ifndef NODE_H
#define NODE_H

#include <string>
#include <iostream>
#include <queue>


class UserNode
{
    public:
    
        enum class QUEUE_STATE {EMPTY, NON_EMPTY};
        UserNode(std::string name);
        std::string getName() const;
        bool getOnline() const;
        void setOnline(bool online);
        std::pair<UserNode::QUEUE_STATE, std::string> getMessage();
        void addMessage(std::string message);


    private:
        bool online_;
    	std::string name_;
	    std::queue<std::string> messages_;
};

#endif
