#pragma execution_character_set ("utf-8")
#include "cuttime.h"

#include <QFile.h>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QIODevice>

#include "log.h"


cuttime::cuttime()
{
}

cuttime::~cuttime()
{
}

bool cuttime::input(QString fileName)
{
	QString line;
	QFile file(fileName);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return false;
	}

	QTextStream in(&file);

	m_eventList.clear();
	while (1) {
		QString line = in.readLine();
		if (line == "")
			break;
		parseLine(line);
	}

	return true;
}

void cuttime::showEvents()
{
	infoLog("----------------------------------------------------------");
	infoLog("Ê±¼ä´Á:");
	for (int i = 0; i < m_eventList.size(); i++) {
		event_t *e = &m_eventList[i];
		if (i == 0)
			infoLog(QString("Start time: %1").arg(e->time.toString("yyyy-MM-dd hh:mm:ss")));
		else if (i == m_eventList.size() - 1)
			infoLog(QString("End time: %1").arg(e->time.toString("yyyy-MM-dd hh:mm:ss")));
		else
			infoLog(QString("[%1]: Start = %2, Tag = %3").arg(i).arg(m_eventList[0].time.secsTo(e->time)).arg(e->tag));
	}
}

bool cuttime::parseLine(QString &line)
{
	event_t event;

	if (line.contains(">>> start >>>")) {
		event.tag = "start";
	} else if (line.contains("||| stop |||")) {
		event.tag = "end";
	} else {
		event.tag = line.mid(23, 8);
	}
	event.time = getDatetime(line);
	m_eventList.append(event);

	return true;
}

QDateTime cuttime::getDatetime(QString &line)
{
	int year = line.mid(1, 4).toInt();
	int mon = line.mid(6, 2).toInt();
	int day = line.mid(9, 2).toInt();
	int hour = line.mid(12, 2).toInt();
	int min = line.mid(15, 2).toInt();
	int sec = line.mid(18, 2).toInt();

	return QDateTime(QDate(year, mon, day), QTime(hour, min, sec));
}

int cuttime::eventCount()
{
	return m_eventList.size() - 2; // except start and end.
}

// get value of -SS option in ffmpeg
int cuttime::getSSArg(int index)
{
	int i = index + 1; // the first event in event_list is start
	int offset = m_eventList[0].time.secsTo(m_eventList[i].time);
	int start = offset - PRE_DUR;
	if (start < 0)
		start = 0;

	return start;
}

QString cuttime::getOutuptName(int i)
{
	i++;  // the first event in event_list is start
	return QString("./output/%1_%2.mkv").arg(i).arg(m_eventList[i].tag);
}
