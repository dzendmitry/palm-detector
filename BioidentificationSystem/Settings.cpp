#include "Settings.h"
bool ADVANCED_OUTPUT;
bool CANNY_STEP;
bool SHOW_CONTOURS;

Settings::Settings(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	connect(ui.checkBoxDebug, SIGNAL(clicked(bool)), this, SLOT(changeDebugMode(bool)));
	connect(ui.checkBoxCanny, SIGNAL(clicked(bool)), this, SLOT(changeCannyMode(bool)));
	connect(ui.checkBoxShowContours, SIGNAL(clicked(bool)), this, SLOT(changeShowContoursMode(bool)));
}

Settings::~Settings()
{}
