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
        bool getStatus() const;
        void setStatus(bool online);
        std::string getMessages();
        void addMessage(std::string message);

        /** Overriden == method for unordered_set
         * @return bool: true if names equal, false if not
         */
        bool operator==(const UserNode& other);

        

    private:
        bool online_;
	std::string name_;

	std::queue<std::string> messages_;
};

#endif
