#include "CommunicationsThread.h"

#include <QSerialPort>
#include <QSemaphore>
#include <fcntl.h>
#include <QSocketNotifier>

#include <QDebug>

CommunicationsThread::CommunicationsThread(QObject *parent) :
	QThread(parent),
#ifdef QSERIAL
	mSerialPort(nullptr),
#else
	mSerialPortFD(-1),
	mSN(nullptr),
#endif
	mBytesInFlight(0)
{
	qDebug() << "hello from CT";
	connect(this, SIGNAL(started()), this, SLOT(initSerial()));
}

CommunicationsThread::~CommunicationsThread()
{
#ifdef QSERIAL
	delete mSerialPort;
#else
	if (mSerialPortFD != -1)
	{
		if (mSN != nullptr)
		{
			mSN->setEnabled(false);
			delete mSN;
		}
		close(mSerialPortFD);
	}
#endif
	qDebug() << "goodbye from CT";
}

void CommunicationsThread::serialConsumedBytes(qint64 bytes)
{
	mBytesInFlight -= bytes;
	//qDebug() << "serial consumed bytes, left:" << mBytesInFlight;
}

void CommunicationsThread::run()
{
	qDebug() << __func__ << "start";
	exec();
	qDebug() << __func__ << "quit";
}

void CommunicationsThread::write(QString command)
{
	//qDebug() << __func__ << "start";
	//qDebug() << "write thread:" << QThread::currentThreadId();
#ifdef QSERIAL
	Q_CHECK_PTR(mSerialPort);
	Q_ASSERT(mSerialPort->isOpen());
	Q_ASSERT(mSerialPort->isWritable());
#endif
	const qint64 length = command.length();
	qint64 writtenCount;
#ifdef QSERIAL
	writtenCount = mSerialPort->write(command.toStdString().c_str(), length);
#else
	writtenCount = ::write(mSerialPortFD, command.toStdString().c_str(), command.size());
#endif
	mBytesInFlight += writtenCount;
	bool flushed = true;
	bool written = true;
	QString errorString;
#ifdef QSERIAL
	flushed = mSerialPort->flush();
	written = mSerialPort->waitForBytesWritten(50);
	errorString = mSerialPort->errorString();
#endif
	if (length != writtenCount || !written || !flushed)
	{
		emit error(QString("Failed to write command \"%1\" of length %2. write() returned %3. written: %5, flushed: %6, Error: %4").arg(
					   command, QString::number(length),
					   QString::number(writtenCount), errorString,
					   QString(written ? "true" : "false"),
					   QString(flushed ? "true" : "false")));
	}
	//qDebug() << __func__ << "end";
}

#define SEP '\r'

void CommunicationsThread::emitPackets(QByteArray fresh)
{
	mBuffer += fresh;
	if (fresh.contains(SEP))
	{
		int start = 0;
		int end;
		while ((end = mBuffer.indexOf(SEP, start)) != -1) {
			emit transferPacket(QByteArray::fromBase64(mBuffer.mid(start, end - start)));
			start = end + 1;
		}
		mBuffer = mBuffer.mid(start);
	}
}

#ifdef QSERIAL
void CommunicationsThread::receiveData()
{
	qDebug() << __func__;
	QByteArray fresh;
	Q_CHECK_PTR(mSerialPort);
	Q_ASSERT(mSerialPort->isOpen());
	Q_ASSERT(mSerialPort->isReadable());
	QString errorString;
	const qint64 toRead = mSerialPort->bytesAvailable();
	Q_ASSERT(toRead > 0);
	char *buffer = new char[toRead];
	const qint64 readCount = mSerialPort->read(buffer, toRead);
	if (readCount != toRead)
	{
		emit error(QString("Failed to read %1 bytes from serial. read returned %2. Error: %3").arg(
					   QString::number(toRead), QString::number(readCount),
					   mSerialPort->errorString()));
	}
	QByteArray fresh(buffer, readCount);
	delete [] buffer;
	emitPackets(fresh);
}
#endif

void CommunicationsThread::setSerial(QSerialPortInfo info)
{
	mSPI = info;
}

void CommunicationsThread::initSerial()
{
	//delete mSerialPort;
	qDebug() << "Name        : " << mSPI.portName();
	qDebug() << "Description : " << mSPI.description();
	qDebug() << "Manufacturer: " << mSPI.manufacturer();
	bool opened = false;
	QString errorString;
#ifdef QSERIAL
	mSerialPort = new QSerialPort(this);
	mSerialPort->setPort(mSPI);
	mSerialPort->setBaudRate(115200);
	opened = mSerialPort->open(QIODevice::ReadWrite);
	errorString = mSerialPort->errorString();
#else
	QString devName = QString("/dev/") + mSPI.portName();
	mSerialPortFD = ::open(devName.toStdString().c_str(), O_RDWR | O_NONBLOCK);
	if (mSerialPortFD >= 0)
	{
		opened = true;
		qDebug() << "serialFD:" << mSerialPortFD;
	}
	else
	{
		opened = false;
		errorString = QString("%1: %2").arg(QString::number(errno), QString(strerror(errno)));
	}
#endif
	qDebug() << "serial.isOpen() == " << opened;
	if (!opened)
	{
		emit error(QString("Failed to open %1: %2").arg(mSPI.portName(), errorString));
		return;
	}
#ifdef QSERIAL
	// flush contents
	mSerialPort->readAll();
	connect(mSerialPort, SIGNAL(readyRead()), this, SLOT(receiveData()));
	connect(mSerialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(serialConsumedBytes(qint64)));
#else
	mSN = new QSocketNotifier(mSerialPortFD, QSocketNotifier::Read, this);
	connect(mSN, SIGNAL(activated(int)), this, SLOT(readFD(int)));
	mSN->setEnabled(true);
#endif
}

#ifndef QSERIAL
void CommunicationsThread::readFD(int fd)
{
	qDebug() << __func__;
	QByteArray fresh;
	char *buffer = new char[1024];
	ssize_t readCount;
	while ((readCount = read(fd, buffer, 1024)) >= 0)
	{
		qDebug() << "read" << readCount << "fresh bytes";
		fresh += QByteArray(buffer, readCount);
	}
	qDebug() << "readCount = " << readCount;
	delete [] buffer;
	emitPackets(fresh);
}
#endif
