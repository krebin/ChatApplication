#ifndef QMessageInputBox_H
#define QMessageInputBox_H

#include <QTextEdit>
#include <QKeyEvent>
#include "MainWindow.h"

class QMessageInputBox: public QTextEdit
{
    Q_OBJECT
    public:
        explicit QMessageInputBox(QWidget *parent = 0);
        ~QMessageInputBox();
        void setMainWindow(MainWindow *mainWindow);

    private:
        void keyPressEvent(QKeyEvent *event);
        MainWindow *_mainWindow;


};

#endif // TEXTEDITRPC_H
