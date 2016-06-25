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

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avio.h"
#include "libavutil/file.h"
}

#define SUBFIX ".mkv"
#define FILELIST_NAME "./output/filelist"

vcutter::vcutter(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	initLog(ui.logWidget);
	ui.progressBar->setVisible(false);

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
	ui.cutBtn->setEnabled(true);
	if (m_videoFile_1 != "")
		ui.mergeBtn->setEnabled(true);
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

void vcutter::getVideoInfo(QString &file)
{
    int ret;
    AVFormatContext *pContext = NULL;
    AVDictionaryEntry *tag = NULL;

	banner();

    av_register_all();
	//avcodec_register_all();

	pContext = avformat_alloc_context();
    ret = avformat_open_input(&pContext, file.toLatin1().data(), NULL, NULL);
	if (ret) {
		errLog(QString("avformat_open_input����, ret = %1").arg(ret));
        return;
	}

	if (ret = avformat_find_stream_info(pContext, NULL)) {
		errLog(QString("avformat_find_stream_info����, ret = %1").arg(ret));
		return;
	}

	infoLog(QString("�ļ��а�����[%1]����").arg(pContext->nb_streams));
	for (int i = 0; i < pContext->nb_streams; i++) {
		infoLog(QString("��[%1]��������Ϣ:").arg(i));
		AVStream *pStream = pContext->streams[i];

		AVRational frameRate =pStream->r_frame_rate;  
		AVRational timeBase = pStream->time_base; 
		int64_t duration = pStream->duration ; 

		AVCodecContext *pCodecContext = pStream->codec ;
		AVMediaType avMediaType = pCodecContext->codec_type;
		AVCodecID codecID = pCodecContext->codec_id ;

		infoLog(QString("  ʱ��: %1��").arg(duration * av_q2d(timeBase)));

		if (avMediaType == AVMEDIA_TYPE_AUDIO) {
			infoLog("  ����: ��Ƶ");
			infoLog(QString("  ������: %1").arg(pCodecContext->sample_rate));
			infoLog(QString("  ͨ����Ϊ: %1").arg(pCodecContext->channels));
		} else if (avMediaType == AVMEDIA_TYPE_VIDEO) {
			int videoWidth = pCodecContext->width;
			int videoHeight = pCodecContext->height;
			AVSampleFormat sampleFmt = pCodecContext->sample_fmt;
			infoLog("  ����: ��Ƶ");
			infoLog(QString("  �ֱ���: %1 x %2 ").arg(videoWidth).arg(videoHeight));
			infoLog(QString("  ֡��Ϊ: %1/%2 fps").arg(frameRate.num).arg(frameRate.den));
		}
		switch(codecID) {
		case  AV_CODEC_ID_AAC:
			infoLog("  �����ʽ: FAAC");
			break;
		case  AV_CODEC_ID_H264:
			infoLog("  �����ʽ: H264");
			break;
		case AV_CODEC_ID_PCM_ALAW:
			infoLog("  �����ʽ: PCM ALAW");
			break;
		default:
			infoLog(QString("  �����ʽδ֪: %1").arg(codecID));
			break;
		}
	}
    avformat_close_input(&pContext);
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
	m_eventFile = "";
	m_videoFile_0 = "";
	m_videoFile_1 = "";
	m_inputVideoFile = "";

	QString str = QFileDialog::getOpenFileName(
			this, "ѡ���¼�log�ļ�", m_inDir, "*.log");

	if (str == "")
		return;

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
	}
	else {
		m_inputVideoFile = "./output/output.mkv";
	}

	m_eventFile = str;
	warLog(QString("ѡ���� %1").arg(file.baseName()));
	warLog(QString("    �¼��ļ� : %1").arg(QFileInfo(m_eventFile).fileName()));
	warLog(QString("    ��Ƶ�ļ�1: %1").arg(QFileInfo(m_videoFile_0).fileName()));
	if (m_videoFile_1 != "")
		warLog(QString("    ��Ƶ�ļ�2: %1").arg(QFileInfo(m_videoFile_1).fileName()));
	warLog(QString("    �������Ƶ�ļ�: %1").arg(m_inputVideoFile));

	getVideoInfo(m_videoFile_0);
	if (m_videoFile_1 != "")
		getVideoInfo(m_videoFile_1);

	// parse event_log file.
	m_cuttime.input(m_eventFile);
	m_cuttime.showEvents();

	enableEditUI();
}

/* slot */
void vcutter::merge()
{
	createFilelist();
	m_progressCount = 0;

	QStringList args;
	QString program = "ffmpeg.exe";
	args << "-y" << "-f" << "concat" << "-i" << "./output/filelist" << "-c" << "copy" << m_inputVideoFile;
	m_proc->start(program, args);

	if (m_proc->waitForStarted() == false) {
		errLog("�����ϲ�����ʧ��");
	} else {
		okLog("�����ϲ�����ɹ�.");
	}

	m_isMerge = true;
	ui.progressBar->setVisible(true);
	m_timer.start(200);
}

void vcutter::splitOne(int offset, int duration, QString output)
{
	infoLog(QString("start: %1, duration: %2, output: %3").arg(offset).arg(duration).arg(output));
	QStringList args;
	QString program = "ffmpeg.exe";
	args << "-y" << 
			"-i" << m_inputVideoFile <<
		    "-ss" << QString::number(offset, 10) <<
		    "-t" << QString::number(duration, 10) <<
		    "-vcodec" << "copy" << "-acodec" << "copy" << output;
	m_proc->start(program, args);

	if (m_proc->waitForStarted() == false) {
		errLog("�����ϲ�����ʧ��");
	} else {
		okLog("�����ϲ�����ɹ�.");
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
		splitOne(m_cuttime.getSSArg(i), DURATION, m_cuttime.getOutuptName(i));
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