#ifndef COMMUNICATIONSTHREAD_H
#define COMMUNICATIONSTHREAD_H

#include <QThread>
#include <QSerialPortInfo>
#include <QSemaphore>

class QSerialPort;
class QSocketNotifier;

class CommunicationsThread : public QThread
{
	Q_OBJECT
public:
	explicit CommunicationsThread(QObject *parent = 0);
	virtual ~CommunicationsThread();
	
	void emitPackets(QByteArray fresh);
signals:
	void error(QString msg);
	void transferPacket(QByteArray packet);
	
public slots:
	void write(QString);
#ifdef QSERIAL
	void receiveData();
#else
	void readFD(int fd);
#endif
	void setSerial(QSerialPortInfo info);

private slots:
	void initSerial();
	void serialConsumedBytes(qint64 bytes);

protected:
	virtual void run();

private:
#ifdef QSERIAL
	QSerialPort *mSerialPort;
#else
	int mSerialPortFD;
	QSocketNotifier *mSN;
#endif
	QByteArray mBuffer;
	QSerialPortInfo mSPI;
	quint64 mBytesInFlight;
};

#endif // COMMUNICATIONSTHREAD_H
