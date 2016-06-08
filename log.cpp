#include "log.h"
#include <qcolor.h>

static QListWidget *logWidget;

void initLog(QListWidget *w)
{
	logWidget = w;
}

static void addLog(QString str, QColor color)
{
	logWidget->addItem(str);
	logWidget->item(logWidget->count() - 1)->setBackgroundColor(color);
}

void errLog(QString str)
{
	addLog(str, Qt::red);
}

void okLog(QString str)
{
	addLog(str, Qt::green);
}

void warLog(QString str)
{
	addLog(str, Qt::yellow);
}

void infoLog(QString str)
{
	addLog(str, Qt::white);
}
