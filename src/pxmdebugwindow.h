#ifndef PXMDEBUGWINDOW_H
#define PXMDEBUGWINDOW_H

#include <QWidget>
#include <QMainWindow>
#include <QTextBrowser>
#include <QGridLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QDebug>
#include <QDialog>
#include <QStringBuilder>

namespace DebugMessageColors{
const QString debugHtml = "<font color=\"Orange\">";
const QString infoHtml = "";
const QString criticalHtml = "<font color=\"Red\">";
const QString warningHtml = "<font color=\"Red\">";
const QString endHtml = "</font><br>";
}

const int DEBUG_WINDOW_HISTORY_LIMIT = 5000;

class PXMDebugWindow : public QMainWindow
{
    Q_OBJECT
    bool atMaximum = false;
public:
    explicit PXMDebugWindow(QWidget *parent = 0);
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QGridLayout *gridLayout_2;
    QPushButton *pushButton;
    QScrollBar *sb;
    static QTextEdit *textEdit;
private slots:
    void adjustScrollBar(int i);
    void rangeChanged(int, int i2);
};

class AppendTextEvent : public QEvent
{
public:
    AppendTextEvent(QString text) : QEvent((Type)type),
        text(text) {}
    QString getText()
    {
        return text;
    }

private:
    static int type;
    QString text;
};

class LoggerSingleton : public QObject
{
public:
    static LoggerSingleton* getInstance()
    {
        if(!loggerInstance)
        {
            loggerInstance = new LoggerSingleton;
        }

        return loggerInstance;
    }
    void setTextEdit(QTextEdit *textEdit)
    {
            logTextEdit = textEdit;
    }
    static int getVerbosityLevel() {return verbosityLevel;}

    static void setVerbosityLevel(int value) {verbosityLevel = value;}

private:
    LoggerSingleton() : QObject() {}
    static LoggerSingleton* loggerInstance;
    QTextEdit *logTextEdit = 0;
    static int verbosityLevel;
protected:
    virtual void customEvent(QEvent *event)
    {
        AppendTextEvent* text = dynamic_cast<AppendTextEvent*>(event);
        if(text)
        {
            logTextEdit->insertHtml(text->getText());
            event->accept();
        }
        else
            QObject::customEvent(event);
    }
};

#endif // PXMDEBUGWINDOW_H
