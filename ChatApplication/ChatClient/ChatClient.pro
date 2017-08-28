#-------------------------------------------------
#
# Project created by QtCreator 2017-08-06T17:08:18
#
#-------------------------------------------------

QT       += core gui


QMAKE_CXXFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`  \
                  `pkg-config --cflags protobuf` \
                  `pkg-config --libs protobuf` \
                  -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
                  -lprotobuf -lpthread\
                  -std=c++11\
                  -I/usr/local/include -pthread\
                  -Wall -g

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ChatAppClient
TEMPLATE = app

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
        main.cpp \
    QMessageReceiverPane.cpp \
    QMessageInputBox.cpp \
    SignalSender.cpp \
    LogInWindow.cpp \
    MainWindow.cpp \
    ChatAppClient.cpp

HEADERS += \
    chatserver.grpc.pb.h \
    chatserver.pb.h \
    ChatServerGlobal.h \
    QMessageReceiverPane.h \
    QMessageInputBox.h \
    SignalSender.h \
    LogInWindow.h \
    MainWindow.h \
    ChatAppClient.hpp

FORMS += \
        loginwindow.ui \
    mainwindow.ui


win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../common/release/ -lcommon
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../common/debug/ -lcommon
else:unix: LIBS += -L$$OUT_PWD/../common/ -lcommon

INCLUDEPATH += $$PWD/../common
DEPENDPATH += $$PWD/../common

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../common/release/libcommon.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../common/debug/libcommon.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../common/release/common.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../common/debug/common.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../common/libcommon.a


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../grpc/libs/opt/protobuf/release/ -lprotoc
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../grpc/libs/opt/protobuf/debug/ -lprotoc
else:unix: LIBS += -L$$PWD/../../../grpc/libs/opt/protobuf/ -lprotoc

INCLUDEPATH += $$PWD/../../../grpc/libs/opt/protobuf
DEPENDPATH += $$PWD/../../../grpc/libs/opt/protobuf

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../../grpc/libs/opt/protobuf/release/libprotoc.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../../grpc/libs/opt/protobuf/debug/libprotoc.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../../grpc/libs/opt/protobuf/release/protoc.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../../grpc/libs/opt/protobuf/debug/protoc.lib
else:unix: PRE_TARGETDEPS += $$PWD/../../../grpc/libs/opt/protobuf/libprotoc.a

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../grpc/libs/opt/protobuf/release/ -lprotobuf
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../grpc/libs/opt/protobuf/debug/ -lprotobuf
else:unix: LIBS += -L$$PWD/../../../grpc/libs/opt/protobuf/ -lprotobuf

INCLUDEPATH += $$PWD/../../../grpc/libs/opt/protobuf
DEPENDPATH += $$PWD/../../../grpc/libs/opt/protobuf

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../../grpc/libs/opt/protobuf/release/libprotobuf.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../../grpc/libs/opt/protobuf/debug/libprotobuf.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../../grpc/libs/opt/protobuf/release/protobuf.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../../grpc/libs/opt/protobuf/debug/protobuf.lib
else:unix: PRE_TARGETDEPS += $$PWD/../../../grpc/libs/opt/protobuf/libprotobuf.a

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../grpc/libs/opt/release/ -lgrpc
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../grpc/libs/opt/debug/ -lgrpc
else:unix: LIBS += -L$$PWD/../../../grpc/libs/opt/ -lgrpc

INCLUDEPATH += $$PWD/../../../grpc/libs/opt
DEPENDPATH += $$PWD/../../../grpc/libs/opt

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../grpc/libs/opt/release/ -lgpr
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../grpc/libs/opt/debug/ -lgpr
else:unix: LIBS += -L$$PWD/../../../grpc/libs/opt/ -lgpr

INCLUDEPATH += $$PWD/../../../grpc/libs/opt
DEPENDPATH += $$PWD/../../../grpc/libs/opt

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../grpc/libs/opt/release/ -lgrpc++_unsecure
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../grpc/libs/opt/debug/ -lgrpc++_unsecure
else:unix: LIBS += -L$$PWD/../../../grpc/libs/opt/ -lgrpc++_unsecure

INCLUDEPATH += $$PWD/../../../grpc/libs/opt
DEPENDPATH += $$PWD/../../../grpc/libs/opt
