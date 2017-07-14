#include "pxmmainwindow.h"

#ifdef _WIN32
#include <lmcons.h>
#include <windows.h>
#else
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDebug>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QSound>
#include <QStringBuilder>
#include <QScopedPointer>
#include <QDir>
#include <QTextEdit>
#include <QKeyEvent>
#include <QStyleFactory>

#include "pxmconsole.h"
#include "pxmagent.h"
#include "pxminireader.h"
#include "ui_pxmaboutdialog.h"
#include "ui_pxmmainwindow.h"
#include "ui_pxmsettingsdialog.h"
#include "ui_manualconnect.h"

using namespace PXMMessageViewer;

class PXMSettingsDialogPrivate
{
   public:
    QFont iniFont;
    QString hostname;
    QString multicastAddress;
    int tcpPort;
    int udpPort;
    int fontSize;
    bool AllowMoreThanOneInstance;
    bool darkColor;
};

PXMWindow::PXMWindow(QString hostname,
                     QSize windowSize,
                     bool mute,
                     bool focus,
                     QUuid local,
                     QUuid globalChat,
                     QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::PXMWindow),
      localHostname(hostname),
      localUuid(local),
      globalChatUuid(globalChat),
      debugWindow(new PXMConsole::Window())
{
    srand(static_cast<unsigned int>(time(NULL)));
    textColorsNext = rand() % textColors.length();
    setupGui();

    ui->focusCheckBox->setChecked(focus);
    ui->muteCheckBox->setChecked(mute);

    this->textEnteredTimer = new QTimer();
    textEnteredTimer->setInterval(500);
    QObject::connect(textEnteredTimer, &QTimer::timeout, this, &PXMWindow::textEnteredCallback);
    this->resize(windowSize);

    // QObject::connect(ui->textEdit, &QTextEdit::textChanged, this, &PXMWindow::typing);
    // QObject::connect(ui->textEdit, &QTextEdit::textChanged, this, &PXMWindow::textEntered);
}
PXMWindow::~PXMWindow()
{
    qDebug() << "Shutdown of PXMWindow Successful";
}
void PXMWindow::setupMenuBar()
{
    QMenu* fileMenu;

    fileMenu = menuBar()->addMenu("&File");
    // QAction* manualConnect = new QAction("&Connect to", this);
    // fileMenu->addAction(manualConnect);
    // QObject::connect(manualConnect, &QAction::triggered, this, &PXMWindow::manualConnect);
    QAction* quitAction = new QAction("&Quit", this);
    fileMenu->addAction(quitAction);
    QObject::connect(quitAction, &QAction::triggered, this, &PXMWindow::quitButtonClicked);

    QMenu* optionsMenu;
    optionsMenu          = menuBar()->addMenu("&Tools");
    QAction* bloomAction = new QAction("&Bloom", this);
    optionsMenu->addAction(bloomAction);
    QObject::connect(bloomAction, &QAction::triggered, this, &PXMWindow::bloomActionsSlot);
    QAction* syncAction = new QAction("&Sync", this);
    optionsMenu->addAction(syncAction);
    QObject::connect(syncAction, &QAction::triggered, this, &PXMWindow::syncActionsSlot);
    QAction* settingsAction = new QAction("&Settings", this);
    optionsMenu->addAction(settingsAction);
    QObject::connect(settingsAction, &QAction::triggered, this, &PXMWindow::settingsActionsSlot);

    QMenu* helpMenu;
    helpMenu             = menuBar()->addMenu("&Help");
    QAction* aboutAction = new QAction("&About", this);
    helpMenu->addAction(aboutAction);
    QObject::connect(aboutAction, &QAction::triggered, this, &PXMWindow::aboutActionSlot);
    QAction* debugAction = new QAction("&Console", this);
    helpMenu->addAction(debugAction);
    QObject::connect(debugAction, &QAction::triggered, this, &PXMWindow::debugActionSlot);
}

