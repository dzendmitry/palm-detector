#ifndef GENERAL_H
#define GENERAL_H

#include "opencv2/opencv.hpp"
#include <QImage>
#include <QStringList>
#include <QDebug>

typedef std::vector<cv::Point> vecOfPoints;

// General 
extern bool ADVANCED_OUTPUT;
extern bool CANNY_STEP;
extern bool SHOW_CONTOURS;
const bool DEBUG = false;

enum ErrorLevels {
	CRITICAL,
	WARNING,
	INFORMATION
};

const int CRITICAL_READ_FRAME_FAILS = 24;
const int OPENMP_THREADS = 2;

// Mean 3 frames
const int THRES_CADDR = 3;

// Aperture filter
const int APERTURE_FILTER = 10;

// Pixel classifier
const int KERNEL_PARAM_S = 100;

// Canny
const int LOW_THRESHOLD = 30;
const int RATIO = 3;
const int APERTURE = 7;

// Morphology
const int DILATION = 0;
const int EROSION = 0;
const int OPENING = 0;
const int CANNY_CONTOUR_MERGE_EPS = 5;

// Hand recognition
const QString HANDS_COMPARE_DIR = "handscompare/";
const QString HANDS_BMP_NAME = "hand.bmp";
const QString STANDARD_HAND_BMP_NAME = "etalon.bmp";
const QString SKL_FILTER = "*.skl";
const QString SEQ_FILTER = "*.seq";
const QString DISSIMILARITY_MEASURE = "DissimilarityMeasure.txt";
const int REGULARIZATION = 3;
const double APPROXIMATION = 0.05;
const int HAND_THRESHOLD = 22;
const int APPROX_POLY = 0;
const int MIN_WH = 50;
const int TOP_HAND_THRES = 2;

// Photo
const QString PHOTO_PATH = "photo/";
const QStringList IMAGE_FORMAT("*.bmp");
const int THUMBNAIL_WIDTH = 106;
const int THUMBNAIL_HEIGHT = 80;

// Find face
const QString FACE_CASCADE_NAME = "haarcascades/haarcascade_frontalface_alt.xml";
const int FACE_WIDTH = 150;
const int FACE_HEIGHT = 150;

QImage Mat2QImage(const cv::Mat& frame);
cv::Mat QImage2Mat(const QImage& image);

QString saveImage(const cv::Mat& image, const QString& name = QString(), const QString& dir = PHOTO_PATH);
cv::Mat loadImage(const QString& imageName);

cv::Mat buildThumbnail(const cv::Mat& image);

QString getImageName(const QString& path);
QString getImagePath(const QString& path);

QString handRecCommands(const cv::Mat &spot);

// LOG
const char* const logFileName = "log.txt";
extern FILE *logFile;
void writeTime(const char* stage, float time);
void writeNewLine();

#endif // GENERAL_H