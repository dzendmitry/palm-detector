#include <time.h>
#include <QMetaType>

#include "ImageProcessor.h"

ImageProcessor::ImageProcessor(QObject *parent)
	: Camera(parent),
	pixelClassifierTrained(false),
	isPhotoMode(false),
	photoProcessingMode(false)
{
	paramS = KERNEL_PARAM_S;
	lowThreshold = LOW_THRESHOLD;
	ratio = RATIO;
	aperture = APERTURE;
	cannyContourMergeEps = CANNY_CONTOUR_MERGE_EPS;
	handThreshold = HAND_THRESHOLD / 10.0;
	approxPoly = APPROX_POLY;
	topHandThres = TOP_HAND_THRES;
	qRegisterMetaType<ImageProcessor::States>("ImageProcessor::States");
}

void ImageProcessor::imageProcState(bool state)
{
	if(state){
		logFile = fopen(logFileName, "w");
		stop = false;
		startState = GET_FRAME;
		pixelClassifierTrained = false;
		isPhotoMode = true;
		processImage(OPEN_CAM);
	} else {
		fclose(logFile);
		cv::destroyAllWindows();
		processImage(CLOSE_CAM);
	}
}

void ImageProcessor::setPhotoMode(bool mode)
{
	isPhotoMode = mode;
	emit photoModeChanged();
}

void ImageProcessor::takePhoto()
{
	if(!Camera::takeFrame())
	{
		emit error("Can't take photo.", QMessageBox::Warning);
		return;
	}
	emit photoTaken();
}

cv::Rect ImageProcessor::getFaceRect()
{
	faceSkinRectMutex.lock();
	cv::Rect rect = faceRect;
	faceSkinRectMutex.unlock();
	return rect;
}

cv::Rect ImageProcessor::getSkinRect()
{
	faceSkinRectMutex.lock();
	cv::Rect rect = skinRect;
	faceSkinRectMutex.unlock();
	return rect;
}

cv::Mat ImageProcessor::getFace()
{
	faceSkinRectMutex.lock();
	cv::Mat mat = face.clone();
	faceSkinRectMutex.unlock();
	return mat;
}

cv::Mat ImageProcessor::getClassifiedSkin()
{
	classifiedSkinMutex.lock();
	cv::Mat mat = classifiedSkin.clone();
	classifiedSkinMutex.unlock();
	return mat;
}

cv::Mat ImageProcessor::getColorizedFrame()
{
	classifiedSkinMutex.lock();
	cv::Mat mat = colorizedFrame.clone();
	classifiedSkinMutex.unlock();
	return mat;
}

cv::Mat ImageProcessor::getCannyEdges()
{
	cannyEdgesMutex.lock();
	cv::Mat mat = cannyEdges.clone();
	cannyEdgesMutex.unlock();
	return mat;
}

cv::Mat ImageProcessor::getBended()
{
	recognizedHandMutex.lock();
	cv::Mat mat = bended.clone();
	recognizedHandMutex.unlock();
	return mat;
}

cv::Mat ImageProcessor::getHandRecognition()
{
	recognizedHandMutex.lock();
	cv::Mat mat = recognizedHand.clone();
	recognizedHandMutex.unlock();
	return mat;
}

std::vector<QString> ImageProcessor::getDissimilarityMeasure()
{
	recognizedHandMutex.lock();
	std::vector<QString> v = dissimilarityMeasure;
	recognizedHandMutex.unlock();
	return v;
}

