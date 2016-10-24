#include <window.h>

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
    m_listwidget->insertItem(0, "--------------------");
    m_listwidget->insertItem(1, "Global Chat");

    m_quitButton = new QPushButton("Quit Debug", this);
    m_quitButton->setGeometry(200, 430, 80, 30);

    m_testButton = new QPushButton("testing", this);
    m_testButton->setGeometry(200, 370, 80, 30);

    m_trayIcon = new QIcon(":/resources/resources/systray.png");

    m_systray = new QSystemTrayIcon(this);

    m_trayMenu = new QMenu(this);
    m_exitAction = new QAction(this);
    m_exitAction = m_trayMenu->addAction("Exit");

    m_systray->setContextMenu(m_trayMenu);
    m_systray->setIcon(*m_trayIcon);
    m_systray->show();

    peers_class = new peerClass(this);

    m_clientThread = new QThread;
    m_client = new mess_client();
    m_client->moveToThread(m_clientThread);
    //connect(m_client, SIGNAL(finished()), m_clientThread, SLOT(quit()));
    //connect(m_client, SIGNAL(finished()), m_client, SLOT(deleteLater()));
    connect(m_clientThread, SIGNAL(finished()), m_clientThread, SLOT(deleteLater()));
    m_clientThread->start();

    //Signals for gui objects
    QObject::connect(m_button, SIGNAL (clicked()), this, SLOT (buttonClicked()));
    QObject::connect(m_button2, SIGNAL (clicked()), this, SLOT (discoverClicked()));
    QObject::connect(m_quitButton, SIGNAL (clicked()), this, SLOT (quitClicked()));
    QObject::connect(m_listwidget, SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT (currentItemChanged(QListWidgetItem*, QListWidgetItem*)));
    QObject::connect(m_textedit, SIGNAL (returnPressed()), this, SLOT (buttonClicked()));
    QObject::connect(m_exitAction, SIGNAL (triggered()), this, SLOT (quitClicked()));
    QObject::connect(m_systray, SIGNAL (activated(QSystemTrayIcon::ActivationReason)), this, SLOT (showWindow(QSystemTrayIcon::ActivationReason)));
    QObject::connect(m_textedit, SIGNAL (textChanged()), this, SLOT (textEditChanged()));
    QObject::connect(m_testButton, SIGNAL (clicked()), this, SLOT (testClicked()));

    //Signals for mess_serv class
    m_serv2 = new mess_serv(this);
    QObject::connect(m_serv2, SIGNAL (mess_rec(const QString, const QString, bool)), this, SLOT (prints(const QString, const QString, bool)) );
    QObject::connect(m_serv2, SIGNAL (new_client(int, const QString)), this, SLOT (new_client(int, const QString)));
    QObject::connect(m_serv2, SIGNAL (peerQuit(int)), this, SLOT (peerQuit(int)));
    QObject::connect(m_serv2, SIGNAL (mess_peers(QString, QString)), this, SLOT (listpeers(QString, QString)));
    QObject::connect(m_serv2, SIGNAL (potentialReconnect(QString)), this, SLOT (potentialReconnect(QString)));
    QObject::connect(m_serv2, SIGNAL (exitRecieved(QString)), this, SLOT (exitRecieved(QString)));
    QObject::connect(m_serv2, SIGNAL (sendIps(int)), this, SLOT (sendIps(int)));
    QObject::connect(m_serv2, SIGNAL (finished()), m_serv2, SLOT (deleteLater()));
    QObject::connect(m_serv2, SIGNAL (ipCheck(QString)), this, SLOT (ipCheck(QString)));
    QObject::connect(m_serv2, SIGNAL (sendName(int)), m_client, SLOT (sendNameSlot(int)));
    m_serv2->start();

    //maybe use portable sleep command here? dont think there is actually a difference
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

    //QTimer *timer = new QTimer(this);
    //QObject::connect(timer, SIGNAL(timeout()), this, SLOT(timerout()));
    //timer->start(2000);
}
void Window::timerout()
{
   qDebug() << "Timer done";
}

