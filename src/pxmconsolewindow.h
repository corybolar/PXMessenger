#ifndef PXMCONSOLEWINDOW_H
#define PXMCONSOLEWINDOW_H

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
#include <QLabel>

namespace PXMConsole {
const int DEBUG_WINDOW_HISTORY_LIMIT = 5000;
const QByteArray debugHtml = "<font color=\"Grey\">";
const QByteArray infoHtml = "";
const QByteArray criticalHtml = "<font color=\"Red\">";
const QByteArray warningHtml = "<font color=\"Orange\">";
const QByteArray endHtml = "</font><br>";

class PXMConsoleWindow : public QMainWindow
{
    Q_OBJECT
    bool atMaximum = false;
public:
    explicit PXMConsoleWindow(QWidget *parent = 0);
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QGridLayout *gridLayout_2;
    QPushButton *pushButton;
    QScrollBar *sb;
    QLabel *verbosity;
    static QTextEdit *textEdit;
public slots:
    void verbosityChanged();
private slots:
    void adjustScrollBar(int i);
    void rangeChanged(int, int i2);
};

class AppendTextEvent : public QEvent
{
public:
    AppendTextEvent(QString text, QColor color) : QEvent((Type)type),
        text(text), textColor(color) {}
    QString getText() {return text;}
    QColor getColor() {return textColor;}

private:
    static int type;
    QString text;
    QColor textColor;
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
            logTextEdit->setTextColor(text->getColor());
            logTextEdit->insertPlainText(text->getText());
            event->accept();
        }
        else
            QObject::customEvent(event);
    }
};
}


#endif // PXMCONSOLEWINDOW_H
