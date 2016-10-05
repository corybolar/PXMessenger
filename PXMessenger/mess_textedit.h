#ifndef MESS_TEXTEDIT_H
#define MESS_TEXTEDIT_H

#include <QObject>
#include <QTextEdit>
#include <QKeyEvent>

class mess_textedit : public QTextEdit
{
    Q_OBJECT
public:
    explicit mess_textedit(QWidget* parent);
    void keyPressEvent(QKeyEvent *event);
signals:
    void returnPressed();
};

#endif // MESS_TEXTEDIT_H