char* Window::returnName()
{
    return this->name;
}

void Window::ipCheck(QString comp)
{
    QStringList temp = comp.split('@');
    QString hname = temp[0];
    QString ipaddr = temp[1];
    for(int i = 0; i < peersLen; i++)
    {
        if(hname.compare(peers_class->peers[i].hostname) == 0)
        {
            return;
        }
    }
    listpeers(hname, ipaddr);
    return;
}

void Window::sendIps(int i)
{
    char type[5] = "/ip/";
    char msg2[((peersLen * 16) * 2)] = {};
    for(int k = 0; k < peersLen; k++)
    {
        if(peers_class->peers[k].isConnected)
        {
            strcat(msg2, peers_class->peers[k].hostname.toStdString().c_str());
            strcat(msg2, "@");
            strcat(msg2, peers_class->peers[k].c_ipaddr);
            strcat(msg2, ":");
        }
    }
    m_client->send_msg(i, msg2, name, type);
}

void Window::testClicked()
{
    QString q = "192.168.1.191";
    int s = socket(AF_INET, SOCK_STREAM, 0);
    m_client->c_connect(s, q.toStdString().c_str());
}

void Window::sendPeerList()
{
    emit sendPeerListSignal(peers_class);
}

void Window::textEditChanged()
{
    if(m_textedit->toPlainText().length() > 1000)
    {
        int diff = m_textedit->toPlainText().length() - 1000;
        QString temp = m_textedit->toPlainText();
        temp.chop(diff);
        m_textedit->setText(temp);
        QTextCursor cursor(m_textedit->textCursor());
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        m_textedit->setTextCursor(cursor);
    }
}

int Window::exitRecieved(QString ipaddr)
{
    for(int i = 0; i < peersLen; i++)
    {
        if(ipaddr.compare(QString::fromUtf8(peers_class->peers[i].c_ipaddr)) == 0)
        {
            peerQuit(peers_class->peers[i].socketdescriptor);
            return 1;
        }
    }
    return 0;
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

    if(m_listwidget->row(item) == m_listwidget->count()-1)
    {
        globalChatAlerted = false;
    }
    else
    {
        peers_class->peers[m_listwidget->row(item)].alerted = false;
    }
    temp = item->text();
    temp.remove(0, 3);
    temp.remove(temp.length()-3, 3);
    item->setText(temp);

    return;
}

