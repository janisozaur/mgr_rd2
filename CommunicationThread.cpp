#include "CommunicationThread.h"

#include <qextserialport.h>
#include <QMutexLocker>
#include <QDebug>

const QString CommunicationThread::senderFormat = "a%1\r";
const QString CommunicationThread::recFormat = "e%1\r";
const QByteArray CommunicationThread::readCommand = QString("r\r").toLocal8Bit();
const QString CommunicationThread::readAllFormat = "p%1%2\r";

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
	int emitted = 0;
	if (fresh.contains(SEP))
	{
		int start = 0;
		int end;
		while ((end = mBuffer.indexOf(SEP, start)) != -1) {
			const QByteArray packetRaw(mBuffer.mid(start, end - start));
			if (start != end)
			{
				const QByteArray packetDecoded(QByteArray::fromBase64(packetRaw));
				if (packetDecoded.at(0) == 'r') {
					qDebug() << packetRaw << ", s = " << int(packetDecoded.at(1)) << ", r = "
							 << int(packetDecoded.at(2)) << ", v = " << int(packetDecoded.at(3));
				} else {
					qDebug() << packetRaw << ", size = " << packetDecoded.size()
							 << ", sender = " << int(packetDecoded.at(1));
					for (int i = 2; i < packetDecoded.size(); i++)
					{
						qDebug() << int(packetDecoded.at(i));
					}
				}
				emitted++;
				emit packetAvailable(packetDecoded);
			}
			start = end + 1;
		}
		QByteArray next(mBuffer.mid(start));
		mBuffer = next;
		//qDebug() << "next: " << next.toHex();
	}
	qDebug() << "emitted" << emitted << "packets";
	if (emitted == 0)
	{
		qDebug() << mBuffer << ", in hex:" << mBuffer.toHex();
	}
}

QByteArray CommunicationThread::readBytes(const int howMany)
{
	QByteArray data;
	while ((!data.endsWith('\r') || data.size() < howMany) && !mAskedToFinish)
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

void CommunicationThread::readSingle(const int sender)
{
	mSerial->write(senderFormat.arg(QString::number(sender).rightJustified(2, '0')).toLocal8Bit());
	const QHash<int, QBitArray> senderCal = mCalibrationData.at(sender);
	const QList<int> receiversCal = senderCal.keys();
	for (int i = 0; i < senderCal.size(); i++)
	//for (int i = 0; i < 20 && !mAskedToFinish; i++)
	{
		const int receiver = receiversCal.at(i);
		const QString recStr = QString::number(receiver);
		mSerial->write(recFormat.arg(recStr.rightJustified(2, '0')).toLocal8Bit());
		mSerial->flush();
		usleep(100);
		mSerial->write(readCommand);
		mSerial->flush();
		//QThread::yieldCurrentThread();
		const QByteArray data = readBytes(9);
		//qDebug() << "data:" << data;
		emitPackets(data);
	}
}

void CommunicationThread::readAll(const int sender)
{
	//mSerial->write(senderFormat.arg(QString::number(sender).rightJustified(2, '0')).toLocal8Bit());
	const QHash<int, QBitArray> senderCal = mCalibrationData.at(sender);
	const QList<int> receiversCal = senderCal.keys();
	const QString senderId(QChar('a' + sender));
	QString receivers;
	receivers.reserve(receiversCal.size());
	for (QList<int>::const_iterator it = receiversCal.constBegin(); it != receiversCal.constEnd(); it++)
	{
		receivers += QChar('a' + (*it));
	}
	const QString command(readAllFormat.arg(senderId, receivers));
	qDebug() << "command:" << command;
	// 2 header bytes, 1 trailing '\r', actual response, +2 for rounding up
	// to base 64
	const int replySize = ((2 + receiversCal.size() + 2) / 3) * 4 + 1;
	const QByteArray actualData(command.toLocal8Bit());
	qDebug() << "actual data:" << actualData << ", in hex:" << actualData.toHex();
	qDebug() << "expecting" << replySize << "bytes in response";
	mSerial->write(actualData);
	mSerial->flush();
	const QByteArray data = readBytes(replySize);
	qDebug() << "response size:" << data.size() << ", it is: " << data << ", in hex:" << data.toHex();
	emitPackets(data);
}

void CommunicationThread::readNonLit()
{
	/*QString receivers;
	receivers.reserve(20);
	for (int i = 0; i < 20; i++)
	{
		receivers += QChar('a' + i);
	}
	// works for mirrored mode, might not work in non-mirror
	const QString senderId(QChar('{'));
	const QString command(readAllFormat.arg(senderId, receivers));
	qDebug() << "command:" << command;
	const QByteArray actualData(command.toLocal8Bit());
	const int replySize = ((2 + receiversCal.size() + 2) / 3) * 4 + 1;
	qDebug() << "actual data:" << actualData << ", in hex:" << actualData.toHex();
	qDebug() << "expecting" << replySize << "bytes in response";
	mSerial->write(actualData);
	mSerial->flush();
	QByteArray data = readBytes(replySize);
	data[0] = 'c';
	qDebug() << "response size:" << data.size() << ", it is: " << data << ", in hex:" << data.toHex();
	emitPackets(data);*/
}

void CommunicationThread::setPortName(QString name)
{
	mPortName = name;
}

void CommunicationThread::run()
{
	mSerial->setPortName(mPortName);
	mSerial->setBaudRate(BAUD115200);
	//mSerial.setBaudRate(BAUD9600);
	mSerial->setDataBits(DATA_8);
	mSerial->setParity(PAR_NONE);
	mSerial->setStopBits(STOP_1);
	mSerial->setFlowControl(FLOW_OFF);
	mSerial->setTimeout(100);
	mSerial->setQueryMode(QextSerialPort::Polling);
	const bool opened = mSerial->open(QIODevice::ReadWrite);
	Q_ASSERT(opened);
	while (!mAskedToFinish)
	{
		qDebug() << "waiting";
		mWriteSemaphore.acquire();
		qDebug() << "acquired";
		if (mAskedToFinish)
		{
			break;
		}
		const ModuleReadType toRead = getModuleID();
		const int sender = toRead.first;
		switch (toRead.second)
		{
			case ReadSingle:
				readSingle(sender);
				break;
			case ReadAll:
				readAll(sender);
				break;
			case ReadNonLit:
				break;
		}

		qDebug() << "polling" << sender;
	}
	mSerial->close();
	delete mSerial;
}

void CommunicationThread::putModuleId(const int id, const ReadType type)
{
	QMutexLocker l(&mModulesMutex);
	Q_UNUSED(l);
	mModuleIDs << qMakePair(id, type);
	mWriteSemaphore.release();
}

ModuleReadType CommunicationThread::getModuleID()
{
	QMutexLocker l(&mModulesMutex);
	Q_UNUSED(l);
	Q_ASSERT(mModuleIDs.size() > 0);
	return mModuleIDs.takeFirst();
}

void CommunicationThread::setCalibration(Calibration cal)
{
	mCalibrationData = cal;
}
