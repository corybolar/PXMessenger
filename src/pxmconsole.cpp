#include "pxmconsole.h"
#include <QStringBuilder>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QScrollBar>
#include <QPushButton>

using namespace PXMConsole;
struct PXMConsole::consoleImple {
    bool atMaximum = false;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QGridLayout *gridLayout_2;
    QScrollBar *sb;
    QLabel *verbosity;
};

QTextEdit * Window::textEdit = 0;
Window::Window(QWidget *parent) : QMainWindow(parent), pimpl__(new PXMConsole::consoleImple())
{
    this->setObjectName("Debug Console");
    this->setWindowTitle("Debug Console");
    pimpl__->centralwidget = new QWidget(this);
    pimpl__->centralwidget->setObjectName(QStringLiteral("centralwidget"));
    pimpl__->gridLayout_2 = new QGridLayout(pimpl__->centralwidget);
    pimpl__->gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
    pimpl__->gridLayout = new QGridLayout();
    pimpl__->gridLayout->setObjectName(QStringLiteral("gridLayout"));
    textEdit = new QTextEdit(pimpl__->centralwidget);
    textEdit->setObjectName(QStringLiteral("plainTextEdit"));
    textEdit->setReadOnly(true);
    textEdit->document()->setMaximumBlockCount(DEBUG_WINDOW_HISTORY_LIMIT);
    LoggerSingleton::getInstance()->setTextEdit(textEdit);
    pushButton = new QPushButton(pimpl__->centralwidget);
    pushButton->setText("Print Info");
    pushButton->setMaximumSize(QSize(250, 16777215));

    pimpl__->verbosity = new QLabel(pimpl__->centralwidget);
    pimpl__->verbosity->setText("Debug Verbosity: " % QString::number(LoggerSingleton::getVerbosityLevel()));
    pimpl__->gridLayout->addWidget(pimpl__->verbosity, 1, 2, 1, 2);

    pimpl__->gridLayout->addWidget(textEdit, 0, 0, 1, 4);
    pimpl__->gridLayout->addWidget(pushButton, 1, 0, 1, 2);

    pimpl__->gridLayout_2->addLayout(pimpl__->gridLayout, 0, 0, 1, 1);

    pimpl__->sb = Window::textEdit->verticalScrollBar();
    pimpl__->sb->setTracking(true);

    this->setCentralWidget(pimpl__->centralwidget);
    this->resize(1000, 300);
    pimpl__->sb->setValue(pimpl__->sb->maximum());
    pimpl__->atMaximum = true;
    QObject::connect(pimpl__->sb, &QAbstractSlider::valueChanged, this, &Window::adjustScrollBar);
    QObject::connect(pimpl__->sb, &QAbstractSlider::rangeChanged, this, &Window::rangeChanged);
}

Window::~Window()
{
    delete pimpl__;
    pimpl__ = 0;
}
void Window::adjustScrollBar(int i)
{
    if(i == pimpl__->sb->maximum())
        pimpl__->atMaximum = true;
    else
        pimpl__->atMaximum = false;
}
void Window::rangeChanged(int, int i2)
{
    if(pimpl__->atMaximum)
    pimpl__->sb->setValue(i2);
}
void Window::verbosityChanged()
{
    pimpl__->verbosity->setText("Debug Verbosity: " % QString::number(LoggerSingleton::getVerbosityLevel()));
}
