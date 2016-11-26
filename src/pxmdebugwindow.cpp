#include "pxmdebugwindow.h"

QTextBrowser * PXMDebugWindow::textEdit = 0;
PXMDebugWindow::PXMDebugWindow(QWidget *parent) : QMainWindow(parent)
{
    this->setObjectName("Debug Console");
    this->setWindowTitle("Debug Console");
    centralwidget = new QWidget(this);
    centralwidget->setObjectName(QStringLiteral("centralwidget"));
    gridLayout_2 = new QGridLayout(centralwidget);
    gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
    gridLayout = new QGridLayout();
    gridLayout->setObjectName(QStringLiteral("gridLayout"));
    textEdit = new QTextBrowser(centralwidget);
    textEdit->setObjectName(QStringLiteral("plainTextEdit"));
    textEdit->setReadOnly(true);
    pushButton = new QPushButton(centralwidget);
    pushButton->setText("Print Info");
    pushButton->setMaximumSize(QSize(250, 16777215));

    gridLayout->addWidget(textEdit, 0, 0, 1, 1);
    gridLayout->addWidget(pushButton, 1, 0, 1, 1);

    gridLayout_2->addLayout(gridLayout, 0, 0, 1, 1);

    sb = PXMDebugWindow::textEdit->verticalScrollBar();
    sb->setTracking(true);

    this->setCentralWidget(centralwidget);
    this->resize(1000, 300);
    sb->setValue(sb->maximum());
    QObject::connect(textEdit, SIGNAL(textChanged()), this, SLOT(textChanged()));
    QObject::connect(sb, SIGNAL(valueChanged(int)), this, SLOT(adjustScrollBar(int)));
    QObject::connect(sb, SIGNAL(rangeChanged(int,int)), this, SLOT(rangeChanged(int, int)));
}
void PXMDebugWindow::adjustScrollBar(int i)
{
    if(i == sb->maximum())
        this->atMaximum = true;
    else
        this->atMaximum = false;
}
void PXMDebugWindow::rangeChanged(int, int i2)
{
    if(this->atMaximum)
        sb->setValue(i2);
}
void PXMDebugWindow::xdebug(QString str)
{
    qDebug().noquote() << str;
}
void PXMDebugWindow::textChanged()
{
    if(PXMDebugWindow::textEdit->toPlainText().length() > DEBUG_WINDOW_HISTORY_LIMIT)
        PXMDebugWindow::textEdit->setText(PXMDebugWindow::textEdit->toPlainText().right(DEBUG_WINDOW_HISTORY_LIMIT));
}
