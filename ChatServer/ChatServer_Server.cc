#include <memory>
#include <iostream>
#include <string>
#include <thread>

#include <grpc++/grpc++.h>
#include "chatserver.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;

using chatserver::ChatServer;

using chatserver::LogInRequest;
using chatserver::LogInReply;
using chatserver::LogOutRequest;
using chatserver::LogOutReply;
using chatserver::SendMessageRequest;
using chatserver::SendMessageReply;
using chatserver::ReceiveMessageRequest;
using chatserver::ReceiveMessageReply;
using chatserver::ListRequest;
using chatserver::ListReply;

class LinkedList
{
    public:

        class Node
	{
	    public:

                class Queue
	        {
		    public:
			Queue()
			{
			    allocateMessages();
			    setFront(0);
			    setRear(0);
			}

			
			void enqueue(std::string message)
			{
			    getMessages()[getFront()] = message;
			    setRear(getRear() + 1);
			}

			std::string dequeueAll()
			{
			    std::string messages = "";
			    while(getFront() != getRear())
			    {
			        messages+=(dequeue());
			    }
			    return messages;
			}

			std::string* getMessages()
			{
			    return messages;
			}

			int getFront()
			{
			    return front;
			}
	
			int getRear()
			{
			    return rear;
			}

		    private:
		        std::string* messages;
			int front;
			int rear;

			void allocateMessages()
			{
			    messages = new std::string[10];
			}
			void setFront(int n)
			{
			    front = n%10;
			}
			void setRear(int n)
			{
			    rear = n%10;
			}
			std::string dequeue()
			{
			    std::string message = getMessages()[getFront()];
			    setFront(getFront() + 1);
			    return message;
			}

	        };

		Node(std::string name)
		{
		    setName(name);
		    setNext(NULL);
		    setPrev(NULL);
		    allocateQueue();
		}

		std::string getName()
		{
		    return name;
		}

		Queue* getMessages()
		{
		    return messages;
		}

		int getNumMessages()
		{
		    return numMessages;
		}

		Node* getNext()
	        {
		    return next;
		}

		Node* getPrev()
		{
		    return prev;
		}

		void setNext(Node* n)
		{
		    next = n;
		}

		void setPrev(Node* n)
		{
		    prev = n;
		}

	    private:
	        std::string name;
		Queue* messages;
		int numMessages;
	        Node* next;
		Node* prev;

		void setName(std::string n)
		{
		    name = n;
		}

		void setNumMessages(int num)
		{
		    numMessages = num;
		}

		void allocateQueue()
		{
		    messages = new Queue();
		}
	};

	Node* getHead()
	{
	    return head;
	}

	int getNumPeople()
	{
	    return numPeople;
	}

	void insertPerson(std::string name)
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

	std::string deletePerson(std::string name)
	{
	    std::string n;
	    if((getNumPeople() == 1) && (getHead()->getName() == name))
	    {
		n = getHead()->getName();
		setHead(NULL);
		setNumPeople(getNumPeople() - 1);
		return n;
	    }

	    Node* curNode = getHead();
	   
	    while(curNode->getName() != name)
		curNode = curNode->getNext(); 
	    
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
	    return n;
	}

        std::string list()
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
	
	Node* search(std::string name)
	{
	    Node* curNode = getHead();
	    while(curNode)
	    {
	        if(curNode->getName() == name)
		{
		    return curNode;
		}
		curNode = curNode->getNext();;
	    }
            return NULL;
	}

    private:
        Node* head;
	int numPeople;	

	void setHead(Node* n)
	{
	    head = n;
	}

	void setNumPeople(int n)
	{
	    numPeople = n;
	}
	
};

class ServerImpl final
{
    public:
	~ServerImpl()
	{
	    server_->Shutdown();
	    cq_->Shutdown();
	}

        void Run()
        {
	    allocateList();
	    std::string server_address("0.0.0.0:50051");

	    ServerBuilder builder;
	    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	    builder.RegisterService(&service_);
	    cq_ = builder.AddCompletionQueue();
	    server_ = builder.BuildAndStart();
	    std::cout << "Server Listening On " << server_address << std::endl;
	    HandleRpcs();
        }

	LinkedList* getList()
	{
	    return list;
	}

    private:

