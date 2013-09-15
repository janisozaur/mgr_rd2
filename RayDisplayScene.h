#ifndef RAYDISPLAYSCENE_H
#define RAYDISPLAYSCENE_H

#include <QGraphicsScene>
#include <QList>
#include <QVector>
#include <QBitArray>
#include "RayDisplayWindow.h"
//#include <opencv2/core/core.hpp>

//#include "Tracker.h"

class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QPolygonF;
//class TrackerInterface;

class RayDisplayScene : public QGraphicsScene
{
	Q_OBJECT
	Q_PROPERTY( bool collisionEnabled READ isCollisionEnabled WRITE setCollisionEnabled )
	Q_PROPERTY( bool drawFakes READ isDrawingFakesEnabled WRITE setDrawFakesEnabled )

	struct Sender
	{
		QGraphicsEllipseItem *r;
		int rotation;
		int side;
	};

	struct Ray
	{
		QLineF line;
		bool visible;
		bool isCornerRay;
	};

public:
	explicit RayDisplayScene(const Calibration cal, const int boxSize, QObject *parent = 0);
	virtual ~RayDisplayScene();
	void initLeds();
	void lightenSender(int senderId, const int &angle);
	void lightenSender(const int senderId, const QVector<QBitArray> &detectors, const QVector<QBitArray> &calibration, const bool clear = true);
	void lightenSender(const int senderId, const QHash<int, QBitArray> &detectors);
	int sendersCount() const;
	bool isCollisionEnabled() const;
	bool isDrawingFakesEnabled() const;
	void updateCollisions();
	float pointToLineDistSquared(const QPointF &point, const QLineF &line) const;

	void clearRayNumbers();
	void clearTriangles();
	QVector<QLineF> & clearCollidedRays(int senderId);
	//cv::Mat cvtrack1(int senderId, QVector<Ray> senderRays);
	//void cvTrack2(cv::Mat cvImage);
	void drawRay(QHash<QPair<int, int>, int> &rectangles, QVector<QGraphicsLineItem *> &rays, const QLineF &line, const bool draw);
	QVector<Circle> exportCircles() const;
	void deleteCircle(QGraphicsEllipseItem *item);

signals:
	void publishSizes(const QVector<QVector<QPointF> > &receivers, const QVector<QVector<QPointF> > &senders);

public slots:
	void initRays(const int &angle);
	void clearRays(const int sender);
	void clearAllRays();
	void clearObstacle();
	void setCollisionEnabled(bool enable);
	void setDrawFakesEnabled(bool enable);
	void drawHeatMap();
	void toggleRayVisibility();
	void setCircleSize(int size);
	void deleteSelected();
	void deleteAll();
	void useNewHeatmap(bool use);
	void importCircles(QVector<Circle> circles);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

private:
	bool isStartingRay(const QVector<Ray> &rays, const int idx) const;
	bool isCornerRay(const QVector<Ray> &rays, const int idx) const;
	bool isFinishingRay(const QVector<Ray> &rays, const int idx) const;
	bool lineRectIntersects(const QLineF &line, const QRectF &rect) const;
	void addCircle(const Circle &c);
	QVector<Sender> mSenders;
	QVector<QGraphicsEllipseItem *> mReceivers;
	QVector<QVector<QGraphicsEllipseItem *> > mSidedReceivers;
	QVector<QVector<QGraphicsLineItem *>> mRays;
	QPolygonF mObstacle;
	QGraphicsPolygonItem *mGraphicsObstacle;
	QVector<QGraphicsEllipseItem *> mCircles;
	QVector<QVector<QLineF> > mCollidedRays;
	bool mCollisionEnabled;
	bool mDrawFakes;
	bool mRaysVisible;
	QVector<QGraphicsEllipseItem *> mCollisions;
	QVector<QVector<QGraphicsLineItem *> > mCollidedRaysGraphics;
	QVector<QList<QGraphicsPolygonItem *> > mTriangles;
	QVector<QGraphicsTextItem *> mRayNumbers;
	const Calibration mCalibration;
	// a hash of pair of (X, Y) rectangle ID, for each sender
	QVector<QHash<QPair<int, int>, int>> mSendersRectanglesPairs;
	QVector<QGraphicsRectItem *> mRectGraphics;
	// rectangle width, height
	const int mRW, mRH;
	int mCircleSize;
	bool mUseNewHeatmap;
//	QVector<QList<QVector<cv::Point2i> > > mCvPolygons;
//	QVector<cv::Mat> mMats;
//	Tracker mTracker;
//	TrackerInterface *mTI;
};

#endif // RAYDISPLAYSCENE_H
