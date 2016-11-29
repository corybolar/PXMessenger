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
signals:

public slots:
private slots:
    void adjustScrollBar(int i);
    void rangeChanged(int, int i2);
    void xdebug(QString str);
};

#endif // PXMDEBUGWINDOW_H
