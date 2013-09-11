#include "TrackerInterface.h"

TrackerInterface::TrackerInterface(const QVector<QVector<QPointF>> &receivers, const QVector<QVector<QPointF>> &senders, QObject *parent) :
	QObject(parent),
	mReceivers(receivers),
	mSenders(senders)
{
}

int TrackerInterface::count(const QVector<QVector<QPointF> > &sizes) const
{
	int localSum = 0;
	for (int i = 0, n = sizes.size(); i < n; i++) {
		localSum += sizes.at(i).size();
	}
	return localSum;
}

bool TrackerInterface::isStartingRay(const QVector<RayStatus> &rays, const int idx) const
{
	Q_ASSERT(idx >= 0);
	Q_ASSERT(idx < rays.size());
	if (Q_UNLIKELY(0 == idx)) {
		return rays.at(idx) == SEEN;
	} else {
		return ((rays.at(idx - 1) == COVERED) && (rays.at(idx) == SEEN));
	}
}

bool TrackerInterface::isFinishingRay(const QVector<RayStatus> &rays, const int idx) const
{
	Q_ASSERT(idx >= 0);
	Q_ASSERT(idx < rays.size());
	if (Q_UNLIKELY(rays.size() - 1 == idx)) {
		return rays.at(idx) == SEEN;
	} else {
		return ((rays.at(idx + 1) == COVERED) && (rays.at(idx) == SEEN));
	}
}

bool TrackerInterface::isCornerRay(const int idx) const
{
	const int n = mReceivers.size();
	int offset = 0;
	for (int i = 0; i < n; i++) {
		const int currentSize = mReceivers.at(i).size();
		if (idx == offset || idx == offset + currentSize - 1) {
			return true;
		}
		offset += currentSize;
	}
	return false;
}

QPointF TrackerInterface::remap2d(const int id, const QVector<QVector<QPointF>> &vector) const
{
	int offset = 0;
	for (int i = 0, n = vector.size(); i < n; i++) {
		const int currentSize = vector.at(i).size();
		if (offset + currentSize > id) {
			return vector.at(i).at(id - offset);
		}
		offset += currentSize;
	}
	Q_ASSERT(false);
	return QPointF();
}

QPointF TrackerInterface::idToSender(const int senderId) const
{
	return remap2d(senderId, mSenders);
}

QPointF TrackerInterface::idToReceiver(const int senderId) const
{
	return remap2d(senderId, mReceivers);
}