void PXMWindow::initListWidget()
{
    ui->listWidget->setSortingEnabled(false);
    ui->listWidget->insertItem(0, QStringLiteral("Global Chat"));
    QListWidgetItem* seperator = new QListWidgetItem(ui->listWidget);
    seperator->setSizeHint(QSize(200, 10));
    seperator->setFlags(Qt::NoItemFlags);
    ui->listWidget->insertItem(1, seperator);
    fsep = new QFrame(ui->listWidget);
    fsep->setFrameStyle(QFrame::HLine | QFrame::Plain);
    fsep->setLineWidth(2);
    ui->listWidget->setItemWidget(seperator, fsep);
    ui->listWidget->item(0)->setData(Qt::UserRole, globalChatUuid);
    ui->stackedWidget->addWidget(new TextWidget(ui->stackedWidget, globalChatUuid));
}
void PXMWindow::createSystemTray()
{
    QIcon trayIcon(":/resources/PXM_Icon.ico");
    this->setWindowIcon(trayIcon);

    sysTrayMenu              = new QMenu(this);
    messSystemTrayExitAction = new QAction(tr("&Exit"), this);
    sysTrayMenu->addAction(messSystemTrayExitAction);
    QObject::connect(messSystemTrayExitAction, &QAction::triggered, this, &PXMWindow::quitButtonClicked);

    sysTray = new QSystemTrayIcon(this);
    sysTray->setIcon(trayIcon);
    sysTray->setContextMenu(sysTrayMenu);
    sysTray->show();
}
void PXMWindow::setupGui()
{
    ui->setupUi(this);
    ui->lineEdit->setText(localHostname);

    if (this->objectName().isEmpty()) {
        this->setObjectName(QStringLiteral("PXMessenger"));
    }

    setupMenuBar();

    initListWidget();

    createSystemTray();

    QFont f = ui->toolButton->font();
    f.setBold(true);
    ui->toolButton->setFont(f);
    f = ui->toolButton_4->font();
    f.setItalic(true);
    ui->toolButton_4->setFont(f);
    f = ui->toolButton_5->font();
    f.setUnderline(true);
    ui->toolButton_5->setFont(f);

    QPixmap pm_sys = QPixmap(ui->comboBox->height(), ui->comboBox->height());
    pm_sys.fill(qApp->palette().text().color());
    ui->comboBox->addItem(QIcon(pm_sys), "System", qApp->palette().text().color());

    QStringList colorNames = QColor::colorNames();
    for (auto& itr : colorNames) {
        QPixmap pm = QPixmap(ui->comboBox->height(), ui->comboBox->height());
        pm.fill(QColor(itr));
        ui->comboBox->addItem(QIcon(pm), itr, itr);
    }
    ui->comboBox->setCurrentIndex(0);

    connectGuiSignalsAndSlots();
}
void PXMWindow::connectGuiSignalsAndSlots()
{
    QObject::connect(ui->sendButton, &QAbstractButton::clicked, this, &PXMWindow::sendButtonClicked);
    QObject::connect(ui->quitButton, &QAbstractButton::clicked, this, &PXMWindow::quitButtonClicked);
    QObject::connect(ui->listWidget, &QListWidget::currentItemChanged, this, &PXMWindow::currentItemChanged);
    QObject::connect(ui->textEdit, &PXMTextEdit::returnPressed, this, &PXMWindow::sendButtonClicked);
    QObject::connect(sysTray, &QSystemTrayIcon::activated, this, &PXMWindow::systemTrayAction);
    QObject::connect(sysTray, &QObject::destroyed, sysTrayMenu, &QObject::deleteLater);
    QObject::connect(ui->textEdit, &QTextEdit::textChanged, this, &PXMWindow::textEditChanged);
    QObject::connect(sysTrayMenu, &QMenu::aboutToHide, sysTrayMenu, &QObject::deleteLater);
    QObject::connect(ui->textEdit, &PXMTextEdit::typing, this, &PXMWindow::typingHandler);
    QObject::connect(ui->textEdit, &PXMTextEdit::endOfTyping, this, &PXMWindow::endOfTypingHandler);
    QObject::connect(ui->toolButton, &QToolButton::toggled, ui->textEdit, &PXMTextEdit::toggleBold);
    QObject::connect(ui->toolButton_4, &QToolButton::toggled, ui->textEdit, &PXMTextEdit::toggleItalics);
    QObject::connect(ui->toolButton_5, &QToolButton::toggled, ui->textEdit, &PXMTextEdit::toggleUnderline);
    // QObject::connect(ui->comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
    //    [=](int index){emit fontColorChange(ui->comboBox->itemData(index).value<QColor>());});
    QObject::connect(ui->comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
                     &PXMWindow::alertFontColor);
    QObject::connect(this, &PXMWindow::fontColorChange, ui->textEdit, &PXMTextEdit::fontColorChange);

    QObject::connect(debugWindow->pushButton, &QAbstractButton::clicked, this, &PXMWindow::printInfoToDebug);
    QObject::connect(ui->pushButton, &QAbstractButton::clicked, [=]() {
        if (ui->listWidget->currentItem() == nullptr) {
            return;
        }
        ui->stackedWidget->invert(ui->listWidget->currentItem()->data(Qt::UserRole).toUuid());
    });
}
void PXMWindow::aboutActionSlot()
{
    PXMAboutDialog* about = new PXMAboutDialog(this, QIcon(":/resources/PXM_Icon.ico"));
    QObject::connect(about, &PXMAboutDialog::finished, about, &PXMAboutDialog::deleteLater);
    about->open();
}
void PXMWindow::settingsActionsSlot()
{
    PXMSettingsDialog* setD = new PXMSettingsDialog(this);
    QObject::connect(setD, &PXMSettingsDialog::nameChange, [=](QString hname) {
        qInfo() << "Self Name Change";
        this->ui->lineEdit->setText(hname);
        emit sendMsg(hname.toUtf8(), PXMConsts::MSG_NAME, QUuid());
    });
    QObject::connect(setD, &PXMSettingsDialog::verbosityChanged, debugWindow.data(),
                     &PXMConsole::Window::verbosityChanged);
    QObject::connect(setD, &PXMSettingsDialog::colorSchemeAlert, [=]() {
        QColor col = this->palette().text().color();
        QVariant var;
        var.setValue(col);
        ui->comboBox->setItemData(0, var, Qt::UserRole);
    });
    setD->open();
}

