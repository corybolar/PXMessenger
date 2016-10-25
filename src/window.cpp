#include <window.h>

MessengerWindow::MessengerWindow()
{
    setFixedSize(700,500);

    gethostname(localHostname, sizeof localHostname);

    peers_class = new PeerClass(this);

    createTextEdit();

    createTextBrowser();

    createLineEdit();

    createButtons();

    createListWidget();

    createSystemTray();

    createMessClient();

    createMessServ();

    createMessTime();

    connectGuiSignalsAndSlots();


    char discovermess[138];
    memset(discovermess, 0, sizeof(discovermess));
    strncpy(discovermess, "/discover\0", 10);
    strcat(discovermess, localHostname);
    this->udpSend(discovermess);
}
void MessengerWindow::createTextEdit()
{
    messTextEdit = new MessengerTextEdit(this);
    messTextEdit->setGeometry(10, 250, 380, 100);
}
void MessengerWindow::createTextBrowser()
{
    messTextBrowser= new QTextBrowser(this);
    messTextBrowser->setGeometry(10, 10, 380, 230);
    messTextBrowser->setText("Use the selection pane on the right to message a FRIEND!");
}
void MessengerWindow::createLineEdit()
{
    messLineEdit = new QLineEdit(this);
    messLineEdit->setGeometry(410, 10, 200, 30);
    messLineEdit->setText(QString::fromUtf8(localHostname));
}
void MessengerWindow::createButtons()
{
    messSendButton = new QPushButton("Send", this);
    messSendButton->setGeometry(160,370,80,30);

    messQuitButton = new QPushButton("Quit", this);
    messQuitButton->setGeometry(160, 430, 80, 30);
}
void MessengerWindow::createListWidget()
{
    messListWidget = new QListWidget(this);
    messListWidget->setGeometry(410, 60, 200, 290);
    messListWidget->insertItem(0, "--------------------");
    messListWidget->insertItem(1, "Global Chat");
    messListWidget->item(0)->setFlags(messListWidget->item(0)->flags() & ~Qt::ItemIsEnabled);
}
void MessengerWindow::createSystemTray()
{
    messSystemTrayIcon = new QIcon(":/resources/resources/systray.png");

    messSystemTray = new QSystemTrayIcon(this);

    messSystemTrayMenu = new QMenu(this);
    messSystemTrayExitAction = new QAction(this);
    messSystemTrayExitAction = messSystemTrayMenu->addAction("Exit");

    messSystemTray->setContextMenu(messSystemTrayMenu);
    messSystemTray->setIcon(*messSystemTrayIcon);
    messSystemTray->show();
}
void MessengerWindow::createMessClient()
{
    messClientThread = new QThread(this);
    messClient = new MessengerClient();
    messClient->moveToThread(messClientThread);
    connect(messClientThread, SIGNAL(finished()), messClientThread, SLOT(deleteLater()));
    connect(peers_class, SIGNAL (sendMsg(int, QString, QString, QString)), messClient, SLOT(sendMsgSlot(int, QString, QString, QString)));
    connect(peers_class, SIGNAL (connectToPeer(int, QString)), messClient, SLOT(connectToPeerSlot(int, QString)));
    connect(messClient, SIGNAL(connectionSuccessful(int)), peers_class, SLOT (connectionSuccessful(int)));
    messClientThread->start();
}
void MessengerWindow::connectGuiSignalsAndSlots()
{
    QObject::connect(messSendButton, SIGNAL (clicked()), this, SLOT (sendButtonClicked()));
    QObject::connect(messQuitButton, SIGNAL (clicked()), this, SLOT (quitButtonClicked()));
    QObject::connect(messListWidget, SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT (currentItemChanged(QListWidgetItem*, QListWidgetItem*)));
    QObject::connect(messTextEdit, SIGNAL (returnPressed()), this, SLOT (sendButtonClicked()));
    QObject::connect(messSystemTrayExitAction, SIGNAL (triggered()), this, SLOT (quitButtonClicked()));
    QObject::connect(messSystemTray, SIGNAL (activated(QSystemTrayIcon::ActivationReason)), this, SLOT (showWindow(QSystemTrayIcon::ActivationReason)));
    QObject::connect(messSystemTray, SIGNAL(destroyed()), messSystemTrayMenu, SLOT(deleteLater()));
    QObject::connect(messTextEdit, SIGNAL (textChanged()), this, SLOT (textEditChanged()));
}
void MessengerWindow::createMessServ()
{
    messServer = new MessengerServer(this);
    QObject::connect(messServer, SIGNAL (mess_rec(const QString, const QString, bool)), this, SLOT (printToTextBrowserServerSlot(const QString, const QString, bool)) );
    QObject::connect(messServer, SIGNAL (new_client(int, const QString)), peers_class, SLOT (newTcpConnection(int, const QString)));
    QObject::connect(messServer, SIGNAL (peerQuit(int)), peers_class, SLOT (peerQuit(int)));
    QObject::connect(messServer, SIGNAL (mess_peers(QString, QString)), peers_class, SLOT (listpeers(QString, QString)));
    QObject::connect(peers_class, SIGNAL (printToTextBrowser(QString, int, bool)), this, SLOT (printToTextBrowser(QString, int, bool)));
    //QObject::connect(messServer, SIGNAL (potentialReconnect(QString)), this, SLOT (potentialReconnect(QString)));
    QObject::connect(messServer, SIGNAL (sendIps(int)), peers_class, SLOT (sendIps(int)));
    QObject::connect(messServer, SIGNAL (finished()), messServer, SLOT (deleteLater()));
    QObject::connect(messServer, SIGNAL (hostnameCheck(QString)), peers_class, SLOT (hostnameCheck(QString)));
    QObject::connect(messServer, SIGNAL (sendName(int)), messClient, SLOT (sendNameSlot(int)));
    QObject::connect(messServer, SIGNAL (setPeerHostname(QString, QString)), peers_class, SLOT (setPeerHostname(QString, QString)));
    QObject::connect(peers_class, SIGNAL (updateMessServFDS(int)), messServer, SLOT (updateMessServFDSSlot(int)));
    QObject::connect(peers_class, SIGNAL (addItalicsOnItem(int)), this, SLOT (addItalicsOnItem(int)));
    QObject::connect(peers_class, SIGNAL (removeItalicsOnItem(int)), this, SLOT (removeItalicsOnItem(int)));
    QObject::connect(peers_class, SIGNAL (updateListWidget(int)), this, SLOT (updateListWidget(int)));
    messServer->start();
}
void MessengerWindow::createMessTime()
{
    messTime = time(0);
    currentTime = localtime( &messTime );
}

