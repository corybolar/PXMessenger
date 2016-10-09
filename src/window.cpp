#include <window.h>

#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <iostream>
#include <sstream>
#include <sys/fcntl.h>
#include <ctime>

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
#define BACKLOG 20

Window::Window()
{
    char discovermess[138];
    setFixedSize(700,500);

    gethostname(name, sizeof name);

    m_textedit = new mess_textedit(this);
    m_textedit->setGeometry(10, 250, 380, 100);

    m_lineedit = new QLineEdit(this);
    m_lineedit->setGeometry(410, 10, 200, 30);
    m_lineedit->setText(QString::fromUtf8(name));

    m_textbrowser= new QTextBrowser(this);
    m_textbrowser->setGeometry(10, 10, 380, 230);
    m_textbrowser->setText("Use the selection pane on the right to message a FRIEND!");

    m_button = new QPushButton("Send", this);
    m_button->setGeometry(10,370,80,30);
    m_button2 = new QPushButton("Discover", this);
    m_button2->setGeometry(10, 430, 80, 30);


    m_listwidget = new QListWidget(this);
    m_listwidget->setGeometry(410, 100, 200, 300);

    m_quitButton = new QPushButton("Quit Debug", this);
    m_quitButton->setGeometry(200, 430, 80, 30);

    m_trayIcon = new QIcon(":/resources/resources/systray.png");

    m_trayMenu = new QMenu(this);
    m_exitAction = new QAction(this);
    m_exitAction = m_trayMenu->addAction("Exit");

    m_systray = new QSystemTrayIcon(this);
    m_systray->setContextMenu(m_trayMenu);
    m_systray->setIcon(*m_trayIcon);
    m_systray->show();

    m_client = new mess_client();
    //Signals for gui objects
    QObject::connect(m_button, SIGNAL (clicked()), this, SLOT (buttonClicked()));
    QObject::connect(m_button2, SIGNAL (clicked()), this, SLOT (discoverClicked()));
    QObject::connect(m_quitButton, SIGNAL (clicked()), this, SLOT (quitClicked()));
    QObject::connect(m_listwidget, SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT (currentItemChanged(QListWidgetItem*, QListWidgetItem*)));
    QObject::connect(m_textedit, SIGNAL (returnPressed()), this, SLOT (buttonClicked()));
    QObject::connect(m_exitAction, SIGNAL (triggered()), this, SLOT (quitClicked()));
    QObject::connect(m_systray, SIGNAL (activated(QSystemTrayIcon::ActivationReason)), this, SLOT (showWindow(QSystemTrayIcon::ActivationReason)));
    QObject::connect(m_textedit, SIGNAL (textChanged()), this, SLOT (textEditChanged()));
    //Signals for mess_discover class
    m_disc = new mess_discover(this);
    QObject::connect(m_disc, SIGNAL (mess_peers(QString, QString)), this, SLOT (listpeers(QString, QString)));
    QObject::connect(m_disc, SIGNAL (potentialReconnect(QString)), this, SLOT (potentialReconnect(QString)));
    QObject::connect(m_disc, SIGNAL (finished()), m_disc, SLOT (deleteLater()));
    QObject::connect(m_disc, SIGNAL (exitRecieved(QString)), this, SLOT (exitRecieved(QString)));
    m_disc->start();
    //Signals for mess_serv class
    m_serv2 = new mess_serv(this);
    QObject::connect(m_serv2, SIGNAL (mess_rec(const QString, const QString)), this, SLOT (prints(const QString, const QString)) );
    QObject::connect(m_serv2, SIGNAL (new_client(int, const QString)), this, SLOT (new_client(int, const QString)));
    QObject::connect(m_serv2, SIGNAL (peerQuit(int)), this, SLOT (peerQuit(int)));
    QObject::connect(m_serv2, SIGNAL (finished()), m_serv2, SLOT (deleteLater()));
    m_serv2->start();

#ifdef _WIN32
    Sleep(500);
#else
    usleep(500000);
#endif
    mess_time = time(0);
    now = localtime( &mess_time );

    memset(discovermess, 0, sizeof(discovermess));
    strncpy(discovermess, "/discover\0", 10);
    strcat(discovermess, name);
    this->udpSend(discovermess);

}
void Window::textEditChanged()
{
    if(m_textedit->toPlainText().length() > 240)
    {
        int diff = m_textedit->toPlainText().length() - 240;
        QString temp = m_textedit->toPlainText();
        temp.chop(diff);
        m_textedit->setText(temp);
        QTextCursor cursor(m_textedit->textCursor());
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        m_textedit->setTextCursor(cursor);
    }
}

