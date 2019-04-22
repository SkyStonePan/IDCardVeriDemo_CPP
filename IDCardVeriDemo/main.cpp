#include "IDCardVeriDemo.h"
#include <QtWidgets/QApplication>
#include <QLibrary>
#include <QSizePolicy>

int main(int argc, char *argv[])
{

	QApplication a(argc, argv);

	IDCardVeriDemo w;

	w.setFixedSize(w.width(), w.height());

	w.show();
	return a.exec();
}
