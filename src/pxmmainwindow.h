#ifndef PXMWINDOW_H
#define PXMWINDOW_H

#include <QPushButton>
#include <QTextEdit>
#include <QScrollArea>
#include <QTextBrowser>
#include <QLineEdit>
#include <QListWidget>
#include <QComboBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>
#include <QAction>
#include <QWindow>
#include <QScrollBar>
#include <QDebug>
#include <QCloseEvent>
#include <QSound>
#include <QAction>
#include <QApplication>
#include <QPalette>
#include <QMainWindow>
#include <QGridLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QStringBuilder>
#include <QDateTime>
#include <QRgb>
#include <QApplication>

#include <sys/types.h>
#include <ctime>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>

#include "pxmtextedit.h"
#include "pxminireader.h"
#include "pxmdefinitions.h"
#include "pxmpeerworker.h"
#include "pxmsettingsdialog.h"
#include "pxmdebugwindow.h"
#include "pxmtextbrowser.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <lmcons.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <resolv.h>
#include <pwd.h>
#endif

class PXMWindow : public QMainWindow
{
    Q_OBJECT

public:
    PXMWindow(initialSettings presets);
    ~PXMWindow();
    PXMWindow(PXMWindow const&) = delete;
    PXMWindow& operator=(PXMWindow const&) = delete;
    PXMWindow& operator=(PXMWindow&&) noexcept = delete;
    PXMWindow(PXMWindow&&) noexcept = delete;
    void startThreadsAndShow();
public slots:
    void bloomActionsSlot();
    void resizeLabel(QRect size);
protected:
    void closeEvent(QCloseEvent *event)  Q_DECL_OVERRIDE;
    void changeEvent(QEvent *event)  Q_DECL_OVERRIDE;
private:
    QGridLayout *layout;
    QWidget *centralwidget;
    QSpacerItem *horizontalSpacer;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *horizontalSpacer_3;
    QSpacerItem *horizontalSpacer_4;
    QMenuBar *menubar;
    QStatusBar *statusbar;
    QPushButton *messSendButton;
    QPushButton *messQuitButton;
    QAction *messSystemTrayExitAction;
    QMenu *messSystemTrayMenu;
    QSystemTrayIcon *messSystemTray;
    PXMTextBrowser *messTextBrowser;
    QLineEdit *messLineEdit;
    QPushButton *m_sendDebugButton;
    QListWidget *messListWidget;
    QCheckBox *muteCheckBox;
    QCheckBox *focusCheckBox;
    QThread *workerThread;
    QLabel *loadingLabel;
    QLabel *browserLabel;
    QUuid ourUUID;
    QFrame *fsep;
    PXMTextEdit *messTextEdit;
    PXMPeerWorker *peerWorker;
    PXMDebugWindow *debugWindow = nullptr;
    QString localHostname;
    unsigned short ourTCPListenerPort;
    unsigned short ourUDPListenerPort;
    QUuid globalChatUuid;
    bool multicastFunctioning;

    /*!
     * \brief getFormattedTime
     *
     * Get the current time and format it to be (23:59:59)
     * \return The formatted time as a QString
     */
    QString getFormattedTime();
    /*!
     * \brief setupHostname
     *
     * Setup the hostname of the current computer.  The format is by default
     * username@computername.  The username can be adjusted in the ini file or
     * from the options dialog from the menu bar.  A corresponding number is
     * added to the username if this is not the first instance of the program
     * for this user.  The username should not be more than 128 characters long.
     * \param uuidNum Number to add to the end of the username, if 0, nothing
     * \param username Username of user, from the config file
     */
    void setupHostname(int uuidNum, QString username);
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
    void createTextBrowser();
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
    void createListWidget();
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
    void createCheckBoxes();
    void setupGui();
private slots:
    int sendButtonClicked();
    void quitButtonClicked();
    void currentItemChanged(QListWidgetItem *item1);
    void textEditChanged();
    void showWindow(QSystemTrayIcon::ActivationReason reason);
    int printToTextBrowser(QString str, QUuid uuid, bool alert);
    void updateListWidget(QUuid uuid, QString hostname);
    void setItalicsOnItem(QUuid uuid, bool italics);
    void aboutActionSlot();
    void settingsActionsSlot();
    void debugActionSlot();
    void warnBox(QString title, QString msg);
signals:
    void connectToPeer(evutil_socket_t, sockaddr_in);
    void sendMsg(QByteArray, PXMConsts::MESSAGE_TYPE, QUuid, QUuid);
    void sendUDP(const char*, unsigned short);
    void retryDiscover();
    void addMessageToPeer(QString, QUuid, bool, bool);
    void requestFullHistory(QUuid);
};

#endif
