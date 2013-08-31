#ifndef COMMUNICATIONTHREAD_H
#define COMMUNICATIONTHREAD_H

#include <QThread>
#include <QSemaphore>
#include <QMutex>
#include <QVector>

class QextSerialPort;

class CommunicationThread : public QThread
{
    Q_OBJECT
public:
    explicit CommunicationThread(QObject *parent = 0);

signals:
    void packetAvailable(QByteArray packet);

public slots:
    void finish();
    void putModuleId(int id);

protected:
    virtual void run();

private:
    void emitPackets(const QByteArray fresh);
    int getModuleID();
    QextSerialPort *mSerial;
    QByteArray mBuffer;
    bool mAskedToFinish;
    QSemaphore mWriteSemaphore;
    QVector<int> mModuleIDs;
    QMutex mModulesMutex;
};

#endif // COMMUNICATIONTHREAD_H
