#pragma once

#include <QAbstractListModel>
#include <QFlags>
#include <QMetaEnum>
#include <QTime>
#include <deque>

/// A simple Qt Model that just allows to add another debug message entry.
/// Limited to 250 entries, before auto purging of the oldest messages.
class Debug : public QAbstractListModel {
    Q_OBJECT
  public:
    enum class NotificationType : int {
        DeviceCommandSend = 0,
        DeviceDataReceived = 1,
        ChannelUsageChange = 2,
        ManualCommand = 4,
        DSOControl = 8,
        DSOLoop = 16,
    };
    Q_ENUM(NotificationType)
  public:
    Debug(QObject *parent = nullptr);

    void addEntry(const QString &msg, Debug::NotificationType typeEnum);
    void removeAll();
    inline void clearFilter() { m_filter = 0; }
    inline void addToFilter(NotificationType v) { m_filter |= (int)v; }

  private:
    struct Entry {
        QString msg;
        QString timestamp;
        QString type;
        Entry(const QString &msg, const QString &timestamp, const QString &type)
            : msg(msg), timestamp(timestamp), type(type) {}
    };
    std::deque<Entry> entries;
    int m_filter = 0;
    // QAbstractItemModel interface
  public:
    virtual int rowCount(const QModelIndex &) const override { return (int)entries.size(); }
    virtual int columnCount(const QModelIndex &) const override { return 3; }
    virtual QVariant data(const QModelIndex &index, int role) const override;
};
Q_DECLARE_METATYPE(Debug::NotificationType)
