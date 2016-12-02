#include <pxmmainwindow.h>

PXMWindow::PXMWindow(initialSettings presets)
{
    //Init
    localHostname = QString();
    globalChatAlerted = false;
    multicastFunctioning = false;
    debugWindow = nullptr;
    //End of Init

    debugWindow = new PXMDebugWindow();
    QObject::connect(debugWindow->pushButton, SIGNAL(clicked(bool)), this, SLOT(printInfoToDebug()));

    ourUUIDString = presets.uuid.toString();

    qDebug() << "Our UUID:" << ourUUIDString;

    setupHostname(presets.uuidNum, presets.username);

    if(presets.tcpPort != 0)
        presets.tcpPort += presets.uuidNum;
    if(presets.udpPort == 0)
        presets.udpPort = 13649;
    ourUDPListenerPort = presets.udpPort;

    messServer = new PXMServer(this, presets.tcpPort, presets.udpPort);

    peerWorker = new PXMPeerWorker(this, localHostname, ourUUIDString, messServer);

    createMessClient();

    createMessServ();

    connectPeerClassSignalsAndSlots();

    setupGui();

    focusCheckBox->setChecked(presets.preventFocus);
    muteCheckBox->setChecked(presets.mute);

    this->resize(presets.windowSize);

    QTimer::singleShot(5000, this, SLOT(discoveryTimerSingleShot()));

    midnightTimer = new QTimer(this);
    midnightTimer->setInterval(MIDNIGHT_TIMER_INTERVAL_MINUTES * 60000);
    QObject::connect(midnightTimer, &QTimer::timeout, this, &PXMWindow::midnightTimerPersistent);
    midnightTimer->start();
}
PXMWindow::~PXMWindow()
{
    midnightTimer->stop();

    peerWorker->disconnect(messServer);
    delete peerWorker;

    if(messServer != 0 && messServer->isRunning())
    {
        bufferevent_write(closeBev, "/exit", 5);
        messServer->wait(5000);
    }
    qDebug() << "Shutdown of PXMServer Successful";

    if(messClientThread != 0 && messClientThread->isRunning())
    {
        messClientThread->quit();
        messClientThread->wait(5000);
    }
    qDebug() << "Shutdown of PXMClientThread Successful";

    delete messClient;
    qDebug() << "Deletion of PXMClient Successful";

    qDeleteAll(globalChat);

    qDebug() << "Shutdown of PXMWindow Successful";
}

