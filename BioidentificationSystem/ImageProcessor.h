#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <time.h>

#include "Camera.h"
#include "PixelClassifier.h"

class ImageProcessor : public Camera
{
	Q_OBJECT

public:
	ImageProcessor(QObject *parent = 0);

	static enum States {
		STOP,
		CLOSE_CAM,
		OPEN_CAM,

		GET_FRAME,
		FIND_FACE,
		GET_SKIN_COLOR,
		TRAIN_PIXEL_CLASSIFIER,
		PIXEL_RECOGNITION,
		CANNY,
		BEND, // Virtual state
		HAND_RECOGNITION,
	};

	void setStop(bool stop)
		{ this->stop = stop; }
	bool stopped()
		{ return stop; }
	
	void setStartState(States state)
		{ startState = state; }
	States getStartState()
		{ return startState; }
	
	bool getPhotoMode()
		{ return isPhotoMode; }

	cv::Rect getFaceRect();
	cv::Rect getSkinRect();
	cv::Mat getFace();
	cv::Mat getClassifiedSkin();
	cv::Mat getClassifiedSkinFiltered();
	cv::Mat getColorizedFrame();
	cv::Mat getCannyEdges();
	cv::Mat getCannyEdgesOrig();
	cv::Mat getMorphologyDilation();
	cv::Mat getPixelsAndCanny();
	cv::Mat getMorphologyErode();
	cv::Mat getMorphologyOpening();
	cv::Mat getBended();
	cv::Mat getHandRecognition();
	std::vector<QString> getDissimilarityMeasure();

	bool findFace();
	void getSkinColor();
	bool trainPixelClassifier();
	void pixelRecognition();
	void doCanny();
	void morphologyDilation();
	void mergePixelsAndCanny(bool horizontal);
	void morphologyErode();
	void morphologyOpening();
	bool handRecognition();

	void setPhotoProcessingMode(bool mode)
		{ this->photoProcessingMode = mode; }

signals:
	void photoTaken();
	void stateCompleted(ImageProcessor::States state);
	void postMessage(QString message);
	void photoModeChanged();

public slots:
	Q_INVOKABLE void processImage(ImageProcessor::States state);

	void imageProcState(bool state);
	void takePhoto();
	void setPhotoMode(bool mode);

	void setFrame(const cv::Mat& frame)
		{ this->frame = frame; faceRect = cv::Rect(); }
	void setSkinColor(const cv::Mat& skinColor)
		{ this->skinColor = skinColor; }
	void setSkinColorRect(const cv::Rect& skinColorRect)
		{ this->skinRect = skinColorRect; }
	
	void setKernelParamS(int paramS)
		{ this->paramS = paramS; pixelClassifierTrained = false; }
	void setPixelClassifierTrained(bool pixelClassifierTrained = false)
		{ this->pixelClassifierTrained = pixelClassifierTrained; }

	void setLowThreshold(int lowThreshold)
		{ this->lowThreshold = lowThreshold; }
	void setRatio(int ratio)
		{ this->ratio = ratio; }
	void setAperture(int aperture)
		{ this->aperture = aperture; }
	void setCannyContourMergeEps(int cannyContourMergeEps)
		{ this->cannyContourMergeEps = cannyContourMergeEps; }
	void setHandThres(int handThreshold)
		{ this->handThreshold = handThreshold / 10.0; }
	void setTopHandThres(int topHandThres)
		{ this->topHandThres = topHandThres; }
	void setApproxPoly(int approxPoly)
		{ this->approxPoly = approxPoly / 10.0; }

private:
	cv::Point getNearestCannyPoint(cv::Point contourPoint, cv::Point prevCanny = cv::Point(-1, -1));
	void mergeLogic(const std::vector<cv::Point>& contour_poly, std::vector<cv::Point>& mergedContour,  int point, cv::Point& prevAssignedP);
	clock_t startTime;
	clock_t allStartTime;

	/* Machine state */
	States startState;
	bool stop;

	/* PHOTO_MODE &&  */
	bool isPhotoMode;
	bool photoProcessingMode;

	/* FIND_FACE_GET_SKIN_COLOR */
	cv::CascadeClassifier faceCascade;
	cv::Rect faceRect;
	cv::Mat face;

	/* GET_SKIN_COLOR */
	cv::Rect skinRect;
	cv::Mat skinColor;
	QMutex faceSkinRectMutex;

	/* TRAIN_PIXEL_CLASSIFIER */
	PixelClassifier pixelClassifier;
	bool pixelClassifierTrained;

	/* PIXEL_RECOGNITION */
	cv::Mat colorizedFrame;
	cv::Mat classifiedSkin;
	QMutex classifiedSkinMutex;
	// Parameters
	int paramS;

	/* CANNY */
	cv::Mat cannyEdges;
	QMutex cannyEdgesMutex;
	// Parameters
	int lowThreshold;
	int ratio;
	int aperture;

	/* BEND */
	cv::Mat bended;
	int cannyContourMergeEps;

	/* HAND_RECOGNITION */
	cv::Mat recognizedHand;
	std::vector<QString> dissimilarityMeasure;
	QMutex recognizedHandMutex;
	// Parameters
	double handThreshold;
	double approxPoly;
	int topHandThres;
};

#endif // IMAGEPROCESSOR_H