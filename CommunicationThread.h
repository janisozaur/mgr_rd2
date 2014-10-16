#ifndef COMMUNICATIONTHREAD_H
#define COMMUNICATIONTHREAD_H

#include <QThread>
#include <QSemaphore>
#include <QMutex>
#include <QVector>
#include <RayDisplayWindow.h>
#include <QString>

#include "common.h"

typedef QPair<int, ReadType> ModuleReadType;

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
    void putModuleId(const int id, const ReadType type);
    void setCalibration(Calibration cal);
    void setPortName(QString name);

protected:
    virtual void run();

private:
    void emitPackets(const QByteArray fresh);
    void readSingle(const int sender);
    void readAll(const int sender);
    void readNonLit();
    ModuleReadType getModuleID();
    QextSerialPort *mSerial;
    QByteArray mBuffer;
    bool mAskedToFinish;
    QSemaphore mWriteSemaphore;
    QVector<ModuleReadType> mModuleIDs;
    QMutex mModulesMutex;
    char mCharBuffer[1024];
    Calibration mCalibrationData;
    QString mPortName;

	const static QString senderFormat;
	const static QString recFormat;
	const static QByteArray readCommand;
	const static QString readAllFormat;
};

#endif // COMMUNICATIONTHREAD_H
