#include "pxmmainwindow.h"
#include "pxmconsole.h"
#include "pxminireader.h"
#include "ui_pxmaboutdialog.h"
#include "ui_pxmmainwindow.h"
#include "ui_pxmsettingsdialog.h"

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
};

PXMWindow::PXMWindow(QString hostname, QSize windowSize, bool mute, bool focus, QUuid globalChat, QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::PXMWindow),
      localHostname(hostname),
      globalChatUuid(globalChat),
      debugWindow(new PXMConsole::Window())
{
    setupGui();

    ui->focusCheckBox->setChecked(focus);
    ui->muteCheckBox->setChecked(mute);

    this->resize(windowSize);
}
PXMWindow::~PXMWindow()
{
    qDebug() << "Shutdown of PXMWindow Successful";
}
void PXMWindow::setupMenuBar()
{
    QMenu* fileMenu;
    QAction* quitAction = new QAction("&Quit", this);

    fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(quitAction);
    QObject::connect(quitAction, &QAction::triggered, this, &PXMWindow::quitButtonClicked);

    QMenu* optionsMenu;
    QAction* settingsAction = new QAction("&Settings", this);
    QAction* bloomAction    = new QAction("&Bloom", this);
    optionsMenu             = menuBar()->addMenu("&Tools");
    optionsMenu->addAction(settingsAction);
    optionsMenu->addAction(bloomAction);
    QObject::connect(settingsAction, &QAction::triggered, this, &PXMWindow::settingsActionsSlot);
    QObject::connect(bloomAction, &QAction::triggered, this, &PXMWindow::bloomActionsSlot);

    QMenu* helpMenu;
    QAction* aboutAction = new QAction("&About", this);
    QAction* debugAction = new QAction("&Debug", this);
    helpMenu             = menuBar()->addMenu("&Help");
    helpMenu->addAction(aboutAction);
    helpMenu->addAction(debugAction);
    QObject::connect(aboutAction, &QAction::triggered, this, &PXMWindow::aboutActionSlot);
    QObject::connect(debugAction, &QAction::triggered, this, &PXMWindow::debugActionSlot);
}

