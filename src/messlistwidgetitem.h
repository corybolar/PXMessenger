#ifndef MESSLISTWIDGETITEM_H
#define MESSLISTWIDGETITEM_H

#include <QObject>
#include <QListWidgetItem>
#include <QWidget>
#include <QUuid>

class MessListWidgetItem : public QListWidgetItem
{
public:
    explicit MessListWidgetItem(QListWidget *parent, QString key);
    explicit MessListWidgetItem(QListWidget* parent = 0);
    void setKey(QUuid keyForHash);
    //~MessListWidgetItem();
private:
    QUuid key;
};

#endif // MESSLISTWIDGETITEM_H
