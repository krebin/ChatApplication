# Makefile for gRPC

# default compiler g++
CXX = g++

# default path
CPPFLAGS += -I/usr/local/include -pthread

CFLAGS = -Wall -g

# default flag c++11
CXXFLAGS += -std=c++11

LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`	  \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -lncurses -lmenu -lform -ldl -g3 

PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = ../proto

vpath %.proto $(PROTOS_PATH)

all: ChatServer_ClientN ChatServer_Client ChatServer_ServerR

ChatServer_ClientN: chatserver.pb.o chatserver.grpc.pb.o ChatServer_ClientN.o UserNode.o 
	$(CXX) $^ $(LDFLAGS) -o $@

ChatServer_Client: chatserver.pb.o chatserver.grpc.pb.o ChatServer_Client.o UserNode.o 
	$(CXX) $^ $(LDFLAGS) -o $@

ChatServer_ServerR: chatserver.pb.o chatserver.grpc.pb.o ChatServer_ServerR.o UserNode.o
	$(CXX) $^ $(LDFLAGS) -o $@



.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

