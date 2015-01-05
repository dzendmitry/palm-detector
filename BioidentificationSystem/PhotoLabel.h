#ifndef PHOTOLABEL
#define PHOTOLABEL

#include <QLabel>
#include <QPoint>
#include <QMouseEvent>
#include <QPainter>
#include <QFileDialog>

#include "opencv2/opencv.hpp"
#include "ImageProcessor.h"

class PhotoLabel : public QLabel
{
	Q_OBJECT

public:
	PhotoLabel(ImageProcessor *imageProcessor, QWidget * parent = 0, Qt::WindowFlags f = 0)
		: QLabel(parent, f),
		drawRectMode(false),
		drawMode(false)
		{
			this->imageProcessor = imageProcessor;
		}

	void setCurrentMat(const cv::Mat& currentMat)
		{ this->currentMat = currentMat.clone(); }
	cv::Mat getCurrentMat()
		{ return currentMat; }

	void setCurrentPixmap(const QPixmap& currentPixmap)
		{ this->currentPixmap = currentPixmap; }

	void setCurrentImageName(const QString& name)
		{ this->currentImageName = name; }
	QString getCurrentImageName()
		{ return currentImageName; }

	void setCurrentImagePath(const QString& currentImagePath)
		{ this->currentImagePath = currentImagePath; }

signals:
	void saved();
	void opened();
	void error(QString, QMessageBox::Icon);
	void processStarted();
	void processError();
	void processStopped();

public slots: /* Actions */
	void setDrawRectMode(bool state) {
		if(state) {
			drawMode = false;
			drawRectMode = true;
		} else
			drawRectMode = false;

		applyCurrentPixmap();
		setPointsToNull();
	}
	void setDrawMode(bool state) {
		if(state) {
			drawRectMode = false;
			drawMode = true;
		} else
			drawMode = false;

		applyCurrentPixmap();
		setPointsToNull();
	}
	void crop() {
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
	void invColors() {
		cv::bitwise_not(currentMat, currentMat);
		applyCurrentPixmap();
	}
	void showPrevImage() {
		currentMat = prevMat.clone();
		applyCurrentPixmap();
		setPointsToNull();
	}
	void setPointsToNull() {
		startPoint = cv::Point(0, 0);
		endPoint = cv::Point(0, 0);
	}
	void saveCurrentImage() {
		QString name = saveImage(currentMat, currentImageName, currentImagePath);
		if(name.isEmpty())
			emit error("Can't save image.", QMessageBox::Critical);
		else
			emit saved();
	}
	void saveCurrentImageAs() {
		QString fileName = QFileDialog::getSaveFileName(this, "Save image", currentImagePath, "Images (*.bmp)");
		if(fileName.isNull())
			return;
		QString name = getImageName(fileName);
		QString path = getImagePath(fileName);
		if(saveImage(currentMat, name, path).isEmpty())
			emit error("Can't save image.", QMessageBox::Critical);
		else
			emit saved();
	}
	void openImage() {
		QString imageName = QFileDialog::getOpenFileName(this, "Open image", QString(), "Images (*.bmp *.jpg *.png)");
		if(imageName.isNull())
			return;
		cv::Mat image = loadImage(imageName);
		if(image.data == NULL)
		{
			emit error("Opening file error.", QMessageBox::Critical);
			return;
		}
		currentMat = image;
		currentImageName = imageName;
		emit opened();
	}
	void process(bool state) {
		if(state) {
			if(!imageProcessor->stopped()) {
				emit error("Stop machine before process!", QMessageBox::Critical);
				emit processError();
				return;
			}
			cv::Rect skinRect(startPoint, endPoint);
			if(skinRect.area() == 0) {
				emit error("Draw rectangle.", QMessageBox::Warning);
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
			currentMat = imageProcessor->getBended();
			applyCurrentPixmap();
			setPointsToNull();
			emit processStopped();
		}
	}
	void apply() {
		prevMat = currentMat.clone();

		QString result;
		try {
			result = handRecCommands(currentMat);
		} catch(std::exception &e) {
			emit error(e.what(), QMessageBox::Critical);
			return;
		}

		// Draw result
		cv::putText(currentMat, result.toStdString(), cv::Point(0, currentMat.rows-1), cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(0, 0, 255), 2);
		applyCurrentPixmap();
		setPointsToNull();
	}

protected:
	void mousePressEvent (QMouseEvent *event) {
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

	void mouseMoveEvent(QMouseEvent *event) {
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

	void mouseReleaseEvent (QMouseEvent *event) {
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

private:
	ImageProcessor *imageProcessor;

	QPixmap currentPixmap;
	cv::Mat prevMat;
	cv::Mat currentMat;

	QString currentImageName;
	QString currentImagePath;

	cv::Point startPoint;
	cv::Point endPoint;
	
	bool drawRectMode;
	bool drawMode;

	void applyCurrentPixmap() {
		currentPixmap = QPixmap::fromImage(Mat2QImage(currentMat));
		setPixmap(currentPixmap);
	}
};

#endif // PHOTOLABEL