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
#include <QMainWindow>
#include <QGridLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>

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
#include <event2/event.h>
#include <event2/bufferevent.h>

#include <mess_serv.h>
#include <mess_textedit.h>
#include <mess_client.h>
#include <messinireader.h>
#include <mess_structs.h>
#include <peerlist.h>
#include <settingsDialog.h>

#ifdef __unix__
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <resolv.h>
#include <pwd.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <lmcons.h>
#endif

class MessengerWindow : public QMainWindow
{
    Q_OBJECT

public:
    //MessengerWindow(QUuid uuid, int uuidNum, QString username, int tcpPort, int udpPort, QSize windowSize);
    MessengerWindow(initialSettings presets);
    ~MessengerWindow();
protected:
    void closeEvent(QCloseEvent *event)  Q_DECL_OVERRIDE;									//Close event handler
    void changeEvent(QEvent *event);
public slots:
    void setListenerPort(unsigned short port);
private slots:
    void sendButtonClicked();													//Send button event
    void quitButtonClicked();														//quit button, used to test the various destructors and close event
    void debugButtonClicked();
    void currentItemChanged(QListWidgetItem *item1);//change text in textbrowser to show correct text history and active send recipient
    void textEditChanged();
    void showWindow(QSystemTrayIcon::ActivationReason reason);
    void printToTextBrowserServerSlot(const QString str, QUuid uuid, bool global);					//calls print, currently identifys recipient based on socket descriptor, needs revision
    void printToTextBrowser(QString str, QUuid uuid, bool message);					//Updates the main text box and stores text history in peerlist struct
    void timerOutSingleShot();

    void updateListWidget(QUuid uuid);														//sort peerlist struct alphabetically by hostname
    void setItalicsOnItem(QUuid uuid, bool italics);
    void aboutActionSlot();
    void settingsActionsSlot();
    void timerOutRepetitive();
private:
    QGridLayout *layout;
    QWidget *centralwidget;
    QSpacerItem *horizontalSpacer;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *horizontalSpacer_3;
    QSpacerItem *horizontalSpacer_4;
    QMenuBar *menubar;
    QStatusBar *statusbar;
    QPushButton *messSendButton;
    QPushButton *messQuitButton;
    QPushButton *messDebugButton;
    QAction *messSystemTrayExitAction;
    QMenu *messSystemTrayMenu;
    QSystemTrayIcon *messSystemTray;
    QTextBrowser *messTextBrowser;
    QLineEdit *messLineEdit;
    QPushButton *m_sendDebugButton;
    QListWidget *messListWidget;
    QCheckBox *muteCheckBox;
    QCheckBox *focusCheckBox;
    QThread *messClientThread;
    QTimer *timer;
    QString ourUUIDString;

    MessengerTextEdit *messTextEdit;
    MessengerClient *messClient;
    MessengerServer *messServer;
    PeerWorkerClass *peerWorker;

    time_t messTime;
    struct tm *currentTime;

    int numberOfValidPeers = 0;														//Length of peers array
    char *localHostname;
    unsigned short ourTCPListenerPort;
    unsigned short ourUDPListenerPort;
    QString globalChat = "";
    QUuid globalChatUuid;
    bool globalChatAlerted = false;

    //void udpSend(const char *msg);											//send a UDP discover request to the broadcast address of the network
    void globalSend(QString msg);

    void focusWindow();
    void removeMessagePendingStatus(QListWidgetItem *item);
    void changeListColor(int row, int style);

    void createTextEdit();
    void createTextBrowser();
    void createLineEdit();
    void createButtons();
    void createListWidget();
    void createSystemTray();
    void connectGuiSignalsAndSlots();
    void createMessClient();
    void createMessServ();
    void createMessTime();
    void connectPeerClassSignalsAndSlots();
    QString getFormattedTime();
    void setupHostname(int uuidNum, QString username);
    void setupLayout();
    void setupTooltips();
    void setupMenuBar();
    void createCheckBoxes();
signals:
    void connectToPeer(evutil_socket_t, QString, QString);
    void sendMsg(evutil_socket_t, QString, QString, QUuid, QString);
    void sendUdp(QString, unsigned short);
    void retryDiscover();
};

#endif // WINDOW_H
