#ifndef RAYDISPLAYWINDOW_H
#define RAYDISPLAYWINDOW_H

#include <QMainWindow>
#include <qextserialport.h>

class RayDisplayScene;
class QTimer;
class QSerialPort;

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
	void onDataAvailable();

signals:
	void serialWrite(QString command);
	void packetAvailable(QByteArray packet);

private:
	void pollSender(const int senderId);
	void emitPackets(const QByteArray fresh);
	Ui::RayDisplayWindow *ui;
	QextSerialPort mSerial;
	RayDisplayScene *mRDS;
	QTimer *mTimer;
	int mSenderId;
	QByteArray mBuffer;
};

#endif // RAYDISPLAYWINDOW_H
