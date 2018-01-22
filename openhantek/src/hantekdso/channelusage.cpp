// SPDX-License-Identifier: GPL-2.0+

#include "channelusage.h"

namespace Dso {

bool ChannelUsage::isUsed(ChannelID channelId) const {
    QMutexLocker locker(usedMutex.get());
    return m_used[channelId].size();
}

void ChannelUsage::addChannelUser(ChannelID channelId, void *object) {
    QMutexLocker locker(usedMutex.get());
    m_used[channelId].insert(object);
    emit usedChanged(channelId, m_used[channelId].size());
}

void ChannelUsage::removeChannelUser(ChannelID channelId, void *object) {
    QMutexLocker locker(usedMutex.get());
    m_used[channelId].erase(object);
    emit usedChanged(channelId, m_used[channelId].size());
}

unsigned ChannelUsage::countUsedChannels() const {
    unsigned c = 0;
    for (auto &b : m_used)
        if (b.size()) ++c;
    return c;
}
}
