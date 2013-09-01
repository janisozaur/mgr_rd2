#include "RayDisplayWindow.h"
#include "ui_RayDisplayWindow.h"
#include "RayDisplayScene.h"
#include "CommunicationThread.h"

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QDebug>

RayDisplayWindow::RayDisplayWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::RayDisplayWindow),
	mRDS(nullptr),
	mTimer(nullptr),
	mSenderId(0),
	mCT(nullptr)
{
	ui->setupUi(this);
}

void RayDisplayWindow::cleanCT()
{
	if (nullptr != mCT)
	{
		if (mCT->isRunning()) {
			mCT->finish();
			if (!mCT->wait(1000)) {
				mCT->terminate();
			}
		}
		delete mCT;
	}
}

RayDisplayWindow::~RayDisplayWindow()
{
	cleanCT();
	delete ui;
	delete mRDS;
	delete mTimer;
}

void RayDisplayWindow::initCT()
{
	mCT = new CommunicationThread(this);
	mCT->setCalibration(mCalibration);
	connect(mCT, SIGNAL(packetAvailable(QByteArray)), this, SLOT(receivePacket(QByteArray)));
	connect(this, SIGNAL(pollSender(int)), mCT, SLOT(putModuleId(int)));
}

void RayDisplayWindow::on_pushButton_clicked()
{
	delete mRDS;
	delete mTimer;
	mRDS = new RayDisplayScene(this);
	ui->graphicsView->setScene(mRDS);
	mRDS->initLeds();

	if (nullptr == mCT)
	{
		initCT();
	}
	mCT->start();

	mTimer = new QTimer(this);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(pollNextSender()));
	connect(this, SIGNAL(packetAvailable(QByteArray)), this, SLOT(receivePacket(QByteArray)));
	mTimer->setInterval(1000);
	mTimer->start();
}

void RayDisplayWindow::pollNextSender()
{
	qDebug() << "polling module" << mSenderId;
	emit pollSender(mSenderId++);
	mSenderId = mSenderId % 20;
}

/*void RayDisplayWindow::pollSender(const int senderId)
{
	//qDebug() << "main thread:" << QThread::currentThreadId();
	QString format("a%1\r");
	QString command(format.arg(QString::number(senderId).rightJustified(2, '0')));
	//emit serialWrite(command);
	mSerial.write(command.toUtf8());
	qDebug() << __func__ << " thread:" << QThread::currentThreadId();
	for (int i = 0; i < 20; i++) {
		if (senderId == i)
		{
			continue;
		}
		QString rformat("e%1\r");
		QString rcommand(rformat.arg(QString::number(i).rightJustified(2, '0')));
		qDebug() << "rcommand" << rcommand << ", " << rcommand.toUtf8().toHex();
		//emit serialWrite(rcommand);
		qint64 written = mSerial.write(rcommand.toUtf8().constData(), rcommand.size());
		mSerial.flush();
		if (written != rcommand.size())
		{
			qDebug() << "HERE BE DRAGONS ##########################";
		}
		QString readCommand("r\r");
		mSerial.write(readCommand.toUtf8());
		mSerial.flush();
		QThread::usleep(10);
	}
}*/

void RayDisplayWindow::readCalibration(QString filename)
{
    QFile file(filename);
    bool opened = file.open(QIODevice::ReadOnly | QIODevice::Text);
    qDebug() << "calibration opened:" << opened;
    QString c = file.readAll();
    QJsonDocument calibJson = QJsonDocument::fromJson(c.toUtf8());
    qDebug() << "isnull" << calibJson.isNull();
    Q_ASSERT(calibJson.isObject());
    QJsonObject calibObj = calibJson.object();
    QJsonValue calibData = calibObj.value("calibration");
    Q_ASSERT(calibData.isArray());
    QJsonArray calibArray = calibData.toArray();
    int sendersCount = calibArray.size();
    mCalibration.reserve(sendersCount);
    for (int i = 0; i < sendersCount; i++)
    {
        QJsonObject sender = calibArray.at(i).toObject();
        QStringList key = sender.keys();
        Q_ASSERT(key.size() == 1);
        bool ok;
        int senderId = key.at(0).toInt(&ok, 0);
        if (!ok)
        {
            qDebug() << "failed to convert key to int:" << key.at(0) <<
                        "in object:" << sender;
        }
        Q_ASSERT(ok);
        if (senderId + 1 > mCalibration.size())
        {
            mCalibration.resize(senderId + 1);
        }
        QJsonArray recArray = sender.value(key.at(0)).toArray();
        int recCount = recArray.size();
        mCalibration[senderId].reserve(recCount);
        for (int j = 0; j < recCount; j++)
        {
            QJsonObject receiver = recArray.at(j).toObject();
            QStringList recKey = receiver.keys();
            Q_ASSERT(recKey.size() == 1);
            int recId = recKey.at(0).toInt(&ok, 0);
            if (!ok)
            {
                qDebug() << "failed to convert key to int:" << recKey.at(0) <<
                            "in object:" << receiver;
            }
            Q_ASSERT(ok);
            QString recVal = receiver.value(recKey.at(0)).toString();
            Q_ASSERT(!recVal.isNull());
            uint recBits = recVal.toUInt(&ok, 0);
            if (!ok)
            {
                qDebug() << "failed to convert value to int:" << recVal <<
                            "in object:" << receiver;
            }
            Q_ASSERT(ok);
            Q_ASSERT(recBits == qBound(uint(0), recBits, uint(255)));
            QBitArray ba(8);
            for (int k = 0; k < 8; k++)
            {
                if (recBits & (1 << k))
                {
                    ba.setBit(k);
                }
            }
            mCalibration[senderId].insert(recId, ba);
        }
    }
}

void RayDisplayWindow::receivePacket(QByteArray packet)
{
	//qDebug() << __func__ << packet;
	//qDebug() << "got new packet, size = " << packet.size();
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

void RayDisplayWindow::on_pushButton_2_clicked()
{
	readCalibration(ui->lineEdit_2->text());
}
