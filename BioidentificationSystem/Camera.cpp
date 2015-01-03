#include <QMetaType>

#include "Camera.h"

Camera::Camera(QObject *parent /* = 0 */)
	: QObject(parent),
	device(0),
	readFrameFails(0),
	readFrameWarningSended(false)
{
	qRegisterMetaType<ErrorLevels>("ErrorLevels");
}

bool Camera::open()
{
	if(!cap.open(device))
		return false;
	return true;
}

bool Camera::takeFrame()
{
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

void Camera::close()
{
	if(cap.isOpened())
		cap.release();
}

cv::Mat Camera::getFrame()
{
	//frameMutex.lock();
	cv::Mat retFrame = frame.clone();
	//frameMutex.unlock();
	return retFrame;
}

Camera::~Camera()
{}