void PXMWindow::bloomActionsSlot()
{
    QMessageBox box;
    box.setWindowTitle("Bloom");
    box.setText(
        "This will resend our initial discovery "
        "packet to the multicast group.  If we "
        "have only found ourselves this is happening "
        "automatically on a 15 second timer.");
    QPushButton* bloomButton = box.addButton(tr("Bloom"), QMessageBox::ActionRole);
    box.addButton(QMessageBox::Abort);
    box.exec();
    if (box.clickedButton() == bloomButton) {
        qDebug() << "User triggered bloom";
        emit sendUDP("/discover");
    }
}
void PXMWindow::syncActionsSlot()
{
    QMessageBox box;
    box.setWindowTitle("Sync");
    box.setText("Sync with peers?");
    QPushButton* bloomButton = box.addButton(tr("Sync"), QMessageBox::ActionRole);
    box.addButton(QMessageBox::Abort);
    box.exec();
    if (box.clickedButton() == bloomButton) {
        qDebug() << "User triggered Sync";
        emit syncWithPeers();
    }
}
void PXMWindow::manualConnect()
{
    ManualConnect* mc = new ManualConnect(this);
    QObject::connect(mc, &ManualConnect::manConnect, this, &PXMWindow::manConnect);
    mc->open();
}

void PXMWindow::typingAlert(QUuid uuid)
{
    QString hostname;
    for (int i = 0; i < ui->listWidget->count(); i++) {
        if (ui->listWidget->item(i)->data(Qt::UserRole) == uuid) {
            hostname = ui->listWidget->item(i)->text();
            break;
        }
    }
    if (hostname == QString()) {
        return;
    }
    ui->stackedWidget->showTyping(uuid, hostname);
}

void PXMWindow::textEnteredAlert(QUuid uuid)
{
    QString hostname;
    for (int i = 0; i < ui->listWidget->count(); i++) {
        if (ui->listWidget->item(i)->data(Qt::UserRole) == uuid) {
            hostname = ui->listWidget->item(i)->text();
            break;
        }
    }
    if (hostname == QString()) {
        return;
    }
    ui->stackedWidget->showEntered(uuid, hostname);
}

void PXMWindow::endOfTextEnteredAlert(QUuid uuid)
{
    ui->stackedWidget->clearInfoLine(uuid);
}

void PXMWindow::alertFontColor(int index)
{
    if (ui->comboBox->itemText(index) == QString("System")) {
        emit fontColorChange(QColor(), true);
    } else {
        emit fontColorChange(ui->comboBox->itemData(index).value<QColor>(), false);
    }
}

void PXMWindow::debugActionSlot()
{
    if (debugWindow) {
        debugWindow->show();
        debugWindow->setWindowState(Qt::WindowActive);
    }
}

