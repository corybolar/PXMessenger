#include "inireader.h"

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

#include <QSettings>
#include <QSize>
#include <QString>
#include <QUuid>
#include <QDebug>

#include "consts.h"

PXMIniReader::PXMIniReader()
    : iniFile(new QSettings(QSettings::IniFormat, QSettings::UserScope, "PXMessenger", "PXMessenger", NULL))
{
}
PXMIniReader::~PXMIniReader()
{
}
bool PXMIniReader::checkAllowMoreThanOne()
{
    if (iniFile->contains("config/AllowMoreThanOneInstance")) {
        return iniFile->value("config/AllowMoreThanOneInstance", false).toBool();
    }
    iniFile->setValue("config/AllowMoreThanOneInstance", false);
    return false;
}
void PXMIniReader::setAllowMoreThanOne(bool value)
{
    iniFile->setValue("config/AllowMoreThanOneInstance", value);
}
unsigned int PXMIniReader::getUUIDNumber() const
{
    unsigned int i  = 0;
    QString uuidStr = "uuid/";
    while (iniFile->value(uuidStr + QString::number(i), QString()) == "INUSE") {
        i++;
    }
    if (iniFile->value(uuidStr + QString::number(i), QString()) == "") {
        iniFile->setValue(uuidStr + QString::number(i), QUuid::createUuid().toString());
    }
    return i;
}
QUuid PXMIniReader::getUUID(unsigned int num, bool takeIt) const
{
    QUuid uuid = iniFile->value("uuid/" + QString::number(num), QUuid()).toUuid();
    if (uuid.isNull()) {
        uuid = QUuid::createUuid();
        iniFile->setValue("uuid/" + QString::number(num), uuid);
    }
    if (takeIt) {
        iniFile->setValue("uuid/" + QString::number(num), "INUSE");
    }
    return uuid;
}
int PXMIniReader::resetUUID(unsigned int num, QUuid uuid)
{
    iniFile->setValue("uuid/" + QString::number(num), uuid.toString());
    return 1;
}
void PXMIniReader::setPort(QString protocol, int portNumber)
{
    iniFile->setValue("net/" + protocol, portNumber);
}
unsigned short PXMIniReader::getPort(QString protocol)
{
    unsigned int portNumber = iniFile->value("net/" + protocol, 0).toUInt();
    if (portNumber == 13649) {
        iniFile->setValue("net/" + protocol, 0);
        portNumber = 0;
    } else if (portNumber != 0 && protocol == QLatin1String("TCP")) {
        portNumber += getUUIDNumber();
    } else if (portNumber >= 65535) {
        qWarning() << "Bad port number in UUID..." << protocol << portNumber;
        portNumber = 0;
    }
    return static_cast<unsigned short>(portNumber);
}
void PXMIniReader::setHostname(QString hostname)
{
    iniFile->setValue("hostname/hostname", hostname.left(PXMConsts::MAX_HOSTNAME_LENGTH));
}
QString PXMIniReader::getHostname(QString defaultHostname)
{
    QString hostname = iniFile->value("hostname/hostname", QString()).toString();
    if (hostname.isEmpty()) {
        iniFile->setValue("hostname/hostname", defaultHostname);
        hostname = defaultHostname;
    }

    return hostname.left(PXMConsts::MAX_HOSTNAME_LENGTH).simplified();
}
void PXMIniReader::setWindowSize(QSize windowSize)
{
    if (windowSize.isValid())
        iniFile->setValue("WindowSize/QSize", windowSize);
}
QSize PXMIniReader::getWindowSize(QSize defaultSize) const
{
    QSize windowSize = iniFile->value("WindowSize/QSize", defaultSize).toSize();
    if (windowSize.isValid()) {
        return windowSize;
    } else {
        windowSize = QSize(700, 500);
        iniFile->setValue("WindowSize/QSize", windowSize);
        return windowSize;
    }
}
void PXMIniReader::setMute(bool mute)
{
    iniFile->setValue("config/Mute", mute);
}
bool PXMIniReader::getMute() const
{
    return iniFile->value("config/Mute", false).toBool();
}
void PXMIniReader::setFocus(bool focus)
{
    iniFile->setValue("config/PreventFocus", focus);
}
bool PXMIniReader::getFocus() const
{
    return iniFile->value("config/PreventFocus", false).toBool();
}
QString PXMIniReader::getFont()
{
    return iniFile->value("config/Font", "").toString();
}
void PXMIniReader::setFont(QString font)
{
    iniFile->setValue("config/Font", font);
}
QString PXMIniReader::getMulticastAddress() const
{
    QString ipFull = iniFile->value("net/MulticastAddress", "").toString();
    if (ipFull.isEmpty() || strlen(ipFull.toLatin1().constData()) > INET_ADDRSTRLEN) {
        return QString::fromLocal8Bit(PXMConsts::DEFAULT_MULTICAST_ADDRESS);
    }
    return ipFull;
}
int PXMIniReader::setMulticastAddress(QString ip)
{
    iniFile->setValue("net/MulticastAddress", ip);

    return 0;
}
int PXMIniReader::getVerbosity() const
{
    return iniFile->value("config/DebugVerbosity", 0).toInt();
}

void PXMIniReader::setVerbosity(int level) const
{
    iniFile->setValue("config/DebugVerbosity", level);
}
bool PXMIniReader::getLogActive() const
{
    return iniFile->value("config/LogActive", false).toBool();
}

void PXMIniReader::setLogActive(bool status) const
{
    iniFile->setValue("config/LogActive", status);
}

bool PXMIniReader::getDarkColorScheme() const
{
    return iniFile->value("config/DarkColorScheme", false).toBool();
}
void PXMIniReader::setDarkColorScheme(bool status) const
{
    iniFile->setValue("config/DarkColorScheme", status);
}
void PXMIniReader::setUpdates(bool status) const
{
#ifdef _WIN32
    iniFile->setValue("config/Autoupdate", status);
#endif
    (void)status;
}
bool PXMIniReader::getUpdates() const
{
#ifdef _WIN32
    return iniFile->value("config/Autoupdate", true).toBool();
#endif
    return false;
}
