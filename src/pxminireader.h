#ifndef MESSINIREADER_H
#define MESSINIREADER_H

#include "pxmconsts.h"
#include <QScopedPointer>

class QSettings;
class QString;
class QSize;
class PXMIniReader
{
    QScopedPointer<QSettings> inisettings;
public:
    PXMIniReader();
    ~PXMIniReader();
    bool checkAllowMoreThanOne();
    int getUUIDNumber() const;
    QUuid getUUID(int num, bool takeIt) const;
    int resetUUID(int num, QUuid uuid);
    unsigned short getPort(QString protocol);
    QString getHostname(QString defaultHostname);
    void setHostname(QString hostname);
    void setPort(QString protocol, int portNumber);
    void setWindowSize(QSize windowSize);
    QSize getWindowSize(QSize defaultSize) const;
    void setMute(bool mute);
    bool getMute() const;
    void setFocus(bool focus);
    bool getFocus() const;
    QString getFont();
    void setFont(QString font);
    int getFontSize() const;
    void setFontSize(int size);
    void setAllowMoreThanOne(bool value);
    int setMulticastAddress(QString ip);
    QString getMulticastAddress() const;
    int getVerbosity() const;
    void setVerbosity(int level) const;
};

#endif // MESSINIREADER_H