void ImageProcessor::processImage(ImageProcessor::States state)
{
	ImageProcessor::States nextState;
	switch(state)
	{
	case CLOSE_CAM:
		stop = true;
		nextState = STOP;
		Camera::close();
		emit closed();
		break;
	case OPEN_CAM:
		if(!Camera::open()) {
			stop = true;
			nextState = STOP;
			emit error("Can't open camera.", QMessageBox::Critical);
			emit closed();
		} else {
			nextState = GET_FRAME;
			emit opened();
		}
		break;
	case GET_FRAME:
		allStartTime = startTime = clock();
		if(!Camera::isOpened()) {
			stop = true;
			nextState = STOP;
		} else if(!Camera::takeFrame()) {
			emit error("Can't read frame.", QMessageBox::Warning);
			nextState = CLOSE_CAM;
		} else if(!stop && isPhotoMode) {
			emit stateCompleted(GET_FRAME);
			nextState = GET_FRAME;
		} else if(!stop) {
			emit stateCompleted(GET_FRAME);
			nextState = FIND_FACE;
		} else
			nextState = STOP;
		break;
	case FIND_FACE:
		startTime = clock();
		if(stop) {
			nextState = STOP;
			break;
		}
		if(!findFace()) {
			emit error("Can't load face cascade file: " + FACE_CASCADE_NAME, QMessageBox::Critical);
			nextState = CLOSE_CAM;
		} else if(face.data == NULL) {
			if(!stop && !isPhotoMode)
				emit postMessage("Face has been LOST.");
			pixelClassifierTrained = false;
			nextState = GET_FRAME;
		} else {
			if(!stop && !isPhotoMode)
				emit postMessage("Face has been found.");
			nextState = GET_SKIN_COLOR;
		}
		if(DEBUG)
			writeTime("FIND_FACE", ((float)(clock()-startTime))/CLOCKS_PER_SEC);
		break;
	case GET_SKIN_COLOR:
		startTime = clock();
		if(stop) {
			nextState = STOP;
			break;
		}
		getSkinColor();
		if(!stop && !isPhotoMode)
			emit stateCompleted(GET_SKIN_COLOR);
		if(!pixelClassifierTrained)
			nextState = TRAIN_PIXEL_CLASSIFIER;
		else
			nextState = PIXEL_RECOGNITION;
		if(DEBUG)
			writeTime("GET_SKIN_COLOR", ((float)(clock()-startTime))/CLOCKS_PER_SEC);
		break;
	case TRAIN_PIXEL_CLASSIFIER:
		startTime = clock();
		if(stop) {
			nextState = STOP;
			break;
		}
		if(!trainPixelClassifier()) {
			emit error("Training pixel classifier failed.", QMessageBox::Critical);
			nextState = CLOSE_CAM;
		} else 
			nextState = PIXEL_RECOGNITION;
		if(DEBUG)
			writeTime("TRAIN_PIXEL_CLASSIFIER", ((float)(clock()-startTime))/CLOCKS_PER_SEC);
		break;
	case PIXEL_RECOGNITION:
		startTime = clock();
		if(stop) {
			nextState = STOP;
			break;
		}
		if(ADVANCED_OUTPUT)
			cv::imshow("PIXEL_RECOGNITION", classifiedSkin);
		pixelRecognition();
		if(!stop && !isPhotoMode)
			emit stateCompleted(PIXEL_RECOGNITION);
		nextState = CANNY;
		if(DEBUG)
			writeTime("PIXEL_RECOGNITION", ((float)(clock()-startTime))/CLOCKS_PER_SEC);
		break;
	case CANNY:
		startTime = clock();
		if(stop) {
			nextState = STOP;
			break;
		}
		if(ADVANCED_OUTPUT)
			cv::imshow("CANNY", cannyEdges);
		doCanny();
		if(!stop && !isPhotoMode)
			emit stateCompleted(CANNY);
		nextState = HAND_RECOGNITION;
		if(DEBUG)
			writeTime("CANNY", ((float)(clock()-startTime))/CLOCKS_PER_SEC);
		break;
	case HAND_RECOGNITION:
		startTime = clock();
		if(stop || !handRecognition()) {
			nextState = STOP;
			break;
		}
		if(!stop && !isPhotoMode) {
			emit stateCompleted(BEND);
			emit stateCompleted(HAND_RECOGNITION);
		}
		nextState = startState;
		if(DEBUG)
		{
			writeTime("HAND_RECOGNITION", ((float)(clock()-startTime))/CLOCKS_PER_SEC);
			writeTime("ALL", ((float)(clock()-allStartTime))/CLOCKS_PER_SEC);
			writeNewLine();
		}
		break;
	default:
		nextState = STOP;
	}

	if(!ADVANCED_OUTPUT)
		cv::destroyAllWindows();

	if(nextState != STOP)
		QMetaObject::invokeMethod(this, 
			"processImage",
			Qt::QueuedConnection,
			Q_ARG(ImageProcessor::States, nextState));
}

