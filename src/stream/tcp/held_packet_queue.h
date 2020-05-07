//--------------------------------------------------------------------------
// Copyright (C) 2020-2020 Cisco and/or its affiliates. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------
//
// held_packet_queue.h author Silviu Minut <sminut@cisco.com>

#ifndef HELD_PACKET_QUEUE_H
#define HELD_PACKET_QUEUE_H

#include <daq_common.h>

#include <ctime>
#include <list>

class TcpStreamTracker;

class HeldPacket
{
public:

    HeldPacket(DAQ_Msg_h msg, uint32_t seq, const timeval& timeout, TcpStreamTracker& trk);

    bool has_expired(const timeval& cur_time)
    {
        return !timercmp(&cur_time, &expiration, <);
    }

    TcpStreamTracker& get_tracker() const { return tracker; }
    DAQ_Msg_h get_daq_msg() const { return daq_msg; }
    uint32_t get_seq_num() const { return seq_num; }

private:
    DAQ_Msg_h daq_msg;
    uint32_t seq_num;
    timeval expiration;
    TcpStreamTracker& tracker;
};

class HeldPacketQueue
{
public:

    using list_t = std::list<HeldPacket>;
    using iter_t = list_t::iterator;

    iter_t append(DAQ_Msg_h msg, uint32_t seq, TcpStreamTracker& trk);
    void erase(iter_t it);
    void execute(const timeval& cur_time, int max_remove);

    void set_timeout(uint32_t ms)
    {
        timeout.tv_sec = ms / 1000;
        timeout.tv_usec = static_cast<suseconds_t>((ms % 1000) * 1000);
    }
    bool empty() const { return q.empty(); }

private:
    timeval timeout = {1, 0};
    list_t q;
};

#endif

