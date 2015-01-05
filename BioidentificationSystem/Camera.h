#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <QMutex>
#include <QMessageBox>

#include "general.h"

class Camera : public QObject
{
	Q_OBJECT

public:
	Camera(QObject *parent = 0)
		: QObject(parent),
		device(0),
		readFrameFails(0),
		readFrameWarningSended(false) {}
	bool isOpened() const
		{ return cap.isOpened(); }
	cv::Mat getFrame() {
		//frameMutex.lock();
		cv::Mat retFrame = frame.clone();
		//frameMutex.unlock();
		return retFrame;
	}

signals:
	void opened();
	void closed();
	void error(QString err, QMessageBox::Icon level);

protected:
	bool open() {
		if(!cap.open(device))
			return false;
		return true;
	}
	bool takeFrame() {
		//if(!frameMutex.tryLock())
		//return true;
		if(!cap.read(frame))
		{
			//frameMutex.unlock();
			readFrameFails++;
			if(readFrameFails == CRITICAL_READ_FRAME_FAILS)
				return false;
			return true;
		}
		//frameMutex.unlock();
		readFrameFails = 0;
		return true;
	}
	void close() {
		if(cap.isOpened())
			cap.release();
	}
	cv::Mat frame;

private:
	cv::VideoCapture cap;
	int device;
	
	int readFrameFails;
	bool readFrameWarningSended;

	QMutex frameMutex;
};

#endif // IMAGEPROCESSOR_H

