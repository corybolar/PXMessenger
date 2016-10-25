#ifndef MESS_TEXTEDIT_H
#define MESS_TEXTEDIT_H

#include <QObject>
#include <QTextEdit>
#include <QKeyEvent>
#include <QWidget>

class MessengerTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit MessengerTextEdit(QWidget* parent);
    void keyPressEvent(QKeyEvent *event);
signals:
    void returnPressed();
};

#endif // MESS_TEXTEDIT_H
