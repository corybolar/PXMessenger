#include "pxmmainwindow.h"
#ifndef __WIN32
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <lmcons.h>
#include <windows.h>
#endif
#include "ui_pxmsettingsdialog.h"
#include "pxminireader.h"
#include "pxmconsole.h"

#include <QStringBuilder>
#include <QMessageBox>
#include <QPushButton>

void PXMSettingsDialog::clickedme(QAbstractButton *button)
{
    if((QPushButton*)button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults))
    {
#ifdef _WIN32
        char localHostname[UNLEN+1];
        TCHAR t_user[UNLEN+1];
        DWORD user_size = UNLEN+1;
        if(GetUserName(t_user, &user_size))
            wcstombs(localHostname, t_user, UNLEN+1);
        else
            strcpy(localHostname, "user");
#else
        char localHostname[sysconf(_SC_GETPW_R_SIZE_MAX)];
        struct passwd *user;
        user = getpwuid(getuid());
        if(!user)
            strcpy(localHostname, "user");
        else
            strcpy(localHostname, user->pw_name);
#endif
        ui->tcpPortSpinBox->setValue(0);
        ui->udpPortSpinBox->setValue(0);
        ui->hostnameLineEdit->setText(QString::fromLatin1(localHostname).left(PXMConsts::MAX_HOSTNAME_LENGTH));
        ui->allowMultipleCheckBox->setChecked(false);
        ui->multicastLineEdit->setText(PXMConsts::DEFAULT_MULTICAST_ADDRESS);
    }
    if((QPushButton*)button == ui->buttonBox->button(QDialogButtonBox::Help))
    {
        QMessageBox::information(this, "Help", "Changes to these settings should not be needed under normal conditions.\n\n"
                                               "Care should be taken in adjusting them as they can prevent PXMessenger from functioning properly.\n\n"
                                               "Allowing more than one instance lets the program be run multiple times under the same user.\n(Default:false)\n\n"
                                               "Hostname will only change the first half of your hostname, the computer name will remain.\n(Default:Your Username)\n\n"
                                               "The listener port should be changed only if needed to bypass firewall restrictions. 0 is Auto.\n(Default:0)\n\n"
                                               "The discover port must be the same for all computers that wish to communicate together. 0 is " +
                                 QString::number(PXMConsts::DEFAULT_UDP_PORT) + ".\n(Default:0)\n\n" +
                                 "Multicast Address must be the same for all computers that wish to discover each other. Changes "
                                 "from the default value should only be necessary if firewall restrictions require it."
                                 "\n(Default:" + PXMConsts::DEFAULT_MULTICAST_ADDRESS+ ")\n\n" +
                                 "Debug Verbosity will increase the number of message printed to both the debugging window\n"
                                 "and stdout.  0 hides warnings and debug messages, 1 only hides debug messages, 2 shows all\n"
                                 "(Default:0)\n\n"
                                 "More information can be found at https://github.com/cbpeckles/PXMessenger.");
    }
}

void PXMSettingsDialog::accept()
{
    PXMConsole::LoggerSingleton *logger = PXMConsole::LoggerSingleton::getInstance();
    logger->setVerbosityLevel(ui->verbositySpinBox->value());
    emit verbosityChanged();
    PXMIniReader iniReader;
    iniReader.setAllowMoreThanOne(ui->allowMultipleCheckBox->isChecked());
    iniReader.setHostname(ui->hostnameLineEdit->text().simplified());
    if(hostname != ui->hostnameLineEdit->text().simplified())
    {
        char computerHostname[255] = {};
        gethostname(computerHostname, sizeof(computerHostname));
        emit nameChange(ui->hostnameLineEdit->text().simplified() % "@" % QString::fromUtf8(computerHostname).left(PXMConsts::MAX_COMPUTER_NAME));
    }
    iniReader.setPort("TCP", ui->tcpPortSpinBox->value());
    iniReader.setPort("UDP", ui->udpPortSpinBox->value());
    iniReader.setFont(qApp->font().toString());
    iniReader.setMulticastAddress(ui->multicastLineEdit->text());
    if(tcpPort != ui->tcpPortSpinBox->value()
                    || udpPort != ui->udpPortSpinBox->value()
                    || AllowMoreThanOneInstance != ui->allowMultipleCheckBox->isChecked()
                    || multicastAddress != ui->multicastLineEdit->text())
    {
        QMessageBox::information(this, "Settings Warning", "Changes to these settings will not take effect until PXMessenger has been restarted");
    }
    QDialog::accept();
}

void PXMSettingsDialog::currentFontChanged(QFont font)
{
    qApp->setFont(font);
}

void PXMSettingsDialog::valueChanged(int size)
{
    iniFont = qApp->font();
    iniFont.setPointSize(size);
    qApp->setFont(iniFont);
}

PXMSettingsDialog::PXMSettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::PXMSettingsDialog)
{
    this->setAttribute(Qt::WA_DeleteOnClose, true);
    ui->setupUi(this);
    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    QRegExp ipRegex ("^" + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange + "$");
    QRegExpValidator *ipValidator = new QRegExpValidator(ipRegex, this);
    ui->multicastLineEdit->setValidator(ipValidator);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setFocus();
    ui->verbositySpinBox->setValue(PXMConsole::LoggerSingleton::getVerbosityLevel());
    QObject::connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QObject::connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &PXMSettingsDialog::clickedme);
    QObject::connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, this, &PXMSettingsDialog::currentFontChanged);
    void (QSpinBox:: *signal)(int) = &QSpinBox::valueChanged;
    QObject::connect(ui->fontSizeSpinBox, signal, this, &PXMSettingsDialog::valueChanged);

    readIni();
}
PXMSettingsDialog::~PXMSettingsDialog()
{
    delete ui;
}

void PXMSettingsDialog::readIni()
{
    PXMIniReader iniReader;
    AllowMoreThanOneInstance = iniReader.checkAllowMoreThanOne();
    hostname = iniReader.getHostname(hostname);
    tcpPort = iniReader.getPort("TCP");
    udpPort = iniReader.getPort("UDP");
    multicastAddress = iniReader.getMulticastAddress();
    fontSize = qApp->font().pointSize();
    ui->fontSizeSpinBox->setValue(fontSize);
    ui->tcpPortSpinBox->setValue(tcpPort);
    ui->udpPortSpinBox->setValue(udpPort);
    ui->hostnameLineEdit->setText(hostname.simplified());
    ui->allowMultipleCheckBox->setChecked(AllowMoreThanOneInstance);
    ui->multicastLineEdit->setText(multicastAddress);
}
