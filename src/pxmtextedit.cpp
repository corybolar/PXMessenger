#include <pxmtextedit.h>


PXMTextEdit::PXMTextEdit(QWidget *parent) : QTextEdit(parent)
{

}
/**
 * @brief 		This override allows us to send messages by pressing the return key
 *				while this widget has focus
 * @param event	Type of event, only one we care about is key() == Key_Return
 */
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
void PXMTextEdit::focusInEvent(QFocusEvent *e)
{
    if(!(this->placeholderText().isEmpty()))
        this->setPlaceholderText("");
}
void PXMTextEdit::focusOutEvent(QFocusEvent *e)
{
    this->setPlaceholderText("Enter a message to send!");
}
