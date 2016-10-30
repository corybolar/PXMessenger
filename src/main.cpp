#include <QApplication>
//extern "C" { #include <iniparser.h> }
#include <iniparser.h>
#include <inireader.h>
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

    const char *ini_name = "test.ini";
    IniReader iniRdr;
    iniRdr.doPlease(ini_name);
    iniRdr.checkAllowMoreThanOneInstance(ini_name);
    int i = iniRdr.getUUIDNumber(ini_name);
    QUuid uuid = iniRdr.getUUID(ini_name, i);
    qDebug() << "------------------" << uuid.toString();

    MessengerWindow *window = new MessengerWindow(uuid, i);
    window->show();

    int result = app.exec();
    delete window;

    iniRdr.resetUUID(i, uuid, ini_name);

    return result;
}
