/********************************************************************************
** Form generated from reading UI file 'Dialog - untitledxQ7926.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef DIALOG_20__2D__20_UNTITLEDXQ7926_H
#define DIALOG_20__2D__20_UNTITLEDXQ7926_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QMessageBox>
#include <messinireader.h>

QT_BEGIN_NAMESPACE

class settingsDialog : public QDialog
{
public:
    explicit settingsDialog(QWidget* parent) : QDialog(parent) {}

    bool AllowMoreThanOneInstance;
    QString hostname;
    int tcpPort;
    int udpPort;

    QGridLayout *gridLayout;
    QSpacerItem *verticalSpacer;
    QCheckBox *checkBox;
    QLabel *label_2;
    QLineEdit *lineEdit;
    QLabel *label;
    QLabel *label_3;
    QLabel *label_4;
    QDialogButtonBox *buttonBox;
    QSpinBox *spinBox;
    QSpinBox *spinBox_2;

    void setupUi()
    {
        if (this->objectName().isEmpty())
            this->setObjectName(QStringLiteral("Settings"));
        this->resize(366, 181);
        this->setMinimumSize(QSize(366, 181));
        this->setMaximumSize(QSize(600, 181));
        this->setLayoutDirection(Qt::LeftToRight);
        this->setSizeGripEnabled(true);
        gridLayout = new QGridLayout(this);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        verticalSpacer = new QSpacerItem(20, 120, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 5, 0, 1, 2);

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

        gridLayout->addWidget(buttonBox, 6, 0, 1, 2);

        spinBox = new QSpinBox(this);
        spinBox->setObjectName(QStringLiteral("spinBox"));
        spinBox->setMaximum(65535);

        gridLayout->addWidget(spinBox, 2, 1, 1, 1);

        spinBox_2 = new QSpinBox(this);
        spinBox_2->setObjectName(QStringLiteral("spinBox_2"));
        spinBox_2->setMaximum(65535);

        gridLayout->addWidget(spinBox_2, 3, 1, 1, 1);


        retranslateUi();
        QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

        QMetaObject::connectSlotsByName(this);
    } // setupUi

    void retranslateUi()
    {
        this->setWindowTitle(QApplication::translate("Settings", "Settings", 0));
        checkBox->setText(QString());
        label_2->setText(QApplication::translate("Settings", "Hostname", 0));
        label->setText(QApplication::translate("Settings", "Allow more than one instance", 0));
        label_3->setText(QApplication::translate("Settings", "Preferred TCP Listener port", 0));
        label_4->setText(QApplication::translate("Settings", "Preferred UDP Listener port", 0));
    } // retranslateUi

    void readIni()
    {
        MessIniReader iniReader;
        AllowMoreThanOneInstance = iniReader.checkAllowMoreThanOne();
        hostname = iniReader.getHostname("");
        tcpPort = iniReader.getPort("TCP");
        udpPort = iniReader.getPort("UDP");
        spinBox->setValue(tcpPort);
        spinBox_2->setValue(udpPort);
        lineEdit->setText(hostname);
        checkBox->setChecked(AllowMoreThanOneInstance);
    }


private slots:
    void accept()
    {
        QSettings inisettings(QSettings::IniFormat, QSettings::UserScope, "PXMessenger", "PXMessenger", NULL);
        inisettings.setValue("config/AllowMoreThanOneInstance", this->checkBox->isChecked());
        inisettings.setValue("hostname/hostname", this->lineEdit->text());
        int tcpPort = this->spinBox->value();
        inisettings.setValue("port/TCP", tcpPort);
        int udpPort = this->spinBox_2->value();
        inisettings.setValue("port/UDP", udpPort);
        QMessageBox::information(this, "", "Changes to these settings will not take effect until PXMessenger has been restarted");
        QDialog::accept();
    }
};

namespace Ui {
    class Settings: public settingsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // DIALOG_20__2D__20_UNTITLEDXQ7926_H
