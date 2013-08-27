#ifndef RAYDISPLAYWINDOW_H
#define RAYDISPLAYWINDOW_H

#include <QMainWindow>

class RayDisplayScene;
class QTimer;
class QSerialPort;
class CommunicationsThread;

namespace Ui {
class RayDisplayWindow;
}

class RayDisplayWindow : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit RayDisplayWindow(QWidget *parent = 0);
	~RayDisplayWindow();
	
private slots:
	void on_pushButton_clicked();
	void pollNextSender();
	void receivePacket(QByteArray packet);
	void error(QString errormsg);

signals:
	void serialWrite(QString command);

private:
	void pollSender(const int senderId);
	Ui::RayDisplayWindow *ui;
	CommunicationsThread *mCT;
	RayDisplayScene *mRDS;
	QTimer *mTimer;
	int mSenderId;
};

#endif // RAYDISPLAYWINDOW_H
