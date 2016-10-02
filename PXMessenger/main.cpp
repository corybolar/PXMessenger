#include <QApplication>
#include <QPushButton>
#include <window.h>
#include <mess_client.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <mess_serv.h>

int main(int argc, char **argv)
{
    QApplication app (argc, argv);

    Window window;
    window.show();

    int result1 = app.exec();

    return result1;
}
