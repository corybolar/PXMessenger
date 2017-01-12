#ifndef PXMWINDOW_H
#define PXMWINDOW_H

#include <QScrollArea>
#include <QListWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>
#include <QDebug>
#include <QCloseEvent>
#include <QSound>
#include <QAction>
#include <QApplication>
#include <QPalette>
#include <QMainWindow>
#include <QMessageBox>
#include <QStringBuilder>
#include <QDateTime>

#include "pxmtextedit.h"
#include "pxminireader.h"
#include "pxmdefinitions.h"
#include "pxmpeerworker.h"
#include "pxmconsolewindow.h"
#include "pxmtextbrowser.h"
#include "ui_pxmaboutdialog.h"
#include "ui_pxmsettingsdialog.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <lmcons.h>
#endif

namespace Ui {
class PXMWindow;
class PXMAboutDialog;
class PXMSettingsDialog;
}

class PXMWindow : public QMainWindow
{
    Q_OBJECT


public:
    PXMWindow(QString hostname, QSize windowSize, bool mute, bool focus, QUuid globalChat);
    ~PXMWindow();
    PXMWindow(PXMWindow const&) = delete;
    PXMWindow& operator=(PXMWindow const&) = delete;
    PXMWindow& operator=(PXMWindow&&) noexcept = delete;
    PXMWindow(PXMWindow&&) noexcept = delete;
    PXMConsole::PXMConsoleWindow *debugWindow = nullptr;
public slots:
    void bloomActionsSlot();
    void resizeLabel(QRect size);
    int printToTextBrowser(QString str, QUuid uuid, bool alert);
    void setItalicsOnItem(QUuid uuid, bool italics);
    void updateListWidget(QUuid uuid, QString hostname);
    void warnBox(QString title, QString msg);
protected:
    void closeEvent(QCloseEvent *event)  Q_DECL_OVERRIDE;
    void changeEvent(QEvent *event)  Q_DECL_OVERRIDE;
private:
    Ui::PXMWindow *ui;
    QGridLayout *layout;
    QWidget *centralwidget;
    QSpacerItem *horizontalSpacer;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *horizontalSpacer_3;
    QSpacerItem *horizontalSpacer_4;
    QAction *messSystemTrayExitAction;
    QMenu *messSystemTrayMenu;
    QSystemTrayIcon *messSystemTray;
    QPushButton *m_sendDebugButton;
    QLabel *loadingLabel;
    QLabel *browserLabel;
    QFrame *fsep;
    QString localHostname;
    QUuid globalChatUuid;

    /*!
     * \brief getFormattedTime
     *
     * Get the current time and format it to be (23:59:59)
     * \return The formatted time as a QString
     */
    QString getFormattedTime();

    /*!
     * \brief focusWindow
     *
     * Brings the window to the foreground if allowed by the window manager.
     * Plays a sound when doing this.
     */
    int focusWindow();
    /*!
     * \brief changeListColor
     *
     * Changes the color of the background for an item in a given row of the
     * QListWidget.  Changes to either red or default.
     * \param row Row of item in QListWidget to change
     * \param style Color to change to.  1 for red, 0 for default.
     */
    int changeListItemColor(QUuid uuid, int style);
    /*!
     * \brief createTextEdit
     *
     * Initializes the text editor for the windows.  This is where messages are
     * typed prior to sending.
     */
    void createTextEdit();
    /*!
     * \brief createTextBrowser
     *
     * Initializes the text browser where messages are displayed upon sending
     * or receiving.
     */
    void setupLabels();
    /*!
     * \brief createLineEdit
     *
     * Initializes a single line editor that holds the full hostname of the
     * program. Currently read-only.
     */
    void createLineEdit();
    /*!
     * \brief createButtons
     *
     * Creates two buttons, one to send, one to quit.  They are connected to the
     * sendButtonClicked and quitButtonClicked slots.
     */
    void createButtons();
    /*!
     * \brief createListWidget
     *
     * Initializes the QListWidget that holds the connected computers.  Two
     * items are added, a seperator and a "Global Chat"
     */
    void initListWidget();
    /*!
     * \brief createSystemTray
     *
     * Initializes the icon and menu for a system tray if it is supported.
     */
    void createSystemTray();
    /*!
     * \brief connectGuiSignalsAndSlots
     *
     * All widget slots are connected to the respective signals here
     */
    void connectGuiSignalsAndSlots();
    /*!
     * \brief createMessClient
     *
     * Initialize a PXMClient.  This object is added to a QThread which runs
     * its own event loop.  All sending of messages and connecting to other
     * hosts is done in this object.
     */
    void startWorkerThread();
    /*!
     * \brief connectPeerClassSignalsAndSlots
     *
     * Connect the many signals and slots moving to and from a PXMPeerWorker
     * object.  PXMPeerWorker is initialized in constructor of PXMMainWindow.
     */
    void connectPeerClassSignalsAndSlots();
    /*!
     * \brief setupLayout
     *
     * Put all gui widgets on the window and in the correct location.
     * This was created in Qt Designer.
     */
    void setupLayout();
    void setupTooltips();
    void setupMenuBar();
    void setupGui();
    int removeBodyFormatting(QByteArray &str);
private slots:
    int sendButtonClicked();
    void quitButtonClicked();
    void currentItemChanged(QListWidgetItem *item1);
    void textEditChanged();
    void systemTrayAction(QSystemTrayIcon::ActivationReason reason);
    void aboutActionSlot();
    void settingsActionsSlot();
    void debugActionSlot();
    void nameChange(QString hname);
signals:
    void connectToPeer(evutil_socket_t, sockaddr_in);
    void sendMsg(QByteArray, PXMConsts::MESSAGE_TYPE, QUuid);
    void sendUDP(const char*);
    void retryDiscover();
    void addMessageToPeer(QString, QUuid, bool, bool);
    void requestFullHistory(QUuid);
};
class PXMAboutDialog : public QDialog
{
    Q_OBJECT

    Ui::PXMAboutDialog *ui;
    QIcon icon;
public:
    PXMAboutDialog(QWidget *parent = 0, QIcon icon = QIcon()) : QDialog(parent), ui(new Ui::PXMAboutDialog), icon(icon)
    {
        ui->setupUi(this);
        ui->label_2->setPixmap(icon.pixmap(QSize(64,64)));
        ui->label->setText("<br><center>PXMessenger v"
                           + qApp->applicationVersion() +
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
    ~PXMAboutDialog()
    {
        delete ui;
    }
};
class PXMSettingsDialog : public QDialog
{
    Q_OBJECT

    bool AllowMoreThanOneInstance;
    QString hostname;
    int tcpPort;
    int udpPort;
    QFont iniFont;
    int fontSize;
    QString multicastAddress;
    Ui::PXMSettingsDialog *ui;
public:
    PXMSettingsDialog(QWidget *parent = 0);
    void readIni();
    ~PXMSettingsDialog();
private slots:
    void clickedme(QAbstractButton *button);
    void accept();
    void currentFontChanged(QFont font);
    void valueChanged(int size);
signals:
    void nameChange(QString);
};

#endif
