/*

	bpadmin.c:	BP node adminstration interface.

									*/
/*									*/
/*	Copyright (c) 2003, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "bpP.h"
#include "crypto.h"

static int	_echo(int *newValue)
{
	static int	state = 0;

	if (newValue)
	{
		if (*newValue == 1)
		{
			state = 1;
		}
		else
		{
			state = 0;
		}
	}

	return state;
}

static void	printText(char *text)
{
	if (_echo(NULL))
	{
		writeMemo(text);
	}

	PUTS(text);
}

static void	handleQuit()
{
	printText("Please enter command 'q' to stop the program.");
}

static void	printSyntaxError(int lineNbr)
{
	char	buffer[80];

	isprintf(buffer, sizeof buffer, "Syntax error at line %d of bpadmin.c",
			lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage()
{
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\tv\tPrint version of ION and crypto suite.");
	PUTS("\t1\tInitialize");
	PUTS("\t   1");
	PUTS("\ta\tAdd");
	PUTS("\t   a scheme <scheme name> '<forwarder cmd>' '<admin app cmd>'");
	PUTS("\t   a endpoint <endpoint name> {q|x} ['<recv script>']");
	PUTS("\t   a protocol <protocol name> <payload bytes per frame> \
<overhead bytes per frame> [<nominal data rate, in bytes/sec>]");
	PUTS("\t   a induct <protocol name> <duct name> '<CLI command>'");
	PUTS("\t   a outduct <protocol name> <duct name> '<CLO command>' [max \
payload length]");
	PUTS("\tc\tChange");
	PUTS("\t   c scheme <scheme name> '<forwarder cmd>' '<admin app cmd>'");
	PUTS("\t   c endpoint <endpoint name> {q|x} ['<recv script>']");
	PUTS("\t   c induct <protocol name> <duct name> '<CLI command>'");
	PUTS("\t   c outduct <protocol name> <duct name> '<CLO command>' [max \
payload length");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} scheme <scheme name>");
	PUTS("\t   {d|i} endpoint <endpoint name>");
	PUTS("\t   {d|i} protocol <protocol name>");
	PUTS("\t   {d|i} induct <protocol name> <duct name>");
	PUTS("\t   {d|i} outduct <protocol name> <duct name>");
	PUTS("\tl\tList");
	PUTS("\t   l scheme");
	PUTS("\t   l endpoint");
	PUTS("\t   l protocol");
	PUTS("\t   l induct [<protocol name>]");
	PUTS("\t   l outduct [<protocol name>]");
	PUTS("\tb\tBlock an outduct");
	PUTS("\t   b outduct <protocol name> <duct name>");
	PUTS("\tu\tUnblock an outduct");
	PUTS("\t   u outduct <protocol name> <duct name>");
	PUTS("\tm\tManage");
	PUTS("\t   m heapmax <max database heap for any single acquisition>");
	PUTS("\tr\tRun another admin program");
	PUTS("\t   r '<admin command>'");
	PUTS("\ts\tStart");
	PUTS("\tx\tStop");
	PUTS("\t   {s|x}");
	PUTS("\t   {s|x} scheme <scheme name>");
	PUTS("\t   {s|x} protocol <protocol name>");
	PUTS("\t   {s|x} induct <protocol name> <duct name>");
	PUTS("\t   {s|x} outduct <protocol name> <duct name>");
	PUTS("\tw\tWatch BP activity");
	PUTS("\t   w { 0 | 1 | <activity spec> }");
	PUTS("\t\tActivity spec is a string of all requested activity \
indication characters, e.g., acz~.  See man(5) for bprc.");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text, ignored by the program>");
}

static void	initializeBp(int tokenCount, char **tokens)
{
	if (tokenCount != 1)
	{
		SYNTAX_ERROR;
		return;
	}

	if (ionAttach() < 0)
	{
		putErrmsg("bpadmin can't attach to ION.", NULL);
		return;
	}

	if (bpInit() < 0)
	{
		putErrmsg("bpadmin can't initialize BP.", NULL);
		return;
	}
}

static int	attachToBp()
{
	if (bpAttach() < 0)
	{
		printText("BP not initialized yet.");
		return -1;
	}

	return 0;
}

static void	executeStart(int tokenCount, char **tokens)
{
	if (strcmp(tokens[1], "scheme") == 0)
	{
		bpStartScheme(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "protocol") == 0)
	{
		bpStartProtocol(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "induct") == 0)
	{
		bpStartInduct(tokens[2], tokens[3]);
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		bpStartOutduct(tokens[2], tokens[3]);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeStop(int tokenCount, char **tokens)
{
	if (strcmp(tokens[1], "scheme") == 0)
	{
		bpStopScheme(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "protocol") == 0)
	{
		bpStopProtocol(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "induct") == 0)
	{
		bpStopInduct(tokens[2], tokens[3]);
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		bpStopOutduct(tokens[2], tokens[3]);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeAdd(int tokenCount, char **tokens)
{
	char		*script;
	BpRecvRule	rule;
	int		nominalRate = 0;
	int		protocolClass = 0;
	unsigned int	maxPayloadLength;

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "scheme") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		addScheme(tokens[2], tokens[3], tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "endpoint") == 0)
	{
		switch (tokenCount)
		{
		case 5:
			script = tokens[4];
			break;

		case 4:
			script = NULL;
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		if (*(tokens[3]) == 'q')
		{
			rule = EnqueueBundle;
		}
		else
		{
			rule = DiscardBundle;
		}

		addEndpoint(tokens[2], rule, script);
		return;
	}

	if (strcmp(tokens[1], "protocol") == 0)
	{
		if (tokenCount < 5 || tokenCount > 7)
		{
			SYNTAX_ERROR;
			return;
		}

		if (tokenCount == 7)
		{
			protocolClass = atol(tokens[6]);
		}

		if (tokenCount == 6)
		{
			nominalRate = atol(tokens[5]);
		}

		addProtocol(tokens[2], atoi(tokens[3]), atoi(tokens[4]),
				nominalRate, protocolClass);
		return;
	}

	if (strcmp(tokens[1], "induct") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		addInduct(tokens[2], tokens[3], tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		switch (tokenCount)
		{
		case 6:
			maxPayloadLength = strtoul(tokens[5], NULL, 0);
			break;

		case 5:
			maxPayloadLength = 0;
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		addOutduct(tokens[2], tokens[3], tokens[4], maxPayloadLength);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeChange(int tokenCount, char **tokens)
{
	char		*script;
	BpRecvRule	rule;
	unsigned int	maxPayloadLen;

	if (tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if (strcmp(tokens[1], "scheme") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		updateScheme(tokens[2], tokens[3], tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "endpoint") == 0)
	{
		switch (tokenCount)
		{
		case 5:
			script = tokens[4];
			break;

		case 4:
			script = NULL;
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		if (*(tokens[3]) == 'q')
		{
			rule = EnqueueBundle;
		}
		else
		{
			rule = DiscardBundle;
		}

		updateEndpoint(tokens[2], rule, script);
		return;
	}

	if (strcmp(tokens[1], "induct") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		updateInduct(tokens[2], tokens[3], tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		switch (tokenCount)
		{
		case 6:
			maxPayloadLen = strtoul(tokens[5], NULL, 0);
			break;

		case 5:
			maxPayloadLen = 0;
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		updateOutduct(tokens[2], tokens[3], tokens[4], maxPayloadLen);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Delete what?");
		return;
	}

	if (strcmp(tokens[1], "scheme") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		removeScheme(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "endpoint") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		removeEndpoint(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "protocol") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		removeProtocol(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "induct") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		removeInduct(tokens[2], tokens[3]);
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		removeOutduct(tokens[2], tokens[3]);
		return;
	}

	SYNTAX_ERROR;
}

static void	printScheme(VScheme *vscheme)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(Scheme, scheme);
	char	fwdCmdBuffer[SDRSTRING_BUFSZ];
	char	*fwdCmd;
	char	admAppCmdBuffer[SDRSTRING_BUFSZ];
	char	*admAppCmd;
	char	buffer[1024];

	GET_OBJ_POINTER(sdr, Scheme, scheme, sdr_list_data(sdr,
			vscheme->schemeElt));
	if (sdr_string_read(sdr, fwdCmdBuffer, scheme->fwdCmd) < 0)
	{
		fwdCmd = "?";
	}
	else
	{
		fwdCmd = fwdCmdBuffer;
	}

	if (sdr_string_read(sdr, admAppCmdBuffer, scheme->admAppCmd) < 0)
	{
		admAppCmd = "?";
	}
	else
	{
		admAppCmd = admAppCmdBuffer;
	}

	isprintf(buffer, sizeof buffer, "%.8s\tfwdpid: %d cmd: %.256s  \
admpid: %d cmd %.256s", scheme->name, vscheme->fwdPid, fwdCmd,
			vscheme->admAppPid, admAppCmd);
	printText(buffer);
}

static void	infoScheme(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	VScheme		*vscheme;
	PsmAddress	elt;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	findScheme(tokens[2], &vscheme, &elt);
	if (elt == 0)
	{
		printText("Unknown scheme.");
	}
	else
	{
		printScheme(vscheme);
	}

	sdr_exit_xn(sdr);
}

static void	printEndpoint(VEndpoint *vpoint)
{
	Sdr		sdr = getIonsdr();
		OBJ_POINTER(Endpoint, endpoint);
		OBJ_POINTER(Scheme, scheme);
	char	buffer[512];
	char	recvRule;
	char	recvScriptBuffer[SDRSTRING_BUFSZ];
	char	*recvScript = recvScriptBuffer;

	GET_OBJ_POINTER(sdr, Endpoint, endpoint, sdr_list_data(sdr,
			vpoint->endpointElt));
	GET_OBJ_POINTER(sdr, Scheme, scheme, endpoint->scheme);
	if (endpoint->recvRule == EnqueueBundle)
	{
		recvRule = 'q';
	}
	else
	{
		recvRule = 'x';
	}

	if (endpoint->recvScript == 0)
	{
		recvScriptBuffer[0] = '\0';
	}
	else
	{
		if (sdr_string_read(sdr, recvScriptBuffer, endpoint->recvScript)
			       	< 0)
		{
			recvScript = "?";
		}
	}

	isprintf(buffer, sizeof buffer, "%.8s:%.128s  %d\trule: %c  script: \
%.256s", scheme->name, endpoint->nss, vpoint->appPid, recvRule, recvScript);
	printText(buffer);
}

static void	infoEndpoint(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	VEndpoint	*vpoint;
	PsmAddress	elt;

	if (tokenCount != 4)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	findEndpoint(tokens[2], tokens[3], NULL, &vpoint, &elt);
	if (elt == 0)
	{
		printText("Unknown endpoint.");
	}
	else
	{
		printEndpoint(vpoint);
	}

	sdr_exit_xn(sdr);
}

static void	printProtocol(ClProtocol *protocol)
{
	printText(protocol->name);
}

static void	infoProtocol(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	ClProtocol	clpbuf;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	fetchProtocol(tokens[2], &clpbuf, &elt);
	if (elt == 0)
	{
		printText("Unknown protocol.");
	}
	else
	{
		printProtocol(&clpbuf);
	}

	sdr_exit_xn(sdr);
}

static void	printInduct(VInduct *vduct)
{
	Sdr		sdr = getIonsdr();
		OBJ_POINTER(Induct, duct);
		OBJ_POINTER(ClProtocol, clp);
	char	cliCmdBuffer[SDRSTRING_BUFSZ];
	char	*cliCmd;
	char	buffer[1024];

	GET_OBJ_POINTER(sdr, Induct, duct, sdr_list_data(sdr,
			vduct->inductElt));
	GET_OBJ_POINTER(sdr, ClProtocol, clp, duct->protocol);
	if (sdr_string_read(sdr, cliCmdBuffer, duct->cliCmd) < 0)
	{
		cliCmd = "?";
	}
	else
	{
		cliCmd = cliCmdBuffer;
	}

	isprintf(buffer, sizeof buffer, "%.8s/%.256s\tpid: %d  cmd: %.256s",
			clp->name, duct->name, vduct->cliPid, cliCmd);
	printText(buffer);
}

static void	infoInduct(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	VInduct		*vduct;
	PsmAddress	elt;

	if (tokenCount != 4)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	findInduct(tokens[2], tokens[3], &vduct, &elt);
	if (elt == 0)
	{
		printText("Unknown induct.");
	}
	else
	{
		printInduct(vduct);
	}

	sdr_exit_xn(sdr);
}

static void	printOutduct(VOutduct *vduct)
{
	Sdr		sdr = getIonsdr();
		OBJ_POINTER(Outduct, duct);
		OBJ_POINTER(ClProtocol, clp);
	char	cloCmdBuffer[SDRSTRING_BUFSZ];
	char	*cloCmd;
	char	buffer[1024];

	GET_OBJ_POINTER(sdr, Outduct, duct, sdr_list_data(sdr,
			vduct->outductElt));
	GET_OBJ_POINTER(sdr, ClProtocol, clp, duct->protocol);

	if (duct->cloCmd == 0)
	{
		cloCmd = "?";
	}
	else if (sdr_string_read(sdr, cloCmdBuffer, duct->cloCmd) < 0)
	{
		cloCmd = "?";
	}
	else
	{
		cloCmd = cloCmdBuffer;
	}

	isprintf(buffer, sizeof buffer, "%.8s/%.256s\tpid: %d  cmd: %.256s \
max: %lu", clp->name, duct->name, vduct->cloPid, cloCmd, duct->maxPayloadLen);
	printText(buffer);
}

static void	infoOutduct(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	VOutduct	*vduct;
	PsmAddress	elt;

	if (tokenCount != 4)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	findOutduct(tokens[2], tokens[3], &vduct, &elt);
	if (elt == 0)
	{
		printText("Unknown outduct.");
	}
	else
	{
		printOutduct(vduct);
	}

	sdr_exit_xn(sdr);
}

static void	executeInfo(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Information on what?");
		return;
	}

	if (strcmp(tokens[1], "scheme") == 0)
	{
		infoScheme(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "endpoint") == 0)
	{
		infoEndpoint(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "protocol") == 0)
	{
		infoProtocol(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "induct") == 0)
	{
		infoInduct(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		infoOutduct(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	listSchemes(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	VScheme		*vscheme;

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sm_list_first(ionwm, (getBpVdb())->schemes); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vscheme = (VScheme *) psp(ionwm, sm_list_data(ionwm, elt));
		printScheme(vscheme);
	}

	sdr_exit_xn(sdr);
}

static void	listEndpointsForScheme(VScheme *vscheme)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	VEndpoint	*vpoint;

	for (elt = sm_list_first(ionwm, vscheme->endpoints); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vpoint = (VEndpoint *) psp(ionwm, sm_list_data(ionwm, elt));
		printEndpoint(vpoint);
	}
}

static void	listEndpoints(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	VScheme		*vscheme;
	PsmAddress	elt;

	switch (tokenCount)
	{
	case 2:
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sm_list_first(ionwm, (getBpVdb())->schemes); elt;
				elt = sm_list_next(ionwm, elt))
		{
			vscheme = (VScheme *) psp(ionwm,
					sm_list_data(ionwm, elt));
			listEndpointsForScheme(vscheme);
		}

		sdr_exit_xn(sdr);
		break;

	case 3:
		CHKVOID(sdr_begin_xn(sdr));
		findScheme(tokens[2], &vscheme, &elt);
		if (elt == 0)
		{
			printText("Unknown scheme.");
		}
		else
		{
			listEndpointsForScheme(vscheme);
		}

		sdr_exit_xn(sdr);
		break;

	default:
		SYNTAX_ERROR;
	}
}

static void	listProtocols(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(ClProtocol, clp);

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getBpConstants())->protocols); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, ClProtocol, clp, sdr_list_data(sdr, elt));
		printProtocol(clp);
	}

	sdr_exit_xn(sdr);
}

static void	listInductsForProtocol(char *protocolName)
{
	PsmPartition	ionwm = getIonwm();
	VInduct		*vduct;
	PsmAddress	elt;

	for (elt = sm_list_first(ionwm, (getBpVdb())->inducts); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vduct = (VInduct *) psp(ionwm, sm_list_data(ionwm, elt));
		if (strcmp(vduct->protocolName, protocolName) == 0)
		{
			printInduct(vduct);
		}
	}
}

static void	listInducts(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	ClProtocol	clpbuf;
	Object		elt;

	switch (tokenCount)
	{
	case 2:
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, (getBpConstants())->protocols);
				elt; elt = sdr_list_next(sdr, elt))
		{
			sdr_read(sdr, (char *) &clpbuf,
				sdr_list_data(sdr, elt), sizeof(ClProtocol));
			listInductsForProtocol(clpbuf.name);
		}

		sdr_exit_xn(sdr);
		break;

	case 3:
		CHKVOID(sdr_begin_xn(sdr));
		fetchProtocol(tokens[2], &clpbuf, &elt);
		if (elt == 0)
		{
			printText("Unknown protocol.");
		}
		else
		{
			listInductsForProtocol(clpbuf.name);
		}

		sdr_exit_xn(sdr);
		break;

	default:
		SYNTAX_ERROR;
	}
}

static void	listOutductsForProtocol(char *protocolName)
{
	PsmPartition	ionwm = getIonwm();
	VOutduct	*vduct;
	PsmAddress	elt;

	for (elt = sm_list_first(ionwm, (getBpVdb())->outducts); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vduct = (VOutduct *) psp(ionwm, sm_list_data(ionwm, elt));
		if (strcmp(vduct->protocolName, protocolName) == 0)
		{
			printOutduct(vduct);
		}
	}
}

static void	listOutducts(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	ClProtocol	clpbuf;
	Object		elt;

	switch (tokenCount)
	{
	case 2:
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, (getBpConstants())->protocols);
				elt; elt = sdr_list_next(sdr, elt))
		{
			sdr_read(sdr, (char *) &clpbuf,
				sdr_list_data(sdr, elt), sizeof(ClProtocol));
			listOutductsForProtocol(clpbuf.name);
		}

		sdr_exit_xn(sdr);
		break;

	case 3:
		CHKVOID(sdr_begin_xn(sdr));
		fetchProtocol(tokens[2], &clpbuf, &elt);
		if (elt == 0)
		{
			printText("Unknown protocol.");
		}
		else
		{
			listOutductsForProtocol(clpbuf.name);
		}

		sdr_exit_xn(sdr);
		break;

	default:
		SYNTAX_ERROR;
	}
}

static void	executeList(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("List what?");
		return;
	}

	if (strcmp(tokens[1], "scheme") == 0)
	{
		listSchemes(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "endpoint") == 0)
	{
		listEndpoints(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "protocol") == 0)
	{
		listProtocols(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "induct") == 0)
	{
		listInducts(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		listOutducts(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeBlock(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Block what?");
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		oK(bpBlockOutduct(tokens[2], tokens[3]));
		return;
	}

	SYNTAX_ERROR;
}

static void	executeUnblock(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Unblock what?");
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		oK(bpUnblockOutduct(tokens[2], tokens[3]));
		return;
	}

	SYNTAX_ERROR;
}

static void	manageHeapmax(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		bpdbObj = getBpDbObject();
	BpDB		bpdb;
	unsigned int	heapmax;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	heapmax = strtoul(tokens[2], NULL, 0);
	if (heapmax < 560)
	{
		writeMemoNote("[?] heapmax must be at least 560", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &bpdb, bpdbObj, sizeof(BpDB));
	bpdb.maxAcqInHeap = heapmax;
	sdr_write(sdr, bpdbObj, (char *) &bpdb, sizeof(BpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change maxAcqInHeap.", NULL);
	}
}

static void	executeManage(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Manage what?");
		return;
	}

	if (strcmp(tokens[1], "heapmax") == 0)
	{
		manageHeapmax(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeRun(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Run what?");
		return;
	}

	if (pseudoshell(tokens[1]) < 0)
	{
		printText("pseudoshell failed.");
	}
	else
	{
		snooze(1);	/*	Give script time to finish.	*/
	}
}