	void allocateList()
	{
	    list = new LinkedList();
	}

	class CallData
	{
	    public:
		
		int getType()
		{
		    return type;
		}

		void setType(int t)
		{
		    type = t;
		}

	    private:
		int type;
	};

        class CallDataLogIn: public CallData
	{
	    public:
		CallDataLogIn(ChatServer::AsyncService* service, ServerCompletionQueue* cq, LinkedList* plist)
			: service_(service), cq_(cq), responder_(&ctx_), status_(CREATE)
		{
		    setType(1);
		    Proceed(plist);
		}

		void Proceed(LinkedList* plist)
		{
		    if(status_ == CREATE)
		    {
			status_ = PROCESS;
			service_->RequestLogIn(&ctx_, &request_, &responder_, cq_, cq_, this);
		    }
		    else if(status_ == PROCESS)
		    {
			new CallDataLogIn(service_, cq_, plist);
			reply_.set_conformation(request_.user());
			plist->insertPerson(request_.user());
			status_ = FINISH;
			responder_.Finish(reply_, Status::OK, this);
		    }
		    else
		    {
			GPR_ASSERT(status_ == FINISH);
			delete this;
		    }
		}

	    private:
		ChatServer::AsyncService* service_;
		ServerCompletionQueue* cq_;
		ServerContext ctx_;
		LogInRequest request_;
		LogInReply reply_;
		ServerAsyncResponseWriter<LogInReply> responder_;
		
		enum CallStatus {CREATE, PROCESS, FINISH};
		CallStatus status_;
	};

	class CallDataReceiveMessage: public CallData
	{
	    public:
		CallDataReceiveMessage(ChatServer::AsyncService* service, ServerCompletionQueue* cq, LinkedList* plist)
			: service_(service), cq_(cq), responder_(&ctx_), status_(CREATE)
		{
		    setType(5);
		    Proceed(plist);
		}

		void Proceed(LinkedList* plist)
		{
		    if(status_ == CREATE)
		    {
			status_ = PROCESS;
			service_->RequestReceiveMessage(&ctx_, &request_, &responder_, cq_, cq_, this);
		    }
		    else if(status_ == PROCESS)
		    {
			new CallDataReceiveMessage(service_, cq_, plist);
			reply_.set_conformation(plist->search(request_.user())->getMessages()->dequeueAll());
			status_ = FINISH;
			responder_.Finish(reply_, Status::OK, this);
		    }
		    else
		    {
			GPR_ASSERT(status_ == FINISH);
			delete this;
		    }
		}

	    private:
		ChatServer::AsyncService* service_;
		ServerCompletionQueue* cq_;
		ServerContext ctx_;
		ReceiveMessageRequest request_;
		ReceiveMessageReply reply_;
		ServerAsyncResponseWriter<ReceiveMessageReply> responder_;
		
		enum CallStatus {CREATE, PROCESS, FINISH};
		CallStatus status_;
	};

        class CallDataSendMessage: public CallData
	{
	    public:
		CallDataSendMessage(ChatServer::AsyncService* service, ServerCompletionQueue* cq, LinkedList* plist)
			: service_(service), cq_(cq), responder_(&ctx_), status_(CREATE)
		{
		    setType(4);
		    Proceed(plist);
		}

		void Proceed(LinkedList* plist)
		{
		    if(status_ == CREATE)
		    {
			status_ = PROCESS;
			service_->RequestSendMessage(&ctx_, &request_, &responder_, cq_, cq_, this);
		    }
		    else if(status_ == PROCESS)
		    {
			new CallDataSendMessage(service_, cq_, plist);

			reply_.set_conformation(plist->search(request_.recipient())->getName());

			std::string message = "Message from " + request_.user() + ":\n" + request_.message();

			plist->search(request_.recipient())->getMessages()->enqueue(message);
			status_ = FINISH;
			responder_.Finish(reply_, Status::OK, this);
		    }
		    else
		    {
			GPR_ASSERT(status_ == FINISH);
			delete this;
		    }
		}

	    private:
		ChatServer::AsyncService* service_;
		ServerCompletionQueue* cq_;
		ServerContext ctx_;
		SendMessageRequest request_;
		SendMessageReply reply_;
		ServerAsyncResponseWriter<SendMessageReply> responder_;
		
