#pragma once

#include <QtWidgets/QMainWindow>
#include <qlistwidget.h>

void initLog(QListWidget *w);

void errLog(QString str);
void okLog(QString str);
void warLog(QString str);
void infoLog(QString str);
