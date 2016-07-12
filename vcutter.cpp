#pragma execution_character_set ("utf-8")

#include "vcutter.h"
#include <qmessagebox.h>
#include "log.h"

#include <QDir.h>
#include <QFileDialog.h>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <qcolor.h>

#define VERSION  "0.2"

#define SUBFIX ".mkv"
#define FILELIST_NAME "./output/filelist"

vcutter::vcutter(QWidget *parent)
	: QMainWindow(parent)
{
	m_proc = NULL;

	ui.setupUi(this);
	initLog(ui.logWidget);
	
	ui.versionLabel->setText(QString("Version: ") + QString(VERSION));

	ui.progressBar->setVisible(false);
	for (int i = -10; i <= 10; i++)
		ui.offsetBox->addItem(QString::number(i, 10));
	ui.offsetBox->setCurrentIndex(ui.offsetBox->findText("0"));
	m_offset = 0;

	m_program = "./ffmpeg.exe";
	if (checkProgram() == false) {
		errLog("û���ҵ�ffmpeg.exe");
		ui.inputBtn->setEnabled(false);
		ui.mergeBtn->setEnabled(false);
		ui.cutBtn->setEnabled(false);
		return;
	}

	m_proc = new QProcess(this);
	connect(m_proc, SIGNAL(finished(int, QProcess::ExitStatus)),
		this, SLOT(procFinished(int, QProcess::ExitStatus)));
	connect(m_proc, SIGNAL(readyReadStandardError()), this, SLOT(on_readerr()));
	connect(m_proc, SIGNAL(readyReadStandardOutput()), this, SLOT(on_read()));

	connect(&m_timer, SIGNAL(timeout()), this, SLOT(timeout()));

	disableEditUI();

	if (!initDirs()) {
		errLog("��ʼ��Ŀ¼ʧ��.");
		return;
	}

	okLog("��ʼ���ɹ�");
}

vcutter::~vcutter()
{
	if (m_proc != NULL)
		delete m_proc;
}

void vcutter::banner()
{
	infoLog("----------------------------------------------------------");
}

void vcutter::createFilelist()
{
	QFile file;
	QFileInfo info(m_videoFile_0);

	file.setFileName(FILELIST_NAME);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		errLog("����Filelist�ļ�����!");
		return;
	} 

	QTextStream out(&file);    
	out << "file '../input/" << info.fileName() << "'" << endl;    
	if (m_videoFile_1 != "") {
		QFileInfo info(m_videoFile_1);
		out << "file '../input/" << info.fileName() << "'" << endl;
	}
	out.flush();

	file.close();
}

void vcutter::deleteFilelist()
{
	QFile::remove(FILELIST_NAME);
}

void vcutter::disableEditUI()
{
	ui.inputBtn->setEnabled(true);
	ui.mergeBtn->setEnabled(false);
	ui.cutBtn->setEnabled(false);
}

void vcutter::enableEditUI()
{
	if (m_needMerge)
		ui.mergeBtn->setEnabled(true);
	else 
		ui.cutBtn->setEnabled(true);
}

bool vcutter::checkDir(QString &sdir)
{
	QDir dir(sdir);
	if (!dir.exists()) {
		dir.mkpath(sdir);
	}

	return true;
}

