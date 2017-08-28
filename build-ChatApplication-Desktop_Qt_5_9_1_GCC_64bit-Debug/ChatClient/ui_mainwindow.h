/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "QMessageInputBox.h"
#include "QMessageReceiverPane.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QMessageInputBox *messageInputBox;
    QPushButton *doneButton;
    QWidget *layoutWidget;
    QVBoxLayout *verticalLayout;
    QPushButton *logOffButton;
    QPushButton *sendMessageButton;
    QPushButton *receiveMessageButton;
    QPushButton *listButton;
    QPushButton *chatButton;
    QLabel *rpcStateLabel;
    QMessageReceiverPane *messageReceiverPane;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(639, 338);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        messageInputBox = new QMessageInputBox(centralwidget);
        messageInputBox->setObjectName(QStringLiteral("messageInputBox"));
        messageInputBox->setGeometry(QRect(10, 200, 481, 81));
        messageInputBox->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        doneButton = new QPushButton(centralwidget);
        doneButton->setObjectName(QStringLiteral("doneButton"));
        doneButton->setGeometry(QRect(500, 260, 21, 21));
        layoutWidget = new QWidget(centralwidget);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        layoutWidget->setGeometry(QRect(500, 10, 131, 181));
        verticalLayout = new QVBoxLayout(layoutWidget);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        logOffButton = new QPushButton(layoutWidget);
        logOffButton->setObjectName(QStringLiteral("logOffButton"));

        verticalLayout->addWidget(logOffButton);

        sendMessageButton = new QPushButton(layoutWidget);
        sendMessageButton->setObjectName(QStringLiteral("sendMessageButton"));

        verticalLayout->addWidget(sendMessageButton);

        receiveMessageButton = new QPushButton(layoutWidget);
        receiveMessageButton->setObjectName(QStringLiteral("receiveMessageButton"));

        verticalLayout->addWidget(receiveMessageButton);

        listButton = new QPushButton(layoutWidget);
        listButton->setObjectName(QStringLiteral("listButton"));

        verticalLayout->addWidget(listButton);

        chatButton = new QPushButton(layoutWidget);
        chatButton->setObjectName(QStringLiteral("chatButton"));

        verticalLayout->addWidget(chatButton);

        rpcStateLabel = new QLabel(centralwidget);
        rpcStateLabel->setObjectName(QStringLiteral("rpcStateLabel"));
        rpcStateLabel->setGeometry(QRect(500, 200, 131, 51));
        messageReceiverPane = new QMessageReceiverPane(centralwidget);
        messageReceiverPane->setObjectName(QStringLiteral("messageReceiverPane"));
        messageReceiverPane->setGeometry(QRect(10, 10, 481, 181));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(messageReceiverPane->sizePolicy().hasHeightForWidth());
        messageReceiverPane->setSizePolicy(sizePolicy);
        messageReceiverPane->viewport()->setProperty("cursor", QVariant(QCursor(Qt::ArrowCursor)));
        messageReceiverPane->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        MainWindow->setCentralWidget(centralwidget);
        layoutWidget->raise();
        messageInputBox->raise();
        doneButton->raise();
        rpcStateLabel->raise();
        messageReceiverPane->raise();
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QStringLiteral("menubar"));
        menubar->setGeometry(QRect(0, 0, 639, 22));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", Q_NULLPTR));
        doneButton->setText(QApplication::translate("MainWindow", "X", Q_NULLPTR));
        logOffButton->setText(QApplication::translate("MainWindow", "Log Off", Q_NULLPTR));
        sendMessageButton->setText(QApplication::translate("MainWindow", "Send Message", Q_NULLPTR));
        receiveMessageButton->setText(QApplication::translate("MainWindow", "Receive Message", Q_NULLPTR));
        listButton->setText(QApplication::translate("MainWindow", "List Online", Q_NULLPTR));
        chatButton->setText(QApplication::translate("MainWindow", "Chat", Q_NULLPTR));
        rpcStateLabel->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
