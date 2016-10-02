#include "mess_recieve.h"
#include <QDebug>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

mess_recieve::mess_recieve(QString s, int new_fd)
{
    hostip = s;
    sock = new_fd;
}
mess_recieve::~mess_recieve()
{
    close(this->sock);
}

void mess_recieve::run()
{
    //qDebug() << "hello";
    while(1) {
        char msg[100] = "";
        char host[12] = "";
        int len = 100;
        int lenread;
        lenread = recv(this->sock, (char *)msg, len, 0);
        //qDebug() << "hello2";
        if (lenread == 0)
        {
            break;
        }
        else if(lenread < len)
            msg[lenread] = '\0';
        else
            msg[len-1] = '\0';
        char form_mess[len - 12] = "";
        for(int i = 0; i <= lenread; i++)
        {
            if(i < 12)
            {
                if(msg[i] != ' ')
                    host[i] = msg[i];
            }
            else
            {
                form_mess[i-13] = msg[i];
            }
        }
        QString qstr = this->hostip + " (" + QString::fromUtf8(host) + "):" + QString::fromUtf8(form_mess);
        emit mess_rec2(qstr);
        if( strcmp(form_mess, "exit") == 0)
        {
            break;
        }
    }
    return;
}
