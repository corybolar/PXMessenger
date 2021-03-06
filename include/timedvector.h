#ifndef TIMEDVECTOR_H
#define TIMEDVECTOR_H

#include <QUuid>
#include <QVector>
#include <QTimer>
#include <QTime>
#include <QDebug>

template <class T>
class TimedVector
{
    struct TimedStruct {
        T* t;
        qint64 epoch;

        TimedStruct() : t(new T()), epoch(QDateTime::currentMSecsSinceEpoch()) {}
        TimedStruct(T t1) : t(new T(t1)), epoch(QDateTime::currentMSecsSinceEpoch()) {}
        ~TimedStruct() noexcept { delete t; }
        TimedStruct(TimedStruct&& v) noexcept : t(v.t), epoch(v.epoch) { v.t = nullptr; }
        TimedStruct(const TimedStruct& v) : t(new T(*(v.t))), epoch(v.epoch) {}
        TimedStruct& operator=(const TimedStruct& v)
        {
            TimedStruct tmp(v);
            *this = std::move(tmp);
            return *this;
        }

        TimedStruct& operator=(TimedStruct&& v) noexcept
        {
            delete t;
            t     = v.t;
            v.t   = nullptr;
            epoch = v.epoch;
            return *this;
        }

        inline bool operator==(TimedStruct v) { return (*v.t == *t && v.epoch == epoch); }
    };
    QVector<TimedStruct> rawVector;
    unsigned int ItemLifeMsecs;

   public:
    TimedVector(unsigned int itemLife) : ItemLifeMsecs(itemLife) {}
    ~TimedVector() {}
    TimedVector(const TimedVector& tv) : ItemLifeMsecs(tv.ItemLifeMsecs)
    {
        for (TimedStruct itr : tv.rawVector) {
            rawVector.append(TimedStruct(itr));
        }
    }

    // TimedVector(TimedVector&& tv) noexcept {}

    inline void append(const T t) { rawVector.append(TimedStruct(t)); }
    bool contains(T t)
    {
        if (rawVector.isEmpty())
            return false;

        for (TimedStruct &itr : rawVector) {
            if (*(itr.t) == t) {
                if (itr.epoch + ItemLifeMsecs > QDateTime::currentMSecsSinceEpoch())
                    return true;
                else {
                    rawVector.takeAt(rawVector.indexOf(itr));
                    return false;
                }
            }
        }
        return false;
    }

    int length()
    {
        pruneItems();
        return rawVector.length();
    }

    int pruneItems()
    {
        int count = 0;
        for (TimedVector::TimedStruct itr : rawVector) {
            if (itr.epoch + ItemLifeMsecs < QDateTime::currentMSecsSinceEpoch()) {
                int index = rawVector.indexOf(itr);
                rawVector.takeAt(index);
                count++;
            }
        }
        return count;
    }

    int remove(T t)
    {
        for (TimedVector::TimedStruct itr : rawVector) {
            if (*(itr.t) == t) {
                rawVector.takeAt(rawVector.indexOf(itr));
                return 0;
            }
        }
        return -1;
    }
};
#endif  // TIMEDVECTOR_H
