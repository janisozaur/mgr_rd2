#ifndef RAYDISPLAYWINDOW_H
#define RAYDISPLAYWINDOW_H

#include <QMainWindow>
#include <qextserialport.h>

class RayDisplayScene;
class QTimer;
class QSerialPort;
class CommunicationThread;

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
private slots:
	void on_pushButton_clicked();
	void pollNextSender();
	void receivePacket(QByteArray packet);
	void error(QString errormsg);

signals:
	void serialWrite(QString command);
	void packetAvailable(QByteArray packet);
	void pollSender(const int senderId);

private:
	Ui::RayDisplayWindow *ui;
	RayDisplayScene *mRDS;
	QTimer *mTimer;
	int mSenderId;
	CommunicationThread *mCT;
};

#endif // RAYDISPLAYWINDOW_H
