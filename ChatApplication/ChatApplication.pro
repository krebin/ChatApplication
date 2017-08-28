#-------------------------------------------------
#
# Project created by QtCreator 2017-08-18T00:59:19
#
#-------------------------------------------------
TEMPLATE = subdirs

SUBDIRS = common ChatClient ChatServer

ChatClient.depends = common
ChatServer.depends = common

CONFIG += ordered