static void	noteWatchValue()
{
	BpVdb	*vdb = getBpVdb();
	Sdr	sdr = getIonsdr();
	Object	dbObj = getBpDbObject();
	BpDB	db;

	if (vdb != NULL && dbObj != 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		sdr_stage(sdr, (char *) &db, dbObj, sizeof(BpDB));
		db.watching = vdb->watching;
		sdr_write(sdr, dbObj, (char *) &db, sizeof(BpDB));
		oK(sdr_end_xn(sdr));
	}
}

static void	switchWatch(int tokenCount, char **tokens)
{
	BpVdb	*vdb = getBpVdb();
	char	buffer[80];
	char	*cursor;

	if (tokenCount < 2)
	{
		printText("Switch watch in what way?");
		return;
	}

	if (strcmp(tokens[1], "1") == 0)
	{
		vdb->watching = -1;
		return;
	}

	vdb->watching = 0;
	if (strcmp(tokens[1], "0") == 0)
	{
		return;
	}

	cursor = tokens[1];
	while (*cursor)
	{
		switch (*cursor)
		{
		case 'a':
			vdb->watching |= WATCH_a;
			break;

		case 'b':
			vdb->watching |= WATCH_b;
			break;

		case 'c':
			vdb->watching |= WATCH_c;
			break;

		case 'm':
			vdb->watching |= WATCH_m;
			break;

		case 'w':
			vdb->watching |= WATCH_w;
			break;

		case 'x':
			vdb->watching |= WATCH_x;
			break;

		case 'y':
			vdb->watching |= WATCH_y;
			break;

		case 'z':
			vdb->watching |= WATCH_z;
			break;

		case '~':
			vdb->watching |= WATCH_abandon;
			break;

		case '!':
			vdb->watching |= WATCH_expire;
			break;

		case '&':
			vdb->watching |= WATCH_refusal;
			break;

		case '#':
			vdb->watching |= WATCH_timeout;
			break;

		case 'j':
			vdb->watching |= WATCH_limbo;
			break;

		case 'k':
			vdb->watching |= WATCH_delimbo;
			break;

		default:
			isprintf(buffer, sizeof buffer,
					"Invalid watch char %c.", *cursor);
			printText(buffer);
		}

		cursor++;
	}
}

