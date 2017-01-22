#include "pxmconsole.h"
#include <QStringBuilder>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QScrollBar>
#include <QPushButton>

using namespace PXMConsole;
struct PXMConsole::WindowPrivate{
    bool atMaximum = false;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QGridLayout *gridLayout_2;
    QScrollBar *sb;
    QLabel *verbosity;
};

QTextEdit * Window::textEdit = 0;
int PXMConsole::AppendTextEvent::type = QEvent::registerEventType();
PXMConsole::LoggerSingleton* PXMConsole::LoggerSingleton::loggerInstance = nullptr;
int PXMConsole::LoggerSingleton::verbosityLevel = 0;
Window::Window(QWidget *parent) : QMainWindow(parent), d_ptr(new PXMConsole::WindowPrivate())
{
    this->setObjectName("Debug Console");
    this->setWindowTitle("Debug Console");
    d_ptr->centralwidget = new QWidget(this);
    d_ptr->centralwidget->setObjectName(QStringLiteral("centralwidget"));
    d_ptr->gridLayout_2 = new QGridLayout(d_ptr->centralwidget);
    d_ptr->gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
    d_ptr->gridLayout = new QGridLayout();
    d_ptr->gridLayout->setObjectName(QStringLiteral("gridLayout"));
    textEdit = new QTextEdit(d_ptr->centralwidget);
    textEdit->setObjectName(QStringLiteral("plainTextEdit"));
    textEdit->setReadOnly(true);
    textEdit->document()->setMaximumBlockCount(DEBUG_WINDOW_HISTORY_LIMIT);
    LoggerSingleton::getInstance()->setTextEdit(textEdit);
    pushButton = new QPushButton(d_ptr->centralwidget);
    pushButton->setText("Print Info");
    pushButton->setMaximumSize(QSize(250, 16777215));

    d_ptr->verbosity = new QLabel(d_ptr->centralwidget);
    d_ptr->verbosity->setText("Debug Verbosity: " % QString::number(LoggerSingleton::getVerbosityLevel()));
    d_ptr->gridLayout->addWidget(d_ptr->verbosity, 1, 2, 1, 2);

    d_ptr->gridLayout->addWidget(textEdit, 0, 0, 1, 4);
    d_ptr->gridLayout->addWidget(pushButton, 1, 0, 1, 2);

    d_ptr->gridLayout_2->addLayout(d_ptr->gridLayout, 0, 0, 1, 1);

    d_ptr->sb = Window::textEdit->verticalScrollBar();
    d_ptr->sb->setTracking(true);

    this->setCentralWidget(d_ptr->centralwidget);
    this->resize(1000, 300);
    d_ptr->sb->setValue(d_ptr->sb->maximum());
    d_ptr->atMaximum = true;
    QObject::connect(d_ptr->sb, &QAbstractSlider::valueChanged, this, &Window::adjustScrollBar);
    QObject::connect(d_ptr->sb, &QAbstractSlider::rangeChanged, this, &Window::rangeChanged);
}

Window::~Window()
{
}
void Window::adjustScrollBar(int i)
{
    if(i == d_ptr->sb->maximum())
        d_ptr->atMaximum = true;
    else
        d_ptr->atMaximum = false;
}
void Window::rangeChanged(int, int i2)
{
    if(d_ptr->atMaximum)
    d_ptr->sb->setValue(i2);
}
void Window::verbosityChanged()
{
    d_ptr->verbosity->setText("Debug Verbosity: " % QString::number(LoggerSingleton::getVerbosityLevel()));
}
