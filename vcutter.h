#ifndef VCUTTER_H
#define VCUTTER_H

#include <QtWidgets/QMainWindow>
#include <QString.h>
#include <qtimer.h>
#include <qprocess.h>
#include "ui_vcutter.h"

#define FILELIST_NAME "./output/filelist"
#define CONCAT_CMD "ffmpeg.exe -f concat -i ./output/filelist -c copy ./output/output.mkv"

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
	Ui::vcutterClass ui;
	QString m_inDir;
	QString m_outDir;
	QString m_videoFile_0;
	QString m_videoFile_1;
	QString m_eventFile;

	QProcess *m_proc;
	QTimer m_timer;
	int m_progressCount;
	int m_procType;

private:
	void banner();
	bool checkDir(QString &sdir);
	bool initDirs();
	bool checkFileExist(QString str);
	void enableEditUI();
	void disableEditUI();
	void createFilelist();
	void deleteFilelist();
};

#endif // VCUTTER_H
