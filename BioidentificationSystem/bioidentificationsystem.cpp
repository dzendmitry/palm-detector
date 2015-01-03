#include <QMessageBox>
#include <QDir>
#include <QFileDialog>
#include <QList>

#include "bioidentificationsystem.h"

BioidentificationSystem::BioidentificationSystem(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	/* Image processor & Thread */
	imageProcessor = new ImageProcessor();
	workerThread = new QThread();
	workerThread->setObjectName("WORKER");
	imageProcessor->moveToThread(workerThread);

	/* PhotoLabel */
	mainImageLabel = new PhotoLabel(imageProcessor, ui.tabPhoto);
	mainImageLabel->setObjectName(QString::fromUtf8("mainImageLabel"));
	mainImageLabel->setFrameShape(QFrame::Box);
	mainImageLabel->setFrameShadow(QFrame::Sunken);
	mainImageLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	ui.gridLayout_2->addWidget(mainImageLabel, 1, 1, 1, 1);

	loadThumbnails();

	/* QToolBar buttons */
	changeCameraState = ui.mainToolBar->addAction(QIcon(":/icons/ico/camera_on_24x24.ico"), "Camera On");
	changeCameraState->setCheckable(true);
	ui.mainToolBar->addSeparator();
	setPhotoMode = ui.mainToolBar->addAction(QIcon(":/icons/ico/photo_mode_24x24.ico"), "Photo mode");
	setPhotoMode->setCheckable(true);
	setPhotoMode->setEnabled(false);
	takePhoto = ui.mainToolBar->addAction(QIcon(":/icons/ico/take_photo_24x24.ico"), "Take photo");
	takePhoto->setEnabled(false);
	ui.mainToolBar->addSeparator();
	reTrain = ui.mainToolBar->addAction(QIcon(":/icons/ico/retrain_24x24.ico"), "Retrain");
	showSettings = ui.mainToolBar->addAction(QIcon(":/icons/ico/settings_24x24.ico"), "Settings");

	/* Connections */
	connections();

	workerThread->start();
}

void BioidentificationSystem::processEnableButtons()
{
	changeCameraState->setEnabled(true);
	ui.toolButtonCancel->setEnabled(true);
	ui.toolButtonCrop->setEnabled(true);
	ui.toolButtonApply->setEnabled(true);
	ui.toolButtonDraw->setEnabled(true);
	ui.toolButtonInvColors->setEnabled(true);
	ui.toolButtonSave->setEnabled(true);
	ui.toolButtonSaveAs->setEnabled(true);
	ui.toolButtonOpen->setEnabled(true);
}

void BioidentificationSystem::processStarted()
{
	changeCameraState->setEnabled(false);
	ui.toolButtonCancel->setEnabled(false);
	ui.toolButtonCrop->setEnabled(false);
	ui.toolButtonApply->setEnabled(false);
	ui.toolButtonDraw->setEnabled(false);
	ui.toolButtonInvColors->setEnabled(false);
	ui.toolButtonSave->setEnabled(false);
	ui.toolButtonSaveAs->setEnabled(false);
	ui.toolButtonOpen->setEnabled(false);	
}

void BioidentificationSystem::processError()
{
	processEnableButtons();
	bool prevSignalState = ui.toolButtonProcess->blockSignals(true);
	ui.toolButtonProcess->setChecked(false);
	ui.toolButtonProcess->blockSignals(prevSignalState);
}

void BioidentificationSystem::processStopped()
{
	processEnableButtons();
	clearLabels();
}

void BioidentificationSystem::updateThumbnail()
{
	QString name = mainImageLabel->getCurrentImageName();
	cv::Mat image = mainImageLabel->getCurrentMat();
	image = buildThumbnail(image);
	QList<QListWidgetItem*> items = ui.photoWidget->findItems(name, Qt::MatchFixedString);
	foreach(QListWidgetItem *item, items)
		if(item->text() == name)
			item->setIcon(QIcon(QPixmap::fromImage(Mat2QImage(image))));
	displayError("Saved.", INFORMATION);
}

void BioidentificationSystem::imageOpened()
{
	cv::Mat thumbnail = buildThumbnail(mainImageLabel->getCurrentMat());
	QString name = mainImageLabel->getCurrentImageName();
	showPhoto(showThumbnail(Mat2QImage(thumbnail), name));
}

