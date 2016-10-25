#include <QApplication>
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
    MessengerWindow window;
    window.show();

    int result1 = app.exec();

    return result1;
}