void Window::currentItemChanged(QListWidgetItem *item1, QListWidgetItem *item2)
{
    int index1 = m_listwidget->row(item1);
    if(index1 == m_listwidget->count() - 1)
    {
       m_textbrowser->setText(globalChat);
    }
    else
    {
        m_textbrowser->setText(peers_class->peers[index1].textBox);
    }
    if(item1->background() == QGuiApplication::palette().alternateBase())
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
        if( ( (QString::fromUtf8(peers_class->peers[i].c_ipaddr)).compare(ipaddr) == 0 ) )
        {
            if(m_listwidget->item(i)->font().italic())
            {
                this->print(peers_class->peers[i].hostname + " on " + ipaddr + " reconnected", i, false);
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
        if(peers_class->peers[i].socketdescriptor == s)
        {
            peers_class->peers[i].isConnected = 0;
            if( !( m_listwidget->item(i)->font().italic() ) )
            {
                QFont mfont = m_listwidget->item(i)->font();
                mfont.setItalic(true);
                m_listwidget->item(i)->setFont(mfont);
                //m_listwidget->item(i)->font().setItalic(true);
                peers_class->peers[i].socketisValid = 0;
                this->print(peers_class->peers[i].hostname + " on " + QString::fromUtf8(peers_class->peers[i].c_ipaddr) + " disconnected", i, false);
                this->assignSocket(&peers_class->peers[i]);
            }
        }
    }
}

void Window::new_client(int s, QString ipaddr)
{
    for(int i = 0; i < peersLen; i++)
    {
        if(strcmp(peers_class->peers[i].c_ipaddr, ipaddr.toStdString().c_str()) == 0)
        {
            peers_class->peers[i].socketdescriptor = s;
            //this->assignSocket(&(peers_class->peers[i]));
            peers_class->peers[i].isConnected = true;
            if(m_listwidget->item(i)->font().italic())
            {
                this->print(peers_class->peers[i].hostname + " on " + ipaddr + " reconnected", i, false);
                QFont mfont = m_listwidget->item(i)->font();
                mfont.setItalic(false);
                m_listwidget->item(i)->setFont(mfont);
            }
            return;
        }
    }
    //If we got here it means this new peer is not in the list, where he came from we'll never know.
    //But actually, when this happens we need to get his hostname.  Temporarily we will make his hostname
    //his ip address but will ask him for his name via the command that follows this comment.
    m_client->send_msg(s, "", "", "/namerequest");
    listpeers(ipaddr, ipaddr);

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
    for( ; ( peers_class->peers[i].isValid ); i++ )
    {
        if( (ipaddr.compare(peers_class->peers[i].c_ipaddr) ) == 0 )
        {
            if( (hname.compare(peers_class->peers[i].hostname)) != 0)
            {
                qDebug() << "Name change for " << peers_class->peers[i].hostname << " @ " << QString::fromUtf8(peers_class->peers[i].c_ipaddr);
                goto skipreturn;
            }
            return;
        }
    }
    peers_class->peers[i].hostname = hname;
    peers_class->peers[i].isValid = true;
    strcpy(peers_class->peers[i].c_ipaddr, ipstr);
    peersLen++;
    assignSocket(&(peers_class->peers[i]));
skipreturn:
    if(m_client->c_connect(peers_class->peers[i].socketdescriptor, peers_class->peers[i].c_ipaddr) >= 0)
    {
        peers_class->peers[i].isConnected = true;
        m_serv2->update_fds(peers_class->peers[i].socketdescriptor);
        m_client->send_msg(peers_class->peers[i].socketdescriptor, "", "", "/request");
    }

    qDebug() << "hostname: " << peers_class->peers[i].hostname << " @ ip:" << QString::fromUtf8(peers_class->peers[i].c_ipaddr);

    sortPeers();

    return;
}


/*
 * This function sorts the peers array by alphabetical order of the hostname
 * */
void Window::sortPeers()
{
    if(peersLen >= 1)
    {
        peers_class->sortPeers(peersLen);
        if( ( m_listwidget->count() - 2 ) < peersLen)
        {
            m_listwidget->insertItem(0, "");
            globalChatIndex++;
        }
        else if( ( m_listwidget->count() - 2 ) > peersLen)
        {
            m_listwidget->removeItemWidget(m_listwidget->item(peersLen));
            globalChatIndex--;
        }
        for(int i = 0; i < ( m_listwidget->count() - 2 ); i++)
        {
            m_listwidget->item(i)->setText(peers_class->peers[i].hostname);
            if(peers_class->peers[i].alerted)
            {
                this->changeListColor(i, 1);
                m_listwidget->item(i)->setText(" * " + m_listwidget->item(i)->text() + " * ");
            }
        }
    }
    else
    {
        m_listwidget->insertItem(0, (peers_class->peers[0].hostname));
        globalChatIndex++;
    }
    return;
}

void Window::closeEvent(QCloseEvent *event)
{
    for(int i = 0; i < peersLen; i++)
    {
#ifdef __unix__
        ::close(peers_class->peers[i].socketdescriptor);
#endif
#ifdef _WIN32
        ::closesocket(peers_class->peers[i].socketdescriptor);
#endif
    }
    if(m_serv2 != 0 && m_serv2->isRunning())
    {
        m_serv2->requestInterruption();
        m_serv2->quit();
        m_serv2->wait();
    }
    m_clientThread->quit();
    m_clientThread->wait();
    delete m_client;
    delete m_trayIcon;
    m_trayMenu->deleteLater();
    delete peers_class;
    m_systray->hide();
    event->accept();
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
    std::cout << "----------------------------------------" << std::endl;
    std::cout << peers_class->peers[0].hostname.toStdString() << std::endl;
    this->sendPeerList();
    std::cout << peers_class->peers[0].hostname.toStdString() << std::endl;
    std::cout << "----------------------------------------" << std::endl;
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
void Window::globalSend(QString msg)
{
    for(int i = 0; i < peersLen; i++)
    {
        if(peers_class->peers[i].isConnected)
        {
            m_client->send_msg(peers_class->peers[i].socketdescriptor, msg.toStdString().c_str(), name, "/global");
        }
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
    QString str = m_textedit->toPlainText();
    const char* c_str = str.toStdString().c_str();

    int index = m_listwidget->currentRow();
    if( ( index == globalChatIndex ) && ( strcmp(c_str, "") != 0) )
    {
        globalSend(str);
        return;
    }

    int s_socket = peers_class->peers[index].socketdescriptor;

    if(peers_class->peers[index].socketisValid)
    {
        if(!(peers_class->peers[index].isConnected))
        {
            if(m_client->c_connect(s_socket, peers_class->peers[index].c_ipaddr) < 0)
            {
                this->print("Could not connect to " + peers_class->peers[index].hostname + " | on socket " + QString::number(peers_class->peers[index].socketdescriptor), index, false);
                return;
            }
            peers_class->peers[index].isConnected = true;
            m_serv2->update_fds(s_socket);
            m_client->send_msg(peers_class->peers[index].socketdescriptor, "", "", "/request");
            //this->sendIps(peers_class->peers[index].socketdescriptor);

        }
        if(strcmp(c_str, "") != 0)
        {
            if((m_client->send_msg(s_socket, c_str, name, "/msg")) == -5)
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
        this->assignSocket(&peers_class->peers[index]);
    }
    return;
}
void Window::changeListColor(int row, int style)
{
    QBrush back = (m_listwidget->item(row)->background());
    if(style == 1)
    {
        m_listwidget->item(row)->setBackground(QGuiApplication::palette().alternateBase());
    }
    if(style == 0)
    {
        m_listwidget->item(row)->setBackground(QGuiApplication::palette().base());
    }
    return;
}
void Window::focusFunction()
{
    if(this->isActiveWindow())
    {
        QSound::play(":/resources/resources/message.wav");
        return;
    }
    if(this->windowState() == Qt::WindowActive)
    {
        QSound::play(":/resources/resources/message.wav");
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
    if(peerindex == globalChatIndex)
    {
        globalChat.append(strnew + "\n");
    }
    else
    {
        peers_class->peers[peerindex].textBox.append(strnew + "\n");
    }
    if(peerindex == m_listwidget->currentRow())
    {
        m_textbrowser->append(strnew);
        qApp->alert(this, 0);
    }
    else if(message)
    {
        this->changeListColor(peerindex, 1);
        if( !( peers_class->peers[peerindex].alerted ) && ( peerindex != m_listwidget->count()-1 ) )
        {
            m_listwidget->item(peerindex)->setText(" * " + m_listwidget->item(peerindex)->text() + " * ");
            peers_class->peers[peerindex].alerted = true;
        }
        else if(peerindex == m_listwidget->count()-1 && !(globalChatAlerted))
        {
            m_listwidget->item(peerindex)->setText(" * " + m_listwidget->item(peerindex)->text() + " * ");
            globalChatAlerted = true;
        }
    }
    return;
}
void Window::prints(const QString str, const QString ipstr, bool global)
{
    if(global)
    {
        this->focusFunction();
        this->print(str, m_listwidget->count()-1, true);
        return;
    }
    for(int i = 0; i < peersLen; i++)
    {
        if(ipstr.compare(QString::fromUtf8(peers_class->peers[i].c_ipaddr)) == 0)
        {
            this->focusFunction();
            this->print(str, i, true);
            return;
        }
    }
    qDebug() << "finding socket in peers failed Window::prints.  Slot from mess_serv";
    return;
}
