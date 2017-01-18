#include <QApplication>
#include <QDir>
#include <QLockFile>
#include <QFontDatabase>
#include <QSplashScreen>
#include <QElapsedTimer>
#include <QStringBuilder>
#include <QDebug>
#include <QMessageBox>
#include <QPushButton>

#include "pxmmainwindow.h"
#include "pxmconsole.h"
#include "pxminireader.h"
#include "pxmpeers.h"
#include "pxmpeerworker.h"
#include <event2/bufferevent.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <lmcons.h>
#else
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

static_assert(sizeof(uint8_t) == 1, "uint8_t not defined as 1 byte");
static_assert(sizeof(uint16_t) == 2, "uint16_t not defined as 2 bytes");
static_assert(sizeof(uint32_t) == 4, "uint32_t not defined as 4 bytes");
Q_DECLARE_METATYPE(QSharedPointer<QString>)

int PXMConsole::AppendTextEvent::type = QEvent::registerEventType();
PXMConsole::LoggerSingleton* PXMConsole::LoggerSingleton::loggerInstance = nullptr;
int PXMConsole::LoggerSingleton::verbosityLevel = 0;

void debugMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    using namespace PXMConsole;
    LoggerSingleton *logger = LoggerSingleton::getInstance();

    switch (logger->getVerbosityLevel()) {
    case 0:
        if(type == QtDebugMsg || type == QtWarningMsg)
            return;
        break;
    case 1:
        if(type == QtDebugMsg)
            return;
    default:
        break;
    }

    QByteArray localMsg = QByteArray();

#ifdef QT_DEBUG
    localMsg.reserve(128);
    QString filename = QString::fromUtf8(context.file);
    filename = filename.right(filename.length() - filename.lastIndexOf(QChar('/')) - 1);
    filename.append(":" + QByteArray::number(context.line));
    filename.append(QString(PXMConsts::DEBUG_PADDING - filename.length(), QChar(' ')));
    localMsg = filename.toUtf8() % msg.toUtf8();
#else
    localMsg = msg.toLocal8Bit();
#endif

    QColor msgColor;
    switch(type) {
    case QtDebugMsg:
        fprintf(stderr, "DEBUG:    %s\n", localMsg.constData());
        msgColor = Qt::gray;
        break;
    case QtWarningMsg:
        fprintf(stderr, "WARNING:  %s\n", localMsg.constData());
        msgColor = Qt::darkYellow;
        break;
    case QtCriticalMsg:
        fprintf(stderr, "CRITICAL: %s\n", localMsg.constData());
        msgColor = Qt::red;
        break;
    case QtFatalMsg:
        fprintf(stderr, "%s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
        break;
    case QtInfoMsg:
        fprintf(stderr, "INFO:     %s\n", localMsg.constData());
        msgColor = QGuiApplication::palette().foreground().color();
        break;
    }
    if(Window::textEdit)
    {
        localMsg.append(QChar('\n'));
        qApp->postEvent(logger, new AppendTextEvent(localMsg, msgColor), Qt::LowEventPriority);
    }
}

QByteArray getUsername()
{
#ifdef _WIN32
    size_t len = UNLEN+1;
    char localHostname[len];
    TCHAR t_user[len];
    DWORD user_size = len;
    if(GetUserName(t_user, &user_size))
    {
        wcstombs(localHostname, t_user, len);
        return QByteArray(localHostname);
    }
    else
        return QByteArray("user");
#else
    struct passwd *user;
    user = getpwuid(getuid());
    if(!user)
        return QByteArray("user");
    else
        return QByteArray(user->pw_name);
#endif
}
QString setupHostname(int uuidNum, QString username)
{
    char computerHostname[256] = {};

    gethostname(computerHostname, sizeof computerHostname);

    if(uuidNum > 0)
    {
        char temp[3];
        sprintf(temp, "%d", uuidNum);
        username.append(QString::fromUtf8(temp));
    }
    username.append("@");
    username.append(QString::fromLocal8Bit(computerHostname).left(PXMConsts::MAX_COMPUTER_NAME));
    return username;
}

void connectPeerAndWindow(PXMPeerWorker* peerWorker, PXMWindow* window)
{
    QObject::connect(peerWorker, &PXMPeerWorker::printToTextBrowser, window, &PXMWindow::printToTextBrowser, Qt::QueuedConnection);
    QObject::connect(peerWorker, &PXMPeerWorker::setItalicsOnItem, window, &PXMWindow::setItalicsOnItem, Qt::QueuedConnection);
    QObject::connect(peerWorker, &PXMPeerWorker::updateListWidget, window, &PXMWindow::updateListWidget, Qt::QueuedConnection);
    QObject::connect(peerWorker, &PXMPeerWorker::warnBox, window, &PXMWindow::warnBox, Qt::AutoConnection);
    QObject::connect(window, &PXMWindow::addMessageToPeer, peerWorker, &PXMPeerWorker::addMessageToPeer, Qt::QueuedConnection);
    QObject::connect(window, &PXMWindow::sendMsg, peerWorker, &PXMPeerWorker::sendMsgAccessor, Qt::QueuedConnection);
    QObject::connect(window, &PXMWindow::sendUDP, peerWorker, &PXMPeerWorker::sendUDPAccessor, Qt::QueuedConnection);
    QObject::connect(window->debugWindow->pushButton, &QAbstractButton::clicked, peerWorker, &PXMPeerWorker::printInfoToDebug, Qt::QueuedConnection);
}

