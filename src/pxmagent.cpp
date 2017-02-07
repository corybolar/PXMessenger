#include <pxmagent.h>
#include <pxmconsole.h>
#include <pxminireader.h>
#include <pxmmainwindow.h>
#include <pxmpeerworker.h>
#include <pxmconsole.h>

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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <lmcons.h>
#elif __unix__
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#else
#error "include headers for querying username"
#endif

Q_DECLARE_METATYPE(QSharedPointer<QString>)

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
    initialSettings()
        : uuid(QUuid()),
          windowSize(QSize(800, 600)),
          username(QString()),
          multicast(QString()),
          uuidNum(0),
          tcpPort(0),
          udpPort(0),
          mute(false),
          preventFocus(false)
    {
    }
};

struct PXMAgentPrivate {
    QScopedPointer<PXMWindow> window;
    PXMPeerWorker* peerWorker;
    PXMIniReader iniReader;
    PXMConsole::Logger* logger;
    QScopedPointer<QLockFile> lockFile;

    initialSettings presets;

    QThread* workerThread = nullptr;

    // Functions
    QByteArray getUsername();
    int setupHostname(const unsigned int uuidNum, QString& username);
};

PXMAgent::PXMAgent(QObject* parent) : QObject(parent), d_ptr(new PXMAgentPrivate)
{
}

PXMAgent::~PXMAgent()
{
    if (d_ptr->workerThread) {
        d_ptr->workerThread->quit();
        d_ptr->workerThread->wait(3000);
        qInfo() << "Shutdown of WorkerThread";
    }

    d_ptr->iniReader.resetUUID(d_ptr->presets.uuidNum, d_ptr->presets.uuid);
}

int PXMAgent::init()
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
            throw("Failed WSAStartup error: " + WSAGetLastError());
        }
    } catch (const char* msg) {
        QMessageBox msgBox(QMessageBox::Critical, "WSAStartup Error", QString::fromUtf8(msg));
        msgBox.exec();
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
    QString tmpDir        = QDir::tempPath();
    d_ptr->lockFile.reset(new QLockFile(tmpDir + "/pxmessenger_" + username + ".lock"));
    if (!allowMoreThanOne) {
        if (!d_ptr->lockFile->tryLock(100)) {
            QMessageBox msgBox(QMessageBox::Warning, qApp->applicationName(),
                               "PXMessenger is already "
                               "running.\r\nOnly one instance "
                               "is allowed");
            msgBox.exec();
            return -1;
        }
    }

    d_ptr->logger = PXMConsole::Logger::getInstance();
    d_ptr->logger->setVerbosityLevel(d_ptr->iniReader.getVerbosity());
    d_ptr->logger->setLogStatus(d_ptr->iniReader.getLogActive());

    d_ptr->presets.uuidNum  = d_ptr->iniReader.getUUIDNumber();
    d_ptr->presets.uuid     = d_ptr->iniReader.getUUID(d_ptr->presets.uuidNum, allowMoreThanOne);
    d_ptr->presets.username = localHostname.left(PXMConsts::MAX_HOSTNAME_LENGTH);
    d_ptr->setupHostname(d_ptr->presets.uuidNum, d_ptr->presets.username);
    d_ptr->presets.tcpPort      = d_ptr->iniReader.getPort("TCP");
    d_ptr->presets.udpPort      = d_ptr->iniReader.getPort("UDP");
    d_ptr->presets.windowSize   = d_ptr->iniReader.getWindowSize(QSize(700, 500));
    d_ptr->presets.mute         = d_ptr->iniReader.getMute();
    d_ptr->presets.preventFocus = d_ptr->iniReader.getFocus();
    d_ptr->presets.multicast    = d_ptr->iniReader.getMulticastAddress();
    QUuid globalChat            = QUuid::createUuid();
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
    QObject::connect(d_ptr->workerThread, &QThread::started, d_ptr->peerWorker, &PXMPeerWorker::currentThreadInit);
    QObject::connect(d_ptr->workerThread, &QThread::finished, d_ptr->peerWorker, &PXMPeerWorker::deleteLater);
    d_ptr->window.reset(new PXMWindow(d_ptr->presets.username, d_ptr->presets.windowSize, d_ptr->presets.mute,
                                      d_ptr->presets.preventFocus, globalChat));

    QObject::connect(d_ptr->peerWorker, &PXMPeerWorker::printToTextBrowser, d_ptr->window.data(),
                     &PXMWindow::printToTextBrowser, Qt::QueuedConnection);
    QObject::connect(d_ptr->peerWorker, &PXMPeerWorker::setItalicsOnItem, d_ptr->window.data(),
                     &PXMWindow::setItalicsOnItem, Qt::QueuedConnection);
    QObject::connect(d_ptr->peerWorker, &PXMPeerWorker::updateListWidget, d_ptr->window.data(),
                     &PXMWindow::updateListWidget, Qt::QueuedConnection);
    QObject::connect(d_ptr->peerWorker, &PXMPeerWorker::warnBox, d_ptr->window.data(), &PXMWindow::warnBox,
                     Qt::AutoConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::addMessageToPeer, d_ptr->peerWorker,
                     &PXMPeerWorker::addMessageToPeer, Qt::QueuedConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::sendMsg, d_ptr->peerWorker, &PXMPeerWorker::sendMsgAccessor,
                     Qt::QueuedConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::sendUDP, d_ptr->peerWorker, &PXMPeerWorker::sendUDPAccessor,
                     Qt::QueuedConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::syncWithPeers, d_ptr->peerWorker, &PXMPeerWorker::beginSync,
                     Qt::QueuedConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::printInfoToDebug, d_ptr->peerWorker,
                     &PXMPeerWorker::printInfoToDebug, Qt::QueuedConnection);
    d_ptr->workerThread->start();

#ifdef QT_DEBUG
    qInfo() << "Built in debug mode";
    qInfo() << "Our UUID:" << d_ptr->presets.uuid.toString();
#else
    qInfo() << "Built in release mode";
    qInfo() << "Our UUID:" << d_ptr->presets.uuid.toString();
    splash.finish(d_ptr->window.data());
    qApp->processEvents();
    while (startupTimer.elapsed() < 1500) {
        qApp->processEvents();
        QThread::currentThread()->msleep(100);
    }
#endif

    d_ptr->window->show();
    return 0;
}

QByteArray PXMAgentPrivate::getUsername()
{
#ifdef _WIN32
    size_t len = UNLEN + 1;
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
    username.append(QString::fromLocal8Bit(computerHostname).left(PXMConsts::MAX_COMPUTER_NAME));
    return 0;
}