static void	switchEcho(int tokenCount, char **tokens)
{
	int	state;

	if (tokenCount < 2)
	{
		printText("Echo on or off?");
		return;
	}

	switch (*(tokens[1]))
	{
	case '0':
		state = 0;
		oK(_echo(&state));
		break;

	case '1':
		state = 1;
		oK(_echo(&state));
		break;

	default:
		printText("Echo on or off?");
	}
}

static int bp_is_up(int tokenCount, char** tokens)
{
	if (strcmp(tokens[1], "p") == 0) //poll
	{
		if (tokenCount < 3) //use default timeout
		{
			int count = 1;
			while (count <= 120 && !bp_agent_is_started())
			{
				microsnooze(250000);
				count++;
			}
			if (count > 120) //bp agent is not started
			{
				printText("BP agent is not started");
				return 0;
			}
			else //bp agent is started
			{
				printText("BP agent is started");
				return 1;
			}
		}
		else //use user supplied timeout
		{
			int max = atoi(tokens[2]) * 4;
			int count = 1;
			while (count <= max && !bp_agent_is_started())
			{
				microsnooze(250000);
				count++;
			}
			if (count > max) //bp agent is not started
			{
				printText("BP agent is not started");
				return 0;
			}
			else //bp agent is started
			{
				printText("BP agent is started");
				return 1;
			}
		}
	}
	else //check once
	{
		if (bp_agent_is_started())
		{
			printText("BP agent is started");
			return 1;
		}
		else
		{
			printText("BP agent is not started");
			return 0;
		}
	}
}

