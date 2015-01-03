#ifndef PHOTOLABEL
#define PHOTOLABEL

#include <QLabel>
#include <QPoint>
#include "opencv2/opencv.hpp"
#include "ImageProcessor.h"

class PhotoLabel : public QLabel
{
	Q_OBJECT

public:
	PhotoLabel(ImageProcessor *imageProcessor, QWidget * parent = 0, Qt::WindowFlags f = 0);
	~PhotoLabel();

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
	void error(QString, ErrorLevels);
	void processStarted();
	void processError();
	void processStopped();

public slots:
	void setDrawRectMode(bool);
	void setDrawMode(bool);
	void crop();
	void invColors();
	void showPrevImage();
	void setPointsToNull();
	void saveCurrentImage();
	void saveCurrentImageAs();
	void openImage();
	void process(bool);
	void apply();

protected:
	void mousePressEvent (QMouseEvent *event);
	void mouseReleaseEvent (QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);

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

	void applyCurrentPixmap();
};

#endif // PHOTOLABEL