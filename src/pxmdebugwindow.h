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

#define DEBUG_WINDOW_HISTORY_LIMIT 5000

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
    static QPlainTextEdit *textEdit;
private slots:
    void adjustScrollBar(int i);
    void rangeChanged(int, int i2);
};

class AppendTextEvent : public QEvent
{
public:
    AppendTextEvent(QString text) : QEvent((Type)type),text(text) {}
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
    void setTextEdit(QPlainTextEdit *textEdit)
    {
            logTextEdit = textEdit;
    }
private:
    LoggerSingleton() : QObject() {}
    static LoggerSingleton* loggerInstance;
    QPlainTextEdit *logTextEdit = 0;
protected:
    virtual void customEvent(QEvent *event)
    {
        AppendTextEvent* text = dynamic_cast<AppendTextEvent*>(event);
        if(text)
        {
            logTextEdit->appendPlainText(text->getText());
            event->accept();
        }
        else
            QObject::customEvent(event);
    }
};

#endif // PXMDEBUGWINDOW_H
