#include <QApplication>
#include <QDebug>
#include <QStringBuilder>
#include <QDateTime>

#include "pxmagent.h"
#include "pxmconsole.h"

void debugMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    using namespace PXMConsole;
    LoggerSingleton* logger = LoggerSingleton::getInstance();

    switch (logger->getVerbosityLevel()) {
        case 0:
            if (type == QtDebugMsg || type == QtWarningMsg)
                return;
            break;
        case 1:
            if (type == QtDebugMsg)
                return;
            break;
        default:
            break;
    }

    QByteArray localMsg = QByteArray();

#ifdef QT_DEBUG
    localMsg.reserve(128);
    QString filename = QString::fromUtf8(context.file);
#ifdef __WIN32
    filename = filename.right(filename.length() - filename.lastIndexOf(QLatin1Char('\\')) - 1);
#else
    filename = filename.right(filename.length() - filename.lastIndexOf(QLatin1Char('/')) - 1);
#endif
    filename.append(QLatin1Char(':') + QByteArray::number(context.line));
    filename.append(QString(PXMConsts::DEBUG_PADDING - filename.length(), QLatin1Char(' ')));
    localMsg = filename.toUtf8() % msg.toUtf8();
#else
    localMsg = msg.toLocal8Bit();
#endif

    QColor msgColor;
    switch (type) {
        case QtDebugMsg:
            localMsg.prepend(QByteArray::fromRawData("DEBUG: ", 7));
            msgColor = Qt::gray;
            break;
        case QtWarningMsg:
            localMsg.prepend(QByteArray::fromRawData("WARN:  ", 7));
            msgColor = Qt::darkYellow;
            break;
        case QtCriticalMsg:
            localMsg.prepend(QByteArray::fromRawData("CRIT:  ", 7));
            msgColor = Qt::red;
            break;
        case QtFatalMsg:
            localMsg.prepend(QByteArray::fromRawData("FATAL: ", 7));
            abort();
            break;
        case QtInfoMsg:
            localMsg.prepend(QByteArray::fromRawData("INFO:  ", 7));
            msgColor = QGuiApplication::palette().foreground().color();
            break;
    }
    localMsg.prepend(QDateTime::currentDateTime().time().toString(QStringLiteral("[hh:mm:ss:zzz] ")).toLatin1());
    localMsg.append(QChar('\n'));
    fprintf(stderr, "%s", localMsg.constData());
    if (Window::textEdit) {
        qApp->postEvent(logger, new AppendTextEvent(localMsg, msgColor), Qt::LowEventPriority);
    }
    if (logger->getLogStatus() && logger->logFile->isOpen()) {
        logger->logFile->write(localMsg.constData(), localMsg.length());
        logger->logFile->flush();
    }
}

int main(int argc, char** argv)
{
    qInstallMessageHandler(debugMessageOutput);

    QApplication app(argc, argv);

    app.setApplicationName("PXMessenger");
    app.setOrganizationName("PXMessenger");
    app.setOrganizationDomain("PXMessenger");
    app.setApplicationVersion("1.4.0");

    int result;
    {
        PXMAgent overlord;
        if (overlord.init()) {
            qCritical().noquote() << QStringLiteral("PXMInit failed");
            return -1;
        }

        result = app.exec();

        qInfo().noquote() << QStringLiteral("Exiting PXMessenger");
    }
    qInfo().noquote() << QStringLiteral("Successful Shutdown with code:") << result;

    return result;
}
