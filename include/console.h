/** @file pxmconsole.h
 * @brief Public header for pxmconsole.cpp
 *
 * PXMConsole is what handles all logging in PXMessenger as well as drive the
 * debug console available through the menu bar.  It needs to be used in the
 * main gui thread as it updates a textbox by default.
 */

#ifndef PXMCONSOLE_H
#define PXMCONSOLE_H

#include <QEvent>
#include <QMainWindow>
#include <QScopedPointer>
#include <QTextEdit>
#include <QFile>
#include <QDir>

class QPushButton;
namespace PXMConsole
{
const int HISTORY_LIMIT = 2000;
static QFile file;
struct WindowPrivate;
class Window : public QMainWindow
{
    Q_OBJECT
    QScopedPointer<WindowPrivate> d_ptr;

   public:
    explicit Window(QWidget* parent = 0);
    ~Window();
    QPushButton* pushButton;
    QPushButton* pushButton1;
    static QTextEdit* textEdit;
   public slots:
    void verbosityChanged();
   private slots:
    void adjustScrollBar(int i);
    void rangeChanged(int, int i2);
};

class AppendTextEvent : public QEvent
{
   public:
    AppendTextEvent(QString text, QColor color) : QEvent(static_cast<Type>(type)), text(text), textColor(color) {}
    QString getText() { return text; }
    QColor getColor() { return textColor; }
   private:
    static int type;
    QString text;
    QColor textColor;
};

class Logger : public QObject
{
   public:
    static Logger* getInstance()
    {
        if (!loggerInstance) {
            loggerInstance = new Logger;
#ifdef __WIN32
            loggerInstance->logFile.reset(new QFile(QDir::currentPath() + "/log.txt", loggerInstance));
#else
            loggerInstance->logFile.reset(new QFile(QDir::homePath() + "/.pxmessenger-log", loggerInstance));
#endif
        }

        return loggerInstance;
    }
    void setTextEdit(QTextEdit* textEdit) { logTextEdit = textEdit; }
    int getVerbosityLevel() { return verbosityLevel; }
    void setVerbosityLevel(int value) { verbosityLevel = value; }
    bool getLogStatus() { return logActive; }
    void setLogStatus(bool stat);
    QScopedPointer<QFile> logFile;

   private:
    Logger() : QObject() {}
    static Logger* loggerInstance;
    QTextEdit* logTextEdit = 0;
    int verbosityLevel     = 0;
    bool logActive         = 0;

   protected:
    virtual void customEvent(QEvent* event);
};
}

#endif  // PXMCONSOLE_H
