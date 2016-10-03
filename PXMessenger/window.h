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
#include <QComboBox>
#include <peerlist.h>
#include <mess_serv.h>

class Window : public QWidget
{
    Q_OBJECT

public:
    explicit Window(QWidget *parent = 0);
    void print(QString str);
    void udpSend(const char *msg);
private slots:
    void buttonClicked();
    void prints(QString str);
    void listpeers(QString hname, QString ipaddr);
    void discoverClicked();
    void debugClicked();
    void quitClicked();
    void new_client(int s, QString ipaddr);
private:
    QPushButton *m_button;
    QPushButton *m_button2;
    QPushButton *m_quitButton;
    QTextEdit *m_textedit;
    QTextBrowser *m_textbrowser;
    QLineEdit *m_lineedit;
    QComboBox *m_combobox;
    QLineEdit *m_sendDebug;
    QPushButton *m_sendDebugButton;
    mess_client *m_client;
    int socketfd;
    QStringList whname, wipaddr;
    peerlist peers[255];
    void sortPeers();
    void displayPeers();
    void assignSocket(struct peerlist *p);
    int peersLen = 0;
    void closeEvent(QCloseEvent *event);
    mess_serv *m_serv2;



signals:

public slots:
};

#endif // WINDOW_H