		enum CallStatus {CREATE, PROCESS, FINISH};
		CallStatus status_;
	};

        class CallDataList: public CallData
	{
	    public:
		CallDataList(ChatServer::AsyncService* service, ServerCompletionQueue* cq, LinkedList* plist)
			: service_(service), cq_(cq), responder_(&ctx_), status_(CREATE)
		{
		    setType(2);
		    Proceed(plist);
		}

		void Proceed(LinkedList* plist)
		{
		    if(status_ == CREATE)
		    {
			status_ = PROCESS;
			service_->RequestList(&ctx_, &request_, &responder_, cq_, cq_, this);
		    }
		    else if(status_ == PROCESS)
		    {
			new CallDataList(service_, cq_, plist);
			reply_.set_list(plist->list());
			status_ = FINISH;
			responder_.Finish(reply_, Status::OK, this);
		    }
		    else
		    {
			GPR_ASSERT(status_ == FINISH);
			delete this;
		    }
		}

	    private:
		ChatServer::AsyncService* service_;
		ServerCompletionQueue* cq_;
		ServerContext ctx_;
		ListRequest request_;
		ListReply reply_;
		ServerAsyncResponseWriter<ListReply> responder_;
		
		enum CallStatus {CREATE, PROCESS, FINISH};
		CallStatus status_;
	};

	class CallDataLogOut: public CallData
	{
	    public:
		CallDataLogOut(ChatServer::AsyncService* service, ServerCompletionQueue* cq, LinkedList* plist)
			: service_(service), cq_(cq), responder_(&ctx_), status_(CREATE)
		{
		    setType(3);
		    Proceed(plist);
		}

		void Proceed(LinkedList* plist)
		{
		    if(status_ == CREATE)
		    {
			status_ = PROCESS;
			service_->RequestLogOut(&ctx_, &request_, &responder_, cq_, cq_, this);
		    }
		    else if(status_ == PROCESS)
		    {
			new CallDataLogOut(service_, cq_, plist);
			reply_.set_conformation(plist->deletePerson(request_.user()));
			status_ = FINISH;
			responder_.Finish(reply_, Status::OK, this);
		    }
		    else
		    {
			GPR_ASSERT(status_ == FINISH);
			delete this;
		    }
		}

	    private:
		ChatServer::AsyncService* service_;
		ServerCompletionQueue* cq_;
		ServerContext ctx_;
		LogOutRequest request_;
		LogOutReply reply_;
		ServerAsyncResponseWriter<LogOutReply> responder_;
		
		enum CallStatus {CREATE, PROCESS, FINISH};
		CallStatus status_;
	};

	

	void HandleRpcs()
	{
	    new CallDataLogIn(&service_, cq_.get(), getList());
	    new CallDataList(&service_, cq_.get(), getList());
	    new CallDataLogOut(&service_, cq_.get(), getList());
	    new CallDataSendMessage(&service_, cq_.get(), getList());
	    new CallDataReceiveMessage(&service_, cq_.get(), getList());
	    void* tag;
	    bool ok;
	    while(true)
	    {
		GPR_ASSERT(cq_->Next(&tag, &ok));
		GPR_ASSERT(ok);
		//static_cast<CallDataLogIn*>(tag)->Proceed(getList());
		//static_cast<CallDataList*>(tag)->Proceed(getList());

		switch(static_cast<CallData*>(tag)->getType())
		{
		    case 1:
			static_cast<CallDataLogIn*>(tag)->Proceed(getList());
			break;
		    case 2:
			static_cast<CallDataList*>(tag)->Proceed(getList());
			break;
		    case 3:
			static_cast<CallDataLogOut*>(tag)->Proceed(getList());
			break;
		    case 4:
			static_cast<CallDataSendMessage*>(tag)->Proceed(getList());
			break;
		    case 5:
			static_cast<CallDataReceiveMessage*>(tag)->Proceed(getList());
			break;
		    default:
			break;
		}
	    }
 	}

    	std::unique_ptr<ServerCompletionQueue> cq_;
    	ChatServer::AsyncService service_;
    	std::unique_ptr<Server> server_;
    	LinkedList* list;
};

int main(int argc, char** argv)
{
    ServerImpl server;
    server.Run();
    return 0;
}