/**
 * @brief 				This is called from mess_serv when it recieves a "/ip" packet list
 * 						This will compare the hostnames to the hostnames we already know about in peer_class
 * @param comp			The string of hostname and ip.  Should be formatted like "hostname@192.168.1.1"
 */
/*
void MessengerWindow::hostnameCheck(QString comp)
{
    QStringList temp = comp.split('@');
    QString hname = temp[0];
    QString ipaddr = temp[1];
    for(int i = 0; i < numberOfValidPeers; i++)
    {
        if(hname.compare(peers_class->knownPeersArray[i].hostname) == 0)
        {
            return;
        }
    }
    listpeers(hname, ipaddr);
    return;
}
*/

/**
 * @brief 				Send our list of ips to another socket.  This formats them from the peers_class object
 * 						to be "hostname@192.168.1.1:hostname2@192.168.1.2:
 * @param i				Socket to send these ips to
 */
/*
void MessengerWindow::sendIps(int i)
{
    char type[5] = "/ip:";
    char msg2[((numberOfValidPeers * 16) * 2)] = {};
    for(int k = 0; k < numberOfValidPeers; k++)
    {
        if(peers_class->knownPeersArray[k].isConnected)
        {
            strcat(msg2, peers_class->knownPeersArray[k].hostname.toStdString().c_str());
            strcat(msg2, "@");
            strcat(msg2, peers_class->knownPeersArray[k].ipAddress);
            strcat(msg2, ":");
        }
    }
    messClient->send_msg(i, msg2, localHostname, type);
}
*/
//Condense the 2 following into one, unsure of how to make the disconnect reconnect feature vary depending on bool
void MessengerWindow::addItalicsOnItem(int i)
{
    if( !( messListWidget->item(i)->font().italic() ) )
    {
        this->printToTextBrowser(peers_class->knownPeersArray[i].hostname + " on " + QString::fromUtf8(peers_class->knownPeersArray[i].ipAddress) + " disconnected", i, false);
        QFont mfont = messListWidget->item(i)->font();
        mfont.setItalic(true);
        messListWidget->item(i)->setFont(mfont);
    }
}
void MessengerWindow::removeItalicsOnItem(int i)
{
    if(messListWidget->item(i)->font().italic())
    {
        this->printToTextBrowser(peers_class->knownPeersArray[i].hostname + " on " + QString::fromUtf8(peers_class->knownPeersArray[i].ipAddress) + " reconnected", i, false);
        QFont mfont = messListWidget->item(i)->font();
        mfont.setItalic(false);
        messListWidget->item(i)->setFont(mfont);
    }
}