void BioidentificationSystem::showPhoto(QListWidgetItem* item)
{
	if(!imageProcessor->stopped())
	{	
		displayError("Turn off process or camera.", INFORMATION);
		return;
	}
	cv::Mat photo = loadImage(item->data(Qt::UserRole).toString() + item->text());
	if(photo.data == NULL)
	{
		displayError("Can't open photo.", CRITICAL);
		return;
	}
	QPixmap pixmap = QPixmap::fromImage(Mat2QImage(photo));
	mainImageLabel->setCurrentMat(photo);
	mainImageLabel->setCurrentPixmap(pixmap);
	mainImageLabel->setPixmap(pixmap);
	mainImageLabel->setCurrentImageName(item->text());
	mainImageLabel->setCurrentImagePath(item->data(Qt::UserRole).toString());
	mainImageLabel->setPointsToNull();
	ui.frameActionsGroup->setEnabled(true);
}

void BioidentificationSystem::cameraOpened()
{
	/* ToolBar panel */
	changeCameraState->setIcon(QIcon(":/icons/ico/camera_off_24x24.ico"));
	changeCameraState->setIconText("Camera Off");

	takePhoto->setEnabled(true);

	setPhotoMode->setEnabled(true);
	setPhotoMode->setChecked(true);

	/* "Actions" panel */
	ui.frameActionsGroup->setEnabled(false);
	ui.toolButtonDrawRect->setChecked(false);
	ui.toolButtonProcess->setChecked(false);
	ui.toolButtonDraw->setChecked(false);
	ui.toolButtonOpen->setEnabled(false);
}

void BioidentificationSystem::cameraClosed()
{
	/* ToolBar panel */
	changeCameraState->setIcon(QIcon(":/icons/ico/camera_on_24x24.ico"));
	changeCameraState->setIconText("Camera On");
	changeCameraState->setChecked(false);

	setPhotoMode->setChecked(false);
	setPhotoMode->setEnabled(false);

	takePhoto->setEnabled(false);
	
	/* "Actions" panel */
	ui.toolButtonOpen->setEnabled(true);
	
	/* StatusBar */
	ui.statusBar->clearMessage();

	/* Labels */
	mainImageLabel->clear();
	clearLabels();
}

void BioidentificationSystem::displayVideoFrame(ImageProcessor::States state)
{
	switch(state)
	{
	case ImageProcessor::GET_FRAME:
		{
			cv::Mat frame = imageProcessor->getFrame();
			mainImageLabel->setPixmap(QPixmap::fromImage(Mat2QImage(frame)));
		}
		break;
	case ImageProcessor::GET_SKIN_COLOR:
		{
			cv::Mat face = imageProcessor->getFace();
			cv::Rect skinRect = imageProcessor->getSkinRect();
			cv::rectangle(face, skinRect, cv::Scalar(0, 0, 255));
			ui.labelSmallFace->setPixmap(QPixmap::fromImage(Mat2QImage(face)));
		}
		break;
	case ImageProcessor::PIXEL_RECOGNITION:
		{
			cv::Mat colorizedFrame = imageProcessor->getColorizedFrame();
			cv::Rect faceRect = imageProcessor->getFaceRect();
			/*cv::Mat frame = imageProcessor->getFrame();
			cv::Mat classifiedSkin = imageProcessor->getClassifiedSkin();
			cv::Mat classifiedSkinFiltered = imageProcessor->getClassifiedSkinFiltered();
			for(int i = 0; i < frame.rows; i++)
				for(int j = 0; j < frame.cols; j++)
				{
					uchar pixColor = classifiedSkinFiltered.at<uchar>(i, j);
					if(pixColor > 0)
						frame.at<cv::Vec3b>(i, j) = cv::Vec3b(0, pixColor, 0);
					if(pixColor == 1)
						frame.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 255);
				}
				*/
			cv::rectangle(colorizedFrame, faceRect, cv::Scalar(255, 0, 0));
			ui.labelSkin->setPixmap(QPixmap::fromImage(Mat2QImage(colorizedFrame)));
		}
		break;
	case ImageProcessor::CANNY:
		{
			cv::Mat cannyEdges = imageProcessor->getCannyEdges();
			ui.labelCanny->setPixmap(QPixmap::fromImage(Mat2QImage(cannyEdges)));
		}
		break;
	case ImageProcessor::MOPRPHOLOGY_DILATION:
		{
			cv::Mat dilate = imageProcessor->getMorphologyDilation();
			ui.labelDilate->setPixmap(QPixmap::fromImage(Mat2QImage(dilate)));
		}
		break;
	case ImageProcessor::MERGE_PIXELS_AND_CANNY:
		{
			cv::Mat pixelsAndCanny = imageProcessor->getPixelsAndCanny();
			ui.labelCannySkin->setPixmap(QPixmap::fromImage(Mat2QImage(pixelsAndCanny)));
		}
		break;
	case ImageProcessor::MORPHOLOGY_ERODE:
		{
			cv::Mat erode = imageProcessor->getMorphologyErode();
			ui.labelErode->setPixmap(QPixmap::fromImage(Mat2QImage(erode)));
		}
		break;
	case ImageProcessor::MORPHOLOGY_OPENING:
		{
			cv::Mat opening = imageProcessor->getMorphologyOpening();
			ui.labelOpening->setPixmap(QPixmap::fromImage(Mat2QImage(opening)));
		}
		break;
	case ImageProcessor::HAND_RECOGNITION:
		{
			cv::Mat hands = imageProcessor->getHandRecognition();
			ui.labelHand->setPixmap(QPixmap::fromImage(Mat2QImage(hands)));
			std::vector<QString> dissimilarityMeasure = imageProcessor->getDissimilarityMeasure();
			if(dissimilarityMeasure.size() > 0)
				foreach(const QString& str, dissimilarityMeasure)
				{
					ui.textEditHandRecognition->insertPlainText(QString("Dissimilarity: ") + str + "\n");
					ui.textEditHandRecognition->moveCursor(QTextCursor::Start);
				}
		}
		break;
	}
}

