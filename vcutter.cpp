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
		errLog("��ʼ��Ŀ¼ʧ��.");
		return;
	}

	if (initEncoder() == false) {
		errLog("��������ʼ��ʧ��.");;
		return;
	}

	okLog("��ʼ���ɹ�");
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

	QString str = QFileDialog::getOpenFileName(
			this, "ѡ���¼�log�ļ�", m_inDir, "*.log");

	if (str == "")
		return;

	QFileInfo file(str);
	QString prefix = file.absolutePath() + "/" + file.baseName();
	m_videoFile_0 = prefix + ".mp4";
	m_videoFile_1 = prefix + "_1.mp4";

	if (checkFileExist(m_videoFile_0) == false) {
		m_videoFile_0 = "";
		errLog(QString("��Ƶ�ļ�[%1]������.").arg(m_videoFile_0));
		return;
	}

	if (checkFileExist(m_videoFile_1) == false) {
		m_videoFile_1 = "";
	}

	m_eventFile = str;
	warLog(QString("ѡ���� %1").arg(file.baseName()));
	warLog(QString("    �¼��ļ� : %1").arg(QFileInfo(m_eventFile).fileName()));
	warLog(QString("    ��Ƶ�ļ�1: %1").arg(QFileInfo(m_videoFile_0).fileName()));
	if (m_videoFile_1 != "")
		warLog(QString("    ʱ���ļ�2: %1").arg(QFileInfo(m_videoFile_1).fileName()));

	getInfo();

	enableEditUI();
}
