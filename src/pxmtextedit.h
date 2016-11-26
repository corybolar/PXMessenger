#ifndef PXMTEXTEDIT_H
#define PXMTEXTEDIT_H

#include <QTextEdit>
#include <QKeyEvent>
#include <QWidget>

class PXMTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit PXMTextEdit(QWidget* parent);
    void keyPressEvent(QKeyEvent *event);
signals:
    void returnPressed();
protected:
    void focusOutEvent(QFocusEvent);
    void focusInEvent(QFocusEvent);
};

#endif // MESS_TEXTEDIT_H
