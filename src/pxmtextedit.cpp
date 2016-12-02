#include <pxmtextedit.h>


PXMTextEdit::PXMTextEdit(QWidget *parent) : QTextEdit(parent)
{

}
void PXMTextEdit::keyPressEvent(QKeyEvent* event)
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
void PXMTextEdit::focusInEvent(QFocusEvent *event)
{
    this->setPlaceholderText("");
    QTextEdit::focusInEvent(event);
}
void PXMTextEdit::focusOutEvent(QFocusEvent *event)
{
    this->setPlaceholderText("Enter a message to send!");
    QTextEdit::focusOutEvent(event);
}