int main(int argc, char **argv)
{
    qInstallMessageHandler(debugMessageOutput);
#ifdef _WIN32
    qRegisterMetaType<intptr_t>("intptr_t");
#endif
    qRegisterMetaType<sockaddr_in>();
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<bufferevent*>();
    qRegisterMetaType<PXMConsts::MESSAGE_TYPE>();
    qRegisterMetaType<QSharedPointer<Peers::BevWrapper>>();
    qRegisterMetaType<QSharedPointer<QString>>();

    QApplication app (argc, argv);

#ifndef QT_DEBUG
    QPixmap splashImage(":/resources/resources/PXMessenger_wBackground.png");
    splashImage = splashImage.scaledToHeight(300);
    QSplashScreen splash(splashImage);
    splash.show();
    QElapsedTimer startupTimer;
    startupTimer.start();
    qApp->processEvents();
#endif
#ifdef _WIN32
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
    {
        printf("Failed WSAStartup error: %d", WSAGetLastError());
        return 1;
    }
#endif

    QApplication::setApplicationName("PXMessenger");
    QApplication::setOrganizationName("PXMessenger");
    QApplication::setOrganizationDomain("PXMessenger");
    QApplication::setApplicationVersion("1.4.0");

    PXMIniReader iniReader;
    initialSettings presets;

    QString username = getUsername();
    QString localHostname = iniReader.getHostname(username);

    bool allowMoreThanOne = iniReader.checkAllowMoreThanOne();
    QString tmpDir = QDir::tempPath();
    QLockFile lockFile(tmpDir + "/pxmessenger_" + username + ".lock");
    if(!allowMoreThanOne)
    {
        if(!lockFile.tryLock(100))
        {
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("You already have this app running."
                           "\r\nOnly one instance is allowed.");
            msgBox.exec();
            return 1;
        }
    }

    PXMConsole::LoggerSingleton *logger = PXMConsole::LoggerSingleton::getInstance();
    logger->setVerbosityLevel(iniReader.getVerbosity());
    presets.uuidNum = iniReader.getUUIDNumber();
    presets.uuid = iniReader.getUUID(presets.uuidNum, allowMoreThanOne);
    presets.username = setupHostname(presets.uuidNum, localHostname.left(PXMConsts::MAX_HOSTNAME_LENGTH));
    presets.tcpPort = iniReader.getPort("TCP");
    presets.udpPort = iniReader.getPort("UDP");
    presets.windowSize = iniReader.getWindowSize(QSize(700, 500));
    presets.mute = iniReader.getMute();
    presets.preventFocus = iniReader.getFocus();
    presets.multicast = iniReader.getMulticastAddress();
    QUuid globalChat = QUuid::createUuid();
    if(!(iniReader.getFont().isEmpty()))
    {
        QFont font;
        font.fromString(iniReader.getFont());
        app.setFont(font);
    }

    QThread *workerThread = new QThread;
    workerThread->setObjectName("WorkerThread");
    PXMPeerWorker *pWorker = new PXMPeerWorker(nullptr, presets.username,
                                               presets.uuid, presets.multicast,
                                               presets.tcpPort, presets.udpPort,
                                               globalChat);
    pWorker->moveToThread(workerThread);
    QObject::connect(workerThread, &QThread::started, pWorker, &PXMPeerWorker::currentThreadInit);
    QObject::connect(workerThread, &QThread::finished, pWorker, &PXMPeerWorker::deleteLater);
    PXMWindow *window = new PXMWindow(presets.username, presets.windowSize,
                                      presets.mute, presets.preventFocus, globalChat);
    connectPeerAndWindow(pWorker, window);
    workerThread->start();

#ifdef QT_DEBUG
    qInfo() << "Running in debug mode";
#else
    qInfo() << "Running in release mode";
    splash.finish(window);
    while(startupTimer.elapsed() < 1500)
    {
        qApp->processEvents();
        usleep(100.000);
    }
#endif

    qInfo() << "Our UUID:" << presets.uuid.toString();

    window->show();

    int result = app.exec();

    if(workerThread)
    {
        workerThread->quit();
        workerThread->wait(3000);
        qInfo() << "Shutdown of WorkerThread";
        delete workerThread;
    }

    if(window)
        delete window;

    iniReader.resetUUID(presets.uuidNum, presets.uuid);
    iniReader.setVerbosity(logger->getVerbosityLevel());

    qInfo() << "Exiting PXMessenger";

    return result;
}