void Window::exitRecieved(QString ipaddr)
{
    for(int i = 0; i < peersLen; i++)
    {
        if(ipaddr.compare(QString::fromUtf8(peers[i].c_ipaddr)) == 0)
        {
            peerQuit(peers[i].socketdescriptor);
            break;
        }
    }
    return;
}

void Window::showWindow(QSystemTrayIcon::ActivationReason reason)
{
    if( ( ( reason == QSystemTrayIcon::DoubleClick ) | ( reason == QSystemTrayIcon::Trigger ) ) && ! ( this->isVisible() ) )
    {
        this->setWindowState(Qt::WindowMaximized);
        this->show();
        this->setWindowState(Qt::WindowActive);
    }
    return;
}

void Window::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::WindowStateChange)
    {
        if(isMinimized())
        {
            this->hide();
            event->ignore();
        }
    }
    QWidget::changeEvent(event);
}

void Window::unalert(QListWidgetItem* item)
{
    QString temp;

    peers[m_listwidget->row(item)].alerted = false;
    temp = item->text();
    temp.remove(0, 3);
    temp.remove(temp.length()-3, 3);
    item->setText(temp);

    return;
}

void Window::currentItemChanged(QListWidgetItem *item1, QListWidgetItem *item2)
{
    int index1 = m_listwidget->row(item1);
    //if(peers[index1].textBox.length() > 0)
    //{
        //if(peers[index1].textBox.at(peers[index1].textBox.length() - 1) == '\n')
        //{
            //peers[index1].textBox.chop(1);
        //}
    //}
    m_textbrowser->setText(peers[index1].textBox);
    if(item1->backgroundColor() == QColor("red"))
    {
        this->changeListColor(index1, 0);
        this->unalert(item1);
    }
    QScrollBar *sb = this->m_textbrowser->verticalScrollBar();
    sb->setValue(sb->maximum());
    return;
}

