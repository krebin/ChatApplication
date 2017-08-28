/********************************************************************************
** Form generated from reading UI file 'loginwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGINWINDOW_H
#define UI_LOGINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_LogInWindow
{
public:
    QWidget *centralWidget;
    QGroupBox *groupBox;
    QWidget *layoutWidget;
    QVBoxLayout *verticalLayout_2;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLineEdit *lineEditUserName;
    QPushButton *pushButtonLogIn;
    QLabel *stateLabel;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;
    QMenuBar *menuBar;

    void setupUi(QMainWindow *LogInWindow)
    {
        if (LogInWindow->objectName().isEmpty())
            LogInWindow->setObjectName(QStringLiteral("LogInWindow"));
        LogInWindow->resize(400, 242);
        centralWidget = new QWidget(LogInWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        groupBox = new QGroupBox(centralWidget);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        groupBox->setGeometry(QRect(70, 40, 271, 121));
        layoutWidget = new QWidget(groupBox);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        layoutWidget->setGeometry(QRect(10, 30, 251, 85));
        verticalLayout_2 = new QVBoxLayout(layoutWidget);
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setContentsMargins(11, 11, 11, 11);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(6);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        label = new QLabel(layoutWidget);
        label->setObjectName(QStringLiteral("label"));

        horizontalLayout->addWidget(label);

        lineEditUserName = new QLineEdit(layoutWidget);
        lineEditUserName->setObjectName(QStringLiteral("lineEditUserName"));

        horizontalLayout->addWidget(lineEditUserName);


        verticalLayout->addLayout(horizontalLayout);

        pushButtonLogIn = new QPushButton(layoutWidget);
        pushButtonLogIn->setObjectName(QStringLiteral("pushButtonLogIn"));

        verticalLayout->addWidget(pushButtonLogIn);


        verticalLayout_2->addLayout(verticalLayout);

        stateLabel = new QLabel(layoutWidget);
        stateLabel->setObjectName(QStringLiteral("stateLabel"));

        verticalLayout_2->addWidget(stateLabel);

        LogInWindow->setCentralWidget(centralWidget);
        mainToolBar = new QToolBar(LogInWindow);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        LogInWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(LogInWindow);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        LogInWindow->setStatusBar(statusBar);
        menuBar = new QMenuBar(LogInWindow);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 400, 22));
        LogInWindow->setMenuBar(menuBar);
#ifndef QT_NO_SHORTCUT
        label->setBuddy(lineEditUserName);
#endif // QT_NO_SHORTCUT

        retranslateUi(LogInWindow);

        QMetaObject::connectSlotsByName(LogInWindow);
    } // setupUi

    void retranslateUi(QMainWindow *LogInWindow)
    {
        LogInWindow->setWindowTitle(QApplication::translate("LogInWindow", "LogInWindow", Q_NULLPTR));
        groupBox->setTitle(QApplication::translate("LogInWindow", "Choose name", Q_NULLPTR));
        label->setText(QApplication::translate("LogInWindow", "Username:", Q_NULLPTR));
        pushButtonLogIn->setText(QApplication::translate("LogInWindow", "Log In", Q_NULLPTR));
        stateLabel->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class LogInWindow: public Ui_LogInWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGINWINDOW_H
