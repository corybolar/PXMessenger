#include <agent.h>

#ifdef _WIN32
#include <lmcons.h>
#include <QSimpleUpdater/include/QSimpleUpdater.h>

#ifdef _WIN64
const char* modName = "windows64";
const char* arch    = "x86_64";
#else
const char* modName = "windows32";
const char* arch    = "x86";
#endif

#elif __unix__
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#else
#error "include headers for querying username"
#endif

#include <QAbstractButton>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QLockFile>
#include <QMessageBox>
#include <QPixmap>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QSplashScreen>
#include <QThread>
#include <QStyle>
#include <QStyleFactory>

#include <console.h>
#include <inireader.h>
#include <mainwindow.h>
#include <peerworker.h>
#include <console.h>

Q_DECLARE_METATYPE(QSharedPointer<QString>)

QPalette PXMAgent::defaultPalette;

struct initialSettings {
    QUuid uuid;
    QSize windowSize;
    QString username;
    QString multicast;
    unsigned int uuidNum;
    unsigned short tcpPort;
    unsigned short udpPort;
    bool mute;
    bool preventFocus;
    bool darkColor;
    initialSettings()
        : uuid(QUuid()),
          windowSize(QSize(800, 600)),
          username(QString()),
          multicast(QString()),
          uuidNum(0),
          tcpPort(0),
          udpPort(0),
          mute(false),
          preventFocus(false),
          darkColor(false)
    {
    }
};

class PXMAgentPrivate
{
   public:
    PXMAgentPrivate() {}
    ~PXMAgentPrivate()
    {
        if (workerThread) {
            workerThread->quit();
            workerThread->wait(3000);
            qInfo() << "Shutdown of WorkerThread Successful";
        }

        iniReader.resetUUID(presets.uuidNum, presets.uuid);
    }

    QScopedPointer<PXMWindow> window;
    PXMPeerWorker* peerWorker;
    PXMIniReader iniReader;
    PXMConsole::Logger* logger;
    QScopedPointer<QLockFile> lockFile;
    QString address = "https://raw.githubusercontent.com/cbpeckles/PXMessenger/master/resources/updates.json";

    initialSettings presets;

#ifdef __WIN32
    QSimpleUpdater* qupdater;
#endif
    QThread* workerThread = nullptr;
    // Functions
    QByteArray getUsername();
    int setupHostname(const unsigned int uuidNum, QString& username);
};

PXMAgent::PXMAgent(QObject* parent) : QObject(parent), d_ptr(new PXMAgentPrivate)
{
    PXMAgent::defaultPalette = QGuiApplication::palette();
}

PXMAgent::~PXMAgent()
{
}

void PXMAgent::changeToDark()
{
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(35, 35, 35));
    palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);

    palette.setColor(QPalette::Highlight, QColor(142, 45, 197).lighter());
    palette.setColor(QPalette::HighlightedText, Qt::black);
    qApp->setPalette(palette);
}

int PXMAgent::init()
{
    QThread::currentThread()->setObjectName("MainThread");
#ifdef __WIN32
    if (d_ptr->iniReader.getUpdates()) {
        d_ptr->qupdater = QSimpleUpdater::getInstance();
        connect(d_ptr->qupdater, SIGNAL(checkingFinished(QString)), this, SLOT(doneChkUpdt(QString)));
        connect(d_ptr->qupdater, &QSimpleUpdater::installerOpened, qApp, &QApplication::quit);
        connect(d_ptr->qupdater, &QSimpleUpdater::updateDeclined, this, &PXMAgent::postInit);
        d_ptr->qupdater->setModuleVersion(d_ptr->address, qApp->applicationVersion());
        d_ptr->qupdater->setModuleName(d_ptr->address, modName);
        d_ptr->qupdater->setNotifyOnUpdate(d_ptr->address, true);
        d_ptr->qupdater->checkForUpdates(d_ptr->address);
    } else {
        postInit();
    }
#else
    this->postInit();
#endif
    return 0;
}

int PXMAgent::postInit()
{
#ifndef QT_DEBUG
    QImage splashImage(":/resources/PXMessenger_wBackground.png");
    QSplashScreen splash(QPixmap::fromImage(splashImage));
    splash.show();
    QElapsedTimer startupTimer;
    startupTimer.start();
    qApp->processEvents();
#endif

#ifdef _WIN32
    try {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            qCritical() << "Failed WSAStartup error:" << WSAGetLastError();
            throw(&"Failed WSAStartup error: "[WSAGetLastError()]);
        }
    } catch (const char* msg) {
        QMessageBox msgBox(QMessageBox::Critical, "WSAStartup Error", QString::fromUtf8(msg));
        msgBox.exec();
        emit alreadyRunning();
        return -1;
    }

    qRegisterMetaType<intptr_t>("intptr_t");
