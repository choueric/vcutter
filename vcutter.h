#ifndef VCUTTER_H
#define VCUTTER_H

#include <QtWidgets/QMainWindow>
#include <QString.h>
#include "ui_vcutter.h"

class vcutter : public QMainWindow
{
	Q_OBJECT

public:
	vcutter(QWidget *parent = 0);
	~vcutter();

public slots:
	void clearLog();
	void getInfo();
	void inputFile();


private:
	Ui::vcutterClass ui;
	QString m_inDir;
	QString m_outDir;
	QString m_videoFile_0;
	QString m_videoFile_1;
	QString m_eventFile;

private:
	void banner();
	bool checkDir(QString &sdir);
	bool initDirs();
	bool checkFileExist(QString str);
	void enableEditUI();
	void disableEditUI();
	bool initEncoder();
	void deinitEncoder();
};

#endif // VCUTTER_H
