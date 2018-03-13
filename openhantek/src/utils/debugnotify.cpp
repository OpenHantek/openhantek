#include "debugnotify.h"

Debug::Debug(QObject *parent) : QAbstractListModel(parent) {
    qRegisterMetaType<Debug::NotificationType>("Debug::NotificationType");
}

void Debug::addEntry(const QString &msg, Debug::NotificationType typeEnum) {
    if (m_filter & (int)typeEnum) return;

    QString typeStr;
    switch (typeEnum) {
    case Debug::NotificationType::DeviceCommandSend:
        typeStr = "Send";
        break;
    case Debug::NotificationType::DeviceDataReceived:
        typeStr = "Received";
        break;
    case Debug::NotificationType::ChannelUsageChange:
        typeStr = "ChannelUsage";
        break;
    case Debug::NotificationType::DSOLoop:
        typeStr = "Loop";
        break;
    case Debug::NotificationType::ManualCommand:
        typeStr = "Manual";
        break;
    case Debug::NotificationType::DSOControl:
        typeStr = "Control";
        break;
    }
    beginInsertRows(QModelIndex(), 0, 1);
    entries.push_back(Entry(msg, QTime::currentTime().toString(), typeStr));
    endInsertRows();
    // Remove last 10 entries if we have more than 250 debug messages already
    if (entries.size() > 250) {
        beginRemoveRows(QModelIndex(), entries.size() - 11, entries.size() - 1);
        entries.erase(entries.begin(), entries.begin() + 10);
        endRemoveRows();
    }
}

void Debug::removeAll() {
    beginResetModel();
    entries.clear();
    endResetModel();
}

QVariant Debug::data(const QModelIndex &index, int role) const {
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        unsigned row = (unsigned)entries.size() - (unsigned)index.row() - 1;
        switch (index.column()) {
        case 0:
            return entries[row].timestamp;
        case 1:
            return entries[row].type;
        case 2:
            return entries[row].msg;
        default:
            return "";
        }
    }
    return QVariant();
}
