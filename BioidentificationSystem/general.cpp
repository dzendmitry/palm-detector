#include <QVector>
#include <QRgb>
#include <QDir>
#include <QTime>

#include <windows.h>
#include <time.h>
#include <gdiplus.h>
using namespace Gdiplus;

#include "general.h"
FILE *logFile;

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if(size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j = 0; j < num; ++j)
	{
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}
	free(pImageCodecInfo);
	return -1;  // Failure
}

void saveBMP(QImage& img, QString path = "hand.bmp") {
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	Bitmap *image = new Bitmap(img.width(), img.height(), img.bytesPerLine(), PixelFormat1bppIndexed, (BYTE*)img.bits());

	CLSID myClsId;
	int retVal = GetEncoderClsid(L"image/bmp", &myClsId);

	Status s = image->Save(path.toStdWString().c_str(), &myClsId, NULL);
	
	delete image;

	GdiplusShutdown(gdiplusToken);
}

QImage Mat2QImage(const cv::Mat &frame)
{
	// 8-bits unsigned, NO. OF CHANNELS=1
	if(frame.type()==CV_8UC1)
	{
		// Set the color table (used to translate colour indexes to qRgb values)
		QVector<QRgb> colorTable;
		for (int i=0; i<256; i++)
			colorTable.push_back(qRgb(i,i,i));
		// Copy input Mat
		const uchar *qImageBuffer = (const uchar*)frame.data;
		// Create QImage with same dimensions as input Mat
		QImage img(qImageBuffer, frame.cols, frame.rows, frame.step, QImage::Format_Indexed8);
		img.setColorTable(colorTable);
		return img;
	}
	// 8-bits unsigned, NO. OF CHANNELS=3
	if(frame.type()==CV_8UC3)
	{
		// Copy input Mat
		const uchar *qImageBuffer = (const uchar*)frame.data;
		// Create QImage with same dimensions as input Mat
		QImage img(qImageBuffer, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
		return img.rgbSwapped();
	}
	else
	{
		qDebug() << "ERROR: Mat could not be converted to QImage.";
		return QImage();
	}
}

cv::Mat QImage2Mat(const QImage& image)
{
	// 8-bit, 4 channel
	if(image.format() == QImage::Format_RGB32)
	{
		cv::Mat mat(image.height(), image.width(), CV_8UC4, const_cast<uchar*>(image.bits()), image.bytesPerLine());
		return mat.clone();
	}
	// 8-bit, 3 channel
	if(image.format() == QImage::Format_RGB888)
	{
		QImage swapped = image.rgbSwapped();
		return cv::Mat( swapped.height(), swapped.width(), CV_8UC3, const_cast<uchar*>(swapped.bits()), swapped.bytesPerLine() ).clone();
	}
	// 8-bit, 1 channel
	if(image.format() == QImage::Format_Indexed8)
	{
		cv::Mat mat(image.height(), image.width(), CV_8UC1, const_cast<uchar*>(image.bits()), image.bytesPerLine() );
		return mat.clone();
	}
	else
	{
		qDebug() << "ERROR: QImage could not be converted to Mat.";
		return cv::Mat();
	}
}

QString saveImage(const cv::Mat& image, const QString& name, const QString& dir)
{
	QDir dirObj;
	if(!dirObj.exists(dir) && !dirObj.mkdir(dir))
		return QString();
	QString fname = dir;
	if(name.isEmpty())
		fname += QString::number(QDateTime::currentMSecsSinceEpoch()) + ".bmp";
	else
		fname += name;
	if(!cv::imwrite((fname).toStdString(), image))
		return QString();
	return fname;
}

cv::Mat loadImage(const QString& imageName)
{
	if(!QFile::exists(imageName))
		return cv::Mat();
	return cv::imread(imageName.toStdString());
}

cv::Mat buildThumbnail(const cv::Mat& image)
{
	cv::Mat thumbnail;
	cv::resize(image, thumbnail, cv::Size(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT));
	return thumbnail;
}

QString getImageName(const QString& path)
{
	QStringList pathList = path.split('/', QString::SkipEmptyParts);
	return pathList.last();
}

QString getImagePath(const QString& path)
{
	QStringList pathList = path.split('/', QString::SkipEmptyParts);
	QString name = pathList.last();
	pathList.removeLast();
	return pathList.join("/") + "/";
}

QString handRecCommands(const cv::Mat &candidate)
{
	QImage img = Mat2QImage(candidate);
	img = img.convertToFormat(QImage::Format_Mono, Qt::MonoOnly);

	QDir dir;
	if(!dir.exists(HANDS_COMPARE_DIR) || !dir.setCurrent(HANDS_COMPARE_DIR))
		throw std::exception((QString("Dir ") + HANDS_COMPARE_DIR + " doesn't exist or can't be changed.").toAscii().data());

	/*if(saveImage(img, HANDS_BMP_NAME, "./").isEmpty())
		throw std::exception((QString("Saving ") + HANDS_BMP_NAME + " error.").toAscii().data());*/
	saveBMP(img);
	QStringList delEntries = dir.entryList(QStringList() << SEQ_FILTER);
	foreach(const QString& entry, delEntries)
		dir.remove(entry);

	clock_t handRecStartTime = clock();
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	// Command 1
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	if(CreateProcessW(L"C:\\windows\\system32\\cmd.exe", const_cast<LPWSTR>((QString("/C BmpToSeq.exe ") + QString::number(REGULARIZATION) + " " + QString::number(APPROXIMATION) + " " + QString::number(MERGING) + " " + QString::number(LEGANDRES) + " " + HANDS_BMP_NAME + " hand.seq").toStdWString().data()), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	} else
		throw std::exception((QString("BmpToSeq.exe ") + HANDS_BMP_NAME + " failed.").toAscii().data());

	// Command 3
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	if(CreateProcessW(L"C:\\windows\\system32\\cmd.exe", const_cast<LPWSTR>((QString("/C StringCompare.exe etalon.seq hand.seq ") + QString::number(MULCT)).toStdWString().data()), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	} else
		throw std::exception("StringCompare.exe error.");
	if(DEBUG) {
		writeTime("HAND_REC_COMMANDS", ((float)(clock()-handRecStartTime))/CLOCKS_PER_SEC);
	}

	// Read result
	QFile resultFile(DISSIMILARITY_MEASURE);
	if(!resultFile.open(QFile::ReadOnly | QFile::Text))
		throw std::exception((QString("Open file ") + DISSIMILARITY_MEASURE + " failed.").toAscii().data());
	QByteArray byteArray = resultFile.readAll();
	if(byteArray.isEmpty())
		throw std::exception((QString("Empty file ") + DISSIMILARITY_MEASURE).toAscii().data());
	QString result(byteArray);
	dir.setCurrent("..");
	return result;
}

void onePixelBorder(cv::Mat& img) {
	// Top border, Bottom border
	for(int j = 0; j < img.cols; j++) {
		img.at<cv::Vec3b>(0, j) = cv::Vec3b(0, 0, 0);
		img.at<cv::Vec3b>(img.rows - 1, j) = cv::Vec3b(0, 0, 0);
	}
	// Left border, Right border
	for(int i = 0; i < img.rows; i++) {
		img.at<cv::Vec3b>(i, 0) = cv::Vec3b(0, 0, 0);
		img.at<cv::Vec3b>(i, img.cols - 1) = cv::Vec3b(0, 0, 0);
	}
}

void writeTime(const char* stage, float time)
{
	if(logFile)
		fprintf(logFile, "%s: %f\n", stage, time);	
}

void writeNewLine()
{
	if(logFile)
		fwrite("\n", sizeof(char), 1, logFile);
}