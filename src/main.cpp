#include <QApplication>
#include <messinireader.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <QUuid>
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
    MessIniReader iniReader;
    bool allowMoreThanOne = iniReader.checkAllowMoreThanOne();
    int uuidnum = iniReader.getUUIDNumber();
    QUuid uuid = iniReader.getUUID(uuidnum);

    MessengerWindow window(uuid, uuidnum);
    window.show();

    int result = app.exec();
    //delete window;

    iniReader.resetUUID(uuidnum, uuid);

    return result;
}
