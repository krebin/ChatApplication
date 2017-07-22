#include <iostream>
#include "UserNode.hpp"
#include "ChatServerGlobal.h"

/** Node Constructor **/
UserNode::UserNode(std::string name): name_(name),
                                      online_(true){}

/** Accessor method for name
 * @return string: name_ member
 */
std::string UserNode::getName() const
{
    return name_;
}

/** Get messages
 * @return messages sent to user
 */
std::pair<UserNode::QUEUE_STATE, std::string> UserNode::getMessage()
{
    std::string message;
    // Return state of message queue and read message
    std::pair<UserNode::QUEUE_STATE, std::string> pair;

    // If queue empty
    if(!messages_.size())
    {
        message = RECEIVE_MESSAGE_EMPTY;
        pair.first = UserNode::QUEUE_STATE::EMPTY;
    }
    // If queue not empty
    else
    {
        // Dequeue single message
        message = messages_.front();
        messages_.pop();
        pair.first = UserNode::QUEUE_STATE::NON_EMPTY;
    }

    pair.second = message;
    return pair;
}

/** Mutator method for online status
 * @param bool online: true if online, false if offline
 */
void UserNode::setOnline(bool online)
{
    online_ = online;
}

/** Accessor method for online status
 * @return online_ member
 */
bool UserNode::getOnline() const
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

