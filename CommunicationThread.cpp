#include "CommunicationThread.h"

#include <qextserialport.h>

CommunicationThread::CommunicationThread(QObject *parent) :
    QThread(parent),
    mSerial(nullptr)
{
}

void CommunicationThread::run()
{
}
