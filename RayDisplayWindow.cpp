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
#include <QSvgGenerator>

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
	connect(this, SIGNAL(pollSender(int,ReadType)), mCT, SLOT(putModuleId(int,ReadType)));
}

void RayDisplayWindow::on_pushButton_clicked()
{
	delete mRDS;
	delete mTimer;
	mRDS = new RayDisplayScene(mCalibration, this);
	ui->graphicsView->setScene(mRDS);
	mRDS->initLeds();

	if (nullptr == mCT)
	{
		initCT();
	}
	mCT->start();

	mTimer = new QTimer(this);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(pollNextSender()));
	connect(ui->startPollPushButton, SIGNAL(clicked()), this, SLOT(pollNextSender()));
	connect(this, SIGNAL(packetAvailable(QByteArray)), this, SLOT(receivePacket(QByteArray)));
	connect(ui->drawHeatMapPushButton, SIGNAL(clicked()), mRDS, SLOT(drawHeatMap()));
	connect(this, SIGNAL(drawHeatmap()), mRDS, SLOT(drawHeatMap()));
	mTimer->setInterval(1000);
	//mTimer->start();
}

int RayDisplayWindow::getNextSenderId()
{
	const int sender = mSenderId++;
	mSenderId = mSenderId % 20;
	return sender;
}

void RayDisplayWindow::pollNextSender()
{
	if (QObject::sender() != nullptr)
	{
		qDebug() << "polling called from" << QObject::sender();
	} else {
		qDebug() << "polling called directly!";
	}
	int sender = getNextSenderId();
	qDebug() << "polling module" << sender;
	//emit pollSender(sender, ReadSingle);
	emit pollSender(sender, ReadAll);
}

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
    qDebug() << mCalibration;
}

void RayDisplayWindow::receivePacket(QByteArray packet)
{
	//qDebug() << __func__ << packet;
	qDebug() << __PRETTY_FUNCTION__ << "got new packet, size = " << packet.size();
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
			QHash<int, QBitArray> receiversValues;
			QBitArray ba(8, true);
			qDebug() << "value:" << int(packet.at(3));
			for (qint8 i = 0; i < 8; i++)
			{
				if (packet.at(3) & (1 << i))
				{
					ba.setBit(i, false);
				}
			}
			receiversValues.insert(receiver, ba);
			mRDS->lightenSender(sender, receiversValues);
			if (ui->contPollingCheckBox->isChecked())
			{
				pollNextSender();
			}
		}
		break;
	case 'p':
		{
			qDebug() << "entered p";
			const qint8 count = packet.size() - 2;
			Q_ASSERT(count > 0);
			const qint8 sender = packet.at(1);
			Q_ASSERT(0 <= sender);
			Q_ASSERT(20 > sender);
			QHash<int, QBitArray> receiversValues;
			const QHash<int, QBitArray> senderCal = mCalibration.at(sender);
			const QList<int> receiversCal = senderCal.keys();
			Q_ASSERT(receiversCal.size() == count);
			// the assert above guarantees this is exactly size of receiversCal
			for (int j = 0; j < count; j++)
			{
				QBitArray ba(8, true);
				const int receiver = receiversCal.at(j);
				for (qint8 i = 0; i < 8; i++)
				{
					if (packet.at(2 + j) & (1 << i))
					{
						ba.setBit(i, false);
					}
				}
				receiversValues.insert(receiver, ba);
			}
			qDebug() << "lighting";
			mRDS->lightenSender(sender, receiversValues);
			if (ui->contPollingCheckBox->isChecked())
			{
				qDebug() << "scheduling next polling";
				QTimer::singleShot(ui->pollingIntervalSpinBox->value(), this, SLOT(pollNextSender()));
				//pollNextSender();
			} else {
				//qDebug() << "################## here be dragons";
			}
		}
	}
	if (ui->refreshHeatmapCheckBox->isChecked())
	{
		emit drawHeatmap();
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

void RayDisplayWindow::on_saveSceneSvgPushButton_clicked()
{
	QSvgGenerator generator;
	generator.setFileName("scene.svg");
	generator.setSize(mRDS->sceneRect().size().toSize());
	QPainter painter;
	painter.begin(&generator);
	mRDS->render(&painter);
	painter.end();
	qDebug() << "saved";
}