/**
 * @brief 				Stops more than 1000 characters being entered into the QTextEdit object as that is as many as we can send
 */
void MessengerWindow::textEditChanged()
{
    if(messTextEdit->toPlainText().length() > 1000)
    {
        int diff = messTextEdit->toPlainText().length() - 1000;
        QString temp = messTextEdit->toPlainText();
        temp.chop(diff);
        messTextEdit->setText(temp);
        QTextCursor cursor(messTextEdit->textCursor());
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        messTextEdit->setTextCursor(cursor);
    }
}

/**
 * @brief 				Signal for out system tray object upon being clicked.  Maximizes the window and brings it into focus
 * @param reason		Qt data type, only care about Trigger
 */
void MessengerWindow::showWindow(QSystemTrayIcon::ActivationReason reason)
{
    if( ( ( reason == QSystemTrayIcon::DoubleClick ) | ( reason == QSystemTrayIcon::Trigger ) ) && ! ( this->isVisible() ) )
    {
        this->setWindowState(Qt::WindowMaximized);
        this->show();
        this->setWindowState(Qt::WindowActive);
    }
    return;
}

/**
 * @brief 				Minimize to tray
 * @param event			QT data type, only care about WindowStateChange.  Pass the rest along
 */
void MessengerWindow::changeEvent(QEvent *event)
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

/**
 * @brief 				QListWidgetItems in the QListWidget change color when a message has been recieved and not viewed
 * 						This changes them back to default color and resets their text
 * @param item			item to be unalerted
 */
void MessengerWindow::removeMessagePendingStatus(QListWidgetItem* item)
{
    QString temp;

    if(messListWidget->row(item) == messListWidget->count()-1)
    {
        globalChatAlerted = false;
    }
    else
    {
        peers_class->knownPeersArray[messListWidget->row(item)].messagePending = false;
    }
    temp = item->text();
    temp.remove(0, 3);
    temp.remove(temp.length()-3, 3);
    item->setText(temp);

    return;
}

/**
 * @brief 				Slot for the QListWidget when the selected item is changed.  We need to change the text in the QTextBrowser, unalert the new item
 * 						and set the scrollbar to be at the bottom of the text browser
 * @param item1			New selected item
 * @param item2			Old selected item.  We do not care about item2 however these are the paramaters of the SIGNAL QListWidget emits
 */
