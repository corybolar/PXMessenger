#ifndef PEERLIST_H
#define PEERLIST_H
#include <QString>
#include <QObject>
#include <QMutex>

#ifdef __unix__
#include <arpa/inet.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

struct peerlist{
    QString hostname = "";
    char c_ipaddr[INET6_ADDRSTRLEN];
    int comboindex = 0;
    int socketdescriptor = 0;
    bool isValid = false;
    bool isConnected = false;
    bool socketisValid = false;
    QString textBox = "";
    bool alerted = false;
};

class peerClass : public QObject
{
    Q_OBJECT
public:
    explicit peerClass(QWidget *parent);
    peerlist peers[255];
    void sortPeers(int len);
private:
    QMutex pMutex;
    Q_DISABLE_COPY(peerClass)
    static int qsortCompare(const void *a, const void *b);
};



#endif // PEERLIST_H
