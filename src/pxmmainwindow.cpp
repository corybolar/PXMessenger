#include <pxmmainwindow.h>
#include "ui_pxmmainwindow.h"
#include "ui_pxmaboutdialog.h"

using namespace PXMMessageViewer;

PXMWindow::PXMWindow(QString hostname, QSize windowSize, bool mute, bool focus, QUuid globalChat) :
    debugWindow(new PXMConsole::PXMConsoleWindow()), ui(new Ui::PXMWindow), localHostname(hostname), globalChatUuid(globalChat)
{
    setupGui();

    ui->focusCheckBox->setChecked(focus);
    ui->muteCheckBox->setChecked(mute);

    this->resize(windowSize);
}
PXMWindow::~PXMWindow()
{
    delete debugWindow;

    delete ui;

    qDebug() << "Shutdown of PXMWindow Successful";
}
void PXMWindow::setupMenuBar()
{
    QMenu *fileMenu;
    QAction *quitAction = new QAction("&Quit", this);

    fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(quitAction);
    QObject::connect(quitAction, &QAction::triggered, this, &PXMWindow::quitButtonClicked);

    QMenu *optionsMenu;
    QAction *settingsAction = new QAction("&Settings", this);
    QAction *bloomAction = new QAction("&Bloom", this);
    optionsMenu = menuBar()->addMenu("&Tools");
    optionsMenu->addAction(settingsAction);
    optionsMenu->addAction(bloomAction);
    QObject::connect(settingsAction, &QAction::triggered, this, &PXMWindow::settingsActionsSlot);
    QObject::connect(bloomAction, &QAction::triggered, this, &PXMWindow::bloomActionsSlot);

    QMenu *helpMenu;
    QAction *aboutAction = new QAction("&About", this);
    QAction *debugAction = new QAction("&Debug", this);
    helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(aboutAction);
    helpMenu->addAction(debugAction);
    QObject::connect(aboutAction, &QAction::triggered, this, &PXMWindow::aboutActionSlot);
    QObject::connect(debugAction, &QAction::triggered, this, &PXMWindow::debugActionSlot);
}

void PXMWindow::setupLabels()
{
    loadingLabel = new QLabel(ui->centralwidget);
    loadingLabel->setText("Select a Friend on the right to begin messaging!");
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->setGeometry(ui->stackedWidget->geometry());
    loadingLabel->show();
    resizeLabel(ui->stackedWidget->geometry());
}

void PXMWindow::resizeLabel(QRect size)
{
    if(loadingLabel)
    {
        loadingLabel->setGeometry(size);
    }
}