void MessengerWindow::currentItemChanged(QListWidgetItem *item1, QListWidgetItem *item2)
{
    int index1 = messListWidget->row(item1);
    if(index1 == messListWidget->count() - 1)
    {
        messTextBrowser->setText(globalChat);
    }
    else
    {
        messTextBrowser->setText(peers_class->knownPeersArray[index1].textBox);
    }
    if(item1->background() == Qt::red)
    {
        this->changeListColor(index1, 0);
        this->removeMessagePendingStatus(item1);
    }
    QScrollBar *sb = this->messTextBrowser->verticalScrollBar();
    sb->setValue(sb->maximum());
    return;
}
/*
//This is a SLOT for a SIGNAL that is no longer emitted, keeping only for reference purposes
void MessengerWindow::potentialReconnect(QString ipaddr)
{
    for(int i = 0; i < numberOfValidPeers; i++)
    {
        if( ( (QString::fromUtf8(peers_class->knownPeersArray[i].ipAddress)).compare(ipaddr) == 0 ) )
        {
            if(messListWidget->item(i)->font().italic())
            {
                this->printToTextBrowser(peers_class->knownPeersArray[i].hostname + " on " + ipaddr + " reconnected", i, false);
                QFont mfont = messListWidget->item(i)->font();
                mfont.setItalic(false);
                messListWidget->item(i)->setFont(mfont);
            }
            return;
        }
    }
}
*/
/**
 * @brief				Slot for a SIGNAL peerQuit from mess_serv.  Signals that a socket has closed its connection
 * 						We change the font in the QListWidget to be italicized and display a notification in the corresponding
 * 						textbox.
 * @param s				Socket descriptor of peer that has disconnected
 */
/*
void MessengerWindow::peerQuit(int s)
{
    for(int i = 0; i < numberOfValidPeers; i++)
    {
        if(peers_class->knownPeersArray[i].socketDescriptor == s)
        {
            peers_class->knownPeersArray[i].isConnected = 0;
            if( !( messListWidget->item(i)->font().italic() ) )
            {
                QFont mfont = messListWidget->item(i)->font();
                mfont.setItalic(true);
                messListWidget->item(i)->setFont(mfont);
                //m_listwidget->item(i)->font().setItalic(true);
                peers_class->knownPeersArray[i].socketisValid = 0;
                this->printToTextBrowser(peers_class->knownPeersArray[i].hostname + " on " + QString::fromUtf8(peers_class->knownPeersArray[i].ipAddress) + " disconnected", i, false);
                //this->assignSocket(&peers_class->knownPeersArray[i]);
            }
        }
    }
}

/**
 * @brief 				Called when the listener accepts a new connection.  We need to add his info a struct in peers_class
 * 						The new connection should already have info of the corresponding peer in the peers_class, we will update it however
 * 						The only time this would not happen, was if a computer that we had no knowledge of somehow had knowledge of us and connected to us
 * 						We do not know his hostname but will ask him for it in the listpeers function.  Temporarily we will set his hostname to be his IP address
 * @param s				Socket of new peer
 * @param ipaddr		Ip address of new peer
 */
/*
void MessengerWindow::new_client(int s, QString ipaddr)
{
    for(int i = 0; i < numberOfValidPeers; i++)
    {
        if(strcmp(peers_class->knownPeersArray[i].ipAddress, ipaddr.toStdString().c_str()) == 0)
        {
            peers_class->knownPeersArray[i].socketDescriptor = s;
            //this->assignSocket(&(peers_class->peers[i]));
            peers_class->knownPeersArray[i].isConnected = true;
            if(messListWidget->item(i)->font().italic())
            {
                this->printToTextBrowser(peers_class->knownPeersArray[i].hostname + " on " + ipaddr + " reconnected", i, false);
                QFont mfont = messListWidget->item(i)->font();
                mfont.setItalic(false);
                messListWidget->item(i)->setFont(mfont);
            }
            //qDebug() << "hi";
            return;
        }
    }
    //If we got here it means this new peer is not in the list, where he came from we'll never know.
    //But actually, when this happens we need to get his hostname.  Temporarily we will make his hostname
    //his ip address but will ask him for his name later on.
    listpeers(ipaddr, ipaddr, true, s);

}
*/
/**
 * @brief 				The Quit Debug button was clicked
 */
void MessengerWindow::quitButtonClicked()
{
    this->close();
}

/**
 * @brief 				A connect peer has sent us his hostname after we asked him too.  Normally this is only done if we used his IP address as
 * 						his hostname.  It is also possible another computer had a conflicting hostname for the same ip address as us.
 * @param hname			The correct hostname for an IP address
 * @param ipaddr		The IP address to assign the hostname too
 */
