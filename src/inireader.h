#ifndef INIREADER_H
#define INIREADER_H
#include <iostream>
#include <fstream>
#include <iniparser.h>
#include <sys/stat.h>

#include <QDebug>
#include <QString>
#include <QUuid>

class IniReader
{
public:
    IniReader();
    int doPlease(QString filename);
    const char *createIni(const char *ini_name);
    int checkAllowMoreThanOneInstance(const char *ini_name);
    QUuid getUUID(const char *ini_name, int num);
    int getUUIDNumber(const char *ini_name);
    void resetUUID(int num, QUuid uuid, const char *ini_name);
};

#endif // INIREADER_H
