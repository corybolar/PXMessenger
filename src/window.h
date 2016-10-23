/*
 * Primary class for the messenger.  Calls threads and setups the window.  Objects in this class possibly need revision from public to private to protected or vice versa.
 */
#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QScrollArea>
#include <QTextBrowser>
#include <QLineEdit>
#include <QListWidget>
#include <QComboBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>
#include <QAction>
#include <QWindow>
#include <QScrollBar>
#include <QDebug>
#include <QCloseEvent>
#include <QSound>
#include <QAction>
#include <QApplication>
#include <QPalette>

#include <sys/types.h>
#include <ctime>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <iostream>
#include <sstream>
#include <sys/fcntl.h>

#include <mess_serv.h>
//#include <mess_discover.h>
#include <mess_textedit.h>
#include <mess_client.h>
#include <peerlist.h>

#ifdef __unix__
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <resolv.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif

#define PORT "13653"

class Window : public QWidget
{
    Q_OBJECT

public:
    Window();
    //void displayPeers2(int place);
    void sendPeerList();
    peerClass *peers_class;
    void sortPeers2();
    int qsortCompare(const void *a, const void *b);
protected:
    void closeEvent(QCloseEvent *event)  Q_DECL_OVERRIDE;									//Close event handler

    void changeEvent(QEvent *event);
private slots:
    void buttonClicked();													//Send button event
    void prints(const QString str, const QString ipstr, bool global);					//calls print, currently identifys recipient based on socket descriptor, needs revision
    void listpeers(QString hname, QString ipaddr);							//Add new peers to the peerlist struct array and call the sort/display functions
    void discoverClicked();													//Discover button event
    void quitClicked();														//quit button, used to test the various destructors and close event
    void new_client(int s, QString ipaddr);									//Networking
    void peerQuit(int s);													//Connected client disconnected
    void potentialReconnect(QString ipaddr);								//client has sent a discover udp to us, test if we have seen him before
    void currentItemChanged(QListWidgetItem *item1, QListWidgetItem *item2);//change text in textbrowser to show correct text history and active send recipient
    void showWindow(QSystemTrayIcon::ActivationReason reason);
    int exitRecieved(QString ipaddr);
    void textEditChanged();
    void testClicked();
    void sendIps(int i);
    void ipCheck(QString comp);
    void timerout();
private:
    QPushButton *m_button;
    QPushButton *m_button2;
    QPushButton *m_quitButton;
    QPushButton *m_testButton;
    QIcon *m_trayIcon;
    QAction *m_exitAction;
    QMenu *m_trayMenu;
    QSystemTrayIcon *m_systray;
    QTextBrowser *m_textbrowser;
    QLineEdit *m_lineedit;
    QPushButton *m_sendDebugButton;
    QListWidget *m_listwidget;
    QThread *m_clientThread;
    mess_textedit *m_textedit;
    mess_client *m_client;
    mess_serv *m_serv2;
    time_t mess_time;
    struct tm *now;
    int peersLen = 0;														//Length of peers array
    char name[128] = {};
    QString globalChat = "";
    int globalChatIndex = 2;

    char* returnName();
    void sortPeers();														//sort peerlist struct alphabetically by hostname
    void assignSocket(struct peerlist *p);									//Give socket value to peerlist array member
    void changeListColor(int row, int style);
    void unalert(QListWidgetItem *item);
    void focusFunction();
    void print(QString str, int peerindex, bool message);					//Updates the main text box and stores text history in peerlist struct
    void udpSend(const char *msg);											//send a UDP discover request to the broadcast address of the network
    void globalSend(QString msg);
signals:
    void sendPeerListSignal(peerClass*);

public slots:
};

#endif // WINDOW_H