void BioidentificationSystem::savePhoto()
{
	cv::Mat frame;
	if(imageProcessor->getPhotoMode())
		frame = imageProcessor->getFrame();
	else
		frame = imageProcessor->getMorphologyOpening();

	QString name = saveImage(frame);
	if(name.isNull())
	{
		displayError("Can't save image.", CRITICAL);
		return;
	}
	cv::Mat thumbnail = buildThumbnail(frame);
	showThumbnail(Mat2QImage(thumbnail), name);
}

QListWidgetItem* BioidentificationSystem::showThumbnail(const QImage &thumbnail, const QString &path)
{
	QListWidgetItem *item = new QListWidgetItem();
	QString name = getImageName(path);
	QString imgPath = getImagePath(path);
	item->setIcon(QIcon(QPixmap::fromImage(thumbnail)));
	item->setText(name);
	item->setData(Qt::UserRole, QVariant(imgPath));
	ui.photoWidget->insertItem(0, item);
	return item;
}

void BioidentificationSystem::loadThumbnails()
{
	ui.photoWidget->clear();
	QDir dir(PHOTO_PATH);
	QStringList photos = dir.entryList(IMAGE_FORMAT, QDir::Files | QDir::Readable, QDir::Name);
	QString photo;
	foreach(photo, photos)
	{
		cv::Mat thumbnail = loadImage(PHOTO_PATH + photo);
		if(thumbnail.data == NULL)
			continue;
		showThumbnail(Mat2QImage(thumbnail), PHOTO_PATH + photo);
	}
}

void BioidentificationSystem::displayError(QString err, ErrorLevels level)
{
	QMessageBox mbox(this);
	mbox.setText(err);
	switch(level)
	{
	case CRITICAL:
		mbox.setIcon(QMessageBox::Critical);
		mbox.setWindowTitle("Error");
		break;
	case WARNING:
		mbox.setIcon(QMessageBox::Warning);
		mbox.setWindowTitle("Warning");
		break;
	case INFORMATION:
		mbox.setIcon(QMessageBox::Information);
		mbox.setWindowTitle("Information");
		break;
	default:
		mbox.setIcon(QMessageBox::NoIcon);
	}
	mbox.exec();
}

void BioidentificationSystem::clearLabels()
{
	ui.labelSkin->clear();
	ui.labelSmallFace->clear();
	clearCannyWindows();
	ui.labelDilate->clear();
	ui.labelErode->clear();
	ui.labelOpening->clear();
	ui.labelHand->clear();
	ui.textEditHandRecognition->clear();
}

void BioidentificationSystem::clearCannyWindows()
{
	ui.labelCanny->clear();
	ui.labelCannySkin->clear();
}

