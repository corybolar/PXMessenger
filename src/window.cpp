#include <window.h>

MessengerWindow::MessengerWindow(QUuid uuid, int uuidNum, QString username, int tcpPort, int udpPort, QSize windowSize)
{
    ourUUIDString = uuid.toString();
    qDebug() << "Our UUID:" << ourUUIDString;

    setupHostname(uuidNum, username);

    if(tcpPort != 0)
        tcpPort += uuidNum;
    if(udpPort == 0)
        udpPort = 13649;

    messServer = new MessengerServer(this, tcpPort, udpPort);

    peerWorker = new PeerWorkerClass(this, localHostname, ourUUIDString, messServer);

    setupLayout();

    createTextBrowser();

    createTextEdit();

    createLineEdit();

    createButtons();

    createListWidget();

    createSystemTray();

    setupMenuBar();

    setupTooltips();

    createMessServ();

    createMessClient();

    createMessTime();

    connectGuiSignalsAndSlots();

    connectPeerClassSignalsAndSlots();

    this->setCentralWidget(centralwidget);

    statusbar = new QStatusBar(this);
    statusbar->setObjectName(QStringLiteral("statusbar"));
    this->setStatusBar(statusbar);
    this->show();
    this->resize(windowSize);
    if(this->timer != NULL)
        qDebug() << "hi";

    QTimer::singleShot(5000, this, SLOT(timerOutSingleShot()));
}
void MessengerWindow::setupMenuBar()
{
    menubar = new QMenuBar(this);
    menubar->setObjectName(QStringLiteral("menubar"));
    menubar->setGeometry(QRect(0, 0, 835, 27));
    menubar->setDefaultUp(false);
    this->setMenuBar(menubar);
    QMenu *fileMenu;
    QAction *quitAction = new QAction("&Quit", this);

    fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(quitAction);
    QObject::connect(quitAction, SIGNAL (triggered()), this, SLOT (quitButtonClicked()));

    QMenu *optionsMenu;
    QAction *settingsAction = new QAction("&Settings", this);
    optionsMenu = menuBar()->addMenu("&Options");
    optionsMenu->addAction(settingsAction);
    QObject::connect(settingsAction, SIGNAL (triggered()), this, SLOT (settingsActionsSlot()));

    QMenu *helpMenu;
    QAction *aboutAction = new QAction("&About", this);
    helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(aboutAction);
    QObject::connect(aboutAction, SIGNAL (triggered()), this, SLOT (aboutActionSlot()));
}

