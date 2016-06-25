#ifndef VCUTTER_H
#define VCUTTER_H

#include <Windows.h>
#include <QtWidgets/QMainWindow>
#include <QString.h>
#include <qtimer.h>
#include <qprocess.h>
#include "ui_vcutter.h"

#include "cuttime.h"


class vcutter : public QMainWindow
{
	Q_OBJECT

public:
	vcutter(QWidget *parent = 0);
	~vcutter();

public slots:
	void clearLog();
	void getVideoInfo(QString &file);
	void inputFile();
	void merge();
	void split();
	void procFinished(int exitCode, QProcess::ExitStatus exitStatus);
	void timeout();
	void on_read();
	void on_readerr();

private:
	QString m_inDir;
	QString m_outDir;
	QString m_videoFile_0;
	QString m_videoFile_1;
	QString m_eventFile;
	QString m_inputVideoFile;

	QProcess *m_proc;
	QTimer m_timer;
	int m_progressCount;
	bool m_isMerge;

	cuttime m_cuttime;
	Ui::vcutterClass ui;

private:
	void banner();
	bool checkDir(QString &sdir);
	bool initDirs();
	bool checkFileExist(QString str);
	void enableEditUI();
	void disableEditUI();
	void createFilelist();
	void deleteFilelist();
	void splitOne(int offset, int duration, QString output);
};

#endif // VCUTTER_H
