#include <QApplication>
#include <QUuid>
#include <QDir>
#include <QMessageBox>
#include <QString>
#include <QLockFile>
#include <QFontDatabase>
#include <pxminireader.h>
#include <pxmdefinitions.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <pxmmainwindow.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

int main(int argc, char **argv)
{
    QApplication app (argc, argv);

#ifdef _WIN32
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
    {
        printf("Failed WSAStartup error: %d", WSAGetLastError());
        return 1;
    }
#endif

    QCoreApplication::setApplicationName("PXMessenger");
    QCoreApplication::setOrganizationName("PXMessenger");
    QCoreApplication::setOrganizationDomain("PXMessenger");
    QCoreApplication::setApplicationVersion("0.9");

    MessIniReader iniReader;
    initialSettings presets;
    char localHostname[250];

    QFont font;
    if(!(iniReader.getFont().isEmpty()))
    {
        font.fromString(iniReader.getFont());
        app.setFont(font);
    }

#ifdef __unix__
    struct passwd *user;
    user = getpwuid(getuid());
    strcpy(localHostname, user->pw_name);
#elif _WIN32
    char user[UNLEN+1];
    TCHAR t_user[UNLEN+1];
    DWORD user_size = UNLEN+1;
    if(GetUserName(t_user, &user_size))
    {
        wcstombs(user, t_user, UNLEN+1);
        strcpy(localHostname, user);
    }
#endif

    QString tmpDir = QDir::tempPath();
    QLockFile lockFile(tmpDir + "/pxmessenger" + QString::fromLatin1(localHostname) + ".lock");

    bool allowMoreThanOne = iniReader.checkAllowMoreThanOne();

    if(allowMoreThanOne)
    {
        presets.uuidNum = iniReader.getUUIDNumber();
    }
    else
    {
        if(!lockFile.tryLock(100)){
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("You already have this app running."
                            "\r\nOnly one instance is allowed.");
            msgBox.exec();
            return 1;
        }
    }
    presets.uuid = iniReader.getUUID(presets.uuidNum, allowMoreThanOne);
    presets.username = iniReader.getHostname(QString::fromLatin1(localHostname));
    presets.tcpPort = iniReader.getPort("TCP");
    presets.udpPort = iniReader.getPort("UDP");
    presets.windowSize = iniReader.getWindowSize(QSize(700, 500));
    presets.mute = iniReader.getMute();
    presets.preventFocus = iniReader.getFocus();

    PXMWindow window(presets);
    //window.show();

    int result = app.exec();

    iniReader.resetUUID(presets.uuidNum, presets.uuid);

    return result;
}