static int	processLine(char *line, int lineLength)
{
	int		tokenCount;
	char		*cursor;
	int		i;
	char		*tokens[9];
	struct timeval	done_time;
	struct timeval	cur_time;
	char		buffer[80];

	tokenCount = 0;
	for (cursor = line, i = 0; i < 9; i++)
	{
		if (*cursor == '\0')
		{
			tokens[i] = NULL;
		}
		else
		{
			findToken(&cursor, &(tokens[i]));
			tokenCount++;
		}
	}

	if (tokenCount == 0)
	{
		return 0;
	}

	/*	Skip over any trailing whitespace.			*/

	while (isspace((int) *cursor))
	{
		cursor++;
	}

	/*	Make sure we've parsed everything.			*/

	if (*cursor != '\0')
	{
		printText("Too many tokens.");
		return 0;
	}

	/*	Have parsed the command.  Now execute it.		*/

	switch (*(tokens[0]))		/*	Command code.		*/
	{
		case 0:			/*	Empty line.		*/
		case '#':		/*	Comment.		*/
			return 0;

		case '?':
		case 'h':
			printUsage();
			return 0;
		case 'v':
			isprintf(buffer, sizeof buffer,
					"%s compiled with crypto suite: %s",
					IONVERSIONNUMBER, crypto_suite_name);
			printText(buffer);
			return 0;

		case '1':
			initializeBp(tokenCount, tokens);
			return 0;

		case 's':
			if (attachToBp() == 0)
			{
				if (tokenCount > 1)
				{
					executeStart(tokenCount, tokens);
				}
				else
				{
					if (bpStart() < 0)
					{
						putErrmsg("Can't start BP.",
								NULL);
						return 0;
					}
				}

				/* Wait for bp to start up. */
				getCurrentTime(&done_time);
				done_time.tv_sec += STARTUP_TIMEOUT;
				while (bp_agent_is_started() == 0)
				{
					snooze(1);
					getCurrentTime(&cur_time);
					if (cur_time.tv_sec >=
					    done_time.tv_sec 
					    && cur_time.tv_usec >=
					    done_time.tv_usec)
					{
						printText("[?] BP start hung\
 up, abandoned.");
						break;
					}
				}

			}

			return 0;

		case 'x':
			if (attachToBp() == 0)
			{
				if (tokenCount > 1)
				{
					executeStop(tokenCount, tokens);
				}
				else
				{
					bpStop();
				}
			}

			return 0;

		case 'a':
			if (attachToBp() == 0)
			{
				executeAdd(tokenCount, tokens);
			}

			return 0;

		case 'c':
			if (attachToBp() == 0)
			{
				executeChange(tokenCount, tokens);
			}

			return 0;

		case 'd':
			if (attachToBp() == 0)
			{
				executeDelete(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (attachToBp() == 0)
			{
				executeInfo(tokenCount, tokens);
			}

			return 0;

		case 'l':
			if (attachToBp() == 0)
			{
				executeList(tokenCount, tokens);
			}

			return 0;

		case 'b':
			if (attachToBp() == 0)
			{
				executeBlock(tokenCount, tokens);
			}

			return 0;

		case 'u':
			if (attachToBp() == 0)
			{
				executeUnblock(tokenCount, tokens);
			}

			return 0;

		case 'm':
			if (attachToBp() == 0)
			{
				executeManage(tokenCount, tokens);
			}

			return 0;

		case 'r':
			executeRun(tokenCount, tokens);
			return 0;

		case 'w':
			if (attachToBp() == 0)
			{
				switchWatch(tokenCount, tokens);
				noteWatchValue();
			}

			return 0;

		case 'e':
			switchEcho(tokenCount, tokens);
			return 0;

		case 't':
			if (attachToBp() == 0)
			{
				exit(bp_is_up(tokenCount, tokens));
			}

		case 'q':
			return -1;	/*	End program.		*/

		default:
			printText("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

#if defined (ION_LWT)
int	bpadmin(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = (argc > 1 ? argv[1] : NULL);
#endif
	int	cmdFile;
	char	line[256];
	int	len;
	
	if (cmdFileName == NULL)		/*	Interactive.	*/
	{
#ifdef FSWLOGGER
		return 0;			/*	No stdout.	*/
#else
		cmdFile = fileno(stdin);
		isignal(SIGINT, handleQuit);
		while (1)
		{
			printf(": ");
			fflush(stdout);
			if (igets(cmdFile, line, sizeof line, &len) == NULL)
			{
				if (len == 0)
				{
					break;
				}

				putErrmsg("igets failed.", NULL);
				break;		/*	Out of loop.	*/
			}
			
			if (len == 0)
			{
				continue;
			}

			if (processLine(line, len))
			{
				break;		/*	Out of loop.	*/
			}
		}
#endif
	}
	else if (strcmp(cmdFileName, ".") == 0)	/*	Shutdown.	*/
	{
		if (attachToBp() == 0)
		{
			bpStop();
		}
	}
	else					/*	Scripted.	*/
	{
		cmdFile = iopen(cmdFileName, O_RDONLY, 0777);
		if (cmdFile < 0)
		{
			PERROR("Can't open command file");
		}
		else
		{
			while (1)
			{
				if (igets(cmdFile, line, sizeof line, &len)
						== NULL)
				{
					if (len == 0)
					{
						break;	/*	Loop.	*/
					}

					putErrmsg("igets failed.", NULL);
					break;		/*	Loop.	*/
				}

				if (len == 0
				|| line[0] == '#')	/*	Comment.*/
				{
					continue;
				}

				if (processLine(line, len))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping bpadmin.");
	ionDetach();

	return 0;
}
