#ifndef SIGNALSENDER_H
#define SIGNALSENDER_H

#include <QObject>

class SignalSender : public QObject
{
    Q_OBJECT
    public:
        explicit SignalSender(QObject *parent = nullptr);
        void setCurrentMessage(std::string currentMessage);
        void emitMessageReceived();
        std::string getCurrentMessage() const;

    signals:
        void messageReceived();

    public slots:

    private:
        std::string _currentMessage;
};

#endif // SIGNALSENDER_H