bool ImageProcessor::findFace()
{
	if(faceCascade.empty() && !faceCascade.load(FACE_CASCADE_NAME.toStdString()))
		return false;
	try {
		std::vector<cv::Rect> faces;
		cv::Mat frameGray;
		cv::cvtColor(frame, frameGray, cv::COLOR_BGR2GRAY);
		cv::equalizeHist(frameGray, frameGray);
		faceCascade.detectMultiScale(frameGray, faces, 1.1, 2, 0|cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));
		faceSkinRectMutex.lock();
		faceRect = cv::Rect();
		face.release();
		if(!faces.empty())
		{
			foreach(const cv::Rect& curFace, faces)
				if(curFace.area() > faceRect.area())
					faceRect = curFace;
			face = frame(faceRect).clone();
			cv::resize(face, face, cv::Size(FACE_WIDTH, FACE_HEIGHT));
		}
	} catch(cv::Exception& e) {
		qDebug() << e.what();
	}
	faceSkinRectMutex.unlock();
	return true;
}

void ImageProcessor::getSkinColor()
{
	faceSkinRectMutex.lock();
	skinRect = cv::Rect(cv::Point(FACE_WIDTH*0.3, FACE_HEIGHT*0.5), cv::Point(FACE_WIDTH*0.7, FACE_HEIGHT*0.64));
	faceSkinRectMutex.unlock();
	skinColor = face(skinRect).clone();
}

bool ImageProcessor::trainPixelClassifier()
{
	std::vector<Pixel> pixels;
	for(int i = 0; i < skinColor.rows; i++)
	{
		for(int j = 0; j < skinColor.cols; j++)
		{
			cv::Vec3b bgrPixel = skinColor.at<cv::Vec3b>(i, j);
			pixels.push_back(Pixel(bgrPixel.val[2], bgrPixel.val[1], bgrPixel.val[0]));
		}
	}
	pixelClassifierTrained  = pixelClassifier.train(pixels, paramS, 1, 0.001);
	if(!pixelClassifierTrained)
		return false;
	return true;
}

void ImageProcessor::pixelRecognition()
{
	classifiedSkinMutex.lock();
	colorizedFrame = frame.clone();
	classifiedSkin = cv::Mat(frame.rows, frame.cols, CV_8UC1);
	classifiedSkin.setTo(cv::Scalar(0));
	#pragma omp parallel num_threads(OPENMP_THREADS)
	{
		#pragma omp for
		for(int i = 0; i < frame.rows; i++)
		{
			for(int j = 0; j < frame.cols; j++)
			{
				cv::Vec3b bgrPixel = frame.at<cv::Vec3b>(i, j);
				if(faceRect.contains(cv::Point(j, i)))
					continue;
				if( pixelClassifier.classify(Pixel(bgrPixel.val[2], bgrPixel.val[1], bgrPixel.val[0])) )
				{
					classifiedSkin.at<uchar>(i, j) = 255;
					colorizedFrame.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 255, 0);
				}
			}
		}
	}
	classifiedSkinMutex.unlock();
}

void ImageProcessor::doCanny()
{
	cv::Mat frameGray;
	cv::cvtColor(frame, frameGray, cv::COLOR_BGR2GRAY);
	cv::blur(frameGray, frameGray, cv::Size(3,3));
	cannyEdgesMutex.lock();
	cv::Canny(frameGray, cannyEdges, lowThreshold, lowThreshold*ratio, aperture);
	cannyEdgesMutex.unlock();
}

