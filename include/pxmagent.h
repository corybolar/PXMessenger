#ifndef PXMAGENT_H
#define PXMAGENT_H

#include <QObject>
#include <QScopedPointer>

#include <pxmmainwindow.h>
#include <pxmpeerworker.h>
#include <QSimpleUpdater/include/QSimpleUpdater.h>

struct PXMAgentPrivate;
class PXMAgent : public QObject
{
    Q_OBJECT

    QScopedPointer<PXMAgentPrivate> d_ptr;

   public:
    // Default
    PXMAgent(QObject* parent = 0);
    // Copy
    PXMAgent(const PXMAgent& agent) = delete;
    // Copy Assignment
    PXMAgent& operator=(const PXMAgent& agent) = delete;
    // Move
    PXMAgent(PXMAgent&& agent) noexcept = delete;
    // Move Assignment
    PXMAgent& operator=(PXMAgent&& agent) noexcept = delete;
    // Destructor
    ~PXMAgent();

    int init();
public slots:
    void updateChangelog(const QString &str);
};

#endif  // PXMAGENT_H
