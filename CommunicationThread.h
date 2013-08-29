#ifndef COMMUNICATIONTHREAD_H
#define COMMUNICATIONTHREAD_H

#include <QThread>

class QextSerialPort;

class CommunicationThread : public QThread
{
    Q_OBJECT
public:
    explicit CommunicationThread(QObject *parent = 0);

signals:

public slots:

protected:
    virtual void run();

private:
    QextSerialPort *mSerial;
};

#endif // COMMUNICATIONTHREAD_H
