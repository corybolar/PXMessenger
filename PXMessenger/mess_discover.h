#ifndef MESS_DISCOVER_H
#define MESS_DISCOVER_H
#include <QThread>


class mess_discover : public QThread
{
    Q_OBJECT
public:
    mess_discover();
    void run();
    void d_listener();								//Listen for UDP packets
signals:
    void mess_peers(QString hname, QString ipaddr);	//return info of discovered peers hostname and ip address
    void potentialReconnect(QString);				//return hostname of a potential reconnected peer
    void exitRecieved(QString);
};

#endif // MESS_DISCOVER_H
