#include "bioidentificationsystem.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	BioidentificationSystem w;
	w.show();
	return a.exec();
}
