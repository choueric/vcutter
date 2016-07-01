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
	m_proc = NULL;

	ui.setupUi(this);
	initLog(ui.logWidget);
	ui.progressBar->setVisible(false);
	for (int i = -10; i <= 10; i++)
		ui.offsetBox->addItem(QString::number(i, 10));
	ui.offsetBox->setCurrentIndex(ui.offsetBox->findText("0"));
	m_offset = 0;

	m_program = "./ffmpeg.exe";
	if (checkProgram() == false) {
		errLog("没有找到ffmpeg.exe");
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
		errLog("初始化目录失败.");
		return;
	}

	okLog("初始化成功");
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
		errLog("创建Filelist文件错误!");
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
	warLog(QString("视频输入目录为: %1").arg(m_inDir));

	m_outDir = QDir::currentPath() + "/output";
	checkDir(m_outDir);
	warLog(QString("视频输出目录为: %1").arg(m_outDir));

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

	pContext = avformat_alloc_context();
    ret = avformat_open_input(&pContext, file.toLatin1().data(), NULL, NULL);
	if (ret) {
		errLog(QString("avformat_open_input错误, ret = %1").arg(ret));
        return;
	}

	if (ret = avformat_find_stream_info(pContext, NULL)) {
		errLog(QString("avformat_find_stream_info错误, ret = %1").arg(ret));
		return;
	}

	//infoLog(QString("文件中包哈了[%1]个流").arg(pContext->nb_streams));
	for (int i = 0; i < pContext->nb_streams; i++) {
		//infoLog(QString("第[%1]个流的信息:").arg(i));
		AVStream *pStream = pContext->streams[i];

		AVRational frameRate =pStream->r_frame_rate;  
		AVRational timeBase = pStream->time_base; 
		int64_t duration = pStream->duration ; 

		AVCodecContext *pCodecContext = pStream->codec ;
		AVMediaType avMediaType = pCodecContext->codec_type;
		AVCodecID codecID = pCodecContext->codec_id ;

		infoLog(QString("视频时长: %1秒").arg(duration * av_q2d(timeBase)));
		break;

		if (avMediaType == AVMEDIA_TYPE_AUDIO) {
			infoLog("  类型: 音频");
#if 0
			infoLog(QString("  采样率: %1").arg(pCodecContext->sample_rate));
			infoLog(QString("  通道数为: %1").arg(pCodecContext->channels));
#endif
		} else if (avMediaType == AVMEDIA_TYPE_VIDEO) {
			int videoWidth = pCodecContext->width;
			int videoHeight = pCodecContext->height;
			AVSampleFormat sampleFmt = pCodecContext->sample_fmt;
			infoLog("  类型: 视频");
#if 0
			infoLog(QString("  分辨率: %1 x %2 ").arg(videoWidth).arg(videoHeight));
			infoLog(QString("  帧率为: %1/%2 fps").arg(frameRate.num).arg(frameRate.den));
#endif
		}
#if 0
		switch(codecID) {
		case  AV_CODEC_ID_AAC:
			infoLog("  编码格式: FAAC");
			break;
		case  AV_CODEC_ID_H264:
			infoLog("  编码格式: H264");
			break;
		case AV_CODEC_ID_PCM_ALAW:
			infoLog("  编码格式: PCM ALAW");
			break;
		default:
			infoLog(QString("  编码格式未知: %1").arg(codecID));
			break;
		}
#endif
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
			this, "选择事件log文件", m_inDir, "*.log");

	if (str == "")
		return;

	QFileInfo file(str);
	QString prefix = file.absolutePath() + "/" + file.baseName();
	m_videoFile_0 = prefix + SUBFIX;
	m_videoFile_1 = prefix + "_1" + SUBFIX;

	if (checkFileExist(m_videoFile_0) == false) {
		m_videoFile_0 = "";
		errLog(QString("视频文件[%1]不存在.").arg(m_videoFile_0));
		return;
	}
	m_inputVideoFile = "./input/" + QFileInfo(m_videoFile_0).fileName();

	if (checkFileExist(m_videoFile_1) == false) {
		m_videoFile_1 = "";
	} else {
		m_needMerge = true;
		m_inputVideoFile = "./output/output.mkv";
	}

	m_eventFile = str;
	warLog(QString("选择了 %1").arg(file.baseName()));
	warLog(QString("    事件文件 : %1").arg(QFileInfo(m_eventFile).fileName()));
	warLog(QString("    视频文件1: %1").arg(QFileInfo(m_videoFile_0).fileName()));
	if (m_videoFile_1 != "")
		warLog(QString("    视频文件2: %1").arg(QFileInfo(m_videoFile_1).fileName()));
	warLog(QString("    输入的视频文件: %1").arg(m_inputVideoFile));

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
	args << "-y" << "-safe" << "0" << "-f" << "concat" << "-i" << "./output/filelist" << "-c" << "copy" << m_inputVideoFile;
	m_proc->start(m_program, args);

	if (m_proc->waitForStarted() == false) {
		errLog("启动合并程序失败");
	} else {
		okLog("启动合并程序成功.");
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
		errLog(QString("失败: start: %1, duration: %2, output: %3").arg(start).arg(duration).arg(output));
	} else {
		okLog(QString("成功: start: %1, duration: %2, output: %3").arg(start).arg(duration).arg(output));
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
	QMessageBox::information(this, "分割完成", "分割已完成");
}

/* slot */
void vcutter::procFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	if (m_isMerge) {
		m_timer.stop();
		ui.progressBar->setValue(100);
		QMessageBox::information(this, "合并完成", "合并已完成");
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