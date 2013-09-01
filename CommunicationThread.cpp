#include "CommunicationThread.h"

#include <qextserialport.h>
#include <QMutexLocker>
#include <QDebug>

CommunicationThread::CommunicationThread(QObject *parent) :
    QThread(parent),
    mSerial(nullptr),
    mAskedToFinish(false)
{
    mSerial = new QextSerialPort(QextSerialPort::Polling, this);
}

void CommunicationThread::finish()
{
    mAskedToFinish = true;
    mWriteSemaphore.release();
}

#define SEP '\r'

void CommunicationThread::emitPackets(const QByteArray fresh)
{
	static int cnt = 0;
	Q_UNUSED(cnt);
	//qDebug() << __func__ << cnt++;
	mBuffer += fresh;
	if (fresh.contains(SEP))
	{
		int start = 0;
		int end;
		while ((end = mBuffer.indexOf(SEP, start)) != -1) {
			const QByteArray packetRaw(mBuffer.mid(start, end - start));
			if (start != end)
			{
				const QByteArray packetDecoded(QByteArray::fromBase64(packetRaw));
				//qDebug() << packetRaw << ", s = " << int(packetDecoded.at(1)) << ", r = "
				//		 << int(packetDecoded.at(2)) << ", v = " << int(packetDecoded.at(3));
				emit packetAvailable(packetDecoded);
			}
			start = end + 1;
		}
		QByteArray next(mBuffer.mid(start));
		mBuffer = next;
		//qDebug() << "next: " << next.toHex();
	}
}

QByteArray CommunicationThread::readBytes(const int howMany)
{
	QByteArray data;
	while (data.size() < howMany && !mAskedToFinish)
	{
		qint64 readCount = mSerial->read(mCharBuffer, 1024);
		if (readCount < 0) {
			qDebug() << "error:" << mSerial->lastError() << mSerial->errorString();
		} else {
			data += QByteArray(mCharBuffer, readCount);
		}
	}
	return data;
}

void CommunicationThread::run()
{
	mSerial->setPortName("/dev/ttyACM0");
	mSerial->setBaudRate(BAUD115200);
	//mSerial.setBaudRate(BAUD9600);
	mSerial->setDataBits(DATA_8);
	mSerial->setParity(PAR_NONE);
	mSerial->setStopBits(STOP_1);
	mSerial->setFlowControl(FLOW_OFF);
	mSerial->setTimeout(100);
	mSerial->setQueryMode(QextSerialPort::Polling);
	bool opened = mSerial->open(QIODevice::ReadWrite);
	Q_ASSERT(opened);
	QString senderFormat("a%1\r");
	QString recFormat("e%1\r");
	while (!mAskedToFinish)
	{
		qDebug() << "waiting";
		mWriteSemaphore.acquire();
		qDebug() << "acquired";
		if (mAskedToFinish)
		{
			break;
		}
		int sender = getModuleID();
		qDebug() << "polling" << sender;
		mSerial->write(senderFormat.arg(QString::number(sender).rightJustified(2, '0')).toLocal8Bit());
		for (int i = 0; i < 20 && !mAskedToFinish; i++)
		{
			mSerial->write(recFormat.arg((QString::number(i)).rightJustified(2, '0')).toLocal8Bit());
			mSerial->flush();
			usleep(100);
			mSerial->write(QString("r\r").toLocal8Bit());
			mSerial->flush();
			//QThread::yieldCurrentThread();
			QByteArray data = readBytes(9);
			//qDebug() << "data:" << data;
			emitPackets(data);
		}
	}
	mSerial->close();
	delete mSerial;
}

void CommunicationThread::putModuleId(int id)
{
	QMutexLocker l(&mModulesMutex);
	Q_UNUSED(l);
	mModuleIDs << id;
	mWriteSemaphore.release();
}

int CommunicationThread::getModuleID()
{
	QMutexLocker l(&mModulesMutex);
	Q_UNUSED(l);
	Q_ASSERT(mModuleIDs.size() > 0);
	return mModuleIDs.takeFirst();
}
