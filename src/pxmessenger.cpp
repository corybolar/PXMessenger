#include <QApplication>
#include <QDebug>
#include <QStringBuilder>

#include "pxmagent.h"
#include "pxmconsole.h"

static_assert(sizeof(uint8_t) == 1, "uint8_t not defined as 1 byte");
static_assert(sizeof(uint16_t) == 2, "uint16_t not defined as 2 bytes");
static_assert(sizeof(uint32_t) == 4, "uint32_t not defined as 4 bytes");

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

int main(int argc, char **argv)
{
    qInstallMessageHandler(debugMessageOutput);

    QApplication app (argc, argv);

    app.setApplicationName("PXMessenger");
    app.setOrganizationName("PXMessenger");
    app.setOrganizationDomain("PXMessenger");
    app.setApplicationVersion("1.4.0");

    PXMAgent overlord;
    if(overlord.init())
    {
        return -1;
    }

    int result = app.exec();

    qInfo() << "Exiting PXMessenger";

    return result;
}
