#ifndef MESS_CLIENT_H
#define MESS_CLIENT_H


class mess_client
{
public:
    mess_client();
    int c_connect(int socketfd, const char *ipaddr);					//Connect a socket to and ip address
    void setHost(const char *host);										//Set a hostname to send with messages REVISE
    int send_msg(int socketfd, const char *msg, const char *ipaddr);	//send a message through an already connected socket to the specified ip address
    char *msg = {};															//unknown
    int partialSend(int socketfd, const char *msg, int len);			//deal with the kernel not sending all of our message in one go
};

#endif // MESS_CLIENT_H
