#ifndef TRACKERINTERFACE_H
#define TRACKERINTERFACE_H

#include <QObject>
#include <QVariantMap>
#include <QVector>
#include <QPointF>

enum RayStatus
{
	NOT_VISIBLE,
	SEEN,
	COVERED,
};

class TrackerInterface : public QObject
{
	Q_OBJECT
public:
	explicit TrackerInterface(const QVector<QVector<QPointF>> &receivers, const QVector<QVector<QPointF>> &senders, QObject *parent);
	virtual QVariantMap trackBlobs(const QVector<RayStatus> rays, const int senderId) = 0;

	int count(const QVector<QVector<QPointF> > &sizes) const;
	bool isStartingRay(const QVector<RayStatus> &rays, const int idx) const;
	bool isFinishingRay(const QVector<RayStatus> &rays, const int idx) const;
	QPointF idToSender(const int senderId) const;
	QPointF idToReceiver(const int receiverId) const;
	bool isCornerRay(const int idx) const;

protected:
	const QVector<QVector<QPointF>> mReceivers;
	const QVector<QVector<QPointF>> mSenders;
	QPointF remap2d(const int id, const QVector<QVector<QPointF>> &vector) const;
};

#endif // TRACKERINTERFACE_H
