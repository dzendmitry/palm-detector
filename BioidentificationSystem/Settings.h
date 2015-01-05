#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include "ui_Settings.h"

#include "general.h"

class Settings : public QDialog
{
	Q_OBJECT

public:
	Settings(QWidget *parent = 0)
		: QDialog(parent) {
			ui.setupUi(this);
			connect(ui.checkBoxDebug, SIGNAL(clicked(bool)), this, SLOT(changeDebugMode(bool)));
			connect(ui.checkBoxCanny, SIGNAL(clicked(bool)), this, SLOT(changeCannyMode(bool)));
			connect(ui.checkBoxShowContours, SIGNAL(clicked(bool)), this, SLOT(changeShowContoursMode(bool)));
	}

signals:
	void cannyModeChanged();

private slots:
	void changeDebugMode(bool state)
		{ ADVANCED_OUTPUT = state; }
	void changeCannyMode(bool state)
		{ CANNY_STEP = state;
		  emit cannyModeChanged(); }
	void changeShowContoursMode(bool state)
		{ SHOW_CONTOURS = state; }

private:
	Ui::Settings ui;
};

#endif // SETTINGS_H
