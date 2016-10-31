#include <window.h>

MessengerWindow::MessengerWindow(QUuid uuid, int uuidNum)
{
    ourUUIDString = uuid.toString();
    setFixedSize(900,600);

    char computerHostname[128];
    gethostname(computerHostname, sizeof computerHostname);

    ourListenerPort = ourListenerPort.number(ourListenerPort.toInt() + uuidNum);
    qDebug() << "Our port to listen on:" << ourListenerPort;

#ifdef __unix__
    struct passwd *user;
    user = getpwuid(getuid());
    strcat(localHostname, user->pw_name);
    strcat(localHostname, "@");
    strcat(localHostname, computerHostname);
    if(uuidNum > 0)
    {
        char temp[3];
        sprintf(temp, "%d", uuidNum);
        strcat(localHostname, temp);
    }
#elif _WIN32
    char user[UNLEN+1];
    DWORD user_size = UNLEN+1;
    if(GetUserName((TCHAR*)user, &user_size))
    {
        strcat(localHostname, user);
        strcat(localHostname, "@");
    }
    strcat(localHostname, computerHostname);
#endif

    peerWorker = new PeerWorkerClass(this, localHostname, ourUUIDString, ourListenerPort);

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

    connectPeerClassSignalsAndSlots();

    //emit sendUdp("/discover" + QString::fromUtf8(localHostname));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerout()));
    timer->start(250);
    qDebug() << "Our UUID:" << ourUUIDString;
}
void MessengerWindow::createTextEdit()
{
    messTextEdit = new MessengerTextEdit(this);
    messTextEdit->setGeometry(10, 380, 620, 120);
}
void MessengerWindow::createTextBrowser()
{
    messTextBrowser= new QTextBrowser(this);
    messTextBrowser->setGeometry(10, 10, 620, 350);
    messTextBrowser->setText("Use the selection pane on the right to message a FRIEND!");
}
void MessengerWindow::createLineEdit()
{
    messLineEdit = new QLineEdit(this);
    messLineEdit->setGeometry(640, 10, 200, 30);
    messLineEdit->setText((QString::fromUtf8(localHostname)));
}
void MessengerWindow::createButtons()
{
    messSendButton = new QPushButton("Send", this);
    messSendButton->setGeometry(260,510,80,30);

    messQuitButton = new QPushButton("Quit", this);
    messQuitButton->setGeometry(260, 550, 80, 30);

    messDebugButton = new QPushButton("Debug", this);
    messDebugButton->setGeometry(460, 510, 80, 30);
    connect(messDebugButton, SIGNAL(clicked()), this, SLOT (debugButtonClicked()));
    //messDebugButton->hide();
}
void MessengerWindow::createListWidget()
{
    globalChatUuid = QUuid::createUuid();

    messListWidget = new QListWidget(this);
    messListWidget->setGeometry(640, 60, 200, 300);
    messListWidget->insertItem(0, "--------------------");
    messListWidget->insertItem(1, "Global Chat");
    messListWidget->item(1)->setData(Qt::UserRole, globalChatUuid);
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
    messClient->setLocalHostname((localHostname));
    messClient->setlocalUUID(ourUUIDString);
    QObject::connect(messClientThread, SIGNAL(finished()), messClientThread, SLOT(deleteLater()));
    QObject::connect(messClient, SIGNAL (resultOfConnectionAttempt(int, bool)), peerWorker, SLOT(resultOfConnectionAttempt(int,bool)));
    QObject::connect(messClient, SIGNAL (resultOfTCPSend(int, QString, QString, bool)), peerWorker, SLOT(resultOfTCPSend(int,QString,QString,bool)));
    QObject::connect(this, SIGNAL (sendMsg(int, QString, QString, QString, QUuid, QString)), messClient, SLOT(sendMsgSlot(int, QString, QString, QString, QUuid, QString)));
    QObject::connect(this, SIGNAL (connectToPeer(int, QString, QString)), messClient, SLOT(connectToPeerSlot(int, QString, QString)));
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
void MessengerWindow::connectPeerClassSignalsAndSlots()
{
    QObject::connect(peerWorker, SIGNAL (sendMsg(int, QString, QString, QString, QUuid, QString)), messClient, SLOT(sendMsgSlot(int, QString, QString, QString, QUuid, QString)));
    QObject::connect(peerWorker, SIGNAL (connectToPeer(int, QString, QString)), messClient, SLOT(connectToPeerSlot(int, QString, QString)));
    QObject::connect(peerWorker, SIGNAL (updateMessServFDS(int)), messServer, SLOT (updateMessServFDSSlot(int)));
    QObject::connect(peerWorker, SIGNAL (printToTextBrowser(QString, QUuid, bool)), this, SLOT (printToTextBrowser(QString, QUuid, bool)));
    QObject::connect(peerWorker, SIGNAL (setItalicsOnItem(QUuid, bool)), this, SLOT (setItalicsOnItem(QUuid, bool)));
    QObject::connect(peerWorker, SIGNAL (updateListWidget(QUuid)), this, SLOT (updateListWidget(QUuid)));
}
void MessengerWindow::createMessServ()
{
    messServer = new MessengerServer(this);
    messServer->setLocalHostname(QString::fromUtf8(localHostname));
    messServer->setListnerPortNumber(ourListenerPort);
    QObject::connect(messServer, SIGNAL (sendMsg(int, QString, QString, QString, QUuid, QString)), messClient, SLOT(sendMsgSlot(int, QString, QString, QString, QUuid, QString)));
    QObject::connect(messServer, SIGNAL (recievedUUIDForConnection(QString, QString, QString, int, QUuid)), peerWorker, SLOT(updatePeerDetailsHash(QString, QString, QString, int, QUuid)));
    QObject::connect(messServer, SIGNAL (messageRecieved(const QString, QUuid, bool)), this, SLOT (printToTextBrowserServerSlot(const QString, QUuid, bool)) );
    QObject::connect(messServer, SIGNAL (finished()), messServer, SLOT (deleteLater()));
    QObject::connect(messServer, SIGNAL (sendName(int, QString, QString)), messClient, SLOT (sendNameSlot(int, QString, QString)));
    QObject::connect(messServer, SIGNAL (newConnectionRecieved(int, QString)), peerWorker, SLOT (newTcpConnection(int, QString)));
    QObject::connect(messServer, SIGNAL (peerQuit(int)), peerWorker, SLOT (peerQuit(int)));
    QObject::connect(messServer, SIGNAL (updNameRecieved(QString, QString)), peerWorker, SLOT (attemptConnection(QString, QString)));
    QObject::connect(messServer, SIGNAL (sendIps(int)), peerWorker, SLOT (sendIps(int)));
    QObject::connect(messServer, SIGNAL (hostnameCheck(QString)), peerWorker, SLOT (hostnameCheck(QString)));
    QObject::connect(messServer, SIGNAL (setPeerHostname(QString, QUuid)), peerWorker, SLOT (setPeerHostname(QString, QUuid)));
    QObject::connect(messServer, SIGNAL (sendUdp(QString)), messClient, SLOT(udpSendSlot(QString)));
    messServer->start();
}
void MessengerWindow::createMessTime()
{
    messTime = time(0);
    currentTime = localtime( &messTime );
}
void MessengerWindow::debugButtonClicked()
{
    for(auto &itr : peerWorker->peerDetailsHash)
    {
        if(itr.hostname == "archlaptop" || itr.hostname == "Computer")
        {
            ::close(itr.socketDescriptor);
            FD_CLR(itr.socketDescriptor, &(messServer->master));
        }
    }
}
QString MessengerWindow::getFormattedTime()
{
    char time_str[12];
    messTime = time(0);
    currentTime = localtime(&messTime);
    strftime(time_str, 12, "(%H:%M:%S) ", currentTime);
    return time_str;
}

void MessengerWindow::timerout()
{
    //emit sendUdp("/discover" + QString::fromUtf8(localHostname));
    timer->stop();
}

//Condense the 2 following into one, unsure of how to make the disconnect reconnect feature vary depending on bool
void MessengerWindow::setItalicsOnItem(QUuid uuid, bool italics)
{
    for(int i = 0; i < messListWidget->count(); i++)
    {
        if(messListWidget->item(i)->data(Qt::UserRole) == uuid)
        {
            QFont mfont = messListWidget->item(i)->font();
            mfont.setItalic(italics);
            messListWidget->item(i)->setFont(mfont);
            break;
        }
    }
    QString changeInConnection;
    if(italics)
        changeInConnection = " disconnected";
    else
        changeInConnection = " reconnected";
    this->printToTextBrowser(this->getFormattedTime() + peerWorker->peerDetailsHash[uuid].hostname + " on " +peerWorker->peerDetailsHash[uuid].ipAddress + changeInConnection, uuid, false);
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
        QUuid temp = (item->data(Qt::UserRole)).toString();
        peerWorker->peerDetailsHash[temp].messagePending = false;
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
    QUuid uuid1 = item1->data(Qt::UserRole).toString();
    if(index1 == messListWidget->count() - 1)
    {
        messTextBrowser->setText(globalChat);
    }
    else
    {
        messTextBrowser->setText(peerWorker->peerDetailsHash[uuid1].textBox);
    }
    if(item1->background() == Qt::red)
    {
        this->changeListColor(messListWidget->row(item1), 0);
        this->removeMessagePendingStatus(item1);
    }
    QScrollBar *sb = this->messTextBrowser->verticalScrollBar();
    sb->setValue(sb->maximum());
    return;
}
/**
 * @brief 				The Quit Debug button was clicked
 */
void MessengerWindow::quitButtonClicked()
{
    this->close();
}

void MessengerWindow::updateListWidget(QUuid uuid)
{
    messListWidget->setUpdatesEnabled(false);
    int count = messListWidget->count() - 1;
    if(count == 1)
    {
        messListWidget->insertItem(0, peerWorker->peerDetailsHash.value(uuid).hostname);
        messListWidget->item(0)->setData(Qt::UserRole, uuid);
        peerWorker->peerDetailsHash[uuid].listWidgetIndex = 0;

    }
    else
    {
        for(int i = 0; i < count; i++)
        {
            QUuid u = messListWidget->item(i)->data(Qt::UserRole).toString();
            QString str = messListWidget->item(i)->text();
            if((str.startsWith(" * ")))
                str = str.mid(3, str.length()-6);
            if(peerWorker->peerDetailsHash.value(uuid).hostname.compare(str) > 0)
            {
                messListWidget->insertItem(i, peerWorker->peerDetailsHash.value((uuid)).hostname);
                messListWidget->item(i)->setData(Qt::UserRole, uuid);
                peerWorker->peerDetailsHash[uuid].listWidgetIndex = i;
                peerWorker->peerDetailsHash[u].listWidgetIndex = i+1;
                break;
            }
            else if(peerWorker->peerDetailsHash.value(uuid).hostname.compare(str) < 0)
            {
                if(i == count)
                {
                    messListWidget->insertItem(i+1, peerWorker->peerDetailsHash.value(uuid).hostname);
                    messListWidget->item(i+1)->setData(Qt::UserRole, uuid);
                }
                else
                    continue;
            }
            else
            {
                qDebug() << "hostnames equal: MessengerWindow::updateListWidget";
            }
            /*
            else if(peerWorker->peerDetailsHash.value(uuid).hostname == str && uuid != u )
            {
                if(peerWorker->peerDetailsHash[uuid].messagePending)
                {
                    messListWidget->item(i)->setText(" * " + peerWorker->peerDetailsHash[uuid].hostname + " * ");
                    messListWidget->item(i)->setData(Qt::UserRole, uuid);
                }
                else
                {
                    messListWidget->item(i)->setText(peerWorker->peerDetailsHash[uuid].hostname);
                    messListWidget->item(i)->setData(Qt::UserRole, uuid);
                }
                break;
            }
            else if(uuid == u)
            {
                if(peerWorker->peerDetailsHash[uuid].messagePending)
                {
                    messListWidget->item(i)->setText(" * " + peerWorker->peerDetailsHash[uuid].hostname + " * ");
                }
                else
                {
                    messListWidget->item(i)->setText(peerWorker->peerDetailsHash[uuid].hostname);
                }
                break;
            }
            */
        }
    }
    messListWidget->setUpdatesEnabled(true);
    return;
}
/**
 * @brief				Garbage collection, called upon sending a close signal to the process.  (X button, Quit Debug button, SIGTERM in linux)
 * @param event
 */
void MessengerWindow::closeEvent(QCloseEvent *event)
{
    for(auto &itr : peerWorker->peerDetailsHash)
    {
#ifdef __unix__
        ::close(itr.socketDescriptor);
#endif
#ifdef _WIN32
        ::closesocket(itr.socketDescriptor);
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
    delete peerWorker;
    //messSystemTray->hide();
    event->accept();
}
/**
 * @brief 				Send a message to every connected peer
 * @param msg			The message to send
 */
void MessengerWindow::globalSend(QString msg)
{
    for(auto &itr : peerWorker->peerDetailsHash)
    {
        if(itr.isConnected)
        {
            emit sendMsg(itr.socketDescriptor, msg, localHostname, "/global", ourUUIDString, "");
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
        this->printToTextBrowser("Choose a computer to message from the selection pane on the right", "", false);
        return;
    }
    QString str = messTextEdit->toPlainText();

    int index = messListWidget->currentRow();
    QUuid uuidOfSelectedItem = messListWidget->item(index)->data(Qt::UserRole).toString();
    if( ( uuidOfSelectedItem == globalChatUuid) && !( str.isEmpty() ) )
    {
        globalSend(str);
        messTextEdit->setText("");
        return;
    }

    if(!(str.isEmpty()))
    {
        if(!(peerWorker->peerDetailsHash[uuidOfSelectedItem].isConnected))
        {
            int s = socket(AF_INET, SOCK_STREAM, 0);
            emit connectToPeer(s, peerWorker->peerDetailsHash[uuidOfSelectedItem].ipAddress, peerWorker->peerDetailsHash[uuidOfSelectedItem].portNumber);
        }
        emit sendMsg(peerWorker->peerDetailsHash[uuidOfSelectedItem].socketDescriptor, str, localHostname, "/msg", ourUUIDString, uuidOfSelectedItem.toString());
        messTextEdit->setText("");
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
void MessengerWindow::printToTextBrowser(QString str, QUuid uuid, bool message)
{
    QString strnew;
    if(message)
    {
        strnew = this->getFormattedTime() + str;
    }
    else
    {
        strnew = str;
    }

    if(uuid.isNull())
    {
        //messTextBrowser->append(strnew);
        return;
    }
    if(!(peerWorker->peerDetailsHash.contains(uuid)) && ( uuid != globalChatUuid ))
    {
        qDebug() << "Message from invalid uuid, rejection";
        return;
    }

    if(uuid == globalChatUuid)
    {
        globalChat.append(strnew + "\n");
    }
    else
    {
        peerWorker->peerDetailsHash[uuid].textBox.append(strnew + "\n");
    }

    if(messListWidget->currentItem() != NULL)
    {
        if(uuid == messListWidget->currentItem()->data(Qt::UserRole))
        {
            messTextBrowser->append(strnew);
            if(message)
                this->focusWindow();
            qApp->alert(this, 0);
        }
    }
    else if(message)
    {
        if(uuid == globalChatUuid)
        {
            this->changeListColor(messListWidget->count() - 1, 1);
            if(!globalChatAlerted)
            {
                messListWidget->item(messListWidget->count() - 1)->setText(" * " + messListWidget->item(messListWidget->count() - 1)->text() + " * ");
                globalChatAlerted = true;
            }
        }
        else if( !( peerWorker->peerDetailsHash[uuid].messagePending ) && ( messListWidget->currentRow() != messListWidget->count()-1 ) )
        {
            this->changeListColor(peerWorker->peerDetailsHash[uuid].listWidgetIndex, 1);
            messListWidget->item(peerWorker->peerDetailsHash[uuid].listWidgetIndex)->setText(" * " + messListWidget->item(peerWorker->peerDetailsHash[uuid].listWidgetIndex)->text() + " * ");
            peerWorker->peerDetailsHash[uuid].messagePending = true;
        }
        this->focusWindow();
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
void MessengerWindow::printToTextBrowserServerSlot(const QString str, QUuid uuid, bool global)
{
    if(global)
    {
        this->printToTextBrowser(str, globalChatUuid, true);
    }
    else
    {
        this->printToTextBrowser(str, uuid, true);
    }
    return;
}
