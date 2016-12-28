#include "pxmmainwindow.h"
#include "pwd.h"
#include "ui_pxmsettingsdialog.h"
#include "QDialogButtonBox"
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
    }
    if((QPushButton*)button == ui->buttonBox->button(QDialogButtonBox::Help))
    {
        QMessageBox::information(this, "Help", "Changes to these settings should not be needed under normal conditions.\n\n"
                                               "Care should be taken in adjusting them as they can prevent PXMessenger from functioning properly.\n\n"
                                               "Allowing more than one instance lets the program be run multiple times under the same user.\n(Default:false)\n\n"
                                               "Hostname will only change the first half of your hostname, the computer name will remain.\n(Default:Your Username)\n\n"
                                               "The listener port should be changed only if needed to bypass firewall restrictions. 0 is Auto.\n(Default:0)\n\n"
                                               "The discover port must be the same for all computers that wish to communicate together. 0 is " +
                                                QString::number(PXMConsts::DEFAULT_UDP_PORT) + ".\n(Default:0)\n\n"
                                               "Debug Verbosity will increase the number of message printed to both the debugging window\n"
                                               "and stdout.  0 hides warnings and debug messages, 1 only hides debug messages, 0 shows all\n"
                                               "(Default:0)\n\n"
                                               "More information can be found at https://github.com/cbpeckles/PXMessenger.");
    }
}

void PXMSettingsDialog::accept()
{
    PXMConsole::LoggerSingleton *logger = PXMConsole::LoggerSingleton::getInstance();
    logger->setVerbosityLevel(ui->verbositySpinBox->value());
    MessIniReader iniReader;
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
    if(tcpPort != ui->tcpPortSpinBox->value() || udpPort != ui->udpPortSpinBox->value() || AllowMoreThanOneInstance != ui->allowMultipleCheckBox->isChecked())
        QMessageBox::information(this, "Settings Warning", "Changes to these settings will not take effect until PXMessenger has been restarted");
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
    ui->buttonBox->button(QDialogButtonBox::Ok)->setFocus();
    QObject::connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QObject::connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &PXMSettingsDialog::clickedme);
    QObject::connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, this, &PXMSettingsDialog::currentFontChanged);
    void (QSpinBox:: *signal)(int) = &QSpinBox::valueChanged;
    QObject::connect(ui->fontSizeSpinBox, signal, this, &PXMSettingsDialog::valueChanged);

    readIni();
}

void PXMSettingsDialog::readIni()
{
    MessIniReader iniReader;
    AllowMoreThanOneInstance = iniReader.checkAllowMoreThanOne();
    hostname = iniReader.getHostname(hostname);
    tcpPort = iniReader.getPort("TCP");
    udpPort = iniReader.getPort("UDP");
    fontSize = qApp->font().pointSize();
    ui->fontSizeSpinBox->setValue(fontSize);
    ui->tcpPortSpinBox->setValue(tcpPort);
    ui->udpPortSpinBox->setValue(udpPort);
    ui->hostnameLineEdit->setText(hostname.simplified());
    ui->allowMultipleCheckBox->setChecked(AllowMoreThanOneInstance);
}
