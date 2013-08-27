#include "RayDisplayWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	RayDisplayWindow w;
	w.show();
	
	return a.exec();
}
