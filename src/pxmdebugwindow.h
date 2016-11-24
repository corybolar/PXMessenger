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

class PXMDebugWindow : public QMainWindow
{
    Q_OBJECT
    bool atMaximum;
public:
    explicit PXMDebugWindow(QWidget *parent = 0);
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QGridLayout *gridLayout_2;
    QPushButton *pushButton;
    QScrollBar *sb;
    static QTextBrowser *textEdit;
signals:

public slots:
private slots:
    void adjustScrollBar(int i);
    void rangeChanged(int i1, int i2);
};

#endif // PXMDEBUGWINDOW_H
