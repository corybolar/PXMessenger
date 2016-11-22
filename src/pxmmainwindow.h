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
#include <QStatusBar>
#include <QMessageBox>
#include <QStringBuilder>
#include <QDateTime>

#include <sys/types.h>
#include <ctime>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>

#include <event2/event.h>
#include <event2/bufferevent.h>

#include "pxmserver.h"
#include "pxmtextedit.h"
#include "pxmclient.h"
#include "pxminireader.h"
#include "pxmdefinitions.h"
#include "pxmpeerworker.h"
#include "pxmsettingsdialog.h"

#ifdef __unix__
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <resolv.h>
#include <pwd.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <lmcons.h>
#endif

class PXMWindow : public QMainWindow
{
    Q_OBJECT

public:
    PXMWindow(initialSettings presets);
    ~PXMWindow();
public slots:
    void midnightTimerPersistent();
    void bloomActionsSlot();
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
    QPushButton *messDebugButton;
    QAction *messSystemTrayExitAction;
    QMenu *messSystemTrayMenu;
    QSystemTrayIcon *messSystemTray;
    QTextBrowser *messTextBrowser;
    QLineEdit *messLineEdit;
    QPushButton *m_sendDebugButton;
    QListWidget *messListWidget;
    QCheckBox *muteCheckBox;
    QCheckBox *focusCheckBox;
    QThread *messClientThread;
    QTimer *discoveryTimer;
    QTimer *midnightTimer;
    QString ourUUIDString;
    QFrame *fsep;
    PXMTextEdit *messTextEdit;
    PXMClient *messClient;
    PXMServer *messServer;
    PXMPeerWorker *peerWorker;
    time_t messTime;
    struct tm *currentTime;
    char *localHostname;
    unsigned short ourTCPListenerPort;
    unsigned short ourUDPListenerPort;
    QString globalChat = "";
    QUuid globalChatUuid;
    bool globalChatAlerted = false;

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
     * \brief globalSend
     *
     * Sends a message with a type /global to all connected peers
     * \param msg Message to send
     */
    void globalSend(QString msg);
    /*!
     * \brief focusWindow
     *
     * Brings the window to the foreground if allowed by the window manager.
     * Plays a sound when doing this.
     */
    void focusWindow();
    /*!
     * \brief changeListColor
     *
     * Changes the color of the background for an item in a given row of the
     * QListWidget.  Changes to either red or default.
     * \param row Row of item in QListWidget to change
     * \param style Color to change to.  1 for red, 0 for default.
     */
    void changeListColor(int row, int style);
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
    void createMessClient();
    /*!
     * \brief createMessServ
     *
     * Initialize a PXMServer.  A PXMServer subclasses QThread and runs its own
     * event loop.  All receiving of data is done in this object.
     */
    void createMessServ();
    /*!
     * \brief createMessTime
     *
     * Initialize the time objects for use with getFormattedTime()
     */
    void createMessTime();
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
private slots:
    void sendButtonClicked();
    void quitButtonClicked();
    void debugButtonClicked();
    void currentItemChanged(QListWidgetItem *item1);
    void textEditChanged();
    void showWindow(QSystemTrayIcon::ActivationReason reason);
    void printToTextBrowser(QString str, QUuid uuid, bool alert);
    void printToTextBrowserServerSlot(const QString str, QUuid uuid, int socket, bool global);
    void updateListWidget(QUuid uuid);
    void setItalicsOnItem(QUuid uuid, bool italics);
    void aboutActionSlot();
    void settingsActionsSlot();
    void discoveryTimerSingleShot();
    void discoveryTimerPersistent();
    void setListenerPort(unsigned short port);
signals:
    void connectToPeer(evutil_socket_t, sockaddr_in);
    void sendMsg(evutil_socket_t, QString, QString, QUuid, QString);
    void sendUDP(const char*, unsigned short);
    void retryDiscover();
};

#endif
