#ifndef COMMON_H
#define COMMON_H

#include <QPointF>

enum ReadType
{
    ReadSingle,
    ReadAll
};

struct Circle
{
	QPointF center;
	float radius;
};

#endif // COMMON_H
