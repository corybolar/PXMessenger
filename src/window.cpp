#include <window.h>

MessengerWindow::MessengerWindow()
{
    setFixedSize(700,550);

    gethostname(localHostname, sizeof localHostname);

    peerWorker = new PeerWorkerClass(this);

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

    emit sendUdp("/discover" + QString::fromUtf8(localHostname));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerout()));
    timer->start(5000);

    QUuid ourUUID = QUuid::createUuid();
    ourUUIDString = ourUUID.toString().mid(1,36).toUpper();
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

    messDebugButton = new QPushButton("Debug", this);
    messDebugButton->setGeometry(160, 490, 80, 30);
    connect(messDebugButton, SIGNAL(clicked()), this, SLOT (debugButtonClicked()));
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
    QObject::connect(messClientThread, SIGNAL(finished()), messClientThread, SLOT(deleteLater()));
    QObject::connect(messClient, SIGNAL (resultOfConnectionAttempt(int, bool)), peerWorker, SLOT(resultOfConnectionAttempt(int,bool)));
    QObject::connect(messClient, SIGNAL (resultOfTCPSend(int, int, QString, bool)), peerWorker, SLOT(resultOfTCPSend(int,int,QString,bool)));
    QObject::connect(this, SIGNAL (sendMsg(int, QString, QString, QString, QString)), messClient, SLOT(sendMsgSlot(int, QString, QString, QString, QString)));
    QObject::connect(this, SIGNAL (connectToPeer(int, QString)), messClient, SLOT(connectToPeerSlot(int, QString)));
    QObject::connect(this, SIGNAL (sendUdp(QString)), messClient, SLOT(udpSendSlot(QString)));
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
    QObject::connect(peerWorker, SIGNAL (sendMsg(int, QString, QString, QString, QString)), messClient, SLOT(sendMsgSlot(int, QString, QString, QString, QString)));
    QObject::connect(peerWorker, SIGNAL (connectToPeer(int, QString)), messClient, SLOT(connectToPeerSlot(int, QString)));
    QObject::connect(peerWorker, SIGNAL (updateMessServFDS(int)), messServer, SLOT (updateMessServFDSSlot(int)));
    QObject::connect(peerWorker, SIGNAL (printToTextBrowser(QString, int, bool)), this, SLOT (printToTextBrowser(QString, int, bool)));
    QObject::connect(peerWorker, SIGNAL (setItalicsOnItem(int, bool)), this, SLOT (setItalicsOnItem(int, bool)));
    QObject::connect(peerWorker, SIGNAL (updateListWidget(int)), this, SLOT (updateListWidget(int)));
}
void MessengerWindow::createMessServ()
{
    messServer = new MessengerServer(this);
    QObject::connect(messServer, SIGNAL (mess_rec(const QString, const QString, QString, bool)), this, SLOT (printToTextBrowserServerSlot(const QString, const QString, QString, bool)) );
    QObject::connect(messServer, SIGNAL (finished()), messServer, SLOT (deleteLater()));
    QObject::connect(messServer, SIGNAL (sendName(int)), messClient, SLOT (sendNameSlot(int)));
    QObject::connect(messServer, SIGNAL (new_client(int, const QString)), peerWorker, SLOT (newTcpConnection(int, const QString)));
    QObject::connect(messServer, SIGNAL (peerQuit(int)), peerWorker, SLOT (peerQuit(int)));
    QObject::connect(messServer, SIGNAL (mess_peers(QString, QString)), peerWorker, SLOT (listpeers(QString, QString)));
    QObject::connect(messServer, SIGNAL (sendIps(int)), peerWorker, SLOT (sendIps(int)));
    QObject::connect(messServer, SIGNAL (hostnameCheck(QString)), peerWorker, SLOT (hostnameCheck(QString)));
    QObject::connect(messServer, SIGNAL (setPeerHostname(QString, QString)), peerWorker, SLOT (setPeerHostname(QString, QString)));
    messServer->start();
}
void MessengerWindow::createMessTime()
{
    messTime = time(0);
    currentTime = localtime( &messTime );
}
void MessengerWindow::debugButtonClicked()
{
    emit sendUdp("/discover" + QString::fromUtf8(localHostname));
}

void MessengerWindow::timerout()
{
    if(peerWorker->numberOfValidPeers <= 1)
    {
        emit sendUdp("/discover" + QString::fromUtf8(localHostname));
        timer->start(5000);
    }
}