void PXMWindow::setupMenuBar()
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
    QObject::connect(quitAction, &QAction::triggered, this, &PXMWindow::quitButtonClicked);

    QMenu *optionsMenu;
    QAction *settingsAction = new QAction("&Settings", this);
    QAction *bloomAction = new QAction("&Bloom", this);
    optionsMenu = menuBar()->addMenu("&Tools");
    //optionsMenu = menuBar()->addMenu("&Bloom");
    optionsMenu->addAction(settingsAction);
    optionsMenu->addAction(bloomAction);
    QObject::connect(settingsAction, &QAction::triggered, this, &PXMWindow::settingsActionsSlot);
    QObject::connect(bloomAction, &QAction::triggered, this, &PXMWindow::bloomActionsSlot);

    QMenu *helpMenu;
    QAction *aboutAction = new QAction("&About", this);
    QAction *debugAction = new QAction("&Debug", this);
    helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(aboutAction);
    helpMenu->addAction(debugAction);
    QObject::connect(aboutAction, &QAction::triggered, this, &PXMWindow::aboutActionSlot);
    QObject::connect(debugAction, &QAction::triggered, this, &PXMWindow::debugActionSlot);
}
void PXMWindow::setupTooltips()
{
#ifndef QT_NO_TOOLTIP
    messSendButton->setToolTip(QApplication::translate("PXMessenger", "<html><head/><body><p>Send a Message</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
    messSendButton->setText(QApplication::translate("PXMessenger", "Send", 0));
#ifndef QT_NO_TOOLTIP
    messQuitButton->setToolTip(QApplication::translate("PXMessenger", "<html><head/><body><p>Quit PXMessenger</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
    messQuitButton->setText(QApplication::translate("PXMessenger", "Quit", 0));
}
void PXMWindow::setupLayout()
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
void PXMWindow::createTextEdit()
{
    messTextEdit = new PXMTextEdit(centralwidget);
    messTextEdit->setObjectName(QStringLiteral("messTextEdit"));
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(messTextEdit->sizePolicy().hasHeightForWidth());
    messTextEdit->setSizePolicy(sizePolicy1);
    messTextEdit->setMaximumSize(QSize(16777215, 150));
    messTextEdit->setPlaceholderText("Enter a message to send here!");


    layout->addWidget(messTextEdit, 2, 0, 1, 3);
}
void PXMWindow::createTextBrowser()
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
void PXMWindow::createLineEdit()
{
    messLineEdit = new QLineEdit(centralwidget);
    messLineEdit->setObjectName(QStringLiteral("messLineEdit"));
    messLineEdit->setMinimumSize(QSize(200, 0));
    messLineEdit->setMaximumSize(QSize(300, 16777215));
    messLineEdit->setText(localHostname);
    messLineEdit->setReadOnly(true);

    layout->addWidget(messLineEdit, 0, 3, 1, 1);
}
void PXMWindow::createButtons()
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

    messDebugButton = new QPushButton(centralwidget);
    connect(messDebugButton, SIGNAL(clicked()), this, SLOT (debugButtonClicked()));
    messDebugButton->setMaximumSize(QSize(250, 16777215));
    messDebugButton->setText("Debug");

    layout->addWidget(messDebugButton, 6, 1, 1, 1);
    messDebugButton->hide();
}
void PXMWindow::createListWidget()
{
    globalChatUuid = QUuid::createUuid();

    messListWidget = new QListWidget(centralwidget);
    messListWidget->setObjectName(QStringLiteral("messListWidget"));
    messListWidget->setMinimumSize(QSize(200, 0));
    messListWidget->setMaximumSize(QSize(300, 16777215));

    layout->addWidget(messListWidget, 1, 3, 2, 1);
    messListWidget->insertItem(0, "Global Chat");
    QListWidgetItem *seperator = new QListWidgetItem(messListWidget);
    seperator->setSizeHint(QSize(200, 10));
    seperator->setFlags(Qt::NoItemFlags);
    messListWidget->insertItem(1, seperator);
    fsep = new QFrame(messListWidget);
    fsep->setFrameStyle(QFrame::HLine | QFrame::Plain);
    fsep->setLineWidth(2);
    messListWidget->setItemWidget(seperator, fsep);
    messListWidget->item(0)->setData(Qt::UserRole, globalChatUuid);
    messListWidget->setSortingEnabled(false);
}
void PXMWindow::createSystemTray()
{
    QIcon trayIcon(":/resources/resources/70529.png");

    messSystemTrayMenu = new QMenu(this);
    messSystemTrayExitAction = new QAction(tr("&Exit"), this);
    messSystemTrayMenu->addAction(messSystemTrayExitAction);

    messSystemTray = new QSystemTrayIcon(this);
    messSystemTray->setIcon(trayIcon);
    messSystemTray->setContextMenu(messSystemTrayMenu);
    messSystemTray->show();
}
void PXMWindow::createCheckBoxes()
{
    muteCheckBox = new QCheckBox(centralwidget);
    muteCheckBox->setObjectName(QStringLiteral("muteCheckBox"));
    muteCheckBox->setLayoutDirection(Qt::RightToLeft);
    muteCheckBox->setText("Mute");
    layout->addWidget(muteCheckBox, 3, 3, 1, 1);

    focusCheckBox = new QCheckBox(centralwidget);
    focusCheckBox->setObjectName(QStringLiteral("focusCheckBox"));
    focusCheckBox->setLayoutDirection(Qt::RightToLeft);
    focusCheckBox->setText("Prevent focus stealing");

    layout->addWidget(focusCheckBox, 5, 3, 1, 1);
}

void PXMWindow::setupGui()
{
    setupLayout();

    createTextBrowser();

    createTextEdit();

    createLineEdit();

    createButtons();

    createListWidget();

    createSystemTray();

    createCheckBoxes();

    setupMenuBar();

    setupTooltips();

    createMessTime();

    connectGuiSignalsAndSlots();

    this->setCentralWidget(centralwidget);

    this->show();
}
void PXMWindow::createMessClient()
{
    messClientThread = new QThread(this);
    messClient = new PXMClient();
    messClientThread->setObjectName("PXMClient QThread");
    messClient->moveToThread(messClientThread);
    QObject::connect(messClientThread, &QThread::finished, messClientThread, &QObject::deleteLater);
    QObject::connect(messClient, &PXMClient::resultOfConnectionAttempt, peerWorker, &PXMPeerWorker::resultOfConnectionAttempt);
    QObject::connect(messClient, &PXMClient::resultOfTCPSend, peerWorker, &PXMPeerWorker::resultOfTCPSend);
    QObject::connect(this, &PXMWindow::sendMsg, messClient, &PXMClient::sendMsgSlot);
    //QObject::connect(this, &PXMWindow::connectToPeer, messClient, &PXMClient::connectToPeer);
    QObject::connect(this, &PXMWindow::sendUDP, messClient, &PXMClient::sendUDP);
    QObject::connect(messServer, &PXMServer::sendUDP, messClient, &PXMClient::sendUDP);
    QObject::connect(messClient, SIGNAL(xdebug(QString)),
                     debugWindow, SLOT(xdebug(QString)), Qt::QueuedConnection);
    messClientThread->start();
}
void PXMWindow::connectGuiSignalsAndSlots()
{
    QObject::connect(messSendButton, &QAbstractButton::clicked, this, &PXMWindow::sendButtonClicked);
    QObject::connect(messQuitButton, &QAbstractButton::clicked, this, &PXMWindow::quitButtonClicked);
    QObject::connect(messListWidget, &QListWidget::currentItemChanged, this, &PXMWindow::currentItemChanged);
    QObject::connect(messTextEdit, &PXMTextEdit::returnPressed, this, &PXMWindow::sendButtonClicked);
    QObject::connect(messSystemTrayExitAction, &QAction::triggered, this, &PXMWindow::quitButtonClicked);
    QObject::connect(messSystemTray, &QSystemTrayIcon::activated, this, &PXMWindow::showWindow);
    QObject::connect(messSystemTray, &QObject::destroyed, messSystemTrayMenu, &QObject::deleteLater);
    QObject::connect(messTextEdit, &QTextEdit::textChanged, this, &PXMWindow::textEditChanged);
    QObject::connect(messSystemTrayMenu, &QMenu::aboutToHide, messSystemTrayMenu, &QObject::deleteLater);;
}
void PXMWindow::connectPeerClassSignalsAndSlots()
{
    QObject::connect(peerWorker, &PXMPeerWorker::sendMsg, messClient, &PXMClient::sendMsgSlot);
    QObject::connect(peerWorker, &PXMPeerWorker::connectToPeer, messClient, &PXMClient::connectToPeer);
    QObject::connect(peerWorker, &PXMPeerWorker::printToTextBrowser, this, &PXMWindow::printToTextBrowser);
    QObject::connect(peerWorker, &PXMPeerWorker::setItalicsOnItem, this, &PXMWindow::setItalicsOnItem);
    QObject::connect(peerWorker, &PXMPeerWorker::updateListWidget, this, &PXMWindow::updateListWidget);
    QObject::connect(peerWorker, &PXMPeerWorker::sendIpsPacket, messClient, &PXMClient::sendIpsSlot);
}
void PXMWindow::createMessServ()
{
    messServer->setLocalHostname(localHostname);
    messServer->setLocalUUID(ourUUIDString);
    QObject::connect(messServer, &PXMServer::authenticationReceived, peerWorker, &PXMPeerWorker::authenticationReceived);
    QObject::connect(messServer, &PXMServer::messageRecieved, this, &PXMWindow::printToTextBrowserServerSlot );
    QObject::connect(messServer, &QThread::finished, messServer, &QObject::deleteLater);
    QObject::connect(messServer, &PXMServer::newTCPConnection, peerWorker, &PXMPeerWorker::newTcpConnection);
    QObject::connect(messServer, &PXMServer::peerQuit, peerWorker, &PXMPeerWorker::peerQuit);
    QObject::connect(messServer, &PXMServer::attemptConnection, peerWorker, &PXMPeerWorker::attemptConnection);
    QObject::connect(messServer, &PXMServer::sendIps, peerWorker, &PXMPeerWorker::sendIps);
    QObject::connect(messServer, &PXMServer::hostnameCheck, peerWorker, &PXMPeerWorker::hostnameCheck);
    QObject::connect(messServer, &PXMServer::setPeerHostname, peerWorker, &PXMPeerWorker::setPeerHostname);
    QObject::connect(messServer, &PXMServer::setListenerPort, peerWorker, &PXMPeerWorker::setListenerPort);
    QObject::connect(messServer, &PXMServer::setListenerPort, this, &PXMWindow::setListenerPort);
    QObject::connect(messServer, &PXMServer::libeventBackend, this, &PXMWindow::setlibeventBackend);
    QObject::connect(messServer, &PXMServer::setCloseBufferevent, this, &PXMWindow::setCloseBufferevent);
    QObject::connect(messServer, &PXMServer::setSelfCommsBufferevent, this, &PXMWindow::setSelfCommsBufferevent);
    QObject::connect(messServer, &PXMServer::multicastIsFunctional, this, &PXMWindow::multicastIsFunctional);
    QObject::connect(messServer, SIGNAL(xdebug(QString)),
                     debugWindow, SLOT(xdebug(QString)), Qt::QueuedConnection);
    messServer->start();
}
void PXMWindow::aboutActionSlot()
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
void PXMWindow::setListenerPort(unsigned short port)
{
    ourTCPListenerPort = port;
}
void PXMWindow::settingsActionsSlot()
{
    PXMSettingsDialog *setD = new PXMSettingsDialog(this);
    setD->setupUi();
    setD->readIni();
    setD->show();
}
void PXMWindow::bloomActionsSlot()
{
    QMessageBox box;
    box.setText("This will resend our initial discovery "
                "packet to the multicast group.  If we "
                "have only found ourselves this is happening "
                "automatically on a 15 second timer.");
    QPushButton *bloomButton = box.addButton(tr("Bloom"), QMessageBox::ActionRole);
    box.addButton(QMessageBox::Abort);
    box.exec();
    if(box.clickedButton() == bloomButton)
    {
        emit sendUDP("/discover", ourUDPListenerPort);
    }
}
void PXMWindow::setlibeventBackend(QString str)
{
    libeventBackend = str;
}
void PXMWindow::setCloseBufferevent(bufferevent *bev)
{
    closeBev = bev;
}
void PXMWindow::setSelfCommsBufferevent(bufferevent *bev)
{
    peerWorker->peerDetailsHash[ourUUIDString].bev = bev;
    peerWorker->peerDetailsHash[ourUUIDString].hostname = localHostname;
    peerWorker->peerDetailsHash[ourUUIDString].identifier = ourUUIDString;
    peerWorker->peerDetailsHash[ourUUIDString].isAuthenticated = false;
    peerWorker->peerDetailsHash[ourUUIDString].isConnected = true;
    peerWorker->peerDetailsHash[ourUUIDString].socketDescriptor = -1;
    updateListWidget(ourUUIDString);
}

void PXMWindow::printInfoToDebug()
{
    int i = 0;
    QString str;
    str.append(QStringLiteral("\n"));
    str.append(QStringLiteral("---Program Info---\n"));
    str.append(QStringLiteral("Program Name: ") % qApp->applicationName() % QStringLiteral("\n"));
    str.append(QStringLiteral("Version: ") % qApp->applicationVersion() % QStringLiteral("\n"));

    str.append(QStringLiteral("---Network Info---\n"));
    str.append(QStringLiteral("Libevent Backend: ") % libeventBackend % QStringLiteral("\n")
              % QStringLiteral("Multicast Address: ") % QString::fromUtf8(MULTICAST_ADDRESS) % QStringLiteral("\n")
              % QStringLiteral("TCP Listener Port: ") % QString::number(ourTCPListenerPort) % QStringLiteral("\n")
              % QStringLiteral("UDP Listener Port: ") % QString::number(ourUDPListenerPort) % QStringLiteral("\n")
              % QStringLiteral("Our UUID: ") % ourUUIDString % QStringLiteral("\n"));
    str.append(QStringLiteral("---Peer Details---\n"));

    for(auto &itr : peerWorker->peerDetailsHash)
    {
        str.append(QStringLiteral("---Peer #") % QString::number(i) % "---\n");
        str.append(QStringLiteral("Hostname: ") % itr.hostname % QStringLiteral("\n"));
        str.append(QStringLiteral("UUID: ") % itr.identifier.toString() % QStringLiteral("\n"));
        str.append(QStringLiteral("IP Address: ") % QString::fromLocal8Bit(inet_ntoa(itr.ipAddressRaw.sin_addr))
                   % QStringLiteral(":") % QString::number(ntohs(itr.ipAddressRaw.sin_port)) % QStringLiteral("\n"));
        str.append(QStringLiteral("IsAuthenticated: ") % QString::fromLocal8Bit((itr.isAuthenticated? "true" : "false")) % QStringLiteral("\n"));
        str.append(QStringLiteral("IsConnected: ") % QString::fromLocal8Bit((itr.isConnected ? "true" : "false")) % QStringLiteral("\n"));
        str.append(QStringLiteral("SocketDescriptor: ") % QString::number(itr.socketDescriptor) % QStringLiteral("\n"));
        str.append(QStringLiteral("History Length: ") % QString::number(itr.messages.count()) % QStringLiteral("\n"));
        if(itr.bev != nullptr)
        {
            QString t;
            str.append(QStringLiteral("Bufferevent: ") % t.asprintf("%8p",itr.bev) % QStringLiteral("\n"));
        }
        else
            str.append(QStringLiteral("Bufferevent: ") % QStringLiteral("NULL") % QStringLiteral("\n"));

        i++;
    }
    str.append(QStringLiteral("-------------\n"));
    str.append(QStringLiteral("Total Peers: ") % QString::number(i) % QStringLiteral("\n"));
    str.append(QStringLiteral("Extra Bufferevents: ") % QString::number(peerWorker->extraBufferevents.count()) % QStringLiteral("\n"));
    str.append(QStringLiteral("-------------"));
    qDebug().noquote() << str;
}

void PXMWindow::createMessTime()
{
    messTime = time(0);
    currentTime = localtime( &messTime );
}
void PXMWindow::setupHostname(int uuidNum, QString username)
{
    char computerHostname[256];

    gethostname(computerHostname, sizeof computerHostname);

    localHostname.append(username);
    if(uuidNum > 0)
    {
        char temp[3];
        sprintf(temp, "%d", uuidNum);
        localHostname.append(QString::fromUtf8(temp));
    }
    localHostname.append("@");
    localHostname.append(computerHostname);
}
void PXMWindow::debugButtonClicked()
{
}
void PXMWindow::debugActionSlot()
{
    if(debugWindow)
    {
        debugWindow->show();
    }
}

QString PXMWindow::getFormattedTime()
{
    char time_str[12];
    messTime = time(0);
    currentTime = localtime(&messTime);
    strftime(time_str, 12, "(%H:%M:%S) ", currentTime);
    return QString::fromUtf8(time_str);
}
void PXMWindow::midnightTimerPersistent()
{
    QDateTime dt = QDateTime::currentDateTime();
    if(dt.time() <= QTime(0, MIDNIGHT_TIMER_INTERVAL_MINUTES, 0, 0))
    {
        QString str = QString();
        QString dashes = "-------------------------------";
        str.append("\n");
        str.append("<center>");
        str.append(dashes.right(7));
        str.append(' ');
        str.append(dt.date().toString());
        str.append(' ');
        str.append(dashes.right(7));
        str.append("<center>");
        str.append("\n");
        for(auto &itr : peerWorker->peerDetailsHash)
        {
            printToTextBrowser(str, itr.identifier, false, false);
        }
        printToTextBrowser(str, globalChatUuid, false, false);
    }
}

void PXMWindow::discoveryTimerPersistent()
{
    if(messListWidget->count() < 4)
    {
        emit sendUDP("/discover", ourUDPListenerPort);
    }
    else
    {
        qDebug() << "Found enough peers";
        discoveryTimer->stop();
    }
}
void PXMWindow::discoveryTimerSingleShot()
{
    if(!multicastFunctioning)
    {
        QMessageBox::warning(this, "Network Problem", "Could not find anyone, even ourselves, on the network\nThis could indicate a problem with your configuration\n\nWe'll keep looking...");
    }
    if(messListWidget->count() < 4)
    {
        discoveryTimer= new QTimer(this);
        discoveryTimer->setInterval(30000);
        QObject::connect(discoveryTimer, &QTimer::timeout, this, &PXMWindow::discoveryTimerPersistent);
        emit sendUDP("/discover", ourUDPListenerPort);
        qDebug() << QStringLiteral("Retrying discovery packet, looking for other computers...");
        discoveryTimer->start();
    }
    else
    {
        qDebug() << "Found enough peers";
    }
}
void PXMWindow::multicastIsFunctional()
{
    multicastFunctioning = true;
}
void PXMWindow::setItalicsOnItem(QUuid uuid, bool italics)
{
    for(int i = 0; i < messListWidget->count(); i++)
    {
        if(messListWidget->item(i)->data(Qt::UserRole) == uuid)
        {
            QFont mfont = messListWidget->item(i)->font();
            if(mfont.italic() == italics)
                return;
            mfont.setItalic(italics);
            messListWidget->item(i)->setFont(mfont);
            QString changeInConnection;
            if(italics)
                changeInConnection = " disconnected";
            else
                changeInConnection = " reconnected";
            this->printToTextBrowser(peerWorker->peerDetailsHash[uuid].hostname % changeInConnection, uuid, false, true);
            return;
        }
    }
}
void PXMWindow::textEditChanged()
{
    if(messTextEdit->toPlainText().length() > 2000)
    {
        int diff = messTextEdit->toPlainText().length() - 2000;
        QString temp = messTextEdit->toPlainText();
        temp.chop(diff);
        messTextEdit->setText(temp);
        QTextCursor cursor(messTextEdit->textCursor());
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        messTextEdit->setTextCursor(cursor);
    }
}
void PXMWindow::showWindow(QSystemTrayIcon::ActivationReason reason)
{
    if( ( ( reason == QSystemTrayIcon::DoubleClick ) | ( reason == QSystemTrayIcon::Trigger ) ) && ! ( this->isVisible() ) )
    {
        this->setWindowState(Qt::WindowMaximized);
        this->show();
        this->setWindowState(Qt::WindowActive);
    }
    return;
}
void PXMWindow::changeEvent(QEvent *event)
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
void PXMWindow::currentItemChanged(QListWidgetItem *item1)
{
    messTextBrowser->setUpdatesEnabled(false);
    messTextBrowser->clear();
    QUuid uuid1 = item1->data(Qt::UserRole).toString();
    if(uuid1 == globalChatUuid)
    {
        for(auto &itr : globalChat)
            messTextBrowser->append(*itr);
    }
    else
    {
        for(auto &itr : peerWorker->peerDetailsHash.value(uuid1).messages)
        {
            messTextBrowser->append(*itr);
        }
    }
    if(item1->background() == Qt::red)
    {
        this->changeListColor(messListWidget->row(item1), 0);
    }
    QScrollBar *sb = this->messTextBrowser->verticalScrollBar();
    sb->setValue(sb->maximum());
    messTextBrowser->setUpdatesEnabled(true);
    return;
}
void PXMWindow::quitButtonClicked()
{
    this->close();
}
void PXMWindow::updateListWidget(QUuid uuid)
{
    messListWidget->setUpdatesEnabled(false);
    for(int i = 2; i < messListWidget->count(); i++)
    {
        if(messListWidget->item(i)->data(Qt::UserRole).toUuid() == uuid)
        {
            if(messListWidget->item(i)->text() != peerWorker->peerDetailsHash.value(uuid).hostname)
            {
                printToTextBrowser(messListWidget->item(i)->text() % " has changed their name to " % peerWorker->peerDetailsHash.value(uuid).hostname, uuid, false, false);
                messListWidget->item(i)->setText(peerWorker->peerDetailsHash.value(uuid).hostname);
                QListWidgetItem *global = messListWidget->takeItem(0);
                messListWidget->sortItems();
                messListWidget->insertItem(0, global);
            }
            setItalicsOnItem(uuid, 0);
            messListWidget->setUpdatesEnabled(true);
            return;
        }
    }

    QListWidgetItem *global = messListWidget->takeItem(0);
    QListWidgetItem *item = new QListWidgetItem(peerWorker->peerDetailsHash.value(uuid).hostname, messListWidget);

    item->setData(Qt::UserRole, uuid);
    messListWidget->addItem(item);
    messListWidget->sortItems();
    messListWidget->insertItem(0, global);

    messListWidget->setUpdatesEnabled(true);
}
void PXMWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "starting closeEvent";
    qDebug() << "trying to hide systray";
    messSystemTray->hide();
    qDebug() << "systray hidden";
    MessIniReader iniReader;
    qDebug() << "creating iniReader";
    iniReader.setWindowSize(this->size());
    iniReader.setMute(muteCheckBox->isChecked());
    iniReader.setFocus(focusCheckBox->isChecked());
    qDebug() << "ini done";

    qDebug() << "closeEvent calling event->accept()";
    event->accept();
}
void PXMWindow::globalSend(QByteArray msg)
{
    for(auto &itr : peerWorker->peerDetailsHash)
    {
        if(itr.isConnected)
        {
            //bufferevent_lock(itr.bev);
            emit sendMsg(itr.bev,  msg, "/global", ourUUIDString, QUuid());
        }
    }
    messTextEdit->setText("");
    return;
}
void PXMWindow::sendButtonClicked()
{
    if(messListWidget->selectedItems().count() == 0)
    {
        this->printToTextBrowser("Choose a computer to message from the selection pane on the right", "", false, false);
        return;
    }

    QByteArray msg = messTextEdit->toPlainText().toUtf8();
    //QByteArray msg = messTextEdit->toHtml().toUtf8();
    //int len1 = msg.length();
    if(!(msg.isEmpty()))
    {
        int index = messListWidget->currentRow();
        QUuid uuidOfSelectedItem = messListWidget->item(index)->data(Qt::UserRole).toString();

        if(uuidOfSelectedItem.isNull())
            return;

        if( ( uuidOfSelectedItem == globalChatUuid) )
        {
            globalSend(msg);
            messTextEdit->setText("");
            return;
        }
        //bufferevent_lock(peerWorker->peerDetailsHash.value(uuidOfSelectedItem).bev);
        emit sendMsg(peerWorker->peerDetailsHash.value(uuidOfSelectedItem).bev, msg, "/msg", ourUUIDString, uuidOfSelectedItem);
        messTextEdit->setText("");
    }

    return;
}
void PXMWindow::changeListColor(int row, int style)
{
    if(style == 1)
    {
        //QBrush back = QGuiApplication::palette().base();
        //QColor backColor = back.color();
        //backColor.setRgb(255 - backColor.red(),255 - backColor.green(),255 - backColor.blue());
        messListWidget->item(row)->setBackground(QBrush(QColor(255,0,0)));
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
void PXMWindow::focusWindow()
{
    if(!(muteCheckBox->isChecked()))
        QSound::play(":/resources/resources/message.wav");

    if(!(this->isMinimized()))
    {
        qApp->alert(this, 0);
    }
    else if(this->isMinimized() && !(focusCheckBox->isChecked()))
    {
        this->show();
        qApp->alert(this, 0);
        this->raise();
        this->setWindowState(Qt::WindowActive);
    }
}
/**
 * @brief 					This will print new messages to the appropriate QString and call the focus function if its necessary
 * @param str				Message to print
 * @param peerindex			index of the peers_class->peers array for which the message was meant for
 * @param message			Bool for whether this message that is being printed should alert the listwidgetitem, play sound, and focus the application
 */
void PXMWindow::printToTextBrowser(QString str, QUuid uuid, bool alert, bool formatAsMessage)
{
    bool foundIt = false;
    int index = 0;
    for(; index < messListWidget->count(); index++)
    {
        if(messListWidget->item(index)->data(Qt::UserRole) == uuid)
        {
            foundIt = true;
            break;
        }
    }
    if(!foundIt)
        return;

    if(formatAsMessage)
        str = this->getFormattedTime() % str;

    if(uuid == globalChatUuid)
    {
        globalChat.append(new QString(str.toUtf8()));
        if(globalChat.size() > MESSAGE_HISTORY_LENGTH)
        {
            delete globalChat.takeFirst();
        }
    }
    else
    {
        peerWorker->peerDetailsHash[uuid].messages.append(new QString(str.toUtf8()));
        if(peerWorker->peerDetailsHash[uuid].messages.size() > MESSAGE_HISTORY_LENGTH)
        {
            delete peerWorker->peerDetailsHash[uuid].messages.takeFirst();
        }
    }

    if(messListWidget->currentItem() != NULL)
    {
        if(uuid == messListWidget->currentItem()->data(Qt::UserRole))
        {
            messTextBrowser->append(str.toHtmlEscaped());
            //messTextBrowser->insertHtml(str);
            if(alert)
                this->focusWindow();
            return;
        }
    }

    if(alert)
    {
        this->changeListColor(index, 1);
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
void PXMWindow::printToTextBrowserServerSlot(QString msg, QUuid uuid, bufferevent* bev, bool global)
{
    if(uuid != ourUUIDString)
    {
        if( !( peerWorker->peerDetailsHash.contains(uuid) ) )
        {
            qDebug() << "Message from invalid uuid, rejection";
            return;
        }

        if(peerWorker->peerDetailsHash.value(uuid).bev != bev)
        {
            bool foundIt = false;
            QUuid uuidSpoofer = "";
            for(auto &itr : peerWorker->peerDetailsHash)
            {
                if(itr.bev == bev)
                {
                    foundIt = true;
                    uuidSpoofer = itr.identifier;
                    break;
                }
            }
            if(foundIt)
                this->printToTextBrowser("This user is trying to spoof another users uuid!", uuidSpoofer, true, false);
            else
                this->printToTextBrowser("Someone is trying to spoof this users uuid!", uuid, true, false);
            return;
        }
    }

    msg.prepend(peerWorker->peerDetailsHash.value(uuid).hostname % ": ");

    if(global)
    {
        uuid = globalChatUuid;
    }

    this->printToTextBrowser(msg, uuid, true, true);
}
