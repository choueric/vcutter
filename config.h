#pragma once

#include <qstring.h>

#ifdef Q_OS_WIN
#include "Windows.h"
#endif

#define VERSION  "0.3"

const QString SUFFIX = ".mp4";
#define FILELIST_NAME "./output/filelist"