void PXMWindow::setItalicsOnItem(QUuid uuid, bool italics)
{
    for (int i = 0; i < ui->listWidget->count(); i++) {
        if (ui->listWidget->item(i)->data(Qt::UserRole) == uuid) {
            QFont mfont = ui->listWidget->item(i)->font();
            if (mfont.italic() == italics)
                return;
            mfont.setItalic(italics);
            ui->listWidget->item(i)->setFont(mfont);
            QString changeInConnection;
            if (italics) {
                changeInConnection = " disconnected";
                this->endOfTextEnteredAlert(uuid);
            } else
                changeInConnection = " reconnected";
            emit addMessageToPeer(ui->listWidget->item(i)->text() % changeInConnection, uuid, uuid, false, true);
            return;
        }
    }
}
void PXMWindow::textEditChanged()
{
    if (ui->textEdit->toPlainText().length() > PXMConsts::TEXT_EDIT_MAX_LENGTH) {
        int diff     = ui->textEdit->toPlainText().length() - PXMConsts::TEXT_EDIT_MAX_LENGTH;
        QString temp = ui->textEdit->toPlainText();
        temp.chop(diff);
        ui->textEdit->setText(temp);
        QTextCursor cursor(ui->textEdit->textCursor());
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        ui->textEdit->setTextCursor(cursor);
    }
}
void PXMWindow::systemTrayAction(QSystemTrayIcon::ActivationReason reason)
{
    if (((reason == QSystemTrayIcon::DoubleClick) || (reason == QSystemTrayIcon::Trigger))) {
        this->show();
        this->raise();
        this->setFocus();
        this->setWindowState(Qt::WindowActive);
    }
    return;
}
void PXMWindow::changeEvent(QEvent* event)
{
    switch (event->type()) {
        case QEvent::WindowStateChange:
            if (this->isMinimized()) {
                this->hide();
            }
            break;
        case QEvent::FocusOut:
            this->setWindowState(Qt::WindowNoState);
            break;
        default:
            break;
    }
    QMainWindow::changeEvent(event);
}
void PXMWindow::currentItemChanged(QListWidgetItem* item1, QListWidgetItem* item2)
{
    // Deal with typing alerts
    if (item2) {
        this->endOfTyping(item2->data(Qt::UserRole).toUuid());
        this->endOfTextEntered(item2->data(Qt::UserRole).toUuid());
    }
    if (this->ui->textEdit->toPlainText() != QString()) {
        this->typing(item1->data(Qt::UserRole).toUuid());
        this->textEntered(item1->data(Qt::UserRole).toUuid());
    }

    QUuid uuid = item1->data(Qt::UserRole).toUuid();

    if (item1->background() != QGuiApplication::palette().base()) {
        this->changeListItemColor(uuid, 0);
    }
    if (ui->stackedWidget->switchToUuid(uuid)) {
        // TODO: Some Exception here
    }
    return;
}
void PXMWindow::quitButtonClicked()
{
    this->close();
}
void PXMWindow::updateListWidget(QUuid uuid, QString hostname)
{
    ui->listWidget->setUpdatesEnabled(false);
    for (int i = 2; i < ui->listWidget->count(); i++) {
        if (ui->listWidget->item(i)->data(Qt::UserRole).toUuid() == uuid) {
            if (ui->listWidget->item(i)->text() != hostname) {
                emit addMessageToPeer(ui->listWidget->item(i)->text() % " has changed their name to " % hostname, uuid,
                                      uuid, false, false);
                ui->listWidget->item(i)->setText(hostname);
                QListWidgetItem* global = ui->listWidget->takeItem(0);
                ui->listWidget->sortItems();
                ui->listWidget->insertItem(0, global);
            }
            setItalicsOnItem(uuid, 0);
            ui->listWidget->setUpdatesEnabled(true);
            return;
        }
    }

    QListWidgetItem* global = ui->listWidget->takeItem(0);
    QListWidgetItem* item   = new QListWidgetItem(hostname, ui->listWidget);

    item->setData(Qt::UserRole, uuid);
    QString textColor = textColors.at(textColorsNext % textColors.length());
    textColorsNext++;
    item->setData(Qt::UserRole + 1, textColor);
    ui->listWidget->addItem(item);
    ui->stackedWidget->newHistory(uuid);
    ui->listWidget->sortItems();
    ui->listWidget->insertItem(0, global);

    ui->listWidget->setUpdatesEnabled(true);
}

void PXMWindow::closeEvent(QCloseEvent* event)
{
#ifndef __linux__
    event->ignore();

    QMessageBox* box = new QMessageBox(QMessageBox::Question, qApp->applicationName(), "Are you sure you want to quit?",
                                       QMessageBox::Yes | QMessageBox::No, this);

    QObject::connect(box->button(QMessageBox::Yes), &QAbstractButton::clicked, this, &PXMWindow::aboutToClose);
    QObject::connect(box->button(QMessageBox::Yes), &QAbstractButton::clicked, qApp, &QApplication::quit);
    QObject::connect(box->button(QMessageBox::No), &QAbstractButton::clicked, box, &QObject::deleteLater);
    QObject::connect(qApp, &QApplication::aboutToQuit, box, &QObject::deleteLater);
    box->show();
#else
    aboutToClose();
    event->accept();
#endif
}
void PXMWindow::aboutToClose()
{
    sysTray->hide();
    PXMIniReader iniReader;
    iniReader.setWindowSize(this->size());
    iniReader.setMute(ui->muteCheckBox->isChecked());
    iniReader.setFocus(ui->focusCheckBox->isChecked());

    debugWindow->hide();
}

void PXMWindow::typingHandler()
{
    if (!ui->listWidget->currentItem()) {
        return;
    }

    QUuid uuid = ui->listWidget->currentItem()->data(Qt::UserRole).toUuid();

    emit typing(uuid);
    if (!textEnteredTimer->isActive()) {
        textEnteredTimer->start();
        emit textEntered(uuid);
    }

    // emit textEntered(uuid);
}

void PXMWindow::textEnteredCallback()
{
    if (!ui->listWidget->currentItem()) {
        return;
    }

    QUuid uuid = ui->listWidget->currentItem()->data(Qt::UserRole).toUuid();

    if (ui->textEdit->toPlainText() == QString()) {
        this->textEnteredTimer->stop();
        emit endOfTextEntered(uuid);
    } else {
        // emit textEntered(uuid);
    }
}

void PXMWindow::endOfTypingHandler()
{
    if (!ui->listWidget->currentItem()) {
        return;
    }

    QUuid uuid = ui->listWidget->currentItem()->data(Qt::UserRole).toUuid();

    emit endOfTyping(uuid);
}

