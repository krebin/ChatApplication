#ifndef CSSTRINGS_H
#define CSSTRINGS_H

enum class LOG_IN_TYPES {INVALID, ALREADY, SUCCESS};

static const std::string DONE = "#done";

static const std::string CHAT_PROMPT = "Now chatting, type anything. Enter \"#done\" to end the Chat.\n\n";

static const std::string CHAT_DONE = "Chat RPC finished.\n\n";

static const std::string SEND_MESSAGES_WHO = "Send messages to: ";

static const std::string SEND_MESSAGES_PROMPT = "Now sending messages. Enter \"#done\" to stop sending messages.\n\n";

static const std::string SEND_MESSAGES_EXIST = "EXISTS";

static const std::string SEND_MESSAGES_NO_EXIST = "The user does not exist. Send messages ending.\n\n";

static const std::string SEND_MESSAGES_CONFIRM = "All messages have been sent to ";

static const std::string SEND_MESSAGES_DONE = "SendMessage RPC finished.\n\n";

static const std::string LOG_OUT_CONFIRM = "Successfully logged out.\n\n";

static const std::string LOG_OUT_DONE = "LogOut RPC finished.\n\n";

static const std::string LOG_OUT_FAIL = "LogOut RPC failed.\n\n";

#endif
