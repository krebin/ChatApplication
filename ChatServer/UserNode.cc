#include <iostream>
#include "UserNode.hpp"

/** Node Constructor **/
UserNode::UserNode(std::string name): name_(name),
                                      online_(true){}

/** Accessor method for name
 * @return string: name_ member
 */
std::string UserNode::getName()
{
    return name_;
}

/** Get messages
 * @return messages sent to user
 */
std::string UserNode::getMessages()
{
    std::string messages = "";
    // dequeue messages and append to empty string
    while(messages_.size())
    {
        messages+=messages_.front();
        messages_.pop();
    }
    return messages;
}

/** Mutator method for online status
 * @param bool online: true if online, false if offline
 */
void UserNode::setStatus(bool online)
{
    online_ = online;
}

/** Accessor method for online status
 * @return online_ member
 */
bool UserNode::getStatus()
{
    return online_;
}

/** Add a message to the message queue
 * @param std::string message: message to add
 */
void UserNode::addMessage(std::string message)
{
    messages_.push(message);
}
