#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <QMutex>

#include "general.h"

class Camera : public QObject
{
	Q_OBJECT

public:
	Camera(QObject *parent = 0);
	bool isOpened() const
		{ return cap.isOpened(); }
	int getCameraNumber()  const
		{ return device; }
	void setCameraNumber(int device = 0) 
		{ if(device > 0) this->device = device; }
	cv::Mat getFrame();
	~Camera();

signals:
	void opened();
	void closed();
	void error(QString err, ErrorLevels level);

protected:
	bool open();
	bool takeFrame();
	void close();
	cv::Mat frame;

private:
	cv::VideoCapture cap;
	int device;
	
	int readFrameFails;
	bool readFrameWarningSended;

	QMutex frameMutex;
};

#endif // IMAGEPROCESSOR_H

