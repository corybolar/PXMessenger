#include "messinireader.h"

MessIniReader::MessIniReader()
{

}
bool MessIniReader::checkAllowMoreThanOne()
{
    QSettings inisettings;
    if(inisettings.contains("config/AllowMoreThanOneInstance"))
    {
        return inisettings.value("config/AllowMoreThanOneInstance", false).toBool();
    }
    inisettings.setValue("config/AllowMoreThanOneInstance", false);
    return false;
}
int MessIniReader::getUUIDNumber()
{
    QSettings inisettings;
    int i = 0;
    QString uuidStr = "uuid/";
    while(inisettings.value(uuidStr + QString::number(i), "") == "INUSE")
    {
        i++;
    }
    if(inisettings.value(uuidStr + QString::number(i), "") == "")
    {
        inisettings.setValue(uuidStr+QString::number(i), QUuid::createUuid());
    }
    return i;
}
QUuid MessIniReader::getUUID(int num)
{
    QSettings inisettings;
    QUuid uuid = inisettings.value("uuid/" + QString::number(num), "").toUuid();
    inisettings.setValue("uuid/" + QString::number(num), "INUSE");
    return uuid;
}
int MessIniReader::resetUUID(int num, QUuid uuid)
{
    QSettings inisettings;
    inisettings.setValue("uuid/" + QString::number(num), uuid.toString());
    return 1;
}
