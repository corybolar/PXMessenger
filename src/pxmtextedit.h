#ifndef PXMTEXTEDIT_H
#define PXMTEXTEDIT_H

#include <QTextEdit>

class PXMTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit PXMTextEdit(QWidget* parent);
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
signals:
    void returnPressed();
protected:
    void focusOutEvent(QFocusEvent *event) Q_DECL_OVERRIDE;
    void focusInEvent(QFocusEvent *event) Q_DECL_OVERRIDE;
};

#endif // MESS_TEXTEDIT_H