int PXMWindow::removeBodyFormatting(QByteArray& str)
{
    QRegularExpression qre("((?<=<body) style.*?\"(?=>))");
    QRegularExpressionMatch qrem = qre.match(str);
    if (qrem.hasMatch()) {
        int startoffset = qrem.capturedStart(1);
        int endoffset   = qrem.capturedEnd(1);
        str.remove(startoffset, endoffset - startoffset);
        return 0;
    } else
        return -1;
}

void PXMWindow::redoFontStyling()
{
    if (ui->toolButton->isChecked()) {
        ui->textEdit->toggleBold(true);
    }
    if (ui->toolButton_4->isChecked()) {
        ui->textEdit->toggleItalics(true);
    }
    if (ui->toolButton_5->isChecked()) {
        ui->textEdit->toggleUnderline(true);
    }
    QColor col = ui->comboBox->itemData(ui->comboBox->currentIndex()).value<QColor>();
    if (ui->comboBox->currentText() != "System") {
        ui->textEdit->fontColorChange(col, false);
    } else {
        ui->textEdit->fontColorChange(col, true);
    }
}

int PXMWindow::sendButtonClicked()
{
    if (!ui->listWidget->currentItem()) {
        return -1;
    }

    if (!(ui->textEdit->toPlainText().isEmpty())) {
        QString fixedURLS = ui->textEdit->toHtml();
        QRegularExpression urls("(https?://([a-zA-Z0-9_-]+.)?[a-zA-Z0-9_-]+[^< ]*)");
        QString body = fixedURLS.right(fixedURLS.length() - fixedURLS.indexOf("<html>"));
        body.replace(urls, "<a href=\"\\1\">\\1</a>");
        fixedURLS      = fixedURLS.left(fixedURLS.indexOf("<html>")) % body;
        QByteArray msg = fixedURLS.toUtf8();
        if (removeBodyFormatting(msg)) {
            qWarning() << "Bad Html";
            qWarning() << msg;
            return -1;
        }
        int index                = ui->listWidget->currentRow();
        QUuid uuidOfSelectedItem = ui->listWidget->item(index)->data(Qt::UserRole).toString();

        if (uuidOfSelectedItem.isNull())
            return -1;

        if ((uuidOfSelectedItem == globalChatUuid)) {
            emit sendMsg(msg, PXMConsts::MSG_GLOBAL, QUuid());
        } else {
            emit sendMsg(msg, PXMConsts::MSG_TEXT, uuidOfSelectedItem);
            emit endOfTextEntered(uuidOfSelectedItem);
        }
        ui->textEdit->setHtml(QString());
        ui->textEdit->setStyleSheet(QString());
        ui->textEdit->clear();
        ui->textEdit->setCurrentCharFormat(QTextCharFormat());
        redoFontStyling();
    } else {
        return -1;
    }
    return 0;
}
int PXMWindow::changeListItemColor(QUuid uuid, int style)
{
    for (int i = 0; i < ui->listWidget->count(); i++) {
        if (ui->listWidget->item(i)->data(Qt::UserRole) == uuid) {
            if (!style)
                ui->listWidget->item(i)->setBackground(QGuiApplication::palette().base());
            else {
                ui->listWidget->item(i)->setBackground(QBrush(QColor(0xFFCD5C5C)));
            }
            break;
        }
    }
    return 0;
}
int PXMWindow::formatMessage(QString& str, QString& hostname, QString color)
{
    QRegularExpression qre("(<p.*?>)");
    QRegularExpressionMatch qrem = qre.match(str);
    int offset                   = qrem.capturedEnd(1);
    QDateTime dt                 = QDateTime::currentDateTime();
    QString date                 = QStringLiteral("(") % dt.time().toString("hh:mm:ss") % QStringLiteral(") ");
    str.insert(offset, QString("<span style=\"white-space: nowrap\" style=\"color: " % color % ";\">" % date %
                               hostname % ":&nbsp;</span>"));

    return 0;
}

int PXMWindow::focusWindow()
{
    if (!(ui->muteCheckBox->isChecked())) {
        QSound::play(":/resources/message.wav");
    }
    if (!this->ui->focusCheckBox->isChecked()) {
        this->raise();
        this->activateWindow();
    }
    this->show();
    qApp->alert(this, 0);
    return 0;
}
int PXMWindow::printToTextBrowser(QSharedPointer<QString> str,
                                  QString hostname,
                                  QUuid uuid,
                                  QUuid sender,
                                  bool alert,
                                  bool fromServer,
                                  bool global)
{
    if (str->isEmpty()) {
        return -1;
    }

    QString color = peerColor;
    if (sender == localUuid && !fromServer) {
        color = selfColor;
    } else if (global) {
        for (int i = 0; i < ui->listWidget->count(); i++) {
            if (ui->listWidget->item(i)->data(Qt::UserRole) == uuid) {
                color = ui->listWidget->item(i)->data(Qt::UserRole + 1).toString();
            }
        }
    }

    if (global) {
        uuid = globalChatUuid;
    }

    if (alert) {
        if (ui->listWidget->currentItem()) {
            if (ui->listWidget->currentItem()->data(Qt::UserRole) != uuid) {
                changeListItemColor(uuid, 1);
            }
        } else {
            changeListItemColor(uuid, 1);
        }
        this->focusWindow();
    }

    this->formatMessage(*str.data(), hostname, color);
    ui->stackedWidget->append(*str.data(), uuid);

    return 0;
}

