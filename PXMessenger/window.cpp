#include "window.h"
#include <QPushButton>
#include <QLineEdit>
#include <string.h>
#include <mess_client.h>
#include <mess_serv.h>
#include <mess_discover.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <resolv.h>
#include <sys/fcntl.h>
#include <peerlist.h>
#include <QCloseEvent>

#define PORT "3490"
#define BACKLOG 20

Window::Window()
{
    setFixedSize(700,500);

    gethostname(name, sizeof name);

    m_textedit = new QTextEdit(this);
    m_textedit->setGeometry(10, 250, 380, 100);

    m_lineedit = new QLineEdit(this);
    m_lineedit->setGeometry(410, 10, 200, 30);
    m_lineedit->setText(QString::fromUtf8(name));

    m_textbrowser= new QTextBrowser(this);
    m_textbrowser->setGeometry(10, 10, 380, 230);

    m_button = new QPushButton("Send", this);
    m_button->setGeometry(10,370,80,30);
    m_button2 = new QPushButton("Discover", this);
    m_button2->setGeometry(10, 430, 80, 30);


    m_listwidget = new QListWidget(this);
    m_listwidget->setGeometry(410, 100, 200, 300);

    m_quitButton = new QPushButton("Quit Debug", this);
    m_quitButton->setGeometry(200, 430, 80, 30);

    m_client = new mess_client();

    QObject::connect(m_button, SIGNAL (clicked()), this, SLOT (buttonClicked()));
    QObject::connect(m_button2, SIGNAL (clicked()), this, SLOT (discoverClicked()));
    //QObject::connect(m_sendDebugButton, SIGNAL (clicked()), this, SLOT (debugClicked()));
    QObject::connect(m_quitButton, SIGNAL (clicked()), this, SLOT (quitClicked()));
    QObject::connect(m_listwidget, SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT (currentItemChanged(QListWidgetItem*, QListWidgetItem*)));

    m_disc = new mess_discover();
    QObject::connect(m_disc, SIGNAL (mess_peers(QString, QString)), this, SLOT (listpeers(QString, QString)));
    QObject::connect(m_disc, SIGNAL (potentialReconnect(QString)), this, SLOT (potentialReconnect(QString)));
    QObject::connect(m_disc, SIGNAL (finished()), m_disc, SLOT (deleteLater()));
    m_disc->start();

    m_serv2 = new mess_serv();
    QObject::connect(m_serv2, SIGNAL (mess_rec(const QString, int)), this, SLOT (prints(const QString, int)) );
    QObject::connect(m_serv2, SIGNAL (new_client(int, const QString)), this, SLOT (new_client(int, const QString)));
    QObject::connect(m_serv2, SIGNAL (peerQuit(int)), this, SLOT (peerQuit(int)));
    QObject::connect(m_serv2, SIGNAL (finished()), m_serv2, SLOT (deleteLater()));
    m_serv2->start();


    sleep(1);
    char discovermess[138];
    memset(discovermess, 0, sizeof(discovermess));
    strncpy(discovermess, "/discover\0", 10);
    strcat(discovermess, name);
    this->udpSend(discovermess);
}
void Window::currentItemChanged(QListWidgetItem *item1, QListWidgetItem *item2)
{
    m_textbrowser->setText(peers[m_listwidget->row(item1)].textBox);
    return;
}

void Window::potentialReconnect(QString ipaddr)
{
    for(int i = 0; i < peersLen; i++)
    {
        if( ( (QString::fromUtf8(peers[i].c_ipaddr)).compare(ipaddr) == 0 ) )
        {
            this->print(peers[i].hostname + " on " + ipaddr + " reconnected", i);
            QFont mfont = m_listwidget->item(i)->font();
            mfont.setItalic(false);
            m_listwidget->item(i)->setFont(mfont);
        }
    }
}

void Window::peerQuit(int s)
{
    for(int i = 0; i < peersLen; i++)
    {
        if(peers[i].socketdescriptor == s)
        {
            peers[i].isConnected = 0;
            QFont mfont = m_listwidget->item(i)->font();
            mfont.setItalic(true);
            m_listwidget->item(i)->setFont(mfont);
            //m_listwidget->item(i)->font().setItalic(true);
            peers[i].socketisValid = 0;
            this->print(peers[i].hostname + " on " + QString::fromUtf8(peers[i].c_ipaddr) + " disconnected", i);
            this->assignSocket(&peers[i]);
        }
    }
}