//Condense the 2 following into one, unsure of how to make the disconnect reconnect feature vary depending on bool
void MessengerWindow::setItalicsOnItem(int i, bool italics)
{
    if(messListWidget->item(i)->font().italic() == italics)
        return;
    QString changeInConnection;
    if(italics)
        changeInConnection = " disconnected";
    else
        changeInConnection = " reconnected";
    this->printToTextBrowser(peerWorker->knownPeersArray[i].hostname + " on " +peerWorker->knownPeersArray[i].ipAddress + changeInConnection, i, false);
    QFont mfont = messListWidget->item(i)->font();
    mfont.setItalic(italics);
    messListWidget->item(i)->setFont(mfont);
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
        peerWorker->knownPeersArray[messListWidget->row(item)].messagePending = false;
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
        messTextBrowser->setText(peerWorker->knownPeersArray[index1].textBox);
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
/**
 * @brief 				The Quit Debug button was clicked
 */
void MessengerWindow::quitButtonClicked()
{
    this->close();
}
void MessengerWindow::updateListWidget(int num)
{
    messListWidget->setUpdatesEnabled(false);
    if(num >= 1)
    {
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
            messListWidget->item(i)->setText(peerWorker->knownPeersArray[i].hostname);
            if(peerWorker->knownPeersArray[i].messagePending)
            {
                this->changeListColor(i, 1);
                messListWidget->item(i)->setText(" * " + messListWidget->item(i)->text() + " * ");
            }
        }
    }
    else
    {
        messListWidget->insertItem(0, (peerWorker->knownPeersArray[0].hostname));
        globalChatIndex++;
    }
    messListWidget->setUpdatesEnabled(true);
        QUuid temp3 = QUuid::createUuid();
        QString temp4 = temp3.toString();
        messListWidget->item(0)->setData(Qt::UserRole, temp4);
    MessListWidgetItem temp;
    temp.setText("hi");
    QString temp1 = messListWidget->item(0)->data(Qt::UserRole).toString();
    QUuid temp2 = temp1;
    qDebug() << temp1;
    return;
}
/**
 * @brief				Garbage collection, called upon sending a close signal to the process.  (X button, Quit Debug button, SIGTERM in linux)
 * @param event
 */
void MessengerWindow::closeEvent(QCloseEvent *event)
{
    for(int i = 0; i < peerWorker->numberOfValidPeers; i++)
    {
#ifdef __unix__
        ::close(peerWorker->knownPeersArray[i].socketDescriptor);
#endif
#ifdef _WIN32
        ::closesocket(peers_class->knownPeersArray[i].socketDescriptor);
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
    for(int i = 0; i < peerWorker->numberOfValidPeers; i++)
    {
        if(peerWorker->knownPeersArray[i].isConnected)
        {
            emit sendMsg(peerWorker->knownPeersArray[i].socketDescriptor, msg, localHostname, "/global", "");
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
        messTextEdit->setText("");
        return;
    }

    int s_socket = peerWorker->knownPeersArray[index].socketDescriptor;

    if(strcmp(c_str, "") != 0)
    {
        if(!(peerWorker->knownPeersArray[index].isConnected))
        {
            emit connectToPeer(peerWorker->knownPeersArray[index].socketDescriptor, peerWorker->knownPeersArray[index].ipAddress);
        }
        emit sendMsg(s_socket, str, localHostname, "/msg", ourUUIDString);
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
        peerWorker->knownPeersArray[peerindex].textBox.append(strnew + "\n");
    }
    if(peerindex == messListWidget->currentRow())
    {
        messTextBrowser->append(strnew);
        qApp->alert(this, 0);
    }
    else if(message)
    {
        this->changeListColor(peerindex, 1);
        if( !( peerWorker->knownPeersArray[peerindex].messagePending ) && ( peerindex != messListWidget->count()-1 ) )
        {
            messListWidget->item(peerindex)->setText(" * " + messListWidget->item(peerindex)->text() + " * ");
            peerWorker->knownPeersArray[peerindex].messagePending = true;
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
void MessengerWindow::printToTextBrowserServerSlot(const QString str, const QString ipstr, QString uuid, bool global)
{
    QUuid temp = QUuid(uuid);
    qDebug() << temp.toString();
    if(global)
    {
        this->focusWindow();
        this->printToTextBrowser(str, messListWidget->count()-1, true);
        return;
    }
    for(int i = 0; i < peerWorker->numberOfValidPeers; i++)
    {
        if(ipstr == peerWorker->knownPeersArray[i].ipAddress)
        {
            this->focusWindow();
            this->printToTextBrowser(str, i, true);
            return;
        }
    }
    qDebug() << "finding socket in peers failed Window::prints.  Slot from mess_serv";
    return;
}
