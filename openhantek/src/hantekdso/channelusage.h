// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "hantekprotocol/types.h"
#include <QMutex>
#include <QObject>
#include <memory>
#include <set>
#include <vector>

namespace Dso {

/**
 * DsoControl has an automatic channel enable mechanism, based on the usage of a hardware channel.
 * You first request this ChannelUsage object from DsoControl and then add/remove channel users.
 * The hardware channels will be enabled/disabled based on usage/reference count.
 */
class ChannelUsage : public QObject {
    Q_OBJECT
  public:
    inline ChannelUsage(unsigned channels) { m_used.resize(channels); }

    /// Return true if the channel is used by a voltage, spectrum graph or a math channel. This method is
    /// thread-safe. HantekDsoControl should use m_used.size() instead in hot code paths.
    bool isUsed(ChannelID channelId) const;
    /// Add a user of this channel. As soon as the channel is used by at least one object, it will
    /// be activated. This method is to be used by the scope settings setVisible methods.
    /// Thread-safe.
    void addChannelUser(ChannelID channelId, void *object);
    /// Remove a user of this channel. As soon as the channel is not used anymore, it will be deactivated.
    /// This method is to be used by the scope settings setVisible methods.
    /// Thread-safe.
    void removeChannelUser(ChannelID channelId, void *object);
    /// Counts the currently used hardware channels.
    /// This method will access shared data between DsoControl and device settings and is thread-safe.
    unsigned countUsedChannels() const;

  private:
    mutable std::unique_ptr<QMutex> usedMutex = std::unique_ptr<QMutex>(new QMutex);
    std::vector<std::set<void *>> m_used; ///< objects, that are using this channel
  signals:
    void usedChanged(ChannelID channelId, bool used);
};
}
