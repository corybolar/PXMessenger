#ifndef MESS_DISCOVER_H
#define MESS_DISCOVER_H
#include <QtCore>


class mess_discover : public QThread
{
    Q_OBJECT
public:
    mess_discover();
    void run();
    void d_listener();
signals:
    void mess_peers(QString hname, QString ipaddr);
    void potentialReconnect(QString);
};

#endif // MESS_DISCOVER_H
