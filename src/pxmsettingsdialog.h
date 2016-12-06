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
#include <QtWidgets/QFontComboBox>

#include "pxminireader.h"
#include "pxmdefinitions.h"

#ifdef _WIN32
#include <lmcons.h>
#else
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

QT_BEGIN_NAMESPACE

class PXMSettingsDialog : public QDialog
{
private slots:
    void clickedme(QAbstractButton *button);
    void accept();
    void currentFontChanged(QFont font);
    void valueChanged(int size);

public:
    explicit PXMSettingsDialog(QWidget* parent) : QDialog(parent) {}

    bool AllowMoreThanOneInstance;
    QString hostname;
    int tcpPort;
    int udpPort;
    QFont iniFont;
    int fontSize;

    QGridLayout *gridLayout;
    QSpacerItem *verticalSpacer;
    QCheckBox *checkBox;
    QLabel *label_2;
    QLineEdit *lineEdit;
    QLabel *label;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_5;
    QLabel *label_6;
    QDialogButtonBox *buttonBox;
    QSpinBox *spinBox;
    QSpinBox *spinBox_2;
    QFontComboBox *fontComboBox;
    QSpinBox *spinBox_3;

    void setupUi();

    void retranslateUi();

    void readIni();

};

namespace Ui {
class Settings: public PXMSettingsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // DIALOG_20__2D__20_UNTITLEDXQ7926_H