void Window::new_client(int s, QString ipaddr)
{
    for(int i = 0; i < peersLen; i++)
    {
        if(strcmp(peers[i].c_ipaddr, ipaddr.toStdString().c_str()) == 0)
        {
            peers[i].socketdescriptor = s;
            //this->assignSocket(&(peers[i]));
            peers[i].isConnected = true;
            return;
        }
    }
}

void Window::quitClicked()
{
    close();
}

/* This is the function called when mess_discover recieves a udp packet starting with "/name:"
 * here we check to see if the ip of the selected host is already in the peers array and if not
 * add it.  peers is then sorted and displayed to the gui.  QStrings are used for ease of
 * passing through the QT signal and slots mechanism.*/
void Window::listpeers(QString hname, QString ipaddr)
{
    QByteArray ba = ipaddr.toLocal8Bit();
    const char *ipstr = ba.data();
    int i = 0;
    for( ; ( peers[i].isValid ); i++ )
    {
        if( (hname.compare(peers[i].hostname) ) == 0 )
        {
            std::cout << ipstr << " | already discovered" << std::endl;
            goto donehere;
        }
    }
    peers[i].hostname = hname;
    peers[i].isValid = true;
    peers[i].comboindex = i;
    strcpy(peers[i].c_ipaddr, ipstr);
    peersLen++;
    assignSocket(&(peers[i]));
    //m_serv2->update_fds(peers[i].socketdescriptor);
    sortPeers();
    displayPeers();

    qDebug() << "hostname: " << peers[i].hostname << " @ ip:" << QString::fromUtf8(peers[i].c_ipaddr);
donehere:
    return;
}
/* This function sorts the peers array by alphabetical order of the hostname
 *
 * TODO:IMPROVE
 *
 * */
void Window::sortPeers()
{
    if(peersLen > 1)
    {
        int i = peersLen-1;
        for( ; i > 0; i-- )
        {
            char temp1[28];
            char temp2[28];

            strcpy(temp1, (peers[i].hostname.toStdString().c_str()));
            strcpy(temp2, (peers[i-1].hostname.toStdString().c_str()));
            int temp1Len = strlen(temp1);
            int temp2Len = strlen(temp2);
            for(int i = 0; i < temp1Len; i++)
            {
                temp1[i] = tolower(temp1[i]);
            }
            for(int i = 0; i < temp2Len; i++)
            {
                temp2[i] = tolower(temp2[i]);
            }
            if( strcmp(temp1, temp2) < 0 )
            {
                struct peerlist p = peers[i-1];
                peers[i-1] = peers[i];
                peers[i] = p;
                peers[i].comboindex = i;
                peers[i-1].comboindex = ( i - 1 );
            }
        }
    }
}
void Window::closeEvent(QCloseEvent *event)
{
    for(int i = 0; i < 3; i++)
    {
        this->udpSend("/exit");
    }
    for(int i = 0; i < peersLen; i++)
    {
        ::close(peers[i].socketdescriptor);
    }
    m_serv2->quit();
    m_disc->quit();
    delete m_client;
    event->accept();
}

/*Display peers array of hostnames to the QComboBox, m_combobox
 * Account for the fact that length of peers list could have changed by +/- 1
 *
 * TODO:IMPROVE
 * */
