/*

	killm.c:	A Windows counterpart to the "killm" script
			used on POSIX machines.  Terminates any
			lingering ION processes, then releases all
			persistent shared-memory objects by telling
			winion to terminate.

									*/
/*	Copyright (c) 2011, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Alan Hylton, NASA Glenn Research Center			*/

#include "ion.h"
#include <tlhelp32.h>

char *processes[] = { "bsscounter.exe", "lt-bsscounter.exe", \
"bssdriver.exe", "lt-bssdriver.exe", \
"bssrecv.exe", "lt-bssrecv.exe", \
"bssStreamingApp.exe", "lt-bssStreamingApp.exe", \
"bsspadmin.exe", "lt-bsspadmin.exe", \
"bsspadmin.exe", "lt-bsspadmin.exe", \
"bsspcli.exe", "lt-bsspcli.exe", \
"bsspclo.exe", "lt-bsspclo.exe", \
"bsspclock.exe", "lt-bsspclock.exe", \
"bsspclock.exe", "lt-bsspclock.exe", \
"cgrfetch.exe", "lt-cgrfetch.exe", \
"bibeclo.exe", "lt-bibeclo.exe", \
"imdadmin.exe", "lt-imdadmin.exe", \
"imcfw.exe", "lt-imcfw.exe", \
"dtpcadmin.exe", "lt-dtpcadmin.exe", \
"dtpcclock.exe", "lt-dtpcclock.exe", \
"dtpcd.exe", "lt-dtpcd.exe", \
"dtpcreceive.exe", "lt-dtpcreceive.exe", \
"dtpcsend.exe", "lt-dtpcsend.exe", \
"ionrestart.exe", "lt-ionrestart.exe", \
"ionwarn.exe", "lt-ionwarn.exe", \
"ionsecadmin.exe", "lt-ionsecadmin.exe", \
"nm_agent.exe", "lt-nm_agent.exe", \
"nm_mgr.exe", "lt-nm_mgr.exe", \
"ramsgate.exe", "lt-ramsgate.exe", \
"bpadmin.exe", "lt-bpadmin.exe", \
"bpchat.exe", "lt-bpchat.exe", \
"bpclock.exe", "lt-bpclock.exe", \
"bptransit.exe", "lt-bptransit.exe", \
"bpcounter.exe", "lt-bpcounter.exe", \
"bpdriver.exe", "lt-bpdriver.exe", \
"bpecho.exe", "lt-bpecho.exe", "bping.exe", \
"lt-bping.exe", "bprecvfile.exe", "lt-bprecvfile.exe", \
"bpsendfile.exe", "lt-bpsendfile.exe", "bpsink.exe", \
"lt-bpsink.exe", "bpsource.exe", "lt-bpsource.exe", \
"bpstats.exe", "lt-bpstats.exe", "bpstats2.exe", \
"lt-bpstats2.exe", "bptrace.exe", "lt-bptrace.exe", \
"brsccla.exe", "lt-brsccla.exe", "brsscla.exe", \
"lt-brsscla.exe", "dgr2file.exe", "lt-dgr2file.exe", \
"dgrcla.exe", "lt-dgrcla.exe", "dtn2admin.exe", \
"lt-dtn2admin.exe", "dtn2adminep.exe", "lt-dtn2adminep.exe", \
"dtn2fw.exe", "lt-dtn2fw.exe", "file2dgr.exe", \
"lt-file2dgr.exe", "file2sdr.exe", "lt-file2sdr.exe", \
"file2sm.exe", "lt-file2sm.exe", "file2tcp.exe", \
"lt-file2tcp.exe", "file2udp.exe", "lt-file2udp.exe", \
"ionadmin.exe", "lt-ionadmin.exe", "ipnadmin.exe", \
"lt-ipnadmin.exe", "ipnadminep.exe", "lt-ipnadminep.exe", \
"ipnfw.exe", "lt-ipnfw.exe", "lgagent.exe", \
"lt-lgagent.exe", "lgsend.exe", "lt-lgsend.exe", \
"ltpadmin.exe", "lt-ltpadmin.exe", "ltpcli.exe", \
"lt-ltpcli.exe", "ltpclo.exe", "lt-ltpclo.exe", \
"ltpclock.exe", "lt-ltpclock.exe", "ltpcounter.exe", \
"lt-ltpcounter.exe", "ltpdriver.exe", "lt-ltpdriver.exe", \
"ltpmeter.exe", "lt-ltpmeter.exe", "owltsim.exe", \
"lt-owltsim.exe", "owlttb.exe", "lt-owlttb.exe", \
"psmshell.exe", "lt-psmshell.exe", "psmwatch.exe", \
"lt-psmwatch.exe", "ramstest.exe", "lt-ramstest.exe", \
"rfxclock.exe", "lt-rfxclock.exe", "sdr2file.exe", \
"lt-sdr2file.exe", "sdrmend.exe", "lt-sdrmend.exe", \
"sdrwatch.exe", "lt-sdrwatch.exe", "sm2file.exe", \
"lt-sm2file.exe", "smlistsh.exe", "lt-smlistsh.exe", \
"tcpcli.exe", "lt-tcpcli.exe", "tcpclo.exe", \
"lt-tcpclo.exe", "stcpcli.exe", "lt-stcpcli.exe", \
"stcpclo.exe", "lt-stcpclo.exe", "tcp2file.exe", \
"lt-tcp2file.exe", "udpcli.exe", "lt-udpcli.exe", \
"udpclo.exe", "lt-udpclo.exe", "udplsi.exe", \
"lt-udplsi.exe", "udplso.exe", "lt-udplso.exe", \
"udp2file.exe", "lt-udp2file.exe"
#ifndef NASA_PROTECTED_FLIGHT_CODE
,"amsbenchr.exe", "lt-amsbenchr.exe", "amsbenchs.exe", \
"lt-amsbenchs.exe", "amsd.exe", "lt-amsd.exe", \
"amshello.exe", "lt-amshello.exe", "amslog.exe", \
"lt-amslog.exe", "amslogport.exe", "lt-amslogport.exe", \
"amsshell.exe", "lt-amsshell.exe", "bputa.exe", \
"lt-bputa.exe", "cfdpadmin.exe", "lt-cfdpadmin.exe", \
"cfdpclock.exe", "lt-cfdpclock.exe", "cfdptest.exe", \
"lt-cfdptest.exe", "bpcp.exe", "lt-bpcp.exe", \
"bpcpd.exe", "lt-bpcpd.exe"
#endif
};

void kill(char *name)
{
	HANDLE		snap;
	HANDLE		proc;
	PROCESSENTRY32	pe;
	char		found = 0;

	snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE)
	{
		printf("Unable to terminate non-winion tasks\n");
		return;
	}

	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(snap, &pe))
	{
		printf("Unable to terminate non-winion tasks\n");
		CloseHandle(snap);
		return;
	}

	do
	{
		if (!strcmp(pe.szExeFile, name))
		{
			found = 1;	
			proc = OpenProcess(PROCESS_TERMINATE, 0,
					pe.th32ProcessID);
			TerminateProcess(proc, 0);
			CloseHandle(proc);
		}
	} while (Process32Next(snap, &pe));

	CloseHandle(snap);
	if (strlen(name) <= 13)
	{
		printf("\t");
	}

	if (found)
	{
		printf("found\n");
	}
	else
	{
		printf("not found\n");
	}
}

int main(int argc, char **argv)
{
	char	*pipeName = "\\\\.\\pipe\\ion.pipe";
	char	msg[5] = {0, 0, 0, 0, 0};

	HANDLE	hPipe;
	DWORD	bytes;
	BOOL	success = FALSE;
	int	i;

	for(i = 0; i < sizeof(processes) / sizeof(processes[0]); i++)
	{
		printf("Trying to kill %s...\t", processes[i]);
		kill(processes[i]);
	}

	printf("\nTrying to kill winion...\n");
	if (WaitNamedPipe(pipeName, 100) == 0)
	{
		printf("Pipe DNE\n");
		return 0;
	}

	hPipe = CreateFile(pipeName, GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if(hPipe == INVALID_HANDLE_VALUE)
	{
		printf("Cannot open pipe\n");
		return 0;
	}

	success = WriteFile(hPipe, msg, 5, &bytes, 0);
	if (!success)
	{
		printf("could not write message\n");
		return 0;
	}

	printf("Sent message\n");
	return 0;
}
