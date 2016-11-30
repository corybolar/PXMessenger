#include <QApplication>
#include <QDir>
#include <QLockFile>
#include <QFontDatabase>
#include <pxminireader.h>
#include <pxmdefinitions.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <pxmmainwindow.h>
#include <pxmdebugwindow.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

QMutex m;
void debugMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch(type) {
    case QtDebugMsg:
        fprintf(stderr, "%s\n", localMsg.constData());
        //fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    }
    if(PXMDebugWindow::textEdit != 0)
    {
        m.lock();
        /*
        QByteArray localMsg2;
        QString padding = "                    ";
        QString filename = QString::fromUtf8(context.file);
        filename = filename.right(filename.length() - filename.lastIndexOf("/") - 1);
        filename.append(":" + QByteArray::number(context.line));
        filename.append(padding.right(25 - filename.length()));
        localMsg2 = filename.toUtf8() + msg.toUtf8();
        */
        switch (type) {
        case QtDebugMsg:
            if(PXMDebugWindow::textEdit != 0)
                PXMDebugWindow::textEdit->appendPlainText(msg.toLocal8Bit());
            break;
        case QtWarningMsg:
            break;
        case QtCriticalMsg:
            break;
        case QtInfoMsg:
            break;
        case QtFatalMsg:
            abort();
        }
        m.unlock();
    }
}

int main(int argc, char **argv)
{
    qInstallMessageHandler(debugMessageOutput);
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
    QApplication::setApplicationVersion("1.1");

    MessIniReader iniReader;
    initialSettings presets;
    char localHostname[250];

    QFont font;
    if(!(iniReader.getFont().isEmpty()))
    {
        font.fromString(iniReader.getFont());
        app.setFont(font);
    }

#ifdef _WIN32
    char user[UNLEN+1];
    TCHAR t_user[UNLEN+1];
    DWORD user_size = UNLEN+1;
    if(GetUserName(t_user, &user_size))
    {
        wcstombs(user, t_user, UNLEN+1);
        strcpy(localHostname, user);
    }
#else
    struct passwd *user;
    user = getpwuid(getuid());
    strcpy(localHostname, user->pw_name);
#endif

    QString tmpDir = QDir::tempPath();
    QLockFile lockFile(tmpDir + "/pxmessenger" + QString::fromLatin1(localHostname) + ".lock");

    bool allowMoreThanOne = iniReader.checkAllowMoreThanOne();

    if(allowMoreThanOne)
    {
        presets.uuidNum = iniReader.getUUIDNumber();
    }
    else
    {
        if(!lockFile.tryLock(100)){
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("You already have this app running."
                           "\r\nOnly one instance is allowed.");
            msgBox.exec();
            return 1;
        }
    }
    presets.uuid = iniReader.getUUID(presets.uuidNum, allowMoreThanOne);
    presets.username = iniReader.getHostname(QString::fromLatin1(localHostname));
    presets.tcpPort = iniReader.getPort("TCP");
    presets.udpPort = iniReader.getPort("UDP");
    presets.windowSize = iniReader.getWindowSize(QSize(700, 500));
    presets.mute = iniReader.getMute();
    presets.preventFocus = iniReader.getFocus();

    PXMWindow window(presets);
    //window.show();

    int result = app.exec();

    iniReader.resetUUID(presets.uuidNum, presets.uuid);

    return result;
}