BioidentificationSystem::~BioidentificationSystem()
{
	workerThread->quit();
	workerThread->wait();
	delete workerThread;
	delete imageProcessor;
}

void BioidentificationSystem::connections()
{
	connect(ui.pushButtonMakeExp, SIGNAL(clicked()), imageProcessor, SLOT(makeExp()));
	connect(changeCameraState, SIGNAL(toggled(bool)), imageProcessor, SLOT(imageProcState(bool)));
	connect(setPhotoMode, SIGNAL(toggled(bool)), imageProcessor, SLOT(setPhotoMode(bool)));
	connect(imageProcessor, SIGNAL(photoModeChanged()), ui.statusBar, SLOT(clearMessage()));
	connect(imageProcessor, SIGNAL(photoModeChanged()), this, SLOT(clearLabels()));
	connect(takePhoto, SIGNAL(triggered()), imageProcessor, SLOT(takePhoto()));
	connect(reTrain, SIGNAL(triggered()), imageProcessor, SLOT(setPixelClassifierTrained()));
	connect(showSettings, SIGNAL(triggered()), &settingsForm, SLOT(open()));
	connect(&settingsForm, SIGNAL(cannyModeChanged()), this, SLOT(clearCannyWindows()));
	connect(ui.photoWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(showPhoto(QListWidgetItem*)));

	connect(ui.toolButtonDrawRect, SIGNAL(toggled(bool)), mainImageLabel, SLOT(setDrawRectMode(bool)));
	connect(ui.toolButtonDrawRect, SIGNAL(toggled(bool)), this, SLOT(uncheckDraw(bool)));
	connect(ui.toolButtonDraw, SIGNAL(toggled(bool)), mainImageLabel, SLOT(setDrawMode(bool)));
	connect(ui.toolButtonDraw, SIGNAL(toggled(bool)), this, SLOT(uncheckDrawRect(bool)));
	connect(ui.toolButtonCrop, SIGNAL(clicked()), mainImageLabel, SLOT(crop()));
	connect(ui.toolButtonInvColors, SIGNAL(clicked()), mainImageLabel, SLOT(invColors()));
	connect(ui.toolButtonSave, SIGNAL(clicked()), mainImageLabel, SLOT(saveCurrentImage()));
	connect(ui.toolButtonCancel, SIGNAL(clicked()), mainImageLabel, SLOT(showPrevImage()));
	connect(ui.toolButtonOpen, SIGNAL(clicked()), mainImageLabel, SLOT(openImage()));
	connect(ui.toolButtonSaveAs, SIGNAL(clicked()), mainImageLabel, SLOT(saveCurrentImageAs()));
	connect(ui.toolButtonProcess, SIGNAL(toggled(bool)), mainImageLabel, SLOT(process(bool)));
	connect(ui.toolButtonApply, SIGNAL(clicked()), mainImageLabel, SLOT(apply()));
	connect(mainImageLabel, SIGNAL(processStarted()), this, SLOT(processStarted()));
	connect(mainImageLabel, SIGNAL(processError()), this, SLOT(processError()));
	connect(mainImageLabel, SIGNAL(processStopped()), this, SLOT(processStopped()));
	connect(mainImageLabel, SIGNAL(opened()), this, SLOT(imageOpened()));
	connect(mainImageLabel, SIGNAL(saved()), this, SLOT(updateThumbnail()));
	connect(mainImageLabel, SIGNAL(error(QString, ErrorLevels)), this, SLOT(displayError(QString, ErrorLevels)));

	connect(ui.horizontalSliderS, SIGNAL(valueChanged(int)), ui.labelSValue, SLOT(setNum(int)));
	connect(ui.horizontalSliderS, SIGNAL(valueChanged(int)), imageProcessor, SLOT(setKernelParamS(int)));
	ui.horizontalSliderS->setValue(KERNEL_PARAM_S);

	connect(ui.horizontalSliderAperture, SIGNAL(valueChanged(int)), ui.labelApertureValue, SLOT(setNum(int)));
	connect(ui.horizontalSliderAperture, SIGNAL(valueChanged(int)), imageProcessor, SLOT(setApertureFilter(int)));
	ui.horizontalSliderAperture->setValue(APERTURE_FILTER);

	connect(ui.horizontalSliderLowThreshold, SIGNAL(valueChanged(int)), ui.labelLowThresholdValue, SLOT(setNum(int)));
	connect(ui.horizontalSliderLowThreshold, SIGNAL(valueChanged(int)), imageProcessor, SLOT(setLowThreshold(int)));
	ui.horizontalSliderLowThreshold->setValue(LOW_THRESHOLD);

	connect(ui.horizontalSliderAperture_2, SIGNAL(valueChanged(int)), ui.labelApertureValue_2, SLOT(setNum(int)));
	connect(ui.horizontalSliderAperture_2, SIGNAL(valueChanged(int)), imageProcessor, SLOT(setAperture(int)));
	ui.horizontalSliderAperture_2->setValue(APERTURE);

	connect(ui.horizontalSliderRatio, SIGNAL(valueChanged(int)), ui.labelRatioValue, SLOT(setNum(int)));
	connect(ui.horizontalSliderRatio, SIGNAL(valueChanged(int)), imageProcessor, SLOT(setRatio(int)));
	ui.horizontalSliderRatio->setValue(RATIO);

	connect(ui.horizontalSliderDilate, SIGNAL(valueChanged(int)), ui.labelDilateSizeValue, SLOT(setNum(int)));
	connect(ui.horizontalSliderDilate, SIGNAL(valueChanged(int)), imageProcessor, SLOT(setDilateSize(int)));
	ui.horizontalSliderDilate->setValue(DILATION);

	connect(ui.horizontalSliderErode, SIGNAL(valueChanged(int)), ui.labelErodeSizeValue, SLOT(setNum(int)));
	connect(ui.horizontalSliderErode, SIGNAL(valueChanged(int)), imageProcessor, SLOT(setErodeSize(int)));
	ui.horizontalSliderErode->setValue(EROSION);

	connect(ui.horizontalSliderOpening, SIGNAL(valueChanged(int)), ui.labelOpeningSizeValue, SLOT(setNum(int)));
	connect(ui.horizontalSliderOpening, SIGNAL(valueChanged(int)), imageProcessor, SLOT(setOpenedMatSize(int)));
	ui.horizontalSliderOpening->setValue(OPENING);

	connect(ui.horizontalSliderCannyContour, SIGNAL(valueChanged(int)), ui.labelDeltaCannyContourValue, SLOT(setNum(int)));
	connect(ui.horizontalSliderCannyContour, SIGNAL(valueChanged(int)), imageProcessor, SLOT(setCannyContourMergeEps(int)));
	ui.horizontalSliderCannyContour->setValue(CANNY_CONTOUR_MERGE_EPS);

	connect(ui.horizontalSliderApproxPoly, SIGNAL(valueChanged(int)), ui.labelApproxPolyValue, SLOT(setNum(int)));
	connect(ui.horizontalSliderApproxPoly, SIGNAL(valueChanged(int)), imageProcessor, SLOT(setApproxPoly(int)));
	ui.horizontalSliderApproxPoly->setValue(APPROX_POLY);

	connect(ui.horizontalSliderThres, SIGNAL(valueChanged(int)), ui.labelHandThresValue, SLOT(setNum(int)));
	connect(ui.horizontalSliderThres, SIGNAL(valueChanged(int)), imageProcessor, SLOT(setHandThres(int)));
	ui.horizontalSliderThres->setValue(HAND_THRESHOLD);

	connect(ui.horizontalSliderTopHandThres, SIGNAL(valueChanged(int)), ui.labelTopHandThresValue, SLOT(setNum(int)));
	connect(ui.horizontalSliderTopHandThres, SIGNAL(valueChanged(int)), imageProcessor, SLOT(setTopHandThres(int)));
	ui.horizontalSliderTopHandThres->setValue(TOP_HAND_THRES);

	connect(imageProcessor, SIGNAL(opened()), this, SLOT(cameraOpened()));
	connect(imageProcessor, SIGNAL(closed()), this, SLOT(cameraClosed()));
	connect(imageProcessor, SIGNAL(photoTaken()), this, SLOT(savePhoto()));

	connect(imageProcessor, SIGNAL(stateCompleted(ImageProcessor::States)), this, SLOT(displayVideoFrame(ImageProcessor::States)));
	connect(imageProcessor, SIGNAL(postMessage(QString)), ui.statusBar, SLOT(showMessage(QString)));
	connect(imageProcessor, SIGNAL(error(QString, ErrorLevels)), this, SLOT(displayError(QString, ErrorLevels)));
}