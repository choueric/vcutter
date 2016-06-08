#pragma execution_character_set ("utf-8")

#include "vcutter.h"
#include <qmessagebox.h>
#include "log.h"

#include <QDir.h>
#include <QFileDialog.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avio.h"
#include "libavutil/file.h"
}

vcutter::vcutter(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	initLog(ui.logWidget);

	disableEditUI();

	if (!initDirs()) {
		errLog("初始化目录失败.");
		return;
	}

	if (initEncoder() == false) {
		errLog("编码器初始化失败.");;
		return;
	}

	okLog("初始化成功");
}

vcutter::~vcutter()
{
	deinitEncoder();
}

void vcutter::banner()
{
	infoLog("----------------------------------------------------------");
}


bool vcutter::initEncoder()
{
	return true;
}

void vcutter::deinitEncoder()
{
}

void vcutter::disableEditUI()
{
	ui.inputBtn->setEnabled(true);

	ui.getInfoBtn->setVisible(false);
	ui.mergeBtn->setEnabled(false);
	ui.cutBtn->setEnabled(false);
}

void vcutter::enableEditUI()
{
	//ui.getInfoBtn->setEnabled(true);
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

void vcutter::getInfo()
{
    int ret;
    AVFormatContext *pContext = NULL;
    AVDictionaryEntry *tag = NULL;

	banner();

    av_register_all();
	//avcodec_register_all();

	pContext = avformat_alloc_context();
    ret = avformat_open_input(&pContext, m_videoFile_0.toLatin1().data(), NULL, NULL);
	if (ret) {
		errLog(QString("avformat_open_input错误, ret = %1").arg(ret));
        return;
	}

	if (ret = avformat_find_stream_info(pContext, NULL)) {
		errLog(QString("avformat_find_stream_info错误, ret = %1").arg(ret));
		return;
	}

	infoLog(QString("文件中包哈了[%1]个流").arg(pContext->nb_streams));
	for (int i = 0; i < pContext->nb_streams; i++) {
		infoLog(QString("第[%1]个流的信息:").arg(i));
		AVStream *pStream = pContext->streams[i];

		AVRational frameRate =pStream->r_frame_rate;  
		AVRational timeBase = pStream->time_base; 
		int64_t duration = pStream->duration ; 

		AVCodecContext *pCodecContext = pStream->codec ;
		AVMediaType avMediaType = pCodecContext->codec_type;
		AVCodecID codecID = pCodecContext->codec_id ;

		infoLog(QString("  时长: %1秒").arg(duration * av_q2d(timeBase)));

		if (avMediaType == AVMEDIA_TYPE_AUDIO) {
			infoLog("  类型: 音频");
			infoLog(QString("  采样率: %1").arg(pCodecContext->sample_rate));
			infoLog(QString("  通道数为: %1").arg(pCodecContext->channels));
		} else if (avMediaType == AVMEDIA_TYPE_VIDEO) {
			int videoWidth = pCodecContext->width;
			int videoHeight = pCodecContext->height;
			AVSampleFormat sampleFmt = pCodecContext->sample_fmt;
			infoLog("  类型: 视频");
			infoLog(QString("  分辨率: %1 x %2 ").arg(videoWidth).arg(videoHeight));
			infoLog(QString("  帧率为: %1/%2 fps").arg(frameRate.num).arg(frameRate.den));
		}
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

	QString str = QFileDialog::getOpenFileName(
			this, "选择事件log文件", m_inDir, "*.log");

	if (str == "")
		return;

	QFileInfo file(str);
	QString prefix = file.absolutePath() + "/" + file.baseName();
	m_videoFile_0 = prefix + ".mp4";
	m_videoFile_1 = prefix + "_1.mp4";

	if (checkFileExist(m_videoFile_0) == false) {
		m_videoFile_0 = "";
		errLog(QString("视频文件[%1]不存在.").arg(m_videoFile_0));
		return;
	}

	if (checkFileExist(m_videoFile_1) == false) {
		m_videoFile_1 = "";
	}

	m_eventFile = str;
	warLog(QString("选择了 %1").arg(file.baseName()));
	warLog(QString("    事件文件 : %1").arg(QFileInfo(m_eventFile).fileName()));
	warLog(QString("    视频文件1: %1").arg(QFileInfo(m_videoFile_0).fileName()));
	if (m_videoFile_1 != "")
		warLog(QString("    时间文件2: %1").arg(QFileInfo(m_videoFile_1).fileName()));

	getInfo();

	enableEditUI();
}
