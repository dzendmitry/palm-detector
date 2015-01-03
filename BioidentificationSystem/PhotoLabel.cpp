#include <QMouseEvent>
#include <QPainter>
#include <QFileDialog>

#include "PhotoLabel.h"

PhotoLabel::PhotoLabel(ImageProcessor *imageProcessor, QWidget *parent, Qt::WindowFlags f)
	: QLabel(parent, f),
	drawRectMode(false),
	drawMode(false)
{
	this->imageProcessor = imageProcessor;
}

void PhotoLabel::apply()
{
	prevMat = currentMat.clone();
	
	QString result;
	try {
		result = handRecCommands(currentMat);
	} catch(std::exception &e) {
		emit error(e.what(), CRITICAL);
		return;
	}

	// Draw result
	cv::putText(currentMat, result.toStdString(), cv::Point(0, currentMat.rows-1), cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(0, 0, 255), 2);
	applyCurrentPixmap();
	setPointsToNull();
}

void PhotoLabel::process(bool state)
{
	if(state) {
		if(!imageProcessor->stopped()) {
			emit error("Stop machine before process!", CRITICAL);
			emit processError();
			return;
		}
		cv::Rect skinRect(startPoint, endPoint);
		if(skinRect.area() == 0) {
			emit error("Draw rectangle.", WARNING);
			emit processError();
			return;
		}
		imageProcessor->setFrame(currentMat);
		imageProcessor->setSkinColor(currentMat(skinRect).clone());
		imageProcessor->setSkinColorRect(skinRect);
		imageProcessor->setPhotoProcessingMode(true);
		imageProcessor->setStop(false);
		imageProcessor->setStartState(ImageProcessor::TRAIN_PIXEL_CLASSIFIER);
		prevMat = currentMat.clone();
		QMetaObject::invokeMethod(imageProcessor,
			"processImage",
			Qt::QueuedConnection,
			Q_ARG(ImageProcessor::States, ImageProcessor::TRAIN_PIXEL_CLASSIFIER));
		emit processStarted();
	} else {
		imageProcessor->setPhotoProcessingMode(false);
		imageProcessor->setStop(true);
		currentMat = imageProcessor->getMorphologyOpening();
		applyCurrentPixmap();
		setPointsToNull();
		emit processStopped();
	}
}

void PhotoLabel::showPrevImage()
{
	currentMat = prevMat.clone();
	applyCurrentPixmap();
	setPointsToNull();
}

void PhotoLabel::setDrawRectMode(bool state)
{
	if(state) {
		drawMode = false;
		drawRectMode = true;
	} else
		drawRectMode = false;
	
	applyCurrentPixmap();
	setPointsToNull();
}
void PhotoLabel::setDrawMode(bool state)
{
	if(state) {
		drawRectMode = false;
		drawMode = true;
	} else
		drawMode = false;

	applyCurrentPixmap();
	setPointsToNull();
}

void PhotoLabel::openImage()
{
	QString imageName = QFileDialog::getOpenFileName(this, "Open image", QString(), "Images (*.bmp *.jpg *.png)");
	if(imageName.isNull())
		return;
	cv::Mat image = loadImage(imageName);
	if(image.data == NULL)
	{
		emit error("Opening file error.", CRITICAL);
		return;
	}
	currentMat = image;
	currentImageName = imageName;
	emit opened();
}

void PhotoLabel::saveCurrentImage()
{
	QString name = saveImage(currentMat, currentImageName, currentImagePath);
	if(name.isEmpty())
		emit error("Can't save image.", CRITICAL);
	else
		emit saved();
}

void PhotoLabel::saveCurrentImageAs()
{
	QString fileName = QFileDialog::getSaveFileName(this, "Save image", currentImagePath, "Images (*.bmp)");
	if(fileName.isNull())
		return;
	QString name = getImageName(fileName);
	QString path = getImagePath(fileName);
	if(saveImage(currentMat, name, path).isEmpty())
		emit error("Can't save image.", CRITICAL);
	else
		emit saved();
}

void PhotoLabel::invColors()
{
	cv::bitwise_not(currentMat, currentMat);
	applyCurrentPixmap();
}

void PhotoLabel::crop()
{
	cv::Rect rect(startPoint, endPoint);
	if(rect.area() <= 0)
		return;
	if( currentMat.cols <= (rect.x + rect.width) ||
		currentMat.rows <= (rect.y + rect.height) )
		return;
	prevMat = currentMat.clone();
	currentMat = currentMat(rect).clone();
	applyCurrentPixmap();
	setPointsToNull();
}

void PhotoLabel::mousePressEvent (QMouseEvent * event)
{
	if(pixmap() == 0)
		return;
	if(drawRectMode)
	{
		startPoint.x = event->x();
		startPoint.y = event->y();
	}
	if(drawMode)
	{
		prevMat = currentMat.clone();
	}
}

void PhotoLabel::mouseMoveEvent(QMouseEvent * event)
{
	if(pixmap() == 0)
		return;
	if(drawRectMode)
	{
		QPixmap pixmap = currentPixmap;
		QPainter painter(&pixmap);
		painter.setPen(QPen(QBrush(Qt::red), 2));
		painter.drawRect(QRect(QPoint(startPoint.x, startPoint.y), QPoint(event->x(), event->y())));
		setPixmap(pixmap);
	}
	if(drawMode)
	{
		cv::circle(currentMat, cv::Point(event->x(), event->y()), 2, cv::Scalar(0, 0, 0), -1);
		applyCurrentPixmap();
	}
}

void PhotoLabel::mouseReleaseEvent (QMouseEvent * event)
{
	if(pixmap() == 0)
		return;
	if(drawRectMode)
	{
		endPoint.x = event->x();
		endPoint.y = event->y();
		if(!imageProcessor->stopped())
		{
			imageProcessor->setSkinColor(currentMat(cv::Rect(startPoint, endPoint)).clone());
			imageProcessor->setSkinColorRect(cv::Rect(startPoint, endPoint));
		}
	}
}

void PhotoLabel::applyCurrentPixmap()
{
	currentPixmap = QPixmap::fromImage(Mat2QImage(currentMat));
	setPixmap(currentPixmap);
}

void PhotoLabel::setPointsToNull()
{
	startPoint = cv::Point(0, 0);
	endPoint = cv::Point(0, 0);
}

PhotoLabel::~PhotoLabel()
{}