void MessengerWindow::setupTooltips()
{
#ifndef QT_NO_TOOLTIP
    messTextBrowser->setToolTip(QApplication::translate("PXMessenger", "<html><head/><body><p>Messages</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
    messLineEdit->setToolTip(QApplication::translate("PXMessenger", "<html><head/><body><p>Hostname</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
    messSendButton->setToolTip(QApplication::translate("PXMessenger", "<html><head/><body><p>Send a Message</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
    messSendButton->setText(QApplication::translate("PXMessenger", "Send", 0));
#ifndef QT_NO_TOOLTIP
    messListWidget->setToolTip(QApplication::translate("PXMessenger", "<html><head/><body><p>Connected Peers</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
    messTextEdit->setToolTip(QApplication::translate("PXMessenger", "<html><head/><body><p>Enter message to send</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
    messQuitButton->setToolTip(QApplication::translate("PXMessenger", "<html><head/><body><p>Quit PXMessenger</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
    messQuitButton->setText(QApplication::translate("PXMessenger", "Quit", 0));
}

void MessengerWindow::setupLayout()
{
    if (this->objectName().isEmpty())
        this->setObjectName(QStringLiteral("PXMessenger"));
    this->resize(835, 567);

    centralwidget = new QWidget(this);
    centralwidget->setObjectName(QStringLiteral("centralwidget"));
    layout = new QGridLayout(centralwidget);
    layout->setObjectName(QStringLiteral("layout"));

    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    layout->addItem(horizontalSpacer, 3, 0, 1, 1);
    horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    layout->addItem(horizontalSpacer_3, 5, 2, 1, 1);
    horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    layout->addItem(horizontalSpacer_2, 3, 2, 1, 1);
    horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    layout->addItem(horizontalSpacer_4, 5, 0, 1, 1);
}
void MessengerWindow::createTextEdit()
{
    messTextEdit = new MessengerTextEdit(centralwidget);
    messTextEdit->setObjectName(QStringLiteral("messTextEdit"));
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(messTextEdit->sizePolicy().hasHeightForWidth());
    messTextEdit->setSizePolicy(sizePolicy1);
    messTextEdit->setMaximumSize(QSize(16777215, 150));

    layout->addWidget(messTextEdit, 2, 0, 1, 3);
}
void MessengerWindow::createTextBrowser()
{
    messTextBrowser = new QTextBrowser(centralwidget);
    messTextBrowser->setObjectName(QStringLiteral("messTextBrowser"));
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(1);
    sizePolicy.setVerticalStretch(2);
    sizePolicy.setHeightForWidth(messTextBrowser->sizePolicy().hasHeightForWidth());
    messTextBrowser->setSizePolicy(sizePolicy);
    messTextBrowser->setMinimumSize(QSize(300, 200));

    layout->addWidget(messTextBrowser, 0, 0, 2, 3);
}
void MessengerWindow::createLineEdit()
{
    messLineEdit = new QLineEdit(centralwidget);
    messLineEdit->setObjectName(QStringLiteral("messLineEdit"));
    messLineEdit->setMinimumSize(QSize(200, 0));
    messLineEdit->setMaximumSize(QSize(300, 16777215));
    messLineEdit->setText(QString::fromUtf8(localHostname));
    //messLineEdit->setReadOnly(true);

    layout->addWidget(messLineEdit, 0, 3, 1, 1);
}
void MessengerWindow::createButtons()
{
    messSendButton = new QPushButton(centralwidget);
    messSendButton->setObjectName(QStringLiteral("messSendButton"));
    messSendButton->setMaximumSize(QSize(250, 16777215));
    messSendButton->setLayoutDirection(Qt::LeftToRight);
    messSendButton->setText("Send");

    layout->addWidget(messSendButton, 3, 1, 1, 1);

    messQuitButton = new QPushButton(centralwidget);
    messQuitButton->setObjectName(QStringLiteral("messQuitButton"));
    messQuitButton->setMaximumSize(QSize(250, 16777215));
    messQuitButton->setText("Quit");

    layout->addWidget(messQuitButton, 5, 1, 1, 1);
    messDebugButton = new QPushButton("Debug", this);
    messDebugButton->setGeometry(460, 510, 80, 30);
    connect(messDebugButton, SIGNAL(clicked()), this, SLOT (debugButtonClicked()));
    //messDebugButton->hide();
}
void MessengerWindow::createListWidget()
{
    globalChatUuid = QUuid::createUuid();

    messListWidget = new QListWidget(centralwidget);
    messListWidget->setObjectName(QStringLiteral("messListWidget"));
    messListWidget->setMinimumSize(QSize(200, 0));
    messListWidget->setMaximumSize(QSize(200, 16777215));

    layout->addWidget(messListWidget, 1, 3, 2, 1);
    //messListWidget->setGeometry(640, 60, 200, 300);
    messListWidget->insertItem(0, "--------------------");
    messListWidget->insertItem(1, "Global Chat");
    messListWidget->item(1)->setData(Qt::UserRole, globalChatUuid);
    messListWidget->item(0)->setFlags(messListWidget->item(0)->flags() & ~Qt::ItemIsEnabled);
}
void MessengerWindow::createSystemTray()
{
    //messSystemTrayIcon = new QIcon(":/resources/resources/systray.png", this);
    QIcon trayIcon(":/resources/resources/systray.png");


    messSystemTrayMenu = new QMenu(this);
    messSystemTrayExitAction = new QAction(tr("&Exit"), this);
    //messSystemTrayExitAction = messSystemTrayMenu->addAction("Exit");
    messSystemTrayMenu->addAction(messSystemTrayExitAction);

    messSystemTray = new QSystemTrayIcon(this);
    messSystemTray->setIcon(trayIcon);
    messSystemTray->setContextMenu(messSystemTrayMenu);
    messSystemTray->show();
}
void MessengerWindow::createMessClient()
{
    messClientThread = new QThread(this);
    messClient = new MessengerClient();
    messClient->moveToThread(messClientThread);
    messClient->setLocalHostname((localHostname));
    messClient->setlocalUUID(ourUUIDString);
    QObject::connect(messClientThread, &QThread::finished, messClientThread, &QObject::deleteLater);
    QObject::connect(messClient, &MessengerClient::resultOfConnectionAttempt, peerWorker, &PeerWorkerClass::resultOfConnectionAttempt);
    QObject::connect(messClient, &MessengerClient::resultOfTCPSend, peerWorker, &PeerWorkerClass::resultOfTCPSend);
    QObject::connect(this, &MessengerWindow::sendMsg, messClient, &MessengerClient::sendMsgSlot);
    QObject::connect(this, &MessengerWindow::connectToPeer, messClient, &MessengerClient::connectToPeerSlot);
    QObject::connect(this, &MessengerWindow::sendUdp, messClient, &MessengerClient::udpSendSlot);
    QObject::connect(messServer, &MessengerServer::sendMsg, messClient, &MessengerClient::sendMsgSlot);
    QObject::connect(messServer, &MessengerServer::sendName, messClient, &MessengerClient::sendNameSlot);
    QObject::connect(messServer, &MessengerServer::sendUdp, messClient, &MessengerClient::udpSendSlot);
    messClientThread->start();
}
void MessengerWindow::connectGuiSignalsAndSlots()
{
    QObject::connect(messSendButton, &QAbstractButton::clicked, this, &MessengerWindow::sendButtonClicked);
    QObject::connect(messQuitButton, &QAbstractButton::clicked, this, &MessengerWindow::quitButtonClicked);
    QObject::connect(messListWidget, &QListWidget::currentItemChanged, this, &MessengerWindow::currentItemChanged);
    QObject::connect(messTextEdit, &MessengerTextEdit::returnPressed, this, &MessengerWindow::sendButtonClicked);
    QObject::connect(messSystemTrayExitAction, &QAction::triggered, this, &MessengerWindow::quitButtonClicked);
    QObject::connect(messSystemTray, &QSystemTrayIcon::activated, this, &MessengerWindow::showWindow);
    QObject::connect(messSystemTray, &QObject::destroyed, messSystemTrayMenu, &QObject::deleteLater);
    QObject::connect(messTextEdit, &QTextEdit::textChanged, this, &MessengerWindow::textEditChanged);
    QObject::connect(messSystemTrayMenu, &QMenu::aboutToHide, messSystemTrayMenu, &QObject::deleteLater);;

}
void MessengerWindow::connectPeerClassSignalsAndSlots()
{
    QObject::connect(peerWorker, &PeerWorkerClass::sendMsg, messClient, &MessengerClient::sendMsgSlot);
    QObject::connect(peerWorker, &PeerWorkerClass::connectToPeer, messClient, &MessengerClient::connectToPeerSlot);
    QObject::connect(peerWorker, &PeerWorkerClass::printToTextBrowser, this, &MessengerWindow::printToTextBrowser);
    QObject::connect(peerWorker, &PeerWorkerClass::setItalicsOnItem, this, &MessengerWindow::setItalicsOnItem);
    QObject::connect(peerWorker, &PeerWorkerClass::updateListWidget, this, &MessengerWindow::updateListWidget);
}
void MessengerWindow::createMessServ()
{
    messServer->setLocalHostname(QString::fromUtf8(localHostname));
    messServer->setLocalUUID(ourUUIDString);
    QObject::connect(messServer, &MessengerServer::recievedUUIDForConnection, peerWorker, &PeerWorkerClass::updatePeerDetailsHash);
    QObject::connect(messServer, &MessengerServer::messageRecieved, this, &MessengerWindow::printToTextBrowserServerSlot );
    QObject::connect(messServer, &QThread::finished, messServer, &QObject::deleteLater);
    QObject::connect(messServer, &MessengerServer::newConnectionRecieved, peerWorker, &PeerWorkerClass::newTcpConnection);
    QObject::connect(messServer, &MessengerServer::peerQuit, peerWorker, &PeerWorkerClass::peerQuit);
    QObject::connect(messServer, &MessengerServer::updNameRecieved, peerWorker, &PeerWorkerClass::attemptConnection);
    QObject::connect(messServer, &MessengerServer::sendIps, peerWorker, &PeerWorkerClass::sendIps);
    QObject::connect(messServer, &MessengerServer::hostnameCheck, peerWorker, &PeerWorkerClass::hostnameCheck);
    QObject::connect(messServer, &MessengerServer::setPeerHostname, peerWorker, &PeerWorkerClass::setPeerHostname);
    QObject::connect(messServer, &MessengerServer::setListenerPort, peerWorker, &PeerWorkerClass::setListenerPort);
    QObject::connect(messServer, &MessengerServer::setListenerPort, peerWorker, &PeerWorkerClass::setListenerPort);
    messServer->start();
}
void MessengerWindow::aboutActionSlot()
{
    QMessageBox::about(this, "About", "<br><center>PXMessenger v"
                                      + qApp->applicationVersion() +
                                      "</center>"
                                      "<br>"
                                      "<center>Author: Cory Bolar</center>"
                                      "<br>"
                                      "<center>"
                                      "<a href=\"https://github.com/cbpeckles/PXMessenger\">"
                                      "https://github.com/cbpeckles/PXMessenger</a>"
                                      "</center>"
                                      "<br>");
}
void MessengerWindow::setListenerPort(QString port)
{
    ourListenerPort = port;
}

void MessengerWindow::settingsActionsSlot()
{
    settingsDialog *setD = new settingsDialog(this);
    setD->setupUi();
    setD->readIni();
    setD->show();
}

void MessengerWindow::createMessTime()
{
    messTime = time(0);
    currentTime = localtime( &messTime );
}
void MessengerWindow::setupHostname(int uuidNum, QString username)
{
    char computerHostname[128];
    gethostname(computerHostname, sizeof computerHostname);

    strcat(localHostname, username.toStdString().c_str());
    if(uuidNum > 0)
    {
        char temp[3];
        sprintf(temp, "%d", uuidNum);
        strcat(localHostname, temp);
    }
    strcat(localHostname, "@");
    strcat(localHostname, computerHostname);
}

void MessengerWindow::debugButtonClicked()
{

}
QString MessengerWindow::getFormattedTime()
{
    char time_str[12];
    messTime = time(0);
    currentTime = localtime(&messTime);
    strftime(time_str, 12, "(%H:%M:%S) ", currentTime);
    return time_str;
}
void MessengerWindow::timerOutRepetitive()
{
    if(messListWidget->count() < 4)
    {
        emit sendUdp("/discover:" + ourListenerPort);
    }
    else
    {
        qDebug() << "Found enough peers";
        timer->stop();
    }
}

void MessengerWindow::timerOutSingleShot()
{
    if(messListWidget->count() < 3)
    {
        QMessageBox::warning(this, "Network Problem", "Could not find anyone, even ourselves, on the network\nThis could indicate a problem with your configuration\n\nWe'll keep looking...");
    }

    if(messListWidget->count() < 4)
    {
        timer = new QTimer(this);
        timer->setInterval(5000);
        QObject::connect(timer, &QTimer::timeout, this, &MessengerWindow::timerOutRepetitive);
        emit sendUdp("/discover:" + ourListenerPort);
        qDebug() << "Retrying discovery packet, looking for other computers...";
        timer->start();
    }
    else
    {
        qDebug() << "Found enough peers";
    }
}
//Condense the 2 following into one, unsure of how to make the disconnect reconnect feature vary depending on bool
void MessengerWindow::setItalicsOnItem(QUuid uuid, bool italics)
{
    for(int i = 0; i < messListWidget->count() - 2; i++)
    {
        if(messListWidget->item(i)->data(Qt::UserRole) == uuid)
        {
            QFont mfont = messListWidget->item(i)->font();
            mfont.setItalic(italics);
            messListWidget->item(i)->setFont(mfont);
            QString changeInConnection;
            if(italics)
                changeInConnection = " disconnected";
            else
                changeInConnection = " reconnected";
            this->printToTextBrowser(this->getFormattedTime() + peerWorker->peerDetailsHash[uuid].hostname + " on " +peerWorker->peerDetailsHash[uuid].ipAddress + changeInConnection, uuid, false);
            return;
        }
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
void MessengerWindow::currentItemChanged(QListWidgetItem *item1)
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
    int count = messListWidget->count() - 2;
    if(count == 0)
    {
        messListWidget->insertItem(0, peerWorker->peerDetailsHash.value(uuid).hostname);
        messListWidget->item(0)->setData(Qt::UserRole, uuid);
        peerWorker->peerDetailsHash[uuid].listWidgetIndex = 0;

    }
    else
    {
        for(int i = 0; i <= count; i++)
        {
            QUuid u = messListWidget->item(i)->data(Qt::UserRole).toString();
            QString str = messListWidget->item(i)->text();
            if((str.startsWith(" * ")))
                str = str.mid(3, str.length()-6);
            if(peerWorker->peerDetailsHash.value(uuid).hostname.compare(str, Qt::CaseInsensitive) == 0)
            {
                if(u == uuid)
                    break;
                else
                    continue;
            }
            if(peerWorker->peerDetailsHash.value(uuid).hostname.compare(str, Qt::CaseInsensitive) < 0)
            {
                messListWidget->insertItem(i, peerWorker->peerDetailsHash.value((uuid)).hostname);
                messListWidget->item(i)->setData(Qt::UserRole, uuid);
                peerWorker->peerDetailsHash[uuid].listWidgetIndex = i;
                if(!(u.isNull()))
                {
                    peerWorker->peerDetailsHash[u].listWidgetIndex = i+1;
                }
                break;
            }
            else if(peerWorker->peerDetailsHash.value(uuid).hostname.compare(str, Qt::CaseInsensitive) > 0)
            {
                if(i == count)
                {
                    messListWidget->insertItem(i, peerWorker->peerDetailsHash.value(uuid).hostname);
                    messListWidget->item(i)->setData(Qt::UserRole, uuid);
                    peerWorker->peerDetailsHash[uuid].listWidgetIndex = i+1;
                    break;
                }
                else
                    continue;
            }
            else
            {
                qDebug() << "Error Displaying a Peer in MessengerWindow::updateListWidget";
            }
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
        if(itr.bev != NULL)
            bufferevent_free(itr.bev);
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
    delete peerWorker;
    messSystemTray->setContextMenu(NULL);
    messSystemTray->hide();
    MessIniReader iniReader;
    qDebug() << this->size();
    iniReader.setWindowSize(this->size());

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
    if(!(str.isEmpty()))
    {
        int index = messListWidget->currentRow();
        QUuid uuidOfSelectedItem = messListWidget->item(index)->data(Qt::UserRole).toString();
        if(uuidOfSelectedItem.isNull())
            return;

        peerDetails destination = peerWorker->peerDetailsHash.value(uuidOfSelectedItem);

        if( ( uuidOfSelectedItem == globalChatUuid) )
        {
            globalSend(str);
            messTextEdit->setText("");
            return;
        }
        if(!(destination.isConnected))
        {
            int s = socket(AF_INET, SOCK_STREAM, 0);
            peerWorker->peerDetailsHash[uuidOfSelectedItem].socketDescriptor = s;
            emit connectToPeer(s, destination.ipAddress, destination.portNumber);
        }
        emit sendMsg(destination.socketDescriptor, str, localHostname, "/msg", ourUUIDString, uuidOfSelectedItem.toString());
        messTextEdit->setText("");
    }

    return;
}
void MessengerWindow::changeListColor(int row, int style)
{
    QBrush back = messListWidget->item(row)->background();
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
    else if(this->windowState() == Qt::WindowActive)
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
            return;
        }
    }
    if(message)
    {
        int globalChatIndex = messListWidget->count() - 1;

        if(uuid == globalChatUuid)
        {
            this->changeListColor(globalChatIndex, 1);
            if(!globalChatAlerted)
            {
                messListWidget->item(globalChatIndex)->setText(" * " + messListWidget->item(globalChatIndex)->text() + " * ");
                globalChatAlerted = true;
            }
        }
        else if( !( peerWorker->peerDetailsHash.value(uuid).messagePending ) && ( messListWidget->currentRow() != globalChatIndex ) )
        {
            bool foundIt = false;
            int index = 0;
            int i = 0;
            for(; i < messListWidget->count(); i++)
            {
                if(messListWidget->item(i)->data(Qt::UserRole) == uuid)
                {
                    index = i;
                    foundIt = true;
                    break;
                }
            }
            if(!foundIt)
                return;
            this->changeListColor(index, 1);
            messListWidget->item(index)->setText(" * " + messListWidget->item(index)->text() + " * ");
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
