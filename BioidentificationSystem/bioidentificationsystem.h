#ifndef BIOIDENTIFICATIONSYSTEM_H
#define BIOIDENTIFICATIONSYSTEM_H

#include <QtGui/QMainWindow>
#include <QThread>

#include "ui_bioidentificationsystem.h"
#include "Settings.h"
#include "ImageProcessor.h"
#include "PhotoLabel.h"

class BioidentificationSystem : public QMainWindow
{
	Q_OBJECT

public:
	BioidentificationSystem(QWidget *parent = 0, Qt::WFlags flags = 0);
	~BioidentificationSystem();

private slots:
	void cameraOpened();
	void cameraClosed();
	void processStarted();
	void processError();
	void processStopped();
	void displayVideoFrame(ImageProcessor::States state);
	void savePhoto();
	void displayError(QString err, ErrorLevels level);
	QListWidgetItem* showThumbnail(const QImage &thumbnail, const QString& path);
	void showPhoto(QListWidgetItem * item);
	void clearLabels();
	void imageOpened();
	void updateThumbnail();
	void uncheckDraw(bool state)
		{ if(state) ui.toolButtonDraw->setChecked(false); }
	void uncheckDrawRect(bool state)
		{ if(state) ui.toolButtonDrawRect->setChecked(false); }
	void clearCannyWindows();

private:
	void connections();
	void loadThumbnails();
	void processEnableButtons();

	Ui::BioidentificationSystemClass ui;
	PhotoLabel *mainImageLabel;
	Settings settingsForm;

	/* QToolBar buttons */
	QAction *changeCameraState; // On/Off Camera
	QAction *setPhotoMode;
	QAction *takePhoto;
	QAction *showSettings;
	QAction *reTrain;

	ImageProcessor *imageProcessor;
	QThread *workerThread;
};

#endif // BIOIDENTIFICATIONSYSTEM_H
