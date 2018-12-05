
#include "tcphttp.h"
#include "csvUp.h"

#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
//	TCPHTTP w;
//	w.show();


	csvUp u;
	u.show();

	return a.exec();
}
