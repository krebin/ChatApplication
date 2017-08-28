#ifndef QMESSAGERECEIVERPANE_H
#define QMESSAGERECEIVERPANE_H

#include <QPlainTextEdit>

class QMessageReceiverPane: public QPlainTextEdit
{
    Q_OBJECT
    public:
        explicit QMessageReceiverPane(QWidget *parent = 0);
        ~QMessageReceiverPane();
};

#endif // QMESSAGERECEIVERPANE_H
