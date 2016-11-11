#ifndef MESS_STRUCTS_H
#define MESS_STRUCTS_H
#include <event2/util.h>
#include <QString>
#include <event2/bufferevent.h>
#include <QUuid>

struct peerDetails{
    bool isValid = false;
    bool isConnected = false;
    bool attemptingToConnect = false;
    bool messagePending = false;
    evutil_socket_t socketDescriptor = 0;
    QString portNumber = "-1";
    int listWidgetIndex = -1;
    QString ipAddress = "";
    QString hostname = "";
    QString textBox = "";
    QUuid identifier;
    bufferevent *bev = nullptr;
};
#endif // MESS_STRUCTS_H
