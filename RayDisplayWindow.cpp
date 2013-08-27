#include "RayDisplayWindow.h"
#include "ui_RayDisplayWindow.h"
#include "RayDisplayScene.h"
#include "CommunicationsThread.h"

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include <QTimer>

#include <QDebug>

RayDisplayWindow::RayDisplayWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::RayDisplayWindow),
	mCT(nullptr),
	mRDS(nullptr),
	mTimer(nullptr),
	mSenderId(0)
{
	ui->setupUi(this);
}

RayDisplayWindow::~RayDisplayWindow()
{
	mCT->quit();
	delete mCT;
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
	QList<QSerialPortInfo> ports;
	//ports << QSerialPortInfo::availablePorts();
	ports << QSerialPortInfo(ui->lineEdit->text());
	foreach (const QSerialPortInfo &info, ports) {
		qDebug() << "Name        : " << info.portName();
		qDebug() << "Description : " << info.description();
		qDebug() << "Manufacturer: " << info.manufacturer();

		if (nullptr != mCT)
		{
			mCT->exit();
			bool clean = mCT->wait(1000);
			if (!clean)
			{
				mCT->terminate();
				qDebug() << "Had to terminate communication thread";
			}
		}
		delete mCT;
		mCT = new CommunicationsThread(this);
		mCT->setSerial(info);
		mCT->start();
		qDebug() << "***** CT running:" << mCT->isRunning();
		connect(mCT, SIGNAL(transferPacket(QByteArray)), this, SLOT(receivePacket(QByteArray)));
		connect(mCT, SIGNAL(error(QString)), this, SLOT(error(QString)));
		connect(this, SIGNAL(serialWrite(QString)), mCT, SLOT(write(QString)));
	}
	mTimer = new QTimer(this);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(pollNextSender()));
	mTimer->setInterval(1000);
	mTimer->start();
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
	emit serialWrite(command);
	for (int i = 0; i < 20; i++) {
		if (senderId == i)
		{
			continue;
		}
		QString rformat("e%1\rr\r");
		QString rcommand(rformat.arg(QString::number(i)));
		emit serialWrite(rcommand);
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
