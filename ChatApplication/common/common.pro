#-------------------------------------------------
#
# Project created by QtCreator 2017-08-06T17:08:18
#
#-------------------------------------------------
TEMPLATE = lib
CONFIG = staticlib

QMAKE_EXTRA_VARIABLES = GRPC_CPP_PLUGIN GRPC_CPP_PLUGIN_PATH
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH = `which $(EXPORT_GRPC_CPP_PLUGIN)`

PROTOS = chatserver.proto
include(protobuf.pri)
include(grpc.pri)

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    chatserver.grpc.pb.cc \
    chatserver.pb.cc

HEADERS += \
    chatserver.grpc.pb.h \
    chatserver.pb.h

