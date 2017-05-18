#ifndef PXMWINDOW_H
#define PXMWINDOW_H

#include <QSystemTrayIcon>
#include <QMainWindow>
#include <QUuid>
#include <QScopedPointer>
#include <QTextEdit>

#include "pxmconsts.h"
//#include "ui_pxmmainwindow.h"
#include "ui_pxmaboutdialog.h"
#include "ui_pxmsettingsdialog.h"

namespace PXMConsole {
class Window;
}
namespace Ui {
class PXMWindow;
class PXMAboutDialog;
class PXMSettingsDialog;
class ManualConnect;
}
class QListWidgetItem;

class PXMWindow : public QMainWindow {
  Q_OBJECT

  const QString selfColor = "#6495ED";  // Cornflower Blue
  const QString peerColor = "#FF0000";  // Red
  const QVector<QString> textColors = {
      "#808000",  // Olive
      "#FFA500",  // Orange
      "#FF00FF",  // Fuschia
      "#DC143C",  // Crimson
      "#FF69B4",  // HotPink
      "#708090",  // SlateGrey
      "#008000",  // Green
      "#00FF00"   // Lime
  };
  unsigned int textColorsNext;
  QScopedPointer<Ui::PXMWindow> ui;
  QAction* messSystemTrayExitAction;
  QMenu* sysTrayMenu;
  QSystemTrayIcon* sysTray;
  QFrame* fsep;
  QString localHostname;
  QUuid localUuid;
  QUuid globalChatUuid;
  QScopedPointer<PXMConsole::Window> debugWindow;
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
   * \brief createTextBrowser
   *
   * Initializes the text browser where messages are displayed upon sending
   * or receiving.
   */
  void setupLabels();
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
  void setupTooltips();
  void setupMenuBar();
  void setupGui();
  int removeBodyFormatting(QByteArray& str);

  int formatMessage(QString &str, QString& hostname, QString color);
public:
  PXMWindow(QString hostname,
            QSize windowSize,
            bool mute,
            bool focus, QUuid local,
            QUuid globalChat,
            QWidget* parent = nullptr);
  ~PXMWindow();
  PXMWindow(PXMWindow const&) = delete;
  PXMWindow& operator=(PXMWindow const&) = delete;
  PXMWindow& operator=(PXMWindow&&) noexcept = delete;
  PXMWindow(PXMWindow&&) noexcept = delete;
 public slots:
  void bloomActionsSlot();
  int printToTextBrowser(QSharedPointer<QString> str, QString hostname, QUuid uuid, bool alert, bool fromServer, bool global);
  void setItalicsOnItem(QUuid uuid, bool italics);
  void updateListWidget(QUuid uuid, QString hostname);
  void warnBox(QString title, QString msg);

 protected:
  void closeEvent(QCloseEvent* event) Q_DECL_OVERRIDE;
  void changeEvent(QEvent* event) Q_DECL_OVERRIDE;
 private slots:
  int sendButtonClicked();
  void quitButtonClicked();
  void currentItemChanged(QListWidgetItem* item1);
  void textEditChanged();
  void systemTrayAction(QSystemTrayIcon::ActivationReason reason);
  void aboutActionSlot();
  void settingsActionsSlot();
  void debugActionSlot();
  void nameChange(QString hname);
  void syncActionsSlot();
  void manualConnect();
  void aboutToClose();
signals:
  void sendMsg(QByteArray, PXMConsts::MESSAGE_TYPE, QUuid);
  void sendUDP(const char*);
  void syncWithPeers();
  void retryDiscover();
  void addMessageToPeer(QString, QUuid, bool, bool);
  void printInfoToDebug();
  void manConnect(QString, int);
};

class PXMAboutDialog : public QDialog {
  Q_OBJECT

  QScopedPointer<Ui::PXMAboutDialog> ui;
  QIcon icon;

 public:
  PXMAboutDialog(QWidget* parent = 0, QIcon icon = QIcon());
  ~PXMAboutDialog() {}
};

class PXMSettingsDialogPrivate;
class PXMSettingsDialog : public QDialog {
  Q_OBJECT

  //QScopedPointer<PXMSettingsDialogPrivate> d_ptr;
  PXMSettingsDialogPrivate *d_ptr;
  QScopedPointer<Ui::PXMSettingsDialog> ui;

 public:
  PXMSettingsDialog(QWidget* parent = 0);
  void readIni();
  ~PXMSettingsDialog();
 private slots:
  void resetDefaults(QAbstractButton* button);
  void accept();
  void currentFontChanged(QFont font);
  void valueChanged(int size);
  void logStateChange(int state);
signals:
  void nameChange(QString);
  void verbosityChanged();
};

class PXMTextEdit : public QTextEdit
{
    Q_OBJECT
   public:
    explicit PXMTextEdit(QWidget* parent);
    void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;
   signals:
    void returnPressed();

   protected:
    void focusOutEvent(QFocusEvent* event) Q_DECL_OVERRIDE;
    void focusInEvent(QFocusEvent* event) Q_DECL_OVERRIDE;
};

class ManualConnect : public QDialog
{
  Q_OBJECT
  Ui::ManualConnect* ui;
  void accept();
public:
  ManualConnect(QWidget *parent);
  ~ManualConnect();
signals:
  void manConnect(QString, int);
};

#endif