/*
void MessengerWindow::setPeerHostname(QString hname, QString ipaddr)
{
    for(int i = 0; i < numberOfValidPeers; i++)
    {
        if(ipaddr.compare(peers_class->knownPeersArray[i].ipAddress) == 0)
        {
            peers_class->knownPeersArray[i].hostname = hname;
            updateListWidget();
            return;
        }
    }

}
*/
/*
void MessengerWindow::listpeers(QString hname, QString ipaddr)
{
    this->listpeers(hname, ipaddr, false, 0);
}

/**
 * @brief				This is the function called when mess_discover recieves a udp packet starting with "/name:"
 * 						here we check to see if the ip of the selected host is already in the peers array and if not
 * 						add it.  peers is then sorted and displayed to the gui.  QStrings are used for ease of
 * 						passing through the QT signal and slots mechanism.
 * @param hname			Hostname of peer to compare to existing hostnames
 * @param ipaddr		IP address of peer to compare to existing IP addresses
 */
/*
void MessengerWindow::listpeers(QString hname, QString ipaddr, bool test, int s)
{
    QByteArray ba = ipaddr.toLocal8Bit();
    const char *ipstr = ba.data();
    int i = 0;
    for( ; ( peers_class->knownPeersArray[i].isValid ); i++ )
    {
        if( (ipaddr.compare(peers_class->knownPeersArray[i].ipAddress) ) == 0 )
        {
            return;
        }
    }
    peers_class->knownPeersArray[i].hostname = hname;
    peers_class->knownPeersArray[i].isValid = true;
    strcpy(peers_class->knownPeersArray[i].ipAddress, ipstr);
    numberOfValidPeers++;
    if( !test )
    {
        //assignSocket(&(peers_class->knownPeersArray[i]));
        if(messClient->c_connect(peers_class->knownPeersArray[i].socketDescriptor, peers_class->knownPeersArray[i].ipAddress) >= 0)
        {
            peers_class->knownPeersArray[i].isConnected = true;
            messServer->update_fds(peers_class->knownPeersArray[i].socketDescriptor);
            if(strcmp(localHostname, peers_class->knownPeersArray[i].hostname.toStdString().c_str()))
            {
                messClient->send_msg(peers_class->knownPeersArray[i].socketDescriptor, "", "", "/request");
            }
        }
    }
    else
    {
        peers_class->knownPeersArray[i].socketDescriptor = s;
        peers_class->knownPeersArray[i].isConnected = true;
        messClient->send_msg(peers_class->knownPeersArray[i].socketDescriptor, "", "", "/request");
    }

    qDebug() << "hostname: " << peers_class->knownPeersArray[i].hostname << " @ ip:" << QString::fromUtf8(peers_class->knownPeersArray[i].ipAddress);

    if(!(hname.compare(ipaddr)))
    {
        struct sockaddr_storage addr;
        socklen_t socklen = sizeof(addr);
        char ipstr2[INET6_ADDRSTRLEN];
        char service[20];

        getpeername(peers_class->knownPeersArray[i].socketDescriptor, (struct sockaddr*)&addr, &socklen);
        //struct sockaddr_in *temp = (struct sockaddr_in *)&addr;
        //inet_ntop(AF_INET, &temp->sin_addr, ipstr2, sizeof ipstr2);
        getnameinfo((struct sockaddr*)&addr, socklen, ipstr2, sizeof(ipstr2), service, sizeof(service), NI_NUMERICHOST);
        qDebug() << "need name, sending namerequest to" << ipaddr;
        int status = messClient->send_msg(peers_class->knownPeersArray[i].socketDescriptor, "", "", "/namerequest");
        qDebug() << status;
    }
    updateListWidget();

    return;
}

/**
 * @brief				This calls the sort function in the peers_class and also updates the QListWidget to
 * 						correctly display the different peers
 */
