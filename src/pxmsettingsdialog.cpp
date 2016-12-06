#include "pxmsettingsdialog.h"

void PXMSettingsDialog::clickedme(QAbstractButton *button)
{
    if((QPushButton*)button == buttonBox->button(QDialogButtonBox::RestoreDefaults))
    {
#ifdef _WIN32
        char localHostname[UNLEN+1];
        TCHAR t_user[UNLEN+1];
        DWORD user_size = UNLEN+1;
        if(GetUserName(t_user, &user_size))
            wcstombs(localHostname, t_user, UNLEN+1);
        else
            strcpy(localHostname, "user\0");
#else
        char localHostname[sysconf(_SC_GETPW_R_SIZE_MAX)];
        struct passwd *user;
        user = getpwuid(getuid());
        if(!user)
            strcpy(localHostname, "user\0");
        else
            strcpy(localHostname, user->pw_name);
#endif
        spinBox->setValue(0);
        spinBox_2->setValue(0);
        lineEdit->setText(QString::fromLatin1(localHostname).left(MAX_HOSTNAME_LENGTH));
        checkBox->setChecked(false);
    }
    if((QPushButton*)button == buttonBox->button(QDialogButtonBox::Help))
    {
        QMessageBox::information(this, "Help", "Changes to these settings should not be needed under normal conditions.\n\n"
                                               "Care should be taken in adjusting them as they can prevent PXMessenger from functioning properly.\n\n"
                                               "Allowing more than on instance lets the program be run multiple times under the same user.\n(Default:false)\n\n"
                                               "Hostname will only change the first half of your hostname, the computer name will remain.\n(Default:Your Username)\n\n"
                                               "The listener port should be changed only if needed to bypass firewall restrictions. 0 is Auto.\n(Default:0)\n\n"
                                               "The discover port must be the same for all computers that wish to communicate together. 0 is 53273.\n(Default:0)\n\n"
                                               "More information can be found at https://github.com/cbpeckles/PXMessenger.");
    }
}

void PXMSettingsDialog::accept()
{
    MessIniReader iniReader;
    iniReader.setAllowMoreThanOne(this->checkBox->isChecked());
    iniReader.setHostname(this->lineEdit->text());
    iniReader.setPort("TCP", this->spinBox->value());
    iniReader.setPort("UDP", this->spinBox_2->value());
    iniReader.setFont(qApp->font().toString());
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

void PXMSettingsDialog::setupUi()
{
    if (this->objectName().isEmpty())
        this->setObjectName(QStringLiteral("Settings"));
    this->setMinimumSize(QSize(420, 181));
    this->setMaximumSize(QSize(800, 300));
    this->setLayoutDirection(Qt::LeftToRight);
    this->setSizeGripEnabled(true);
    gridLayout = new QGridLayout(this);
    gridLayout->setObjectName(QStringLiteral("gridLayout"));
    verticalSpacer = new QSpacerItem(20, 120, QSizePolicy::Minimum, QSizePolicy::Expanding);

    gridLayout->addItem(verticalSpacer, 6, 0, 1, 2);

    checkBox = new QCheckBox(this);
    checkBox->setObjectName(QStringLiteral("checkBox"));
    checkBox->setLayoutDirection(Qt::RightToLeft);

    gridLayout->addWidget(checkBox, 0, 1, 1, 1);

    label_2 = new QLabel(this);
    label_2->setObjectName(QStringLiteral("label_2"));
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
    label_2->setSizePolicy(sizePolicy);

    gridLayout->addWidget(label_2, 1, 0, 1, 1);

    lineEdit = new QLineEdit(this);
    lineEdit->setObjectName(QStringLiteral("lineEdit"));
    lineEdit->setMaxLength(MAX_HOSTNAME_LENGTH);

    gridLayout->addWidget(lineEdit, 1, 1, 1, 1);

    label = new QLabel(this);
    label->setObjectName(QStringLiteral("label"));

    gridLayout->addWidget(label, 0, 0, 1, 1);

    label_3 = new QLabel(this);
    label_3->setObjectName(QStringLiteral("label_3"));

    gridLayout->addWidget(label_3, 2, 0, 1, 1);

    label_4 = new QLabel(this);
    label_4->setObjectName(QStringLiteral("label_4"));

    gridLayout->addWidget(label_4, 3, 0, 1, 1);

    buttonBox = new QDialogButtonBox(this);
    buttonBox->setObjectName(QStringLiteral("buttonBox"));
    buttonBox->setLayoutDirection(Qt::LeftToRight);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Help|QDialogButtonBox::Ok|QDialogButtonBox::RestoreDefaults);
    buttonBox->setCenterButtons(true);

    gridLayout->addWidget(buttonBox, 7, 0, 1, 2);

    spinBox = new QSpinBox(this);
    spinBox->setObjectName(QStringLiteral("spinBox"));
    spinBox->setMaximum(65535);

    gridLayout->addWidget(spinBox, 2, 1, 1, 1);

    spinBox_2 = new QSpinBox(this);
    spinBox_2->setObjectName(QStringLiteral("spinBox_2"));
    spinBox_2->setMaximum(65535);

    gridLayout->addWidget(spinBox_2, 3, 1, 1, 1);

    fontComboBox = new QFontComboBox(this);
    gridLayout->addWidget(fontComboBox, 4, 1, 1, 1);

    label_5 = new QLabel(this);
    gridLayout->addWidget(label_5, 4, 0, 1, 1);

    label_6 = new QLabel(this);
    gridLayout->addWidget(label_6, 5, 0, 1, 1);

    spinBox_3 = new QSpinBox(this);
    spinBox_3->setMaximum(24);
    spinBox_3->setMinimum(8);
    spinBox_3->setValue(qApp->font().pointSize());
    gridLayout->addWidget(spinBox_3, 5, 1, 1, 1);


    retranslateUi();
    QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QObject::connect(buttonBox, &QDialogButtonBox::clicked, this, &PXMSettingsDialog::clickedme);
    QObject::connect(fontComboBox, &QFontComboBox::currentFontChanged, this, &PXMSettingsDialog::currentFontChanged);
    void (QSpinBox:: *signal)(int) = &QSpinBox::valueChanged;
    QObject::connect(spinBox_3, signal, this, &PXMSettingsDialog::valueChanged);

    //QMetaObject::connectSlotsByName(this);
}

void PXMSettingsDialog::retranslateUi()
{
    this->setWindowTitle(QApplication::translate("Settings", "Settings", 0));
    checkBox->setText(QString());
    label_2->setText(QApplication::translate("Settings", "Hostname", 0));
    label->setText(QApplication::translate("Settings", "Allow more than one instance", 0));
    label_3->setText(QApplication::translate("Settings", "Preferred TCP Listener port", 0));
    label_4->setText(QApplication::translate("Settings", "Preferred UDP Listener port", 0));
    label_5->setText(QApplication::translate("Settings", "Font Family", 0));
    label_6->setText(QApplication::translate("Settings", "Font Size", 0));
}

void PXMSettingsDialog::readIni()
{
    MessIniReader iniReader;
    AllowMoreThanOneInstance = iniReader.checkAllowMoreThanOne();
    hostname = iniReader.getHostname("");
    tcpPort = iniReader.getPort("TCP");
    udpPort = iniReader.getPort("UDP");
    fontSize = qApp->font().pointSize();
    spinBox_3->setValue(fontSize);
    spinBox->setValue(tcpPort);
    spinBox_2->setValue(udpPort);
    lineEdit->setText(hostname);
    checkBox->setChecked(AllowMoreThanOneInstance);
}
