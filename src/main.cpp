#include <QApplication>
#include <QUuid>
#include <QDir>
#include <QMessageBox>
#include <QString>
#include <messinireader.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <window.h>

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
    QCoreApplication::setApplicationVersion("0.3.2");
    MessIniReader iniReader;
    char localHostname[250];

#ifdef __unix__
    struct passwd *user;
    user = getpwuid(getuid());
    strcat(localHostname, user->pw_name);
#elif _WIN32
    char user[UNLEN+1];
    TCHAR t_user[UNLEN+1];
    DWORD user_size = UNLEN+1;
    if(GetUserName(t_user, &user_size))
    {
        wcstombs(user, t_user, UNLEN+1);
        strcat(localHostname, user);
    }
#endif

    QString tmpDir = QDir::tempPath();
    QLockFile lockFile(tmpDir + "/pxmessenger" + QString::fromUtf8(localHostname) + ".lock");

    bool allowMoreThanOne = iniReader.checkAllowMoreThanOne();

    int uuidnum = 0;
    if(allowMoreThanOne)
    {
        uuidnum = iniReader.getUUIDNumber();
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
    QUuid uuid = iniReader.getUUID(uuidnum, allowMoreThanOne);

    MessengerWindow window(uuid, uuidnum);
    window.show();

    int result = app.exec();
    //delete window;

    iniReader.resetUUID(uuidnum, uuid);

    return result;
}
