#pragma once

#include <qobject.h>
#include <qstring.h>
#include <qlist.h>
#include <qdatetime.h>

#define PRE_DUR 3
#define NEXT_DUR 7
#define DURATION (PRE_DUR + NEXT_DUR)

struct event_t {
	QString tag;
	QDateTime time;
};

class cuttime: public QObject
{
	Q_OBJECT

public:
	cuttime();
	~cuttime();

	bool input(QString fileName);
	void showEvents();
	int eventCount();
	int getSSArg(int index);
	QString getOutuptName(int index);

private:
	QList<event_t> m_eventList;

	bool parseLine(QString &line);
	QDateTime getDatetime(QString &line);
};