void Window::displayPeers()
{
    if(peersLen == m_listwidget->count())
    {
        for(int i = 0; i < peersLen; i++)
        {
            //m_combobox->setItemText(i, peers[i].hostname);
            //qlistpeers[i].setText(peers[i].hostname);
            m_listwidget->item(i)->setText(peers[i].hostname);
        }
    }
    if(peersLen < m_listwidget->count())
    {
        for(int i = 0; i < peersLen; i++)
        {
            //m_combobox->setItemText(i, peers[i].hostname);
            m_listwidget->item(i)->setText(peers[i].hostname);
        }
        //m_combobox->removeItem(peersLen+1);
        m_listwidget->removeItemWidget(m_listwidget->item(peersLen+1));
    }
    if(peersLen > m_listwidget->count())
    {
        for(int i = 0; i < peersLen-1; i++)
        {
            //m_combobox->setItemText(i, peers[i].hostname);
            m_listwidget->item(i)->setText(peers[i].hostname);
        }
        //m_combobox->addItem(peers[peersLen-1].hostname);
        m_listwidget->addItem(peers[peersLen-1].hostname);
        if(m_listwidget->count() == 1)
        {
            m_listwidget->setCurrentItem(m_listwidget->item(0));
        }
    }
    std::cout << sizeof(peers[0]) << std::endl;
    return;

}
/* Assign sockets to peers added to the peers array*/
void Window::assignSocket(struct peerlist *p)
{
    p->socketdescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if(p->socketdescriptor < 0)
        perror("socket:");
    else
        p->socketisValid = 1;
    return;
}
/* dummy function to allow discoverSend to be called on its own */
void Window::discoverClicked()
{
    char discovermess[138];
    strncpy(discovermess, "/discover\0", 10);
    strcat(discovermess, name);
    this->udpSend(discovermess);
}

/*Send the "/discover" message to the local networks broadcast address
 * this will only send to computers in the 255.255.255.0 subnet*/
void Window::udpSend(const char* msg)
{
    //int status;
    int port2 = 3491;
    struct sockaddr_in broadaddr;
    int socketfd2;

    if ( (socketfd2 = (socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
        perror("socket:");
    int t1 = 1;
    if (setsockopt(socketfd2, SOL_SOCKET, SO_BROADCAST, &t1, sizeof (int)))
        perror("setsockopt failed:");
    memset(&broadaddr, 0, sizeof(broadaddr));
    broadaddr.sin_family = AF_INET;
    broadaddr.sin_addr.s_addr = inet_addr("192.168.1.255");
    broadaddr.sin_port = htons(port2);
    //char *msg = "/discover";
    int len = strlen(msg);

    sendto(socketfd2, msg, len+1, 0, (struct sockaddr *)&broadaddr, sizeof(broadaddr));
}
/* Send message button function.  Calls m_client to both connect and send a message to the provided ip_addr*/
void Window::buttonClicked()
{
    std::string str = m_textedit->toPlainText().toStdString();
    const char* c_str = str.c_str();

    int index = m_listwidget->currentRow();

    int s_socket = peers[index].socketdescriptor;
    m_client->setHost(m_lineedit->text().toStdString().c_str());

    if(peers[index].socketisValid)
    {
        if(!(peers[index].isConnected))
        {
            //s_socket = socket(AF_INET, SOCK_STREAM, 0);
            if(m_client->c_connect(s_socket, peers[index].c_ipaddr) < 0)
            {
                this->print("Could not connect to " + peers[index].hostname + " on socket " + QString::number(peers[index].socketdescriptor), index);
                return;
            }
            peers[index].isConnected = true;
            m_serv2->update_fds(s_socket);
        }
        if(strcmp(c_str, "") != 0)
        {
            if((m_client->send_msg(s_socket, c_str, peers[index].c_ipaddr)) == -5)
            {
                this->print("Peer has closed connection, send failed", index);
                return;
            }
            this->print(m_lineedit->text() + ": " + m_textedit->toPlainText() + " on socket: " + QString::number(peers[index].socketdescriptor), index);
            m_textedit->setText("");
        }
    }
    else
    {
        this->print("Peer Disconnected", index);
        this->assignSocket(&peers[index]);
    }
    return;
}
void Window::print(const QString str, int peerindex)
{
    //peers[peerindex].textBox = peers[peerindex].textBox + str + "\n";
    peers[peerindex].textBox.append(str + "\n");
    if(peerindex == m_listwidget->currentRow())
    {
        //m_textbrowser->setText(m_textbrowser->toPlainText() + str + "\n");
        m_textbrowser->append(str);
    }
    return;
}
void Window::prints(const QString str, int s)
{
    for(int i = 0; i < peersLen; i++)
    {
        if(s == peers[i].socketdescriptor)
        {
            this->print(str, i);
            return;
        }
    }
    qDebug() << "finding socket in peers failed Window::prints.  Slot from mess_serv";
    return;
}
