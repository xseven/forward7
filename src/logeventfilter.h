#pragma once

#include <QEvent>
#include <QObject>

struct LogEvent : public QEvent {
    static const QEvent::Type logEventType = static_cast<QEvent::Type>(2748);
    LogEvent(const QString& msg)
        : QEvent(logEventType)
        , msg(msg)
    {
    }

    QString message() const
    {
        return msg;
    }

private:
    QString msg;
};

class LogEventFilter : public QObject {
public:
    explicit LogEventFilter(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
    bool eventFilter(QObject* watched, QEvent* event)
    {
        if (event->type() == LogEvent::logEventType) {
            return false;
        }
        return QObject::eventFilter(watched, event);
    }
};