bool vcutter::initDirs()
{
	m_inDir = QDir::currentPath() + "/input";
	checkDir(m_inDir);
	warLog(QString("��Ƶ����Ŀ¼Ϊ: %1").arg(m_inDir));

	m_outDir = QDir::currentPath() + "/output";
	checkDir(m_outDir);
	warLog(QString("��Ƶ���Ŀ¼Ϊ: %1").arg(m_outDir));

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool vcutter::checkFileExist(QString file)
{
	QFileInfo f(file);
	if (f.exists() == false) {
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

/* slot */
void vcutter::clearLog()
{
	ui.logWidget->clear();
}

/* slot */
void vcutter::inputFile()
{
	m_tagFile = "";
	m_videoFile_0 = "";
	m_videoFile_1 = "";
	m_inputVideoFile = "";
	m_needMerge = false;

	QString str = QFileDialog::getOpenFileName(
			this, "ѡ��tag�ļ�", m_inDir, "*.log");

	if (str == "") {
		errLog("û��ѡ��Ϸ���tag�ļ�.");
		return;
	}

	QFileInfo file(str);
	QString prefix = file.absolutePath() + "/" + file.baseName();
	m_videoFile_0 = prefix + SUBFIX;
	m_videoFile_1 = prefix + "_1" + SUBFIX;

	if (checkFileExist(m_videoFile_0) == false) {
		m_videoFile_0 = "";
		errLog(QString("��Ƶ�ļ�[%1]������.").arg(m_videoFile_0));
		return;
	}
	m_inputVideoFile = "./input/" + QFileInfo(m_videoFile_0).fileName();

	if (checkFileExist(m_videoFile_1) == false) {
		m_videoFile_1 = "";
	} else {
		m_needMerge = true;
		m_inputVideoFile = "./output/output.mkv";
	}

	m_tagFile = str;
	warLog(QString("ѡ���� %1").arg(file.baseName()));
	warLog(QString("    �¼��ļ� : %1").arg(QFileInfo(m_tagFile).fileName()));
	warLog(QString("    ��Ƶ�ļ�1: %1").arg(QFileInfo(m_videoFile_0).fileName()));
	if (m_videoFile_1 != "")
		warLog(QString("    ��Ƶ�ļ�2: %1").arg(QFileInfo(m_videoFile_1).fileName()));
	warLog(QString("    �������Ƶ�ļ�: %1").arg(m_inputVideoFile));

	// parse event_log file.
	m_cuttime.input(m_tagFile);
	m_cuttime.showEvents();

	enableEditUI();
}

/* slot */
void vcutter::merge()
{
	createFilelist();
	m_progressCount = 0;

	QStringList args;
	args << "-y" << "-safe" << "0" << "-f" << "concat" << "-i" << "./output/filelist" << "-c" << "copy" << m_inputVideoFile;
	m_proc->start(m_program, args);

	if (m_proc->waitForStarted() == false) {
		errLog("�����ϲ�����ʧ��");
	} else {
		okLog("�����ϲ�����ɹ�.");
	}

	m_isMerge = true;
	ui.progressBar->setVisible(true);
	m_timer.start(200);
}

void vcutter::splitOne(int start, int duration, QString output)
{
	QStringList args;
	args << "-y" << 
			"-i" << m_inputVideoFile <<
		    "-ss" << QString::number(start, 10) <<
		    "-t" << QString::number(duration, 10) <<
		    "-vcodec" << "copy" << "-acodec" << "copy" << output;
	m_proc->start(m_program, args);

	if (m_proc->waitForStarted() == false) {
		errLog(QString("ʧ��: start: %1, duration: %2, output: %3").arg(start).arg(duration).arg(output));
	} else {
		okLog(QString("�ɹ�: start: %1, duration: %2, output: %3").arg(start).arg(duration).arg(output));
	}

	m_proc->waitForFinished(-1);
}

/* slot */
void vcutter::split()
{
	ui.progressBar->setVisible(true);
	m_isMerge = false;

	int count = m_cuttime.eventCount();
	ui.progressBar->setMinimum(0);
	ui.progressBar->setMaximum(count);

	for (int i = 0; i < count; i++) {
		splitOne(m_cuttime.getSSArg(i) + m_offset, DURATION, m_cuttime.getOutuptName(i));
		ui.progressBar->setValue(i);
		qApp->processEvents();
	}
	ui.progressBar->setValue(count);
	QMessageBox::information(this, "�ָ����", "�ָ������");
}

/* slot */
void vcutter::procFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	if (m_isMerge) {
		m_timer.stop();
		ui.progressBar->setValue(100);
		QMessageBox::information(this, "�ϲ����", "�ϲ������");
		ui.progressBar->setVisible(false);
		deleteFilelist();
		m_needMerge = false;
		ui.cutBtn->setEnabled(true);
	}
}

/* timer timeout slot */
void vcutter::timeout()
{
	ui.progressBar->setValue(m_progressCount++);
	if (m_progressCount == 100)
		m_progressCount = 1;
}

void vcutter::on_read()
{
	QProcess *pProces = (QProcess *)sender();
	QString result = pProces->readAllStandardOutput();
	ui.logTextEdit->append(result);
}

void vcutter::on_readerr()
{
	QProcess *pProces = (QProcess *)sender();
	QString result = pProces->readAllStandardError();
	ui.logTextEdit->append(result);
}

void vcutter::offsetChanged(int index)
{
	m_offset = ui.offsetBox->itemText(index).toInt();
}

bool vcutter::checkProgram()
{
	QFileInfo info(m_program);
	if (info.exists() == false)
		return false;
	return true;
}
