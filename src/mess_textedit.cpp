#include <QTextEdit>
#include <QWidget>

#include <mess_textedit.h>


mess_textedit::mess_textedit(QWidget *parent) : QTextEdit(parent)
{

}

void mess_textedit::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Return)
    {
        emit returnPressed();
    }
    else
    {
        QTextEdit::keyPressEvent(event);
    }
}
