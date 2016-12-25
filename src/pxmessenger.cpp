#include <QApplication>
#include <QDir>
#include <QLockFile>
#include <QFontDatabase>
#include <QSharedPointer>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "pxmmainwindow.h"
#include "pxmdebugwindow.h"
#include "pxminireader.h"
#include "pxmdefinitions.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

static_assert(sizeof(uint8_t) == 1, "uint8_t not defined as 1 byte");
static_assert(sizeof(uint16_t) == 2, "uint16_t not defined as 2 bytes");
static_assert(sizeof(uint32_t) == 4, "uint32_t not defined as 4 bytes");

int AppendTextEvent::type = QEvent::registerEventType();
LoggerSingleton* LoggerSingleton::loggerInstance = nullptr;
int LoggerSingleton::verbosityLevel = 0;

void debugMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    using namespace DebugMessageColors;
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

    QByteArray localMsg;
    QString htmlMessage;
#ifdef QT_DEBUG
    QString filename = QString::fromUtf8(context.file);
    filename = filename.right(filename.length() - filename.lastIndexOf(QChar('/')) - 1);
    filename.append(":" + QByteArray::number(context.line));
    htmlMessage.append(filename);
    for(int i = 0; i < PXMConsts::DEBUG_PADDING - filename.length();i++)
    {
        htmlMessage.append(QStringLiteral("&nbsp;"));
    }
    filename.append(QString(PXMConsts::DEBUG_PADDING - filename.length(), QChar(' ')));
    localMsg = filename.toUtf8() % msg.toUtf8();
    htmlMessage = htmlMessage % msg.toUtf8();
#else
    localMsg = msg.toLocal8Bit();
    htmlMessage = msg.toLocal8Bit();
#endif
    for(int i = 0; i < htmlMessage.length(); i++)
    {
        switch(htmlMessage.at(i).toLatin1())
        {
        case '\n':
            htmlMessage.replace(i, 1, "<br>");
            break;
        case '\r':
            htmlMessage.replace(i, 1, "<br>");
            break;
        default:
            break;
        }
    }
    switch(type) {
    case QtDebugMsg:
        fprintf(stderr, "%s\n", localMsg.constData());
        htmlMessage = debugHtml % htmlMessage % endHtml;
        break;
    case QtWarningMsg:
        fprintf(stderr, "%s\n", localMsg.constData());
        htmlMessage = warningHtml % htmlMessage % endHtml;
        break;
    case QtCriticalMsg:
        fprintf(stderr, "%s\n", localMsg.constData());
        htmlMessage = criticalHtml % htmlMessage % endHtml;
        break;
    case QtFatalMsg:
        fprintf(stderr, "%s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
        break;
    case QtInfoMsg:
        fprintf(stderr, "%s\n", localMsg.constData());
        htmlMessage = htmlMessage % endHtml;
        break;
    }
    if(PXMDebugWindow::textEdit != 0)
    {
        qApp->postEvent(logger, new AppendTextEvent(htmlMessage), Qt::LowEventPriority);
    }
}

char* getHostname()
{
#ifdef _WIN32
    size_t len = UNLEN+1;
    char* localHostname = new char[len];
    memset(localHostname, 0, len);
    TCHAR t_user[len];
    DWORD user_size = len;
    if(GetUserName(t_user, &user_size))
        wcstombs(localHostname, t_user, len);
    else
        strcpy(localHostname, "user");
#else
    size_t len = sysconf(_SC_GETPW_R_SIZE_MAX);
    char* localHostname = new char[len];
    memset(localHostname, 0, len);
    struct passwd *user;
    user = getpwuid(getuid());
    if(!user)
        strcpy(localHostname, "user");
    else
        strcpy(localHostname, user->pw_name);
#endif

    return localHostname;
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

    QApplication app (argc, argv);

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
    QApplication::setApplicationVersion("1.3.0");

    MessIniReader iniReader;
    initialSettings presets;

    QFont font;
    if(!(iniReader.getFont().isEmpty()))
    {
        font.fromString(iniReader.getFont());
        app.setFont(font);
    }

    char* localHostname = getHostname();

    QString tmpDir = QDir::tempPath();
    QLockFile lockFile(tmpDir + "/pxmessenger" + QString::fromLatin1(localHostname) + ".lock");

    bool allowMoreThanOne = iniReader.checkAllowMoreThanOne();

    if(allowMoreThanOne)
    {
        presets.uuidNum = iniReader.getUUIDNumber();
    }
    else
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

    LoggerSingleton *logger = LoggerSingleton::getInstance();
    logger->setVerbosityLevel(iniReader.getVerbosity());
    presets.uuid = iniReader.getUUID(presets.uuidNum, allowMoreThanOne);
    presets.username = iniReader.getHostname(QString::fromLatin1(localHostname));
    presets.tcpPort = iniReader.getPort("TCP");
    presets.udpPort = iniReader.getPort("UDP");
    presets.windowSize = iniReader.getWindowSize(QSize(700, 500));
    presets.mute = iniReader.getMute();
    presets.preventFocus = iniReader.getFocus();
    presets.multicast = iniReader.getMulticastAddress();

    int result;
    {
        PXMWindow window(presets);

#ifdef QT_DEBUG
        qInfo() << "Running in debug mode";
#else
        qInfo() << "Running in release mode";
#endif

        window.startThreadsAndShow();

        result = app.exec();
    }

    iniReader.resetUUID(presets.uuidNum, presets.uuid);
    iniReader.setVerbosity(logger->getVerbosityLevel());

    delete[] localHostname;

    qDebug() << "Exiting PXMessenger";

    return result;
}
