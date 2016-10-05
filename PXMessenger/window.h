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
#include <mess_client.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <QListWidget>
#include <QComboBox>
#include <peerlist.h>
#include <mess_serv.h>
#include <mess_discover.h>
#include <mess_textedit.h>

class Window : public QWidget
{
    Q_OBJECT

public:
    Window();
    void print(QString str, int peerindex, bool message);									//Updates the main text box and stores text history in peerlist struct
    void udpSend(const char *msg);											//send a UDP discover request to the broadcast address of the network
protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;									//Close event handler

private slots:
    void buttonClicked();													//Send button event
    void prints(const QString str, const QString ipstr);										//calls print, currently identifys recipient based on socket descriptor, needs revision
    void listpeers(QString hname, QString ipaddr);							//Add new peers to the peerlist struct array and call the sort/display functions
    void discoverClicked();													//Discover button event
    void quitClicked();														//quit button, used to test the various destructors and close event
    void new_client(int s, QString ipaddr);									//Networking
    void peerQuit(int s);													//Connected client disconnected
    void potentialReconnect(QString ipaddr);								//client has sent a discover udp to us, test if we have seen him before
    void currentItemChanged(QListWidgetItem *item1, QListWidgetItem *item2);//change text in textbrowser to show correct text history and active send recipient
private:
    QPushButton *m_button;
    QPushButton *m_button2;
    QPushButton *m_quitButton;
    mess_textedit *m_textedit;
    QTextBrowser *m_textbrowser;
    QLineEdit *m_lineedit;
    QPushButton *m_sendDebugButton;
    QListWidget *m_listwidget;
    mess_client *m_client;
    mess_discover *m_disc;
    std::vector<QString> textWindow;
    int socketfd;															//possibly obsolete
    peerlist peers[255];													//array of peerlist structures that holds info about connected computers
    void sortPeers();														//sort peerlist struct alphabetically by hostname
    void displayPeers();													//update the QListWidget to show the connected peers
    void assignSocket(struct peerlist *p);									//Give socket value to peerlist array member
    int peersLen = 0;														//Length of peers array
    mess_serv *m_serv2;
    char name[128] = {};
    void changeListColor(int row, int style);
    void unalert(QListWidgetItem *item);
signals:

public slots:
};

#endif // WINDOW_H
