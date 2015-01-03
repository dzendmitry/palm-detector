#include <time.h>
#include <QMetaType>

#include "ImageProcessor.h"

ImageProcessor::ImageProcessor(QObject *parent)
	: Camera(parent),
	pixelClassifierTrained(false),
	horizontal(true),
	mergeTimes(0),
	isPhotoMode(false),
	photoProcessingMode(false),
	caddr(1),
	doRGBArray(false)
{
	paramS = KERNEL_PARAM_S;
	apertureFilter = APERTURE_FILTER;
	lowThreshold = LOW_THRESHOLD;
	ratio = RATIO;
	aperture = APERTURE;
	dilateSize = DILATION;
	erodeSize = EROSION;
	openedMatSize = OPENING;
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
		caddr = 1;
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
		emit error("Can't take photo.", WARNING);
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

cv::Mat ImageProcessor::getClassifiedSkinFiltered()
{
	classifiedSkinMutex.lock();
	cv::Mat mat = classifiedSkinFiltered.clone();
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

cv::Mat ImageProcessor::getCannyEdgesOrig()
{
	cannyEdgesMutex.lock();
	cv::Mat mat = cannyEdgesOrig.clone();
	cannyEdgesMutex.unlock();
	return mat;
}

cv::Mat ImageProcessor::getMorphologyDilation()
{
	dilatedMutex.lock();
	cv::Mat mat = dilated.clone();
	dilatedMutex.unlock();
	return mat;
}

cv::Mat ImageProcessor::getPixelsAndCanny()
{
	pixelsAndCannyMutex.lock();
	cv::Mat mat = pixelsAndCanny.clone();
	pixelsAndCannyMutex.unlock();
	return mat;
}

cv::Mat ImageProcessor::getMorphologyErode()
{
	erodedMutex.lock();
	cv::Mat mat = eroded.clone();
	erodedMutex.unlock();
	return mat;
}

cv::Mat ImageProcessor::getMorphologyOpening()
{
	openedMatMutex.lock();
	cv::Mat mat = openedMat.clone();
	openedMatMutex.unlock();
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
			emit error("Can't open camera.", CRITICAL);
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
			emit error("Can't read frame.", WARNING);
			nextState = CLOSE_CAM;
		} else if(!stop && isPhotoMode) {
			emit stateCompleted(GET_FRAME);
			nextState = GET_FRAME;
		} else if(!stop) {
			/* SMOOTH_FILTER */
			/*int currCaddr = caddr % THRES_CADDR;
			if( currCaddr == 0 ) {
				c3 = frame.clone();
				for(int i = 0; i < frame.rows; i++)
				{
					for(int j = 0; j < frame.cols; j++)
					{
						frame.at<cv::Vec3b>(i, j).val[0] = (c1.at<cv::Vec3b>(i, j).val[0] + c2.at<cv::Vec3b>(i, j).val[0] + c3.at<cv::Vec3b>(i, j).val[0]) / 3;
						frame.at<cv::Vec3b>(i, j).val[1] = (c1.at<cv::Vec3b>(i, j).val[1] + c2.at<cv::Vec3b>(i, j).val[1] + c3.at<cv::Vec3b>(i, j).val[1]) / 3;
						frame.at<cv::Vec3b>(i, j).val[2] = (c1.at<cv::Vec3b>(i, j).val[2] + c2.at<cv::Vec3b>(i, j).val[2] + c3.at<cv::Vec3b>(i, j).val[2]) / 3;
					}
				}
				caddr = 1;
				emit stateCompleted(GET_FRAME);
				if(DEBUG)
					writeTime("GET_FRAME", ((float)(clock()-startTime))/CLOCKS_PER_SEC);
				nextState = FIND_FACE;
			} else {
				switch(currCaddr) {
				case 1:
					c1 = frame.clone();
					break;
				case 2:
					c2 = frame.clone();
					break;
				}
				caddr++;
				nextState = GET_FRAME;
			}*/
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
			emit error("Can't load face cascade file: " + FACE_CASCADE_NAME, CRITICAL);
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
			emit error("Training pixel classifier failed.", CRITICAL);
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
		{
			cv::imshow("CANNY", cannyEdges);
			//cv::imshow("CANNY_ORIG", cannyEdgesOrig);
			//cv::imshow("FRAME_GRAY", frameGray);
			//cv::imshow("FRAME_GRAY_ORIG", frameGrayOrig);
		}
		doCanny();
		if(!stop && !isPhotoMode)
			emit stateCompleted(CANNY);
		mergeTimes = 0;
		nextState = MOPRPHOLOGY_DILATION;
		if(DEBUG)
			writeTime("CANNY", ((float)(clock()-startTime))/CLOCKS_PER_SEC);
		break;
	case MOPRPHOLOGY_DILATION: // NOT USED
		{
			startTime = clock();
			if(stop) {
				nextState = STOP;
				break;
			}
			if(ADVANCED_OUTPUT)
				cv::imshow("MOPRPHOLOGY_DILATION", dilated);
			morphologyDilation();
			if(!stop && !isPhotoMode)
				emit stateCompleted(MOPRPHOLOGY_DILATION);
			nextState = MORPHOLOGY_ERODE;
			if(DEBUG)
				writeTime("MOPRPHOLOGY_DILATION", ((float)(clock()-startTime))/CLOCKS_PER_SEC);
		}
		break;
	case MERGE_PIXELS_AND_CANNY:
		startTime = clock();
		if(stop) {
			nextState = STOP;
			break;
		}
		if(ADVANCED_OUTPUT)
			cv::imshow("MERGE_PIXELS_AND_CANNY", pixelsAndCanny);
		switch(mergeTimes)
		{
		case 0:
			mergePixelsAndCanny(true);
			mergeTimes++;
			nextState = MERGE_PIXELS_AND_CANNY;
		case 1:
			mergePixelsAndCanny(false);
			mergeTimes++;
			nextState = MERGE_PIXELS_AND_CANNY;
		default:
			if(!stop && !isPhotoMode)
				emit stateCompleted(MERGE_PIXELS_AND_CANNY);
			mergeTimes = 0;
			nextState = MORPHOLOGY_ERODE;
		}
		if(DEBUG)
			writeTime("MERGE_PIXELS_AND_CANNY", ((float)(clock()-startTime))/CLOCKS_PER_SEC);
		break;
	case MORPHOLOGY_ERODE:
		startTime = clock();
		if(stop) {
			nextState = STOP;
			break;
		}
		if(ADVANCED_OUTPUT)
			cv::imshow("MORPHOLOGY_ERODE", eroded);
		morphologyErode();
		if(!stop && !isPhotoMode)
			emit stateCompleted(MORPHOLOGY_ERODE);
		nextState = MORPHOLOGY_OPENING;
		if(DEBUG)
			writeTime("MORPHOLOGY_ERODE", ((float)(clock()-startTime))/CLOCKS_PER_SEC);
		break;
	case MORPHOLOGY_OPENING:
		startTime = clock();
		if(stop) {
			nextState = STOP;
			break;
		}
		if(ADVANCED_OUTPUT)
			cv::imshow("MORPHOLOGY_OPENING", openedMat);
		morphologyOpening();
		//if(!stop && !isPhotoMode)
		//	emit stateCompleted(MORPHOLOGY_OPENING);
		nextState = HAND_RECOGNITION;
		if(DEBUG)
			writeTime("MORPHOLOGY_OPENING", ((float)(clock()-startTime))/CLOCKS_PER_SEC);
		break;
	case HAND_RECOGNITION:
		startTime = clock();
		if(stop || !handRecognition()) {
			nextState = STOP;
			break;
		}
		if(!stop && !isPhotoMode) {
			emit stateCompleted(MORPHOLOGY_OPENING);
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
	int p = (paramS-50) / 6;
	pixelClassifierTrained  = pixelClassifier50.train(pixels, 50 + p, 1, 0.001);
	pixelClassifierTrained &= pixelClassifier60.train(pixels, 50+2*p, 1, 0.001);
	pixelClassifierTrained &= pixelClassifier70.train(pixels, 50+3*p, 1, 0.001);
	pixelClassifierTrained &= pixelClassifier80.train(pixels, 50+4*p, 1, 0.001);
	pixelClassifierTrained &= pixelClassifier90.train(pixels, 50+5*p, 1, 0.001);
	pixelClassifierTrained &= pixelClassifier100.train(pixels,50+6*p, 1, 0.001);
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
	classifiedSkinBin = cv::Mat(frame.rows, frame.cols, CV_8UC1);
	classifiedSkinBin.setTo(cv::Scalar(0));
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
				if( pixelClassifier100.classify(Pixel(bgrPixel.val[2], bgrPixel.val[1], bgrPixel.val[0])) ) 
				{
					classifiedSkin.at<uchar>(i, j) = 100;
					classifiedSkinBin.at<uchar>(i, j) = 255;
					colorizedFrame.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 100, 0);
					if( pixelClassifier90.classify(Pixel(bgrPixel.val[2], bgrPixel.val[1], bgrPixel.val[0])) ) 
					{
						classifiedSkin.at<uchar>(i, j) = 130;
						colorizedFrame.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 130, 0);
						if( pixelClassifier80.classify(Pixel(bgrPixel.val[2], bgrPixel.val[1], bgrPixel.val[0])) ) 
						{
							classifiedSkin.at<uchar>(i, j) = 160;
							colorizedFrame.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 160, 0);
							if( pixelClassifier70.classify(Pixel(bgrPixel.val[2], bgrPixel.val[1], bgrPixel.val[0])) ) 
							{
								classifiedSkin.at<uchar>(i, j) = 190;
								colorizedFrame.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 190, 0);
								if( pixelClassifier60.classify(Pixel(bgrPixel.val[2], bgrPixel.val[1], bgrPixel.val[0])) ) 
								{
									classifiedSkin.at<uchar>(i, j) = 220;
									colorizedFrame.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 220, 0);
									if( pixelClassifier50.classify(Pixel(bgrPixel.val[2], bgrPixel.val[1], bgrPixel.val[0])) )
									{
										classifiedSkin.at<uchar>(i, j) = 250;
										colorizedFrame.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 250, 0);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	/* APERTURE_FILTER */
	classifiedSkinFiltered = classifiedSkin.clone();
	/*#pragma omp parallel num_threads(OPENMP_THREADS)
	{
		unsigned colors[2];
		#pragma omp for
		for(int i = 0; i < classifiedSkin.rows; i++)
		{
			for(int j = 0; j < (classifiedSkin.cols - apertureFilter); j++)
			{
				colors[BLACK] = colors[WHITE] = 0;
				for(int k = j; k < (j + apertureFilter); k++)
				{
					if(classifiedSkin.at<uchar>(i, k) > 0)
						colors[WHITE]++;
					else
						colors[BLACK]++;
				}
				uchar color = 255;
				if(colors[BLACK] > colors[WHITE])
					color = 0;
				for(int k = j; k < (j + apertureFilter); k++)
				{
					uchar pixelColor = classifiedSkin.at<uchar>(i, k);
					if( (color == 0) || ( (color == 255) && (pixelColor == 0) ) )
						classifiedSkinFiltered.at<uchar>(i, k) = color;
				}
			}
		}
	}*/
	
	/* 3-D ARRAY */
	if(doRGBArray)
	{
		float ***rgbArray = new float**[256];
		for(int i = 0; i < 256; i++)
			rgbArray[i] = new float*[256];
		for(int i = 0; i < 256; i++)
			for(int j = 0; j < 256; j++)
				rgbArray[i][j] = new float[256];

		#pragma omp parallel num_threads(OPENMP_THREADS)
		{
		#pragma omp for
		for(int r = 0; r < 256; r++) // R 
			for(int g = 0; g < 256; g++) // G
				for(int b = 0; b < 256; b++) // B
				{
					rgbArray[r][g][b] = 0;
					if( pixelClassifier100.classify(Pixel(r, g, b)) ) 
					{
						rgbArray[r][g][b] = 0.1;
						if( pixelClassifier90.classify(Pixel(r, g, b)) ) 
						{
							rgbArray[r][g][b] = 0.2;
							if( pixelClassifier80.classify(Pixel(r, g, b)) ) 
							{
								rgbArray[r][g][b] = 0.4;
								if( pixelClassifier70.classify(Pixel(r, g, b)) ) 
								{
									rgbArray[r][g][b] = 0.6;
									if( pixelClassifier60.classify(Pixel(r, g, b)) ) 
									{
										rgbArray[r][g][b] = 0.8;
										if( pixelClassifier50.classify(Pixel(r, g, b)) )
											rgbArray[r][g][b] = 1.0;
									}
								}
							}
						}
					}
				}
		}

				/* SAVE SKIN RECT */
				FILE *skinRectF = fopen("ex/skin_rect.txt", "w");
				if(skinRectF)
					fprintf(skinRectF, "x: %d y: %d\nwidth: %d height: %d", skinRect.x, skinRect.y, skinRect.width, skinRect.height);
				fclose(skinRectF);
				/* SAVE SKIN COLOR */
				saveImage(skinColor, "skin_rect.bmp", "ex/");
				/* SAVE IMG */
				saveImage(frame, "frame.bmp", "ex/");
				/* SAVE IMG WITH SKIN RECT */
				cv::Mat mat = frame.clone();
				cv::rectangle(mat, skinRect, cv::Scalar(0, 0, 255), 1);
				saveImage(mat, "frame_skin_rect.bmp", "ex/");
				/* SAVE COLORIZED IMG */
				saveImage(colorizedFrame, "colorized_frame.bmp", "ex/");
				/* SAVE RGB ARRAY */
				FILE *rgbArrayF = fopen("ex/rgb_array.txt", "w");
				for(int r = 0; r < 256; r++)
					for(int g = 0; g < 256; g++)
						for(int b = 0; b < 256; b++)
							if(rgbArrayF)
								fprintf(rgbArrayF, "%.1f ", rgbArray[r][g][b]);
				fclose(rgbArrayF);

				for(int j = 0; j < 256; j++)
					for(int k = 0; k < 256; k++)
						delete [] rgbArray[j][k];
				for(int i = 0; i < 256; i++)
					delete [] rgbArray[i];
				delete [] rgbArray;

		doRGBArray = false;
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

void ImageProcessor::morphologyDilation()
{
	cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size( 2*dilateSize + 1, 2*dilateSize+1 ), cv::Point( dilateSize, dilateSize ) );
	dilatedMutex.lock();
	cv::dilate(classifiedSkinBin, dilated, element);
	dilatedMutex.unlock();
}

void ImageProcessor::mergePixelsAndCanny(bool horizontal)
{
	int X = 0, Y = 0;
	if(horizontal)
	{
		X = classifiedSkinBin.rows;
		Y = classifiedSkinBin.cols;
	} else {
		X = classifiedSkinBin.cols;
		Y = classifiedSkinBin.rows;
	}
	pixelsAndCannyMutex.lock();
	if(horizontal)
		pixelsAndCanny = dilated.clone();
	#pragma omp parallel num_threads(OPENMP_THREADS)
	{
		long colors[2] = {0, 0};
		#pragma omp for
		for(int i = 0; i < X; i++)
		{
			colors[BLACK] = colors[WHITE] = 0;
			int lastCorner = 0;
			for(int j = 0; j < Y; j++)
			{
				uchar cannyPixel;
				uchar classifiedPixel;
				if(horizontal)
				{
					classifiedPixel = classifiedSkinBin.at<uchar>(i, j);
					cannyPixel = dilated.at<uchar>(i,j);
				} else {
					classifiedPixel = classifiedSkinBin.at<uchar>(j, i);
					cannyPixel = dilated.at<uchar>(j, i);
				}

				if(cannyPixel > 0)
				{
					uchar currentColor = 255;
					if(colors[BLACK] > colors[WHITE])
						currentColor = 0;
					for(int k = lastCorner; k < j; k++)
					{
						if(horizontal)
							pixelsAndCanny.at<uchar>(i, k) = currentColor;
						else 
							pixelsAndCanny.at<uchar>(k, i) = currentColor;
					}
					lastCorner = j + 1;
					colors[BLACK] = colors[WHITE] = 0;
				}
				else
				{
					if(!classifiedPixel)
						colors[BLACK]++;
					else 
						colors[WHITE]++;
				}
			}
		}
	}
	pixelsAndCannyMutex.unlock();
}

void ImageProcessor::morphologyErode()
{
	cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size( 2*erodeSize + 1, 2*erodeSize+1 ), cv::Point( erodeSize, erodeSize ) );
	erodedMutex.lock();
	cv::erode(dilated, eroded, element);
	erodedMutex.unlock();
}

void ImageProcessor::morphologyOpening()
{
	cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size( 2*openedMatSize + 1, 2*openedMatSize+1 ), cv::Point( openedMatSize, openedMatSize ) );
	openedMatMutex.lock();
	cv::morphologyEx(eroded, openedMat, cv::MORPH_OPEN, element);
	opened8Mat = openedMat.clone();					 // ATTENTION! DIRTY HACK!
	cv::cvtColor(openedMat, openedMat, CV_GRAY2RGB); // ATTENTION! DIRTY HACK!
	openedMatMutex.unlock();
}

bool ImageProcessor::handRecognition()
{
	recognizedHandMutex.lock();
	recognizedHand = frame.clone();
	dissimilarityMeasure.clear();
	std::vector<QString>().swap(dissimilarityMeasure);
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(opened8Mat, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
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
				/*std::vector<cv::Point> tmpv;
				for(int pointI = 0; pointI < contours[i].size(); pointI++) 
				{
					int neighbours = 0;
					for(int pointJ = 0; pointJ < contours[i].size(); pointJ++) 
					{
						if( contours[i][pointI] == contours[i][pointJ] )
							continue;
						if( abs(contours[i][pointI].x - contours[i][pointJ].x) > 1 )
							continue;
						if( abs(contours[i][pointI].y - contours[i][pointJ].y) > 1 )
							continue;
						neighbours++;
					}
					if(neighbours == 2)
						tmpv.push_back(contours[i][pointI]);
				}
				mergedContours[i] = contours[i] = tmpv;*/

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
				
				// Contour filling. White color
				cv::drawContours(openedMat, mergedContours, i, cv::Scalar(255, 255, 255), -1);
			}
			boundRect = cv::boundingRect(cv::Mat(mergedContours[i]));
			cv::Mat applicant = openedMat(boundRect).clone();
			cv::bitwise_not(applicant, applicant);
			QString result;
			try {
				result = handRecCommands(applicant);
			} catch(std::exception &e) {
				emit error(e.what(), CRITICAL);
				recognizedHandMutex.unlock();
				return false;
			}
			if(SHOW_CONTOURS)
			{
				cv::drawContours(openedMat, contours, i, cv::Scalar(0, 0, 255), 1);
				cv::drawContours(openedMat, mergedContours, i, cv::Scalar(0, 255, 0), 1);
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

void ImageProcessor::BGR2Gray(const cv::Mat& src, cv::Mat& dst)	// NOT USED
{
	for(int i = 0; i < src.rows; i++)
		for(int j = 0; j < src.cols; j++)
			dst.at<uchar>(i, j) = ( (0.3 * src.at<cv::Vec3b>(i, j).val[0]) + (0.3 * src.at<cv::Vec3b>(i, j).val[1]) + (0.4 * src.at<cv::Vec3b>(i, j).val[2]) );
}

ImageProcessor::~ImageProcessor()
{}