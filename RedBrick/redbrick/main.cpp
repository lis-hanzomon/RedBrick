/* 
 * This file is part of RedBrick.
 * Copyright (c) 2018 Link Information Systems Co., Ltd.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "daemonize.h"
#include "RaspiOnvifPlatform.h"
#include "mhonvif/SZ_OnvifProcess.h"

// #define OUTPUT_DEBUG_LOG
#include "mhengine/MH_DebugLog.h"

#include <syslog.h>

static SZ_OnvifProcess* process = NULL;

static void signal_handler(int signal_number)
{
	process->SetTerminate();
}

static int MainLoop()
{
	syslog(LOG_INFO, "Started RedBrick.\n");
	
	SZ17_Initialize(NULL);

	DBG_TRACE("### Process Start ###");
	
	RaspiOnvifPlatform* platform = new RaspiOnvifPlatform();
	if (!platform->Initialize("/etc/redbrick"))
	{
		return 0;
	}
	
	process = new SZ_OnvifProcess(platform);

	DBG_PRINT("### Onvif Server Start ###");
	
	if (process->Start())
	{
		signal(SIGTERM, signal_handler);
		
		process->WaitForTerminate();
		
		signal(SIGTERM, NULL);
	}
	
	DBG_PRINT("### Onvif Server Stop ###");
	
	process->Stop();
	delete process;
	process = NULL;
	
	platform->Terminate();
	delete platform;
	platform = NULL;
	
	DBG_TRACE("### End ###");
	
	SZ17_Terminate();
	
	syslog(LOG_INFO, "Stopped RedBrick.\n");

	return 0;
}

int main(int argc, char* argv[])
{
	if(!daemonize("/var/run/redbrick.pid", "redbrick", LOG_PID, LOG_DAEMON))
	{
		fprintf(stderr, "failed to daemonize.\n");
		return 2;
	}
	return MainLoop();
}