void MessengerWindow::updateListWidget(int num)
{
    if(num >= 1)
    {
        qDebug() << messListWidget->count();
        if( ( messListWidget->count() - 2 ) < num)
        {
            messListWidget->insertItem(0, "");
            globalChatIndex++;
        }
        else if( ( messListWidget->count() - 2 ) > num)
        {
            messListWidget->removeItemWidget(messListWidget->item(num));
            globalChatIndex--;
        }
        for(int i = 0; i < ( messListWidget->count() - 2 ); i++)
        {
            messListWidget->item(i)->setText(peers_class->knownPeersArray[i].hostname);
            if(peers_class->knownPeersArray[i].messagePending)
            {
                this->changeListColor(i, 1);
                messListWidget->item(i)->setText(" * " + messListWidget->item(i)->text() + " * ");
            }
        }
    }
    else
    {
        messListWidget->insertItem(0, (peers_class->knownPeersArray[0].hostname));
        globalChatIndex++;
    }
    return;
}

/**
 * @brief				Garbage collection, called upon sending a close signal to the process.  (X button, Quit Debug button, SIGTERM in linux)
 * @param event
 */
void MessengerWindow::closeEvent(QCloseEvent *event)
{
    for(int i = 0; i < peers_class->numberOfValidPeers; i++)
    {
#ifdef __unix__
        ::close(peers_class->knownPeersArray[i].socketDescriptor);
#endif
#ifdef _WIN32
        ::closesocket(peers_class->peers[i].socketdescriptor);
#endif
    }
    if(messServer != 0 && messServer->isRunning())
    {
        messServer->requestInterruption();
        messServer->quit();
        messServer->wait();
    }
    messClientThread->quit();
    messClientThread->wait();
    delete messClient;
    delete messSystemTrayIcon;
    messSystemTray->hide();
    //messSystemTray->deleteLater();
    //delete now;
    delete peers_class;
    //messSystemTray->hide();
    event->accept();
}

/* Assign sockets to peers added to the peers array
void MessengerWindow::assignSocket(struct peerDetails *p)
{
    p->socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if(p->socketDescriptor < 0)
    perror("socket:");
    else
    p->socketisValid = 1;
    return;
}*/

/*Send the "/discover" message to the local networks broadcast address
 * this will only send to computers in the 255.255.255.0 subnet*/
void MessengerWindow::udpSend(const char* msg)
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
/**
 * @brief 				Send a message to every connected peer
 * @param msg			The message to send
 */
void MessengerWindow::globalSend(QString msg)
{
    for(int i = 0; i < peers_class->numberOfValidPeers; i++)
    {
        if(peers_class->knownPeersArray[i].isConnected)
        {
            messClient->send_msg(peers_class->knownPeersArray[i].socketDescriptor, msg.toStdString().c_str(), localHostname, "/global");
        }
    }
    messTextEdit->setText("");
    return;
}

/* Send message button function.  Calls m_client to both connect and send a message to the provided ip_addr*/
void MessengerWindow::sendButtonClicked()
{
    if(messListWidget->selectedItems().count() == 0)
    {
        this->printToTextBrowser("Choose a computer to message from the selection pane on the right", -1, false);
        return;
    }
    QString str = messTextEdit->toPlainText();
    const char* c_str = str.toStdString().c_str();

    int index = messListWidget->currentRow();
    if( ( index == globalChatIndex ) && ( strcmp(c_str, "") != 0) )
    {
        globalSend(str);
        return;
    }

    int s_socket = peers_class->knownPeersArray[index].socketDescriptor;

    if(peers_class->knownPeersArray[index].socketisValid)
    {
        if(!(peers_class->knownPeersArray[index].isConnected))
        {
            if(messClient->c_connect(s_socket, peers_class->knownPeersArray[index].ipAddress) < 0)
            {
                this->printToTextBrowser("Could not connect to " + peers_class->knownPeersArray[index].hostname + " | on socket " + QString::number(peers_class->knownPeersArray[index].socketDescriptor), index, false);
                return;
            }
            peers_class->knownPeersArray[index].isConnected = true;
            messServer->update_fds(s_socket);
            messClient->send_msg(peers_class->knownPeersArray[index].socketDescriptor, "", "", "/request");
            //this->sendIps(peers_class->peers[index].socketdescriptor);

        }
        if(strcmp(c_str, "") != 0)
        {
            if((messClient->send_msg(s_socket, c_str, localHostname, "/msg")) == -5)
            {
                this->printToTextBrowser("Peer has closed connection, send failed", index, false);
                return;
            }
            else{
                this->printToTextBrowser(messLineEdit->text() + ": " + messTextEdit->toPlainText(), index, true);
            }
            messTextEdit->setText("");
        }
    }
    else
    {
        this->printToTextBrowser("Peer Disconnected", index, false);
        //this->assignSocket(&peers_class->knownPeersArray[index]);
    }
    return;
}
void MessengerWindow::changeListColor(int row, int style)
{
    QBrush back = (messListWidget->item(row)->background());
    if(style == 1)
    {
        messListWidget->item(row)->setBackground(Qt::red);
    }
    if(style == 0)
    {
        messListWidget->item(row)->setBackground(QGuiApplication::palette().base());
    }
    return;
}
/**
 * @brief 				Called when we want to modify the focus or visibility of our program (ie. we get a message)
 */
