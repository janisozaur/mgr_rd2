#include "RayDisplayWindow.h"
#include "ui_RayDisplayWindow.h"
#include "RayDisplayScene.h"
#include "CommunicationThread.h"

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>

#include <QDebug>

RayDisplayWindow::RayDisplayWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::RayDisplayWindow),
	mRDS(nullptr),
	mTimer(nullptr),
	mSenderId(0)
{
	ui->setupUi(this);
}

RayDisplayWindow::~RayDisplayWindow()
{
	delete ui;
	delete mRDS;
	delete mTimer;
}

void RayDisplayWindow::on_pushButton_clicked()
{
	delete mRDS;
	delete mTimer;
	mRDS = new RayDisplayScene(this);
	ui->graphicsView->setScene(mRDS);
	mRDS->initLeds();

	mSerial.setPortName("/dev/ttyACM0");
	mSerial.setBaudRate(BAUD115200);
	mSerial.setDataBits(DATA_8);
	mSerial.setParity(PAR_NONE);
	mSerial.setStopBits(STOP_1);
	mSerial.setFlowControl(FLOW_OFF);
	mSerial.setTimeout(100);
	qDebug() << "connecting to " << mSerial.portName();
	bool opened = mSerial.open(QIODevice::ReadWrite);
	if (!opened) {
		QMessageBox::critical(this, "error", QString("error opening serial port ") + mSerial.errorString());
	}
	mSerial.readAll();

	mTimer = new QTimer(this);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(pollNextSender()));
	connect(&mSerial, SIGNAL(readyRead()), this, SLOT(onDataAvailable()));
	connect(this, SIGNAL(packetAvailable(QByteArray)), this, SLOT(receivePacket(QByteArray)));
	mTimer->setInterval(1000);
	mTimer->start();
}

void RayDisplayWindow::onDataAvailable()
{
	qDebug() << __func__;
	Q_ASSERT(mSerial.isOpen());
	Q_ASSERT(mSerial.isReadable());
	QString errorString;
	const qint64 toRead = mSerial.bytesAvailable();
	Q_ASSERT(toRead > 0);
	char *buffer = new char[toRead];
	const qint64 readCount = mSerial.read(buffer, toRead);
	if (readCount != toRead)
	{
		errorString = mSerial.errorString();
		emit error(QString("Failed to read %1 bytes from serial. read returned %2. Error: %3").arg(
								   QString::number(toRead), QString::number(readCount),
								   errorString));
	}
	QByteArray fresh(buffer, readCount);
	delete [] buffer;
	emitPackets(fresh);
}

#define SEP '\r'

void RayDisplayWindow::emitPackets(const QByteArray fresh)
{
	qDebug() << __func__;
	mBuffer += fresh;
	if (fresh.contains(SEP))
	{
		int start = 0;
		int end;
		while ((end = mBuffer.indexOf(SEP, start)) != -1) {
			emit packetAvailable(QByteArray::fromBase64(mBuffer.mid(start, end - start)));
			start = end + 1;
		}
		mBuffer = mBuffer.mid(start);
	}
}

void RayDisplayWindow::pollNextSender()
{
	qDebug() << "polling module" << mSenderId;
	pollSender(mSenderId++);
	mSenderId = mSenderId % 20;
}

void RayDisplayWindow::pollSender(const int senderId)
{
	//qDebug() << "main thread:" << QThread::currentThreadId();
	QString format("a%1\r");
	QString command(format.arg(QString::number(senderId)));
	//emit serialWrite(command);
	mSerial.write(command.toUtf8());
	for (int i = 0; i < 20; i++) {
		if (senderId == i)
		{
			continue;
		}
		QString rformat("e%1\rr\r");
		QString rcommand(rformat.arg(QString::number(i)));
		//emit serialWrite(rcommand);
		mSerial.write(rcommand.toUtf8());
	}
}

void RayDisplayWindow::receivePacket(QByteArray packet)
{
	qDebug() << __func__ << packet;
	//Q_ASSERT(packet.size() > 0);
	if (packet.size() <= 0)
	{
		qDebug() << "packet of 0/negative size!";
		return;
	}
	switch (packet.at(0))
	{
	case 'r':
		{
			const qint8 count = packet.size() - 3;
			Q_ASSERT(count > 0);
			const qint8 sender = packet.at(1);
			Q_ASSERT(0 <= sender);
			Q_ASSERT(20 > sender);
			const qint8 receiver = packet.at(2);
			Q_ASSERT(0 <= receiver);
			Q_ASSERT(20 > receiver);
			QVector<QBitArray> receiversValues(20, QBitArray(8));
			QBitArray ba(8);
			for (qint8 i = 0; i < 8; i++)
			{
				if (packet.at(3) & (1 << i))
				{
					ba.setBit(i);
				}
			}
			receiversValues[receiver] = ba;
			mRDS->lightenSender(sender, receiversValues);
		}
		break;
	case 'p':
		{
			const qint8 count = packet.size() - 2;
			Q_ASSERT(count > 0);
			const qint8 sender = packet.at(1);
			Q_ASSERT(0 <= sender);
			Q_ASSERT(20 > sender);
			QVector<QBitArray> receiversValues;
			for (int j = 0; j < 20; j++)
			{
				QBitArray ba(8);
				for (qint8 i = 0; i < 8; i++)
				{
					if (packet.at(2 + j) & (1 << i))
					{
						ba.setBit(i);
					}
				}
				receiversValues << ba;
			}
			mRDS->lightenSender(sender, receiversValues);
		}
	}
}

void RayDisplayWindow::error(QString errormsg)
{
	qDebug() << errormsg;
	ui->statusBar->showMessage(errormsg);
}
