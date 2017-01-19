#ifndef PXMCONSOLE_H
#define PXMCONSOLE_H

#include <QMainWindow>
#include <QEvent>
#include <QTextEdit>

class QPushButton;
namespace PXMConsole {
const int DEBUG_WINDOW_HISTORY_LIMIT = 5000;
struct WindowPrivate;
class Window : public QMainWindow
{
    Q_OBJECT
    WindowPrivate *d_ptr;
public:
    explicit Window(QWidget *parent = 0);
    ~Window();
    QPushButton *pushButton;
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


#endif // PXMCONSOLE_H