void MessengerWindow::focusWindow()
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

/**
 * @brief 					This will print new messages to the appropriate QString and call the focus function if its necessary
 * @param str				Message to print
 * @param peerindex			index of the peers_class->peers array for which the message was meant for
 * @param message			Bool for whether this message that is being printed should alert the listwidgetitem, play sound, and focus the application
 */
void MessengerWindow::printToTextBrowser(const QString str, int peerindex, bool message)
{
    QString strnew;
    if(message)
    {
        char time_str[12];
        messTime = time(0);
        currentTime = localtime(&messTime);
        strftime(time_str, 12, "(%H:%M:%S) ", currentTime);
        strnew = QString::fromUtf8(time_str) + str;
    }
    else
    {
        strnew = str;
    }
    if(peerindex < 0)
    {
        messTextBrowser->append(strnew);
        return;
    }
    if(peerindex == globalChatIndex)
    {
        globalChat.append(strnew + "\n");
    }
    else
    {
        peers_class->knownPeersArray[peerindex].textBox.append(strnew + "\n");
    }
    if(peerindex == messListWidget->currentRow())
    {
        messTextBrowser->append(strnew);
        qApp->alert(this, 0);
    }
    else if(message)
    {
        this->changeListColor(peerindex, 1);
        if( !( peers_class->knownPeersArray[peerindex].messagePending ) && ( peerindex != messListWidget->count()-1 ) )
        {
            messListWidget->item(peerindex)->setText(" * " + messListWidget->item(peerindex)->text() + " * ");
            peers_class->knownPeersArray[peerindex].messagePending = true;
        }
        else if(peerindex == messListWidget->count()-1 && !(globalChatAlerted))
        {
            messListWidget->item(peerindex)->setText(" * " + messListWidget->item(peerindex)->text() + " * ");
            globalChatAlerted = true;
        }
    }
    return;
}

/**
 * @brief 					This is the slot for a SIGNAL from the mess_serv class when it has recieved a message that it needs to print
 * 							basically this only finds which peer the message is meant for or it if it was mean to be global
 * @param str				The message to print
 * @param ipstr				The ip address of the peer this message has come from
 * @param global			Whether this message should be displayed in the global textbox
 */
void MessengerWindow::printToTextBrowserServerSlot(const QString str, const QString ipstr, bool global)
{
    if(global)
    {
        this->focusWindow();
        this->printToTextBrowser(str, messListWidget->count()-1, true);
        return;
    }
    for(int i = 0; i < peers_class->numberOfValidPeers; i++)
    {
        if(ipstr.compare(QString::fromUtf8(peers_class->knownPeersArray[i].ipAddress)) == 0)
        {
            this->focusWindow();
            this->printToTextBrowser(str, i, true);
            return;
        }
    }
    qDebug() << "finding socket in peers failed Window::prints.  Slot from mess_serv";
    return;
}