#endif
    qRegisterMetaType<struct sockaddr_in>();
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<bufferevent*>();
    qRegisterMetaType<PXMConsts::MESSAGE_TYPE>();
    qRegisterMetaType<QSharedPointer<Peers::BevWrapper>>();
    qRegisterMetaType<QSharedPointer<QString>>();

    QString username      = d_ptr->getUsername();
    QString localHostname = d_ptr->iniReader.getHostname(username);

    bool allowMoreThanOne = d_ptr->iniReader.checkAllowMoreThanOne();
    if (allowMoreThanOne) {
        d_ptr->presets.uuidNum = d_ptr->iniReader.getUUIDNumber();
        d_ptr->presets.uuid    = d_ptr->iniReader.getUUID(d_ptr->presets.uuidNum, allowMoreThanOne);
    } else {
        d_ptr->presets.uuid = d_ptr->iniReader.getUUID(0, allowMoreThanOne);
    }
    QString tmpDir = QDir::tempPath();
    d_ptr->lockFile.reset(new QLockFile(tmpDir + "/pxmessenger_" + d_ptr->presets.uuid.toString() + ".lock"));
    if (!allowMoreThanOne) {
        if (!d_ptr->lockFile->tryLock(100)) {
            QMessageBox msgBox(QMessageBox::Warning, qApp->applicationName(), QString("PXMessenger is already "
                                                                                      "running.\r\nOnly one instance "
                                                                                      "is allowed"));
            msgBox.exec();
            emit alreadyRunning();
            return -1;
        }
    } else {
        d_ptr->lockFile->lock();
    }

    d_ptr->logger = PXMConsole::Logger::getInstance();
    d_ptr->logger->setVerbosityLevel(d_ptr->iniReader.getVerbosity());
    d_ptr->logger->setLogStatus(d_ptr->iniReader.getLogActive());

    d_ptr->presets.username = localHostname.left(PXMConsts::MAX_HOSTNAME_LENGTH);
    d_ptr->setupHostname(d_ptr->presets.uuidNum, d_ptr->presets.username);
    d_ptr->presets.tcpPort      = d_ptr->iniReader.getPort("TCP");
    d_ptr->presets.udpPort      = d_ptr->iniReader.getPort("UDP");
    d_ptr->presets.windowSize   = d_ptr->iniReader.getWindowSize(QSize(700, 500));
    d_ptr->presets.mute         = d_ptr->iniReader.getMute();
    d_ptr->presets.preventFocus = d_ptr->iniReader.getFocus();
    d_ptr->presets.multicast    = d_ptr->iniReader.getMulticastAddress();
    d_ptr->presets.darkColor    = d_ptr->iniReader.getDarkColorScheme();

    if (d_ptr->presets.darkColor) {
        changeToDark();
    }
    QUuid globalChat = QUuid::createUuid();
    if (!(d_ptr->iniReader.getFont().isEmpty())) {
        QFont font;
        font.fromString(d_ptr->iniReader.getFont());
        qApp->setFont(font);
    }

    d_ptr->workerThread = new QThread(this);
    d_ptr->workerThread->setObjectName("WorkerThread");
    d_ptr->peerWorker =
        new PXMPeerWorker(nullptr, d_ptr->presets.username, d_ptr->presets.uuid, d_ptr->presets.multicast,
                          d_ptr->presets.tcpPort, d_ptr->presets.udpPort, globalChat);
    d_ptr->peerWorker->moveToThread(d_ptr->workerThread);
    // QObject::connect(d_ptr->workerThread, &QThread::started, d_ptr->peerWorker, &PXMPeerWorker::init);
    QObject::connect(d_ptr->workerThread, &QThread::finished, d_ptr->peerWorker, &PXMPeerWorker::deleteLater);
    d_ptr->window.reset(new PXMWindow(d_ptr->presets.username, d_ptr->presets.windowSize, d_ptr->presets.mute,
                                      d_ptr->presets.preventFocus, d_ptr->presets.uuid, globalChat));

    QObject::connect(d_ptr->peerWorker, &PXMPeerWorker::msgRecieved, d_ptr->window.data(),
                     &PXMWindow::printToTextBrowser, Qt::QueuedConnection);
    QObject::connect(d_ptr->peerWorker, &PXMPeerWorker::connectionStatusChange, d_ptr->window.data(),
                     &PXMWindow::setItalicsOnItem, Qt::QueuedConnection);
    QObject::connect(d_ptr->peerWorker, &PXMPeerWorker::newAuthedPeer, d_ptr->window.data(),
                     &PXMWindow::updateListWidget, Qt::QueuedConnection);
    QObject::connect(d_ptr->peerWorker, &PXMPeerWorker::warnBox,
                     [=](QString title, QString msg) { QMessageBox::warning(d_ptr->window.data(), title, msg); });
    QObject::connect(d_ptr->window.data(), SIGNAL(addMessageToPeer(QString, QUuid, QUuid, bool, bool)),
                     d_ptr->peerWorker, SLOT(addMessageToPeer(QString, QUuid, QUuid, bool, bool)),
                     Qt::QueuedConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::sendMsg, d_ptr->peerWorker, &PXMPeerWorker::sendMsgAccessor,
                     Qt::QueuedConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::sendUDP, d_ptr->peerWorker, &PXMPeerWorker::sendUDPAccessor,
                     Qt::QueuedConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::syncWithPeers, d_ptr->peerWorker, &PXMPeerWorker::beginPeerSync,
                     Qt::QueuedConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::printInfoToDebug, d_ptr->peerWorker,
                     &PXMPeerWorker::printInfoToDebug, Qt::QueuedConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::typing, d_ptr->peerWorker, &PXMPeerWorker::typing,
                     Qt::QueuedConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::endOfTextEntered, d_ptr->peerWorker,
                     &PXMPeerWorker::endOfTextEntered, Qt::QueuedConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::textEntered, d_ptr->peerWorker, &PXMPeerWorker::textEntered,
                     Qt::QueuedConnection);
    QObject::connect(d_ptr->peerWorker, &PXMPeerWorker::typingAlert, d_ptr->window.data(), &PXMWindow::typingAlert);
    QObject::connect(d_ptr->peerWorker, &PXMPeerWorker::textEnteredAlert, d_ptr->window.data(),
                     &PXMWindow::textEnteredAlert);
    QObject::connect(d_ptr->peerWorker, &PXMPeerWorker::endOfTextEnteredAlert, d_ptr->window.data(),
                     &PXMWindow::endOfTextEnteredAlert);