PXMAboutDialog::PXMAboutDialog(QWidget* parent, QIcon icon) : QDialog(parent), ui(new Ui::PXMAboutDialog), icon(icon)
{
    ui->setupUi(this);
    ui->label_2->setPixmap(icon.pixmap(QSize(64, 64)));
    ui->label->setText("<br><center>PXMessenger v" + qApp->applicationVersion() +
                       "</center>"
                       "<br>"
                       "<center>Author: Cory Bolar</center>"
                       "<br>"
                       "<center>"
                       "<a href=\"https://github.com/cbpeckles/PXMessenger\">"
                       "https://github.com/cbpeckles/PXMessenger</a>"
                       "</center>"
                       "<br><br><br><br>");
    ui->label->setOpenExternalLinks(true);
    this->resize(ui->label->width(), this->height());
    this->setAttribute(Qt::WA_DeleteOnClose, true);
}
void PXMSettingsDialog::colorSchemeChange(int index)
{
#ifdef _WIN32
    if (index == 0) {
        qInfo() << "Avalaible styles: " << QStyleFactory::keys();
        qApp->setPalette(PXMAgent::defaultPalette);
        if (QStyleFactory::keys().contains("WindowsVista")) {
            qApp->setStyle(QStyleFactory::create("WindowsVista"));
        } else if (QStyleFactory::keys().contains("WindowsXP")) {
            qApp->setStyle(QStyleFactory::create("WindowsXP"));
        } else if (QStyleFactory::keys().contains("Windows")) {
            qApp->setStyle(QStyleFactory::create("Windows"));
        } else {
            qWarning() << "Bad style change "
                       << "Avalaible styles: " << QStyleFactory::keys();
            qApp->setStyle(QStyleFactory::create(QStyleFactory::keys().first()));
        }
    } else if (index == 1) {
        PXMAgent::changeToDark();
    }
#endif
    (void)index;
}

PXMSettingsDialog::PXMSettingsDialog(QWidget* parent)
    : QDialog(parent), d_ptr(new PXMSettingsDialogPrivate), ui(new Ui::PXMSettingsDialog)
{
    this->setAttribute(Qt::WA_DeleteOnClose, true);
    ui->setupUi(this);
    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    QRegExp ipRegex("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");
    QRegExpValidator* ipValidator = new QRegExpValidator(ipRegex, this);
    ui->multicastLineEdit->setValidator(ipValidator);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setFocus();
    ui->verbositySpinBox->setValue(PXMConsole::Logger::getInstance()->getVerbosityLevel());
    QObject::connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QObject::connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &PXMSettingsDialog::resetDefaults);
    QObject::connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, this,
                     &PXMSettingsDialog::currentFontChanged);
    void (QSpinBox::*signal)(int) = &QSpinBox::valueChanged;
    QObject::connect(ui->fontSizeSpinBox, signal, this, &PXMSettingsDialog::valueChanged);

    QObject::connect(ui->comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
                     &PXMSettingsDialog::colorSchemeChange);

#ifndef _WIN32
    ui->comboBox->setVisible(false);
    ui->label->setVisible(false);
    ui->label_2->setVisible(false);
    ui->checkBox->setVisible(false);
#endif
    readIni();

    // Must be connected after ini is read to prevent box from coming up
    // upon opening settings dialog
    QObject::connect(ui->LogActiveCheckBox, &QCheckBox::stateChanged, this, &PXMSettingsDialog::logStateChange);
    ui->listWidget->setCurrentRow(0);
}

void PXMSettingsDialog::logStateChange(int state)
{
    if (state == Qt::Checked) {
#ifdef __WIN32
        QString logLocation = QDir::currentPath() + "\\log.txt";
        QMessageBox::information(this, "Log Status Change",
                                 "You have enabled file logging.\n"
                                 "Log location will be " +
                                     logLocation);
#elif __unix__
        QMessageBox::information(this, "Log Status Change",
                                 "You have enabled file logging.\n"
                                 "Log location will be ~\\.pxmessenger-log");
#else
#error "implement message box with log location"
#endif
    }
}

