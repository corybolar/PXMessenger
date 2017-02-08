#ifndef PXMAGENT_H
#define PXMAGENT_H

#include <QObject>
#include <QScopedPointer>

#include <pxmmainwindow.h>
#include <pxmpeerworker.h>

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

public slots:
    void updateChangelog(const QString &str);
    int preInit();
private slots:
    int init();
};

#endif  // PXMAGENT_H