bool ImageProcessor::handRecognition()
{
	recognizedHandMutex.lock();
	cv::cvtColor(classifiedSkin, bended, CV_GRAY2RGB);
	recognizedHand = frame.clone();
	dissimilarityMeasure.clear();
	std::vector<QString>().swap(dissimilarityMeasure);
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(classifiedSkin, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
	std::vector<std::vector<cv::Point>> mergedContours = contours;
	for( int i = 0; i < contours.size(); i++ ) {
		cv::Rect boundRect = cv::boundingRect(cv::Mat(contours[i]));
		if( (boundRect.width > MIN_WH) && (boundRect.height > MIN_WH) && 
			(photoProcessingMode ||
			((boundRect.width > (faceRect.width * 0.5)) && (boundRect.height > (faceRect.height * 0.5)) &&
			(boundRect.width < (faceRect.width * topHandThres)) && (boundRect.height < (faceRect.height * topHandThres))))) 
		{
			if(SHOW_CONTOURS)
			{
				clock_t bendingStartTime = clock();
				int startPoint = 0;
				cv::Point assignedPoint(-1, -1);
				// Find start point
				for(int point = 0; point < contours[i].size(); point++) {
					if(cannyEdges.at<uchar>(contours[i][point]) > 0) {
						startPoint = point;
						assignedPoint = contours[i][point];
						break;
					}
					cv::Point nearestCannyPoint = getNearestCannyPoint(contours[i][point]);
					if(nearestCannyPoint == cv::Point(-1, -1))
						continue;
					if(assignedPoint == cv::Point(-1, -1)) {
						startPoint = point;
						assignedPoint = nearestCannyPoint;
					} else {
						if( (abs(nearestCannyPoint.x - contours[i][point].x) + abs(nearestCannyPoint.y - contours[i][point].y)) < 
							(abs(assignedPoint.x - contours[i][startPoint].x) + abs(assignedPoint.y - contours[i][startPoint].y)) ) 
						{
								startPoint = point;
								assignedPoint = nearestCannyPoint;
						}
					}
				}
				// Bypass contour
				if(assignedPoint != cv::Point(-1, -1)) {
					cv::Point prevAssignedP = assignedPoint;
					for(int point = startPoint+1; point < contours[i].size(); point++) {
						mergeLogic(contours[i], mergedContours[i], point, prevAssignedP);
					}
					prevAssignedP = assignedPoint;
					for(int point = startPoint-1; point >= 0; point--) {
						mergeLogic(contours[i], mergedContours[i], point, prevAssignedP);
					}
				}
				
				if(DEBUG) {
					writeTime("BENDING", ((float)(clock()-bendingStartTime))/CLOCKS_PER_SEC);
				}

				// Contour filling. White color
				cv::drawContours(bended, mergedContours, i, cv::Scalar(255, 255, 255), -1);
			}
			boundRect = cv::boundingRect(cv::Mat(mergedContours[i]));
			cv::Mat applicant(bended.rows, bended.cols, CV_8UC3);
			cv::drawContours(applicant, mergedContours, i, cv::Scalar(255, 255, 255), -1);
			applicant = applicant(boundRect).clone();
			cv::Mat applicantContoursMat;
			cv::cvtColor(applicant, applicantContoursMat, CV_BGR2GRAY);
			std::vector<std::vector<cv::Point>> appContours;
			cv::findContours(applicantContoursMat, appContours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
			cv::drawContours(applicant, appContours, -1, cv::Scalar(255, 255, 255), -1);
			cv::bitwise_not(applicant, applicant);
			QString result;
			try {
				clock_t handRecStartTime = clock();
				result = handRecCommands(applicant);
				if(DEBUG) {
					writeTime("HAND_REC_PROC", ((float)(clock()-handRecStartTime))/CLOCKS_PER_SEC);
				}
			} catch(std::exception &e) {
				emit error(e.what(), QMessageBox::Critical);
				recognizedHandMutex.unlock();
				return false;
			}
			if(SHOW_CONTOURS)
			{
				cv::drawContours(bended, contours, i, cv::Scalar(0, 0, 255), 1);
				cv::drawContours(bended, mergedContours, i, cv::Scalar(0, 255, 0), 1);
			}
			if(result.toDouble() <= handThreshold) {
				cv::drawContours(recognizedHand, mergedContours, i, cv::Scalar(0, 255, 255), 1);
				cv::putText(recognizedHand, result.toStdString(), cv::Point(0, recognizedHand.rows-1), cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(0, 255, 255), 2);
				dissimilarityMeasure.push_back(result);
			}
		}
	}
	recognizedHandMutex.unlock();
	return true;
}

void ImageProcessor::mergeLogic(const std::vector<cv::Point>& contour_poly, std::vector<cv::Point>& mergedContour,  int point, cv::Point& prevAssignedP)
{
	if(cannyEdges.at<uchar>(contour_poly[point]) > 0) {
		prevAssignedP = contour_poly[point];
		return;
	}
	cv::Point nearestCannyPoint = getNearestCannyPoint(contour_poly[point], prevAssignedP);
	if(nearestCannyPoint == cv::Point(-1, -1)) {
		prevAssignedP = contour_poly[point];
	} else {
		mergedContour[point] = nearestCannyPoint;
		prevAssignedP = nearestCannyPoint;
	}
}

cv::Point ImageProcessor::getNearestCannyPoint(cv::Point contourPoint, cv::Point prevCanny)
{
	cv::Point nearestPoint(-1, -1);
	std::vector<cv::Point> nearCannyPixels;
	for(int j = 1; j <= cannyContourMergeEps; j++)
	{
		if( ((contourPoint.x - j) < 0) || ((contourPoint.x + j) >= cannyEdges.cols) ||  
			((contourPoint.y - j) < 0) || ((contourPoint.y + j) >= cannyEdges.rows) ) // Check screen boundaries
			break;
		cv::Point point(contourPoint.x - j, contourPoint.y - j);
		for(int k = 0; k < (2 * j); k++, point.x++) // From top-left corner to top-right corner
			if( cannyEdges.at<uchar>(point) > 0 )
				nearCannyPixels.push_back(cv::Point(point));
		for(int k = 0; k < (2 * j); k++, point.y++) // From top-right corner to bottom-right corner
			if( cannyEdges.at<uchar>(point) > 0 )
				nearCannyPixels.push_back(cv::Point(point));
		for(int k = 0; k < (2 * j); k++, point.x--) // From bottom-right corner to bottom-left corner
			if( cannyEdges.at<uchar>(point) > 0 )
				nearCannyPixels.push_back(cv::Point(point));
		for(int k = 0; k < (2 * j); k++, point.y--) // From bottom-left corner to top-left corner
			if( cannyEdges.at<uchar>(point) > 0 )
				nearCannyPixels.push_back(cv::Point(point));
		// Find nearest Canny pixel
		if(nearCannyPixels.size())
		{
			if(prevCanny == cv::Point(-1, -1)) {
				nearestPoint = nearCannyPixels[0];
				for(int k = 1; k < nearCannyPixels.size(); k++)
					if( (abs(contourPoint.x - nearCannyPixels[k].x) < abs(contourPoint.x - nearestPoint.x)) ||
						(abs(contourPoint.y - nearCannyPixels[k].y) < abs(contourPoint.y - nearestPoint.y)) )
						nearestPoint = nearCannyPixels[k];
				break;
			} else {
				if(nearestPoint == cv::Point(-1, -1))
					nearestPoint = nearCannyPixels[0];
				for(int k = 0; k < nearCannyPixels.size(); k++) {
					if(nearCannyPixels[k] == prevCanny)
						continue;
					if( (abs(nearCannyPixels[k].x - contourPoint.x) + abs(nearCannyPixels[k].y - contourPoint.y) + abs(nearCannyPixels[k].x - prevCanny.x) + abs(nearCannyPixels[k].y - prevCanny.y)) < 
						(abs(nearestPoint.x - contourPoint.x) + abs(nearestPoint.y - contourPoint.y) + abs(nearestPoint.x - prevCanny.x) + abs(nearestPoint.y - prevCanny.y)) )
						nearestPoint = nearCannyPixels[k];
				}
			}
		}
	}
	return nearestPoint;
}