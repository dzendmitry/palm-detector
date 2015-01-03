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
	~ImageProcessor();

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
		MOPRPHOLOGY_DILATION,
		MERGE_PIXELS_AND_CANNY,	// NOT USED
		MORPHOLOGY_ERODE,
		MORPHOLOGY_OPENING,
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
	void setApertureFilter(int apertureFilter)
		{ this->apertureFilter = apertureFilter; }
	void setPixelClassifierTrained(bool pixelClassifierTrained = false)
		{ this->pixelClassifierTrained = pixelClassifierTrained; }
	void makeExp()
		{ doRGBArray = true; }

	void setLowThreshold(int lowThreshold)
		{ this->lowThreshold = lowThreshold; }
	void setRatio(int ratio)
		{ this->ratio = ratio; }
	void setAperture(int aperture)
		{ this->aperture = aperture; }
	void setDilateSize(int dilateSize)
		{ this->dilateSize = dilateSize; }
	void setErodeSize(int erodeSize)
		{ this->erodeSize = erodeSize; }
	void setOpenedMatSize(int openedMatSize)
		{ this->openedMatSize = openedMatSize; }
	void setCannyContourMergeEps(int cannyContourMergeEps)
		{ this->cannyContourMergeEps = cannyContourMergeEps; }
	void setHandThres(int handThreshold)
		{ this->handThreshold = handThreshold / 10.0; }
	void setTopHandThres(int topHandThres)
		{ this->topHandThres = topHandThres; }
	void setApproxPoly(int approxPoly)
		{ this->approxPoly = approxPoly / 10.0; }

private:
	void BGR2Gray(const cv::Mat& src, cv::Mat& dst); // NOT USED
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

	/* GET_FRAME */
	cv::Mat c1, c2, c3;
	int caddr;

	/* FIND_FACE_GET_SKIN_COLOR */
	cv::CascadeClassifier faceCascade;
	cv::Rect faceRect;
	cv::Mat face;

	/* GET_SKIN_COLOR */
	cv::Rect skinRect;
	cv::Mat skinColor;
	QMutex faceSkinRectMutex;

	/* TRAIN_PIXEL_CLASSIFIER */
	PixelClassifier pixelClassifier50;
	PixelClassifier pixelClassifier60;
	PixelClassifier pixelClassifier70;
	PixelClassifier pixelClassifier80;
	PixelClassifier pixelClassifier90;
	PixelClassifier pixelClassifier100;
	bool pixelClassifierTrained;

	/* PIXEL_RECOGNITION */
	cv::Mat classifiedSkin;
	cv::Mat classifiedSkinBin;
	cv::Mat classifiedSkinFiltered;
	cv::Mat colorizedFrame;
	QMutex classifiedSkinMutex;
	// Parameters
	int paramS;
	int apertureFilter;
	bool doRGBArray;

	/* CANNY */
	cv::Mat cannyEdges;
	cv::Mat cannyEdgesOrig;
	cv::Mat frameGray;
	cv::Mat frameGrayOrig;
	QMutex cannyEdgesMutex;
	// Parameters
	int lowThreshold;
	int ratio;
	int aperture;

	/* MOPRPHOLOGY_DILATION */
	cv::Mat dilated;
	QMutex dilatedMutex;
	// Parameters
	int dilateSize;

	/* MERGE_PIXELS_AND_CANNY */
	enum BinaryColors {BLACK, WHITE};
	cv::Mat pixelsAndCanny;
	bool horizontal;
	int mergeTimes;
	QMutex pixelsAndCannyMutex;

	/* MORPHOLOGY_ERODE */
	cv::Mat eroded;
	QMutex erodedMutex;
	// Parameters
	int erodeSize;

	/* MORPHOLOGY_OPENING */
	cv::Mat openedMat;
	cv::Mat opened8Mat;
	QMutex openedMatMutex;
	// Parameters
	int openedMatSize;
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