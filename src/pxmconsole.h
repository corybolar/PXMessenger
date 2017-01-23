#ifndef PXMCONSOLE_H
#define PXMCONSOLE_H

#include <QEvent>
#include <QMainWindow>
#include <QScopedPointer>
#include <QTextEdit>

class QPushButton;
namespace PXMConsole
{
const int HISTORY_LIMIT = 5000;
struct WindowPrivate;
class Window : public QMainWindow
{
    Q_OBJECT
    QScopedPointer<WindowPrivate> d_ptr;

   public:
    explicit Window(QWidget* parent = 0);
    ~Window();
    QPushButton* pushButton;
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
    AppendTextEvent(QString text, QColor color) :
      QEvent(static_cast<Type>(type)), text(text), textColor(color) {}
    QString getText() { return text; }
    QColor getColor() { return textColor; }
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
        if (!loggerInstance) {
            loggerInstance = new LoggerSingleton;
        }

        return loggerInstance;
    }
    void setTextEdit(QTextEdit* textEdit) { logTextEdit = textEdit; }
    static int getVerbosityLevel() { return verbosityLevel; }
    static void setVerbosityLevel(int value) { verbosityLevel = value; }
   private:
    LoggerSingleton() : QObject() {}
    static LoggerSingleton* loggerInstance;
    QTextEdit* logTextEdit = 0;
    static int verbosityLevel;

   protected:
    virtual void customEvent(QEvent* event);
};
}

#endif  // PXMCONSOLE_H
