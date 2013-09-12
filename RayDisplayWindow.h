#ifndef RAYDISPLAYWINDOW_H
#define RAYDISPLAYWINDOW_H

#include <QMainWindow>
#include <qextserialport.h>
#include <QHash>
#include <QVector>
#include <QBitArray>
#include "common.h"

class RayDisplayScene;
class QTimer;
class QSerialPort;
class CommunicationThread;

typedef QVector<QHash<int, QBitArray>> Calibration;

namespace Ui {
class RayDisplayWindow;
}

class RayDisplayWindow : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit RayDisplayWindow(QWidget *parent = 0);
	~RayDisplayWindow();
	
	void cleanCT();
	void initCT();
	int getNextSenderId();
private slots:
	void on_pushButton_clicked();
	void pollNextSender();
	void receivePacket(QByteArray packet);
	void error(QString errormsg);

	void on_pushButton_2_clicked();

	void on_saveSceneSvgPushButton_clicked();

signals:
	void serialWrite(QString command);
	void packetAvailable(QByteArray packet);
	void pollSender(const int senderId, const ReadType type);
	void drawHeatmap();

private:
	void readCalibration(QString filename);
	Ui::RayDisplayWindow *ui;
	RayDisplayScene *mRDS;
	QTimer *mTimer;
	int mSenderId;
	CommunicationThread *mCT;
	Calibration mCalibration;
};

#endif // RAYDISPLAYWINDOW_H