void PXMWindow::initListWidget()
{
    ui->messListWidget->setSortingEnabled(false);
    ui->messListWidget->insertItem(0, QStringLiteral("Global Chat"));
    QListWidgetItem* seperator = new QListWidgetItem(ui->messListWidget);
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

    messSystemTrayMenu       = new QMenu(this);
    messSystemTrayExitAction = new QAction(tr("&Exit"), this);
    messSystemTrayMenu->addAction(messSystemTrayExitAction);
    QObject::connect(messSystemTrayExitAction, &QAction::triggered, this, &PXMWindow::quitButtonClicked);

    messSystemTray = new QSystemTrayIcon(this);
    messSystemTray->setIcon(trayIcon);
    messSystemTray->setContextMenu(messSystemTrayMenu);
    messSystemTray->show();
}
void PXMWindow::setupGui()
{
    ui->setupUi(this);
    ui->messLineEdit->setText(localHostname);

    if (this->objectName().isEmpty()) {
        this->setObjectName(QStringLiteral("PXMessenger"));
    }

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
    QObject::connect(messSystemTrayMenu, &QMenu::aboutToHide, messSystemTrayMenu, &QObject::deleteLater);
    ;

    QObject::connect(debugWindow->pushButton, &QAbstractButton::clicked, this, &PXMWindow::printInfoToDebug);
}
void PXMWindow::aboutActionSlot()
{
    PXMAboutDialog* about = new PXMAboutDialog(this, QIcon(":/resources/resources/PXM_Icon.ico"));
    QObject::connect(about, &PXMAboutDialog::finished, about, &PXMAboutDialog::deleteLater);
    about->open();
}
void PXMWindow::settingsActionsSlot()
{
    PXMSettingsDialog* setD = new PXMSettingsDialog(this);
    QObject::connect(setD, &PXMSettingsDialog::nameChange, this, &PXMWindow::nameChange);
    QObject::connect(setD, &PXMSettingsDialog::verbosityChanged, debugWindow.data(),
                     &PXMConsole::Window::verbosityChanged);
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

void PXMWindow::warnBox(QString title, QString msg)
{
    QMessageBox::warning(this, title, msg);
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
    for (int i = 0; i < ui->messListWidget->count(); i++) {
        if (ui->messListWidget->item(i)->data(Qt::UserRole) == uuid) {
            QFont mfont = ui->messListWidget->item(i)->font();
            if (mfont.italic() == italics)
                return;
            mfont.setItalic(italics);
            ui->messListWidget->item(i)->setFont(mfont);
            QString changeInConnection;
            if (italics)
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
    if (ui->messTextEdit->toPlainText().length() > PXMConsts::TEXT_EDIT_MAX_LENGTH) {
        int diff     = ui->messTextEdit->toPlainText().length() - PXMConsts::TEXT_EDIT_MAX_LENGTH;
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
        default:
            break;
    }
    QMainWindow::changeEvent(event);
}
void PXMWindow::currentItemChanged(QListWidgetItem* item1)
{
    QUuid uuid = item1->data(Qt::UserRole).toString();

    if (item1->background() != QGuiApplication::palette().base()) {
        this->changeListItemColor(uuid, 0);
    }
    if (ui->stackedWidget->switchToUuid(uuid)) {
        // Some Exception here
    }
    return;
}
void PXMWindow::quitButtonClicked()
{
    this->close();
}
void PXMWindow::updateListWidget(QUuid uuid, QString hostname)
{
    ui->messListWidget->setUpdatesEnabled(false);
    for (int i = 2; i < ui->messListWidget->count(); i++) {
        if (ui->messListWidget->item(i)->data(Qt::UserRole).toUuid() == uuid) {
            if (ui->messListWidget->item(i)->text() != hostname) {
                emit addMessageToPeer(ui->messListWidget->item(i)->text() % " has changed their name to " % hostname,
                                      uuid, false, false);
                ui->messListWidget->item(i)->setText(hostname);
                QListWidgetItem* global = ui->messListWidget->takeItem(0);
                ui->messListWidget->sortItems();
                ui->messListWidget->insertItem(0, global);
            }
            setItalicsOnItem(uuid, 0);
            ui->messListWidget->setUpdatesEnabled(true);
            return;
        }
    }

    QListWidgetItem* global = ui->messListWidget->takeItem(0);
    QListWidgetItem* item   = new QListWidgetItem(hostname, ui->messListWidget);

    item->setData(Qt::UserRole, uuid);
    ui->messListWidget->addItem(item);
    ui->stackedWidget->addWidget(new TextWidget(ui->stackedWidget, uuid));
    ui->messListWidget->sortItems();
    ui->messListWidget->insertItem(0, global);

    ui->messListWidget->setUpdatesEnabled(true);
}

void PXMWindow::closeEvent(QCloseEvent* event)
{
#ifndef __linux__
    if (QMessageBox::No == QMessageBox::warning(this, "PXMessenger", "Are you sure you want to quit PXMessenger?",
                                                QMessageBox::Yes, QMessageBox::No)) {
        event->ignore();
        return;
    }
#endif

    messSystemTray->hide();
    PXMIniReader iniReader;
    iniReader.setWindowSize(this->size());
    iniReader.setMute(ui->muteCheckBox->isChecked());
    iniReader.setFocus(ui->focusCheckBox->isChecked());

    debugWindow->hide();
    event->accept();
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

int PXMWindow::sendButtonClicked()
{
    if (!ui->messListWidget->currentItem()) {
        return -1;
    }

    if (!(ui->messTextEdit->toPlainText().isEmpty())) {
        QByteArray msg = ui->messTextEdit->toHtml().toUtf8();
        if (removeBodyFormatting(msg)) {
            qWarning() << "Bad Html";
            qWarning() << msg;
            return -1;
        }
        int index                = ui->messListWidget->currentRow();
        QUuid uuidOfSelectedItem = ui->messListWidget->item(index)->data(Qt::UserRole).toString();

        if (uuidOfSelectedItem.isNull())
            return -1;

        if ((uuidOfSelectedItem == globalChatUuid)) {
            emit sendMsg(msg, PXMConsts::MSG_GLOBAL, QUuid());
        } else {
            emit sendMsg(msg, PXMConsts::MSG_TEXT, uuidOfSelectedItem);
        }
        ui->messTextEdit->setHtml(QString());
        ui->messTextEdit->setStyleSheet(QString());
        ui->messTextEdit->clear();
        ui->messTextEdit->setCurrentCharFormat(QTextCharFormat());
    } else {
        return -1;
    }
    return 0;
}
int PXMWindow::changeListItemColor(QUuid uuid, int style)
{
    for (int i = 0; i < ui->messListWidget->count(); i++) {
        if (ui->messListWidget->item(i)->data(Qt::UserRole) == uuid) {
            if (!style)
                ui->messListWidget->item(i)->setBackground(QGuiApplication::palette().base());
            else {
                ui->messListWidget->item(i)->setBackground(QBrush(QColor(0xFFCD5C5C)));
            }
            break;
        }
    }
    return 0;
}
int PXMWindow::focusWindow()
{
    if (!(ui->muteCheckBox->isChecked())) {
        QSound::play(":/resources/resources/message.wav");
    }

    if (!(this->isMinimized()) && (this->windowState() != Qt::WindowActive)) {
        qApp->alert(this, 0);
    } else if (this->isMinimized()) {
        if (!this->ui->focusCheckBox->isChecked()) {
            this->setWindowState(Qt::WindowActive);
            this->setFocus();
        } else
            this->setWindowState(Qt::WindowNoState);
        this->show();
        qApp->alert(this, 0);
    }
    return 0;
}
int PXMWindow::printToTextBrowser(QSharedPointer<QString> str, QUuid uuid, bool alert)
{
    if (str->isEmpty()) {
        return -1;
    }
    if (alert) {
        if (ui->messListWidget->currentItem()) {
            if (ui->messListWidget->currentItem()->data(Qt::UserRole) != uuid) {
                changeListItemColor(uuid, 1);
            }
        } else {
            changeListItemColor(uuid, 1);
        }
        this->focusWindow();
    }

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
    this->setAttribute(Qt::WA_DeleteOnClose, true);
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
    ui->verbositySpinBox->setValue(PXMConsole::LoggerSingleton::getVerbosityLevel());
    QObject::connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QObject::connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &PXMSettingsDialog::clickedme);
    QObject::connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, this,
                     &PXMSettingsDialog::currentFontChanged);
    void (QSpinBox::*signal)(int) = &QSpinBox::valueChanged;
    QObject::connect(ui->fontSizeSpinBox, signal, this, &PXMSettingsDialog::valueChanged);

    readIni();
}

void PXMSettingsDialog::clickedme(QAbstractButton* button)
{
    if (dynamic_cast<QPushButton*>(button) == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)) {
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
        ui->tcpPortSpinBox->setValue(0);
        ui->udpPortSpinBox->setValue(0);
        ui->hostnameLineEdit->setText(QString::fromLatin1(&localHostname[0]).left(PXMConsts::MAX_HOSTNAME_LENGTH));
        ui->allowMultipleCheckBox->setChecked(false);
        ui->multicastLineEdit->setText(PXMConsts::DEFAULT_MULTICAST_ADDRESS);
    }
    if (dynamic_cast<QPushButton*>(button) == ui->buttonBox->button(QDialogButtonBox::Help)) {
        QMessageBox::information(this, "Help",
                                 "Changes to these settings should not be needed under normal "
                                 "conditions.\n\n"
                                 "Care should be taken in adjusting them as they can prevent "
                                 "PXMessenger from functioning properly.\n\n"
                                 "Allowing more than one instance lets the program be run multiple "
                                 "times under the same user.\n(Default:false)\n\n"
                                 "Hostname will only change the first half of your hostname, the "
                                 "computer name will remain.\n(Default:Your Username)\n\n"
                                 "The listener port should be changed only if needed to bypass firewall "
                                 "restrictions. 0 is Auto.\n(Default:0)\n\n"
                                 "The discover port must be the same for all computers that wish to "
                                 "communicate together. 0 is " +
                                     QString::number(PXMConsts::DEFAULT_UDP_PORT) + ".\n(Default:0)\n\n" +
                                     "Multicast Address must be the same for all computers that wish to "
                                     "discover each other. Changes "
                                     "from the default value should only be necessary if firewall "
                                     "restrictions require it."
                                     "\n(Default:" +
                                     PXMConsts::DEFAULT_MULTICAST_ADDRESS + ")\n\n" +
                                     "Debug Verbosity will increase the number of message printed to "
                                     "both the debugging window\n"
                                     "and stdout.  0 hides warnings and debug messages, 1 only hides "
                                     "debug messages, 2 shows all\n"
                                     "(Default:0)\n\n"
                                     "More information can be found at "
                                     "https://github.com/cbpeckles/PXMessenger.");
    }
}

void PXMSettingsDialog::accept()
{
    PXMConsole::LoggerSingleton* logger = PXMConsole::LoggerSingleton::getInstance();
    logger->setVerbosityLevel(ui->verbositySpinBox->value());
    emit verbosityChanged();
    PXMIniReader iniReader;
    iniReader.setAllowMoreThanOne(ui->allowMultipleCheckBox->isChecked());
    iniReader.setHostname(ui->hostnameLineEdit->text().simplified());
    if (d_ptr->hostname != ui->hostnameLineEdit->text().simplified()) {
        char computerHostname[255] = {};
        gethostname(computerHostname, sizeof(computerHostname));
        emit nameChange(ui->hostnameLineEdit->text().simplified() % "@" %
                        QString::fromUtf8(computerHostname).left(PXMConsts::MAX_COMPUTER_NAME));
    }
    iniReader.setPort("TCP", ui->tcpPortSpinBox->value());
    iniReader.setPort("UDP", ui->udpPortSpinBox->value());
    iniReader.setFont(qApp->font().toString());
    iniReader.setMulticastAddress(ui->multicastLineEdit->text());
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
    ui->fontSizeSpinBox->setValue(d_ptr->fontSize);
    ui->tcpPortSpinBox->setValue(d_ptr->tcpPort);
    ui->udpPortSpinBox->setValue(d_ptr->udpPort);
    ui->hostnameLineEdit->setText(d_ptr->hostname.simplified());
    ui->allowMultipleCheckBox->setChecked(d_ptr->AllowMoreThanOneInstance);
    ui->multicastLineEdit->setText(d_ptr->multicastAddress);
}

PXMSettingsDialog::~PXMSettingsDialog()
{
    delete d_ptr;
}
