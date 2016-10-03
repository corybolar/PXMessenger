#ifndef PEERLIST_H
#define PEERLIST_H
#include <QString>
#include <arpa/inet.h>

struct peerlist{
    QString hostname = "";
    char c_ipaddr[INET6_ADDRSTRLEN];
    int comboindex = 0;
    int socketdescriptor = 0;
    bool isValid = false;
    bool isConnected = false;
    bool socketisValid = false;
};

#endif // PEERLIST_H
