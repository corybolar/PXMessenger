#ifndef MESSINIREADER_H
#define MESSINIREADER_H
#include <QString>
#include <QDebug>
#include <QUuid>
#include <QSettings>
#include <fstream>
#include <iostream>

class MessIniReader
{
public:
    MessIniReader();
    bool checkAllowMoreThanOne();
    int getUUIDNumber();
    QUuid getUUID(int num);
    int resetUUID(int num, QUuid uuid);
};

#endif // MESSINIREADER_H
