#include "pxmtextbrowser.h"

void PXMTextBrowser::resizeEvent(QResizeEvent *event)
{
    emit resizeLabel(this->geometry());
    QTextBrowser::resizeEvent(event);
}
