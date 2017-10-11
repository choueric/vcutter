#include "vcutter.h"
#include <QtWidgets/QApplication>

#include "config.h"

//#define DEBUG_CONSOLE

#ifdef DEBUG_CONSOLE
void setup_console()
{
	AllocConsole();
	freopen("CONOUT$", "w+t", stdout);
	freopen("CONIN$", "r+t", stdin); 
}

void release_console()
{
	//char ch = getchar();
	FreeConsole();
}
#endif

int main(int argc, char *argv[])
{
	int ret;

#ifdef DEBUG_CONSOLE
	setup_console();
#endif
	QApplication a(argc, argv);
	vcutter w;
	w.show();

	ret = a.exec();
#ifdef DEBUG_CONSOLE
	release_console();
#endif
	return ret;
}
