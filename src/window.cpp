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
    //timer->start(5000);

    QUuid ourUUID = QUuid::createUuid();
    ourUUIDString = ourUUID.toString();

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
    globalChatUuid = QUuid::createUuid();

    messListWidget = new QListWidget(this);
    messListWidget->setGeometry(410, 60, 200, 290);
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
    QObject::connect(messClientThread, SIGNAL(finished()), messClientThread, SLOT(deleteLater()));
    QObject::connect(messClient, SIGNAL (resultOfConnectionAttempt(int, bool)), peerWorker, SLOT(resultOfConnectionAttempt(int,bool)));
    QObject::connect(messClient, SIGNAL (resultOfTCPSend(int, QString, QString, bool)), peerWorker, SLOT(resultOfTCPSend(int,QString,QString,bool)));
    QObject::connect(this, SIGNAL (sendMsg(int, QString, QString, QString, QUuid)), messClient, SLOT(sendMsgSlot(int, QString, QString, QString, QUuid)));
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
    QObject::connect(peerWorker, SIGNAL (sendMsg(int, QString, QString, QString, QUuid)), messClient, SLOT(sendMsgSlot(int, QString, QString, QString, QUuid)));
    QObject::connect(peerWorker, SIGNAL (connectToPeer(int, QString)), messClient, SLOT(connectToPeerSlot(int, QString)));
    QObject::connect(peerWorker, SIGNAL (updateMessServFDS(int)), messServer, SLOT (updateMessServFDSSlot(int)));
    QObject::connect(peerWorker, SIGNAL (printToTextBrowser(QString, QUuid, bool)), this, SLOT (printToTextBrowser(QString, QUuid, bool)));
    QObject::connect(peerWorker, SIGNAL (setItalicsOnItem(QUuid, bool)), this, SLOT (setItalicsOnItem(QUuid, bool)));
    QObject::connect(peerWorker, SIGNAL (updateListWidget(int)), this, SLOT (updateListWidget(int)));
}
void MessengerWindow::createMessServ()
{
    messServer = new MessengerServer(this, localHostname);
    QObject::connect(messServer, SIGNAL (sendMsg(int, QString, QString, QString, QUuid)), messClient, SLOT(sendMsgSlot(int, QString, QString, QString, QUuid)));
    QObject::connect(messServer, SIGNAL (recievedUUIDForConnection(QString, QString, bool, int, QUuid)), peerWorker, SLOT(updatePeerDetailsHash(QString, QString, bool, int, QUuid)));
    QObject::connect(messServer, SIGNAL (messageRecieved(const QString, const QString, QUuid, bool)), this, SLOT (printToTextBrowserServerSlot(const QString, const QString, QUuid, bool)) );
    QObject::connect(messServer, SIGNAL (finished()), messServer, SLOT (deleteLater()));
    QObject::connect(messServer, SIGNAL (sendName(int, QString)), messClient, SLOT (sendNameSlot(int, QString)));
    QObject::connect(messServer, SIGNAL (newConnectionRecieved(int, QString, QUuid)), peerWorker, SLOT (newTcpConnection(int, QString, QUuid)));
    QObject::connect(messServer, SIGNAL (peerQuit(int)), peerWorker, SLOT (peerQuit(int)));
    QObject::connect(messServer, SIGNAL (updNameRecieved(QString, QString)), peerWorker, SLOT (updatePeerDetailsHash(QString, QString)));
    QObject::connect(messServer, SIGNAL (sendIps(int)), peerWorker, SLOT (sendIps(int)));
    QObject::connect(messServer, SIGNAL (hostnameCheck(QString)), peerWorker, SLOT (hostnameCheck(QString)));
    QObject::connect(messServer, SIGNAL (setPeerHostname(QString, QUuid)), peerWorker, SLOT (setPeerHostname(QString, QUuid)));
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
        emit sendMsg(itr.socketDescriptor, "this is a garbage uuid", localHostname, "/msg", QUuid::createUuid());
}

void MessengerWindow::timerout()
{
    if(peerWorker->peerDetailsHash.size() <= 1)
    {
        emit sendUdp("/discover" + QString::fromUtf8(localHostname));
        timer->start(5000);
    }
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
    this->printToTextBrowser(peerWorker->peerDetailsHash[uuid].hostname + " on " +peerWorker->peerDetailsHash[uuid].ipAddress + changeInConnection, uuid, false);
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
void MessengerWindow::updateListWidget(int num)
{
    messListWidget->setUpdatesEnabled(false);
    int count2 = messListWidget->count();
    for(int i = 0; i < count2 - 2; i++)
    {
        delete messListWidget->takeItem(0);
    }
    count2 = messListWidget->count();
    int count = peerWorker->peerDetailsHash.size()-1;
    for(auto &itr : peerWorker->peerDetailsHash)
    {
        itr.listWidgetIndex = count;
        messListWidget->insertItem(0, itr.hostname);
        if(itr.messagePending)
        {
            this->changeListColor(0, 0);
            messListWidget->item(0)->setText(" * " + itr.hostname + "*");
        }
        if(!(itr.isConnected))
        {
            messListWidget->item(0)->font().setItalic(1);
        }
        messListWidget->item(0)->setData(Qt::UserRole, itr.identifier);
        count--;
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
            emit sendMsg(itr.socketDescriptor, msg, localHostname, "/global", itr.identifier);
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
            emit connectToPeer(peerWorker->peerDetailsHash[uuidOfSelectedItem].socketDescriptor, peerWorker->peerDetailsHash[uuidOfSelectedItem].ipAddress);
        }
        emit sendMsg(peerWorker->peerDetailsHash[uuidOfSelectedItem].socketDescriptor, str, localHostname, "/msg", uuidOfSelectedItem);
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
void MessengerWindow::printToTextBrowser(const QString str, QUuid uuid, bool message)
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

    if(uuid.isNull())
    {
        //messTextBrowser->append(strnew);
        return;
    }
    if(!(peerWorker->peerDetailsHash[uuid].isValid) && ( uuid != globalChatUuid ))
    {
        peerWorker->peerDetailsHash.remove(uuid);
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
void MessengerWindow::printToTextBrowserServerSlot(const QString str, const QString ipstr, QUuid uuid, bool global)
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