void Window::potentialReconnect(QString ipaddr)
{
    for(int i = 0; i < peersLen; i++)
    {
        if( ( (QString::fromUtf8(peers[i].c_ipaddr)).compare(ipaddr) == 0 ) )
        {
            if(m_listwidget->item(i)->font().italic())
            {
                this->print(peers[i].hostname + " on " + ipaddr + " reconnected", i, false);
                QFont mfont = m_listwidget->item(i)->font();
                mfont.setItalic(false);
                m_listwidget->item(i)->setFont(mfont);
            }
            return;
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
            if( !( m_listwidget->item(i)->font().italic() ) )
            {
                QFont mfont = m_listwidget->item(i)->font();
                mfont.setItalic(true);
                m_listwidget->item(i)->setFont(mfont);
                //m_listwidget->item(i)->font().setItalic(true);
                peers[i].socketisValid = 0;
                this->print(peers[i].hostname + " on " + QString::fromUtf8(peers[i].c_ipaddr) + " disconnected", i, false);
                this->assignSocket(&peers[i]);
            }
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
            return;
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
    //displayPeers();

    qDebug() << "hostname: " << peers[i].hostname << " @ ip:" << QString::fromUtf8(peers[i].c_ipaddr);
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
        bool insert = false;
        int index;
        for( ; i > 0; i-- )
        {
            char temp1[28] = {};
            char temp2[28] = {};

            strcpy(temp1, (peers[i].hostname.toStdString().c_str()));
            strcpy(temp2, (peers[i-1].hostname.toStdString().c_str()));
            int temp1Len = strlen(temp1);
            int temp2Len = strlen(temp2);
            for(int k = 0; k < temp1Len; k++)
            {
                temp1[k] = tolower(temp1[k]);
            }
            for(int k = 0; k < temp2Len; k++)
            {
                temp2[k] = tolower(temp2[k]);
            }
            if( strcmp(temp1, temp2) < 0 )
            {
                struct peerlist p = peers[i-1];
                peers[i-1] = peers[i];
                peers[i] = p;
                peers[i].comboindex = i;
                peers[i-1].comboindex = ( i - 1 );
                insert = true;
                index = i;
            }
        }
        if(insert)
        {
            m_listwidget->insertItem(index-1, peers[index-1].hostname);
        }
        else
        {
            m_listwidget->addItem(peers[peersLen-1].hostname);
        }
    }
    else
    {
        m_listwidget->insertItem(0, (peers[0].hostname));
    }
}

void Window::closeEvent(QCloseEvent *event)
{
    for(int i = 0; i < 1; i++)
    {
        this->udpSend("/exit");
    }
    for(int i = 0; i < peersLen; i++)
    {
#ifdef __unix__
        ::close(peers[i].socketdescriptor);
#endif
#ifdef _WIN32
        ::closesocket(peers[i].socketdescriptor);
#endif
    }
    if(m_serv2 != 0 && m_serv2->isRunning())
    {
        m_serv2->requestInterruption();
        m_serv2->wait();
    }
    if(m_disc != 0 && m_disc->isRunning() )
    {
        m_disc->requestInterruption();
        m_disc->wait();
    }
    delete m_client;
    delete m_trayIcon;
    m_systray->hide();
    event->accept();
}

/*Display peers array of hostnames to the QComboBox, m_combobox
 * Account for the fact that length of peers list could have changed by +/- 1
 *
 * TODO:IMPROVE
 * */
/*void Window::displayPeers()
{
    if(peersLen == m_listwidget->count())
    {
    QFont temp;
    for(int i = 0; i < peersLen; i++)
    {
        //m_combobox->setItemText(i, peers[i].hostname);
        //qlistpeers[i].setText(peers[i].hostname);
        temp = m_listwidget->item(i)->font();
        m_listwidget->item(i)->setText(peers[i].hostname);
    }
    }
    if(peersLen < m_listwidget->count())
    {
    for(int i = 0; i < peersLen; i++)
    {
        m_listwidget->item(i)->setText(peers[i].hostname);
    }
    m_listwidget->removeItemWidget(m_listwidget->item(peersLen+1));
    }
    if(peersLen > m_listwidget->count())
    {
    //QListWidget *temp = m_listwidget->clone;
    for(int i = 0; i < peersLen-1; i++)
    {
        m_listwidget->item(i)->setText(peers[i].hostname);
    }
    m_listwidget->addItem(peers[peersLen-1].hostname);
    if(m_listwidget->count() == 1)
    {
        //m_listwidget->setCurrentItem(m_listwidget->item(0));
    }
    }
    return;

}
*/
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
    int len;
    int port2 = 13654;
    struct sockaddr_in broadaddr;
    int socketfd2;

    if ( (socketfd2 = (socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
        perror("socket:");
#ifdef __unix__
    int t1 = 1;
    if (setsockopt(socketfd2, SOL_SOCKET, SO_BROADCAST, &t1, sizeof (int)))
        perror("setsockopt failed:");
#endif
#ifdef _WIN32
    if (setsockopt(socketfd2, SOL_SOCKET, SO_BROADCAST, "true", sizeof(int)))
        perror("setsockopt failed:");
#endif
    memset(&broadaddr, 0, sizeof(broadaddr));
    broadaddr.sin_family = AF_INET;
    broadaddr.sin_addr.s_addr = inet_addr("192.168.1.255");
    broadaddr.sin_port = htons(port2);
    len = strlen(msg);

    for(int i = 0; i < 2; i++)
    {
        sendto(socketfd2, msg, len+1, 0, (struct sockaddr *)&broadaddr, sizeof(broadaddr));
    }
}
/* Send message button function.  Calls m_client to both connect and send a message to the provided ip_addr*/
void Window::buttonClicked()
{
    if(m_listwidget->selectedItems().count() == 0)
    {
        this->print("Choose a computer to message from the selection pane on the right", -1, false);
        return;
    }
    std::string str = m_textedit->toPlainText().toStdString();
    const char* c_str = str.c_str();

    int index = m_listwidget->currentRow();

    int s_socket = peers[index].socketdescriptor;

    if(peers[index].socketisValid)
    {
        if(!(peers[index].isConnected))
        {
            if(m_client->c_connect(s_socket, peers[index].c_ipaddr) < 0)
            {
                this->print("Could not connect to " + peers[index].hostname + " | on socket " + QString::number(peers[index].socketdescriptor), index, false);
                return;
            }
            peers[index].isConnected = true;
            m_serv2->update_fds(s_socket);
        }
        if(strcmp(c_str, "") != 0)
        {
            if(strlen(c_str) > 240)
            {
                this->print("Message too long, messages must be shorter than 240 characters", index, false);
            }
            else if((m_client->send_msg(s_socket, c_str, name)) == -5)
            {
                this->print("Peer has closed connection, send failed", index, false);
                return;
            }
            else{
                this->print(m_lineedit->text() + ": " + m_textedit->toPlainText(), index, true);
            }
            m_textedit->setText("");
        }
    }
    else
    {
        this->print("Peer Disconnected", index, false);
        this->assignSocket(&peers[index]);
    }
    return;
}
void Window::changeListColor(int row, int style)
{
    QBrush back = (m_listwidget->item(row)->background());
    if(style == 1)
    {
        m_listwidget->item(row)->setBackground(Qt::red);
    }
    if(style == 0)
    {
        m_listwidget->item(row)->setBackground(Qt::white);
    }
    return;
}
void Window::focusFunction()
{
    if(this->windowState() == Qt::WindowActive)
    {
        return;
    }
    else if(!(this->isMinimized()))
    {
        QSound::play(":/resources/resources/message.wav");
        qApp->alert(this, 0);
    }
    else if(this->isMinimized())
    {
        QSound::play(":/resources/resources/message.wav");
        this->show();
        qApp->alert(this, 0);
        this->raise();
        this->setWindowState(Qt::WindowActive);
    }
    else
    {
        QSound::play(":/resources/resources/message.wav");
    }
    return;
}

void Window::print(const QString str, int peerindex, bool message)
{
    QString strnew;
    if(message)
    {
        char time_str[12];
        mess_time = time(0);
        now = localtime(&mess_time);
        strftime(time_str, 12, "(%H:%M:%S) ", now);
        strnew = QString::fromUtf8(time_str) + str;
    }
    else
    {
        strnew = str;
    }
    if(peerindex < 0)
    {
        m_textbrowser->append(strnew);
        return;
    }
    peers[peerindex].textBox.append(strnew + "\n");
    if(peerindex == m_listwidget->currentRow())
    {
        m_textbrowser->append(strnew);
        this->focusFunction();
        qApp->alert(this, 0);

    }
    else if(message)
    {
        this->changeListColor(peerindex, 1);
        this->focusFunction();
        if(!(peers[peerindex].alerted))
        {
            m_listwidget->item(peerindex)->setText(" * " + m_listwidget->item(peerindex)->text() + " * ");
            peers[peerindex].alerted = true;
        }
    }
    return;
}
void Window::prints(const QString str, const QString ipstr)
{
    for(int i = 0; i < peersLen; i++)
    {
        if(ipstr.compare(QString::fromUtf8(peers[i].c_ipaddr)) == 0)
        {
            this->print(str, i, true);
            return;
        }
    }
    qDebug() << "finding socket in peers failed Window::prints.  Slot from mess_serv";
    return;
}
