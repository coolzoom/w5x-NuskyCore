/*
 * Copyright (C) 2013-2018 NuskyCore <http://www.nuskycore.org/>
 * Copyright (C) 2008-2018 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2018 MaNGOS <https://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/** \file
    \ingroup Nuskyd
 */

#include "Common.h"
#include "Config.h"
#include "Log.h"
#include "RARunnable.h"
#include "World.h"

#include <ace/Reactor_Impl.h>
#include <ace/TP_Reactor.h>
#include <ace/Dev_Poll_Reactor.h>
#include <ace/Acceptor.h>
#include <ace/SOCK_Acceptor.h>

#include "RASocket.h"

RARunnable::RARunnable()
{
    ACE_Reactor_Impl* imp;

#if defined (ACE_HAS_EVENT_POLL) || defined (ACE_HAS_DEV_POLL)
    imp = new ACE_Dev_Poll_Reactor();
    imp->max_notify_iterations (128);
    imp->restart (1);
#else
    imp = new ACE_TP_Reactor();
    imp->max_notify_iterations (128);
#endif

    m_Reactor = new ACE_Reactor (imp, 1);
}

RARunnable::~RARunnable()
{
    delete m_Reactor;
}

void RARunnable::run()
{
    if (!sConfigMgr->GetBoolDefault("Ra.Enable", false))
        return;

    ACE_Acceptor<RASocket, ACE_SOCK_ACCEPTOR> acceptor;

    uint16 raPort = uint16(sConfigMgr->GetIntDefault("Ra.Port", 3443));
    std::string stringIp = sConfigMgr->GetStringDefault("Ra.IP", "0.0.0.0");
    ACE_INET_Addr listenAddress(raPort, stringIp.c_str());

    if (acceptor.open(listenAddress, m_Reactor) == -1)
    {
        NC_LOG_ERROR("server.worldserver", "NuskyCore RA can not bind to port %d on %s", raPort, stringIp.c_str());
        return;
    }

    NC_LOG_INFO("server.worldserver", "Starting NuskyCore RA on port %d on %s", raPort, stringIp.c_str());

    while (!World::IsStopped())
    {
        ACE_Time_Value interval(0, 100000);
        if (m_Reactor->run_reactor_event_loop(interval) == -1)
            break;
    }

    NC_LOG_DEBUG("server.worldserver", "NuskyCore RA thread exiting");
}