void PXMSettingsDialog::resetDefaults(QAbstractButton* button)
{
    if (dynamic_cast<QPushButton*>(button) == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)) {
        if (ui->listWidget->currentRow() == 0) {
#ifdef _WIN32
            QScopedArrayPointer<char> localHostname(new char[UNLEN + 1]);
            QScopedArrayPointer<TCHAR> t_user(new TCHAR[UNLEN + 1]);
            DWORD user_size = UNLEN + 1;
            if (GetUserName(t_user.data(), &user_size))
                wcstombs(&localHostname[0], &t_user[0], UNLEN + 1);
            else
                strcpy(&localHostname[0], "user");
#else
            QScopedArrayPointer<char> localHostname(new char[sysconf(_SC_GETPW_R_SIZE_MAX)]);
            struct passwd* user = 0;
            user                = getpwuid(getuid());
            if (!user)
                strcpy(&localHostname[0], "user");
            else
                strcpy(&localHostname[0], user->pw_name);
#endif
            ui->hostnameLineEdit->setText(QString::fromLatin1(&localHostname[0]).left(PXMConsts::MAX_HOSTNAME_LENGTH));
            ui->comboBox->setCurrentIndex(0);
            ui->checkBox->setChecked(true);
        } else if (ui->listWidget->currentRow() == 1) {
            ui->tcpPortSpinBox->setValue(0);
            ui->udpPortSpinBox->setValue(0);
            ui->multicastLineEdit->setText(PXMConsts::DEFAULT_MULTICAST_ADDRESS);
        } else if (ui->listWidget->currentRow() == 2) {
            ui->allowMultipleCheckBox->setChecked(false);
            ui->LogActiveCheckBox->setChecked(false);
            ui->verbositySpinBox->setValue(0);
        } else {
            return;
        }
    }
    if (dynamic_cast<QPushButton*>(button) == ui->buttonBox->button(QDialogButtonBox::Help)) {
        QString helpMessage;
        if (ui->listWidget->currentRow() == 0) {
            helpMessage =
                "Hostname will only change the first half of your "
                "hostname, the "
                "computer name will remain.\n(Default:Your "
                "Username)<br><br>"
                "Changes to default font and size affect your client"
                "only, they will have no impact on what other users see<br><br>"
                "Automatic updates will query if there are new releases for PXMessenger"
                "available and will offer to download them for you<br><br>";
        } else if (ui->listWidget->currentRow() == 1) {
            helpMessage =
                "Changes to these settings should not be needed under "
                "normal "
                "conditions.<br><br>"
                "Care should be taken in adjusting them as they can "
                "prevent "
                "PXMessenger from functioning properly.<br><br>"
                "The listener port should be changed only if needed to "
                "bypass firewall "
                "restrictions. 0 is Auto.<br>(Default:0)<br><br>"
                "The discover port must be the same for all computers "
                "that wish to "
                "communicate together. 0 is " +
                QString::number(PXMConsts::DEFAULT_UDP_PORT) + ".<br>(Default:0)<br><br>" +
                "Multicast Address must be the same for all "
                "computers that wish to "
                "discover each other. Changes "
                "from the default value should only be "
                "necessary if firewall "
                "restrictions require it."
                "<br><br>(Default:" +
                PXMConsts::DEFAULT_MULTICAST_ADDRESS + ")";
        } else if (ui->listWidget->currentRow() == 2) {
            helpMessage =
                "Allowing more than one instance lets the program be "
                "run multiple "
                "times under the same user.\n(Default:false)<br><br>"
                "Debug Verbosity will increase the number of "
                "message printed to "
                "both the debugging window<br>"
                "and stdout.  0 hides warnings and debug "
                "messages, 1 only hides "
                "debug messages, 2 shows all<br>"
                "(Default:0)";
        } else {
            return;
        }

        QMessageBox msgBox;
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setText(
            helpMessage +
            "<br><br>More information can be found at <br>"
            "<a href=\"https://github.com/cbpeckles/PXMessenger.\">https://github.com/cbpeckles/PXMessenger</a>");

        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();
    }
}

void PXMSettingsDialog::accept()
{
    PXMIniReader iniReader;
    PXMConsole::Logger* logger = PXMConsole::Logger::getInstance();

    logger->setVerbosityLevel(ui->verbositySpinBox->value());
    iniReader.setVerbosity(ui->verbositySpinBox->value());
    emit verbosityChanged();

    iniReader.setAllowMoreThanOne(ui->allowMultipleCheckBox->isChecked());
    iniReader.setHostname(ui->hostnameLineEdit->text().simplified());
    if (d_ptr->hostname != ui->hostnameLineEdit->text().simplified()) {
        char computerHostname[255] = {};
        gethostname(computerHostname, sizeof(computerHostname));
        emit nameChange(ui->hostnameLineEdit->text().simplified() % "@" %
                        QString::fromUtf8(computerHostname).left(PXMConsts::MAX_COMPUTER_NAME_LENGTH));
    }
    iniReader.setPort("TCP", ui->tcpPortSpinBox->value());
    iniReader.setPort("UDP", ui->udpPortSpinBox->value());
    iniReader.setFont(qApp->font().toString());
    iniReader.setLogActive(ui->LogActiveCheckBox->isChecked());
    iniReader.setDarkColorScheme(ui->comboBox->currentIndex());
    logger->setLogStatus(ui->LogActiveCheckBox->isChecked());
    iniReader.setMulticastAddress(ui->multicastLineEdit->text());
    iniReader.setUpdates(ui->checkBox->isChecked());
    if (d_ptr->tcpPort != ui->tcpPortSpinBox->value() || d_ptr->udpPort != ui->udpPortSpinBox->value() ||
        d_ptr->AllowMoreThanOneInstance != ui->allowMultipleCheckBox->isChecked() ||
        d_ptr->multicastAddress != ui->multicastLineEdit->text()) {
        QMessageBox::information(this, "Settings Warning",
                                 "Changes to these settings will not take effect "
                                 "until PXMessenger has been restarted");
    }
    QDialog::accept();
}