#ifdef QT_DEBUG
    qInfo().noquote() << "Built in debug mode";
#else
    qInfo().noquote() << "Built in release mode";
    splash.finish(d_ptr->window.data());
    qApp->processEvents();
    while (startupTimer.elapsed() < 1500) {
        qApp->processEvents();
        QThread::currentThread()->msleep(100);
    }
#endif
#ifdef _WIN32
    qInfo().noquote() << "Architecture:" << arch;
#endif
    qInfo().noquote() << "Our UUID:" << d_ptr->presets.uuid.toString();

    d_ptr->workerThread->start();

    d_ptr->window->show();

    return 0;
}
void PXMAgent::doneChkUpdt(const QString&)
{
#ifdef __WIN32
    if (!d_ptr->qupdater->getUpdateAvailable(d_ptr->address)) {
        qDebug() << "No update Available";
        this->postInit();
    } else {
        qDebug() << "Update Available";
    }

#endif
}

QByteArray PXMAgentPrivate::getUsername()
{
#ifdef _WIN32
    const size_t len = UNLEN + 1;
    char localHostname[len];
    TCHAR t_user[len];
    DWORD user_size = len;
    if (GetUserName(t_user, &user_size)) {
        wcstombs(localHostname, t_user, len);
        return QByteArray(localHostname);
    } else
        return QByteArray("user");
#elif __unix__
    struct passwd* user;
    user = getpwuid(getuid());
    if (!user)
        return QByteArray("user");
    else
        return QByteArray(user->pw_name);
#else
#error "implement username query"
#endif
}

int PXMAgentPrivate::setupHostname(const unsigned int uuidNum, QString& username)
{
    char computerHostname[256] = {};

    gethostname(computerHostname, sizeof computerHostname);

    if (uuidNum > 0) {
        char temp[3];
        sprintf(temp, "%d", uuidNum);
        username.append(QString::fromUtf8(temp));
    }
    username.append("@");
    username.append(QString::fromLocal8Bit(computerHostname).left(PXMConsts::MAX_COMPUTER_NAME_LENGTH));
    return 0;
}
