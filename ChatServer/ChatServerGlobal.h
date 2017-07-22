#ifndef CSSTRINGS_H
#define CSSTRINGS_H


#define ALPHA_START 65
#define ALPHA_END 122



static const std::string DONE = "#done";

static const std::string CHAT_PROMPT = "Now chatting, type anything. Enter \"#done\" to end the Chat.\n\n";

static const std::string CHAT_DONE = "Chat RPC finished.\n\n";

static const std::string CHAT_FAIL = "Chat RPC failed.\n\n";

static const std::string SEND_MESSAGES_WHO = "Send messages to: ";

static const std::string SEND_MESSAGE_PROMPT = "Now sending messages. Enter \"#done\" to stop sending messages.\n\n";

static const std::string SEND_MESSAGE_EXIST = "EXISTS";

static const std::string SEND_MESSAGE_NO_EXIST = "The user does not exist.\n\n";

static const std::string SEND_MESSAGE_CONFIRM = "All messages have been sent to ";

static const std::string SEND_MESSAGE_DONE = "SendMessage RPC finished.\n\n";

static const std::string SEND_MESSAGE_FAIL = "SendMessage RPC failed.\n\n";

static const std::string LOG_OUT_CONFIRM = "Successfully logged out.\n\n";

static const std::string LOG_OUT_DONE = "LogOut RPC finished.\n\n";

static const std::string LOG_OUT_FAIL = "LogOut RPC failed.\n\n";

static const std::string RECEIVE_MESSAGE_NONE = "No new messages.\n\n";

static const std::string RECEIVE_MESSAGE_EMPTY = "EMPTY";

static const std::string RECEIVE_MESSAGE_DONE = "ReceiveMessage RPC finished.\n\n";

static const std::string RECEIVE_MESSAGE_FAIL = "ReceiveMessage RPC failed.\n\n";

#endif
