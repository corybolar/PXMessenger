#ifndef MESS_CLIENT_H
#define MESS_CLIENT_H


class mess_client
{
public:
    mess_client();
    int c_connect(int socketfd, const char *ipaddr);
    void setMsg(const char *msg);
    void setHost(const char *host);
    int send_msg(int socketfd, const char *msg, const char *ipaddr);
    char *msg;
    int partialSend(int socketfd, const char *msg, int len);
};

#endif // MESS_CLIENT_H
