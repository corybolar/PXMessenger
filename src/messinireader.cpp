#include "messinireader.h"

MessIniReader::MessIniReader()
{
    inisettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "PXMessenger", "PXMessenger", NULL);
}
MessIniReader::~MessIniReader()
{
    delete inisettings;
}

bool MessIniReader::checkAllowMoreThanOne()
{
    if(inisettings->contains("config/AllowMoreThanOneInstance"))
    {
        return inisettings->value("config/AllowMoreThanOneInstance", false).toBool();
    }
    inisettings->setValue("config/AllowMoreThanOneInstance", false);
    return false;
}
int MessIniReader::getUUIDNumber()
{
    int i = 0;
    QString uuidStr = "uuid/";
    while(inisettings->value(uuidStr + QString::number(i), "") == "INUSE")
    {
        i++;
    }
    if(inisettings->value(uuidStr + QString::number(i), "") == "")
    {
        inisettings->setValue(uuidStr+QString::number(i), QUuid::createUuid());
    }
    return i;
}
QUuid MessIniReader::getUUID(int num, bool takeIt)
{
    if(inisettings->value("uuid/" + QString::number(num), "") == "")
    {
        inisettings->setValue("uuid/" + QString::number(num), QUuid::createUuid().toString());
    }
    QUuid uuid = inisettings->value("uuid/" + QString::number(num), "").toUuid();
    if(takeIt)
    {
        inisettings->setValue("uuid/" + QString::number(num), "INUSE");
    }
    return uuid;
}
int MessIniReader::resetUUID(int num, QUuid uuid)
{
    inisettings->setValue("uuid/" + QString::number(num), uuid.toString());
    return 1;
}
void MessIniReader::setPort(QString protocol, int portNumber)
{
    inisettings->setValue("port/" + protocol, portNumber);
}
int MessIniReader::getPort(QString protocol)
{
    int portNumber = inisettings->value("port/" + protocol, 0).toInt();
    if( portNumber == 0)
    {
        inisettings->setValue("port/" + protocol, 0);
        return 0;
    }
    else
    {
        return portNumber;
    }

}
void MessIniReader::setHostname(QString hostname)
{
    inisettings->setValue("hostname/hostname", hostname);

}
QString MessIniReader::getHostname(QString defaultHostname)
{
    QString hostname = inisettings->value("hostname/hostname", "").toString();
    if(hostname.isEmpty())
    {
        inisettings->setValue("hostname/hostname", defaultHostname);
        return defaultHostname;
    }
    else
    {
        return hostname;
    }

}
void MessIniReader::setWindowSize(QSize windowSize)
{
    if(windowSize.isValid())
        inisettings->setValue("WindowSize/QSize", windowSize);
}
QSize MessIniReader::getWindowSize(QSize defaultSize)
{
    QSize windowSize = inisettings->value("WindowSize/QSize", defaultSize).toSize();
    if(windowSize.isValid())
        return windowSize;
    else
    {
        windowSize = QSize(700, 500);
        inisettings->setValue("WindowSize/QSize", windowSize);
        return windowSize;
    }

}
