#ifndef COMMUNICATIONTHREAD_H
#define COMMUNICATIONTHREAD_H

#include <QThread>
#include <QSemaphore>
#include <QMutex>
#include <QVector>
#include <RayDisplayWindow.h>

class QextSerialPort;

class CommunicationThread : public QThread
{
    Q_OBJECT
public:
    explicit CommunicationThread(QObject *parent = 0);
    QByteArray readBytes(const int howMany);

signals:
    void packetAvailable(QByteArray packet);

public slots:
    void finish();
    void putModuleId(int id);
    void setCalibration(Calibration cal);

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
    char mCharBuffer[1024];
    Calibration mCalibrationData;
};

#endif // COMMUNICATIONTHREAD_H