void PXMWindow::initListWidget()
{
    ui->messListWidget->setSortingEnabled(false);
    ui->messListWidget->insertItem(0, QStringLiteral("Global Chat"));
    QListWidgetItem *seperator = new QListWidgetItem(ui->messListWidget);
    seperator->setSizeHint(QSize(200, 10));
    seperator->setFlags(Qt::NoItemFlags);
    ui->messListWidget->insertItem(1, seperator);
    fsep = new QFrame(ui->messListWidget);
    fsep->setFrameStyle(QFrame::HLine | QFrame::Plain);
    fsep->setLineWidth(2);
    ui->messListWidget->setItemWidget(seperator, fsep);
    ui->messListWidget->item(0)->setData(Qt::UserRole, globalChatUuid);
    ui->stackedWidget->addWidget(new TextWidget(ui->stackedWidget, globalChatUuid));
}
void PXMWindow::createSystemTray()
{
    QIcon trayIcon(":/resources/resources/PXM_Icon.ico");
    this->setWindowIcon(trayIcon);

    messSystemTrayMenu = new QMenu(this);
    messSystemTrayExitAction = new QAction(tr("&Exit"), this);
    messSystemTrayMenu->addAction(messSystemTrayExitAction);
    QObject::connect(messSystemTrayExitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    messSystemTray = new QSystemTrayIcon(this);
    messSystemTray->setIcon(trayIcon);
    messSystemTray->setContextMenu(messSystemTrayMenu);
    messSystemTray->show();
}
void PXMWindow::setupGui()
{
    ui->setupUi(this);
    ui->messLineEdit->setText(localHostname);

    if (this->objectName().isEmpty())
        this->setObjectName(QStringLiteral("PXMessenger"));

    setupLabels();

    setupMenuBar();

    initListWidget();

    createSystemTray();

    connectGuiSignalsAndSlots();
}
void PXMWindow::connectGuiSignalsAndSlots()
{
    QObject::connect(ui->messSendButton, &QAbstractButton::clicked, this, &PXMWindow::sendButtonClicked);
    QObject::connect(ui->messQuitButton, &QAbstractButton::clicked, this, &PXMWindow::quitButtonClicked);
    QObject::connect(ui->messListWidget, &QListWidget::currentItemChanged, this, &PXMWindow::currentItemChanged);
    QObject::connect(ui->messTextEdit, &PXMTextEdit::returnPressed, this, &PXMWindow::sendButtonClicked);
    QObject::connect(messSystemTray, &QSystemTrayIcon::activated, this, &PXMWindow::systemTrayAction);
    QObject::connect(messSystemTray, &QObject::destroyed, messSystemTrayMenu, &QObject::deleteLater);
    QObject::connect(ui->messTextEdit, &QTextEdit::textChanged, this, &PXMWindow::textEditChanged);
    QObject::connect(messSystemTrayMenu, &QMenu::aboutToHide, messSystemTrayMenu, &QObject::deleteLater);;
    QObject::connect(ui->stackedWidget, &StackedWidget::resizeLabel, this, &PXMWindow::resizeLabel);
}
void PXMWindow::aboutActionSlot()
{
    PXMAboutDialog *about = new PXMAboutDialog(this, QIcon(":/resources/resources/PXM_Icon.ico"));
    QObject::connect(about, &PXMAboutDialog::finished, about, &PXMAboutDialog::deleteLater);
    about->open();
 }
void PXMWindow::settingsActionsSlot()
{
    PXMSettingsDialog *setD = new PXMSettingsDialog(this);
    QObject::connect(setD, &PXMSettingsDialog::nameChange, this, &PXMWindow::nameChange);
    QObject::connect(setD, &PXMSettingsDialog::verbosityChanged, debugWindow, &PXMConsole::PXMConsoleWindow::verbosityChanged);
    setD->open();
}

void PXMWindow::nameChange(QString hname)
{
    qInfo() << "Self Name Change";
    this->ui->messLineEdit->setText(hname);
    emit sendMsg(hname.toUtf8(), PXMConsts::MSG_NAME, QUuid());
}

void PXMWindow::bloomActionsSlot()
{
    QMessageBox box;
    box.setWindowTitle("Bloom");
    box.setText("This will resend our initial discovery "
                "packet to the multicast group.  If we "
                "have only found ourselves this is happening "
                "automatically on a 15 second timer.");
    QPushButton *bloomButton = box.addButton(tr("Bloom"), QMessageBox::ActionRole);
    box.addButton(QMessageBox::Abort);
    box.exec();
    if(box.clickedButton() == bloomButton)
    {
        qDebug() << "User triggered bloom";
        emit sendUDP("/discover");
    }
}

void PXMWindow::warnBox(QString title, QString msg)
{
    QMessageBox::warning(this, title, msg);
}

void PXMWindow::debugActionSlot()
{
    if(debugWindow)
    {
        debugWindow->show();
        debugWindow->setWindowState(Qt::WindowActive);
    }
}

void PXMWindow::setItalicsOnItem(QUuid uuid, bool italics)
{
    for(int i = 0; i < ui->messListWidget->count(); i++)
    {
        if(ui->messListWidget->item(i)->data(Qt::UserRole) == uuid)
        {
            QFont mfont = ui->messListWidget->item(i)->font();
            if(mfont.italic() == italics)
                return;
            mfont.setItalic(italics);
            ui->messListWidget->item(i)->setFont(mfont);
            QString changeInConnection;
            if(italics)
                changeInConnection = " disconnected";
            else
                changeInConnection = " reconnected";
            emit addMessageToPeer(ui->messListWidget->item(i)->text() % changeInConnection, uuid, false, true);
            return;
        }
    }
}
void PXMWindow::textEditChanged()
{
    if(ui->messTextEdit->toPlainText().length() > PXMConsts::TEXT_EDIT_MAX_LENGTH)
    {
        int diff = ui->messTextEdit->toPlainText().length() - PXMConsts::TEXT_EDIT_MAX_LENGTH;
        QString temp = ui->messTextEdit->toPlainText();
        temp.chop(diff);
        ui->messTextEdit->setText(temp);
        QTextCursor cursor(ui->messTextEdit->textCursor());
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        ui->messTextEdit->setTextCursor(cursor);
    }
}
void PXMWindow::systemTrayAction(QSystemTrayIcon::ActivationReason reason)
{
    if( ( ( reason == QSystemTrayIcon::DoubleClick ) || ( reason == QSystemTrayIcon::Trigger ) ) )
    {
        this->show();
        this->raise();
        this->setFocus();
        this->setWindowState(Qt::WindowActive);
    }
    return;
}
void PXMWindow::changeEvent(QEvent *event)
{
    switch(event->type())
    {
    case QEvent::WindowStateChange:
        if(this->isMinimized())
        {
            this->hide();
        }
        break;
    default:
        break;
    }
    QMainWindow::changeEvent(event);
}
void PXMWindow::currentItemChanged(QListWidgetItem *item1)
{
    //loadingLabel->setText("Loading history...");
    //ui->messTextBrowser->clear();
    //ui->messTextBrowser->setStyleSheet(QString());
    QUuid uuid = item1->data(Qt::UserRole).toString();
    loadingLabel->hide();

    //emit requestFullHistory(uuid);
    if(item1->background() != QGuiApplication::palette().base())
    {
        this->changeListItemColor(uuid, 0);
    }
    bool foundIt = false;
    for(int i = 0; i < ui->stackedWidget->count(); i++)
    {
        if(qobject_cast<TextWidget*>(ui->stackedWidget->widget(i))->getIdentifier() == uuid)
        {
            foundIt = true;
            ui->stackedWidget->setCurrentIndex(i);
            break;
        }
    }
    if(!foundIt)
    {
        //Some Exception here
    }
    //QScrollBar *sb = this->ui->messTextBrowser->verticalScrollBar();
    //sb->setValue(sb->maximum());
    return;
}
void PXMWindow::quitButtonClicked()
{
    this->close();
}
void PXMWindow::updateListWidget(QUuid uuid, QString hostname)
{
    ui->messListWidget->setUpdatesEnabled(false);
    for(int i = 2; i < ui->messListWidget->count(); i++)
    {
        if(ui->messListWidget->item(i)->data(Qt::UserRole).toUuid() == uuid)
        {
            if(ui->messListWidget->item(i)->text() != hostname)
            {
                emit addMessageToPeer(ui->messListWidget->item(i)->text() % " has changed their name to " % hostname, uuid, false, false);
                ui->messListWidget->item(i)->setText(hostname);
                QListWidgetItem *global = ui->messListWidget->takeItem(0);
                ui->messListWidget->sortItems();
                ui->messListWidget->insertItem(0, global);
            }
            setItalicsOnItem(uuid, 0);
            ui->messListWidget->setUpdatesEnabled(true);
            return;
        }
    }

    QListWidgetItem *global = ui->messListWidget->takeItem(0);
    QListWidgetItem *item = new QListWidgetItem(hostname, ui->messListWidget);

    item->setData(Qt::UserRole, uuid);
    ui->messListWidget->addItem(item);
    ui->stackedWidget->addWidget(new TextWidget(ui->stackedWidget, uuid));
    ui->messListWidget->sortItems();
    ui->messListWidget->insertItem(0, global);

    ui->messListWidget->setUpdatesEnabled(true);
}

void PXMWindow::closeEvent(QCloseEvent *event)
{
#ifndef __unix
    if(QMessageBox::No == QMessageBox::warning(this, "PXMessenger", "Are you sure you want to quit PXMessenger?", QMessageBox::Yes, QMessageBox::No))
    {
        event->ignore();
        return;
    }
#endif

    qDebug() << "trying to hide systray";
    messSystemTray->hide();
    qDebug() << "systray hidden";
    PXMIniReader iniReader;
    qDebug() << "creating iniReader";
    iniReader.setWindowSize(this->size());
    iniReader.setMute(ui->muteCheckBox->isChecked());
    iniReader.setFocus(ui->focusCheckBox->isChecked());
    qDebug() << "ini done";

    debugWindow->hide();
    qDebug() << "closeEvent calling event->accept()";
    event->accept();
}
int PXMWindow::removeBodyFormatting(QByteArray& str)
{
    QRegularExpression qre("((?<=<body) style.*?\"(?=>))");
    QRegularExpressionMatch qrem = qre.match(str);
    if(qrem.hasMatch())
    {
        int startoffset = qrem.capturedStart(1);
        int endoffset = qrem.capturedEnd(1);
        str.remove(startoffset, endoffset - startoffset);
        return 0;
    }
    else
        return -1;
}

int PXMWindow::sendButtonClicked()
{
    if(!ui->messListWidget->currentItem())
    {
        return -1;
    }
    QByteArray msg = ui->messTextEdit->toHtml().toUtf8();

    if(!(msg.isEmpty()))
    {
        if(removeBodyFormatting(msg))
        {
            qWarning() << "Bad Html";
            qWarning() << msg;
            return -1;
        }
        int index = ui->messListWidget->currentRow();
        QUuid uuidOfSelectedItem = ui->messListWidget->item(index)->data(Qt::UserRole).toString();

        if(uuidOfSelectedItem.isNull())
            return -1;

        if( ( uuidOfSelectedItem == globalChatUuid) )
        {
            emit sendMsg(msg, PXMConsts::MSG_GLOBAL, QUuid());
        }
        else
        {
            emit sendMsg(msg, PXMConsts::MSG_TEXT, uuidOfSelectedItem);
        }
        ui->messTextEdit->setText(QString());
        ui->messTextEdit->setStyleSheet(QString());
    }
    else
    {
        return -1;
    }
    return 0;
}
int PXMWindow::changeListItemColor(QUuid uuid, int style)
{
    for(int i = 0; i < ui->messListWidget->count(); i++)
    {
        if(ui->messListWidget->item(i)->data(Qt::UserRole) == uuid)
        {
            if(!style)
                ui->messListWidget->item(i)->setBackground(QGuiApplication::palette().base());
            else
            {
                //ui->messListWidget->item(i)->setBackground(QBrush(QColor(0xFF6495ED)));
                ui->messListWidget->item(i)->setBackground(QBrush(Qt::red));
            }
            break;
        }
    }
    return 0;
}
int PXMWindow::focusWindow()
{
    if(!(ui->muteCheckBox->isChecked()))
        QSound::play(":/resources/resources/message.wav");

    if(!(this->isMinimized()) && (this->windowState() != Qt::WindowActive))
    {
        qApp->alert(this, 0);
    }
    else if(this->isMinimized())
    {
        if(!this->ui->focusCheckBox->isChecked())
        {
            this->setWindowState(Qt::WindowActive);
            this->setFocus();
        }
        else
            this->setWindowState(Qt::WindowNoState);
        this->show();
        qApp->alert(this, 0);
    }
    return 0;
}
int PXMWindow::printToTextBrowser(QSharedPointer<QString> str, QUuid uuid, bool alert)
{
    if(str->isEmpty())
    {
        if(loadingLabel)
            loadingLabel->hide();
        return -1;
    }
    if(alert)
    {
        if(ui->messListWidget->currentItem())
        {
            if(ui->messListWidget->currentItem()->data(Qt::UserRole) != uuid)
            {
                changeListItemColor(uuid, 1);
            }
        }
        else
        {
            changeListItemColor(uuid, 1);
        }
        this->focusWindow();
    }

    ui->stackedWidget->append(*str.data(), uuid);

    if(loadingLabel)
        loadingLabel->hide();

    return 0;
}