void PXMSettingsDialog::currentFontChanged(QFont font)
{
    qApp->setFont(font);
}

void PXMSettingsDialog::valueChanged(int size)
{
    d_ptr->iniFont = qApp->font();
    d_ptr->iniFont.setPointSize(size);
    qApp->setFont(d_ptr->iniFont);
}

void PXMSettingsDialog::readIni()
{
    PXMIniReader iniReader;
    d_ptr->AllowMoreThanOneInstance = iniReader.checkAllowMoreThanOne();
    d_ptr->hostname                 = iniReader.getHostname(d_ptr->hostname);
    d_ptr->tcpPort                  = iniReader.getPort("TCP");
    d_ptr->udpPort                  = iniReader.getPort("UDP");
    d_ptr->multicastAddress         = iniReader.getMulticastAddress();
    d_ptr->fontSize                 = qApp->font().pointSize();
    d_ptr->darkColor                = iniReader.getDarkColorScheme();
    ui->fontSizeSpinBox->setValue(d_ptr->fontSize);
    ui->tcpPortSpinBox->setValue(d_ptr->tcpPort);
    ui->udpPortSpinBox->setValue(d_ptr->udpPort);
    ui->hostnameLineEdit->setText(d_ptr->hostname.simplified());
    ui->allowMultipleCheckBox->setChecked(d_ptr->AllowMoreThanOneInstance);
    ui->multicastLineEdit->setText(d_ptr->multicastAddress);
    ui->LogActiveCheckBox->setChecked(iniReader.getLogActive());
    ui->comboBox->setCurrentIndex(d_ptr->darkColor);
}

PXMSettingsDialog::~PXMSettingsDialog()
{
    delete d_ptr;
}

PXMTextEdit::PXMTextEdit(QWidget* parent) : QTextEdit(parent)
{
    QObject::connect(this, &PXMTextEdit::textChanged, this, &PXMTextEdit::typing);
}
void PXMTextEdit::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return) {
        emit returnPressed();
        emit endOfTyping();
        emit endOfTextEntered();
    } else if (event->key() != Qt::Key_Backspace && event->key() != Qt::Key_Delete) {
        // emit typing();
        QTextEdit::keyPressEvent(event);
    } else {
        QTextEdit::keyPressEvent(event);
    }
}

void PXMTextEdit::fontColorChange(QColor color, bool system)
{
    if (!system) {
        this->setTextColor(color);
    } else {
        QTextCharFormat tcf = this->currentCharFormat();
        tcf.clearForeground();
        this->setCurrentCharFormat(tcf);
    }
}

void PXMTextEdit::toggleItalics(bool checked)
{
    QTextCharFormat tcf = this->currentCharFormat();
    QFont f             = tcf.font();
    f.setItalic(checked);
    tcf.setFont(f);
    this->setCurrentCharFormat(tcf);
}

void PXMTextEdit::toggleBold(bool checked)
{
    QTextCharFormat tcf = this->currentCharFormat();
    QFont f             = tcf.font();
    f.setBold(checked);
    tcf.setFont(f);
    this->setCurrentCharFormat(tcf);
}

void PXMTextEdit::toggleUnderline(bool checked)
{
    QTextCharFormat tcf = this->currentCharFormat();
    QFont f             = tcf.font();
    f.setUnderline(checked);
    tcf.setFont(f);
    this->setCurrentCharFormat(tcf);
}
void PXMTextEdit::focusInEvent(QFocusEvent* event)
{
    this->setPlaceholderText("");
    QTextEdit::focusInEvent(event);
}
void PXMTextEdit::focusOutEvent(QFocusEvent* event)
{
    this->setPlaceholderText("Enter a message to send!");
    QTextEdit::focusOutEvent(event);
}

ManualConnect::ManualConnect(QWidget* parent) : QDialog(parent), ui(new Ui::ManualConnect)
{
    ui->setupUi(this);
    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    QRegExp ipRegex("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");
    QRegExpValidator* ipValidator = new QRegExpValidator(ipRegex, this);
    ui->lineEdit->setValidator(ipValidator);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Connect");
    QObject::connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ManualConnect::reject);
    QObject::connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ManualConnect::accept);
    this->setAttribute(Qt::WA_DeleteOnClose, true);
}
void ManualConnect::accept()
{
    emit manConnect(ui->lineEdit->text(), ui->spinBox->value());
    QDialog::accept();
}

ManualConnect::~ManualConnect()
{
    delete ui;
}
