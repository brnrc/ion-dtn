/*
	ipnadmin.c:	BP routing adminstration interface for
			the IPN endpoint ID scheme.
									*/
/*									*/
/*	Copyright (c) 2006, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "ipnfw.h"

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

	isprintf(buffer, sizeof buffer, "Syntax error at line %d of ipnadmin.c",
			lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage()
{
	PUTS("Syntax of 'duct expression' is:");
	PUTS("\t<protocol name>/<outduct name>[,<dest induct name>]");
	PUTS("Syntax of 'qualifier' is:");
	PUTS("\t{ <source service nbr> | * } { <source node nbr> | * }");
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\tv\tPrint version of ION.");
	PUTS("\ta\tAdd");
	PUTS("\t   a plan <node nbr> <default duct expression>");
	PUTS("\t   a planrule <node nbr> <qualifier> <duct expression>");
	PUTS("\t   a group <first node nbr> <last node nbr> <endpoint ID>");
	PUTS("\t   a grouprule <first node nbr> <last node nbr> <qualifier> \
<endpoint ID>");
	PUTS("\tc\tChange");
	PUTS("\t   c plan <node nbr> <default duct expression>");
	PUTS("\t   c planrule <node nbr> <qualifier> <duct expression>");
	PUTS("\t   c group <first node nbr> <last node nbr> <endpoint ID>");
	PUTS("\t   c grouprule <first node nbr> <last node nbr> <qualifier> \
<endpoint ID>");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} plan <node nbr>");
	PUTS("\t   {d|i} planrule <node nbr> <qualifier>");
	PUTS("\t   {d|i} group <first node nbr> <last node nbr>");
	PUTS("\t   {d|i} grouprule <first node nbr> <last node nbr> \
<qualifier>");
	PUTS("\tl\tList");
	PUTS("\t   l group");
	PUTS("\t   l plan");
	PUTS("\t   l planrule <node nbr>");
	PUTS("\t   l grouprule <first node nbr> <last node nbr>");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static int	parseDuctExpression(char *token, DuctExpression *expression)
{
	char		*cursor = NULL;
	char		*protocolName;
	char		*outductName;
	VOutduct	*vduct;
	PsmAddress	vductElt;

	memset((char *) expression, 0, sizeof(DuctExpression));
	protocolName = token;
	cursor = strchr(token, '/');
	if (cursor == NULL)
	{
		putErrmsg("Malformed duct expression: <protocol>/<duct name>",
				protocolName);
		return 0;
	}

	*cursor = '\0';		/*	Delimit protocol name.	*/
	cursor++;
	outductName = cursor;

	/*	If there's a destination duct name, note end of
	 *	outduct name and start of destination duct name.*/

	cursor = strchr(cursor, ',');
	if (cursor == NULL)
	{
		/*	End of token delimits outduct name.	*/

		expression->destDuctName = NULL;
	}
	else
	{
		*cursor = '\0';	/*	Delimit outduct name.	*/
		cursor++;
		expression->destDuctName = cursor;
	}

	findOutduct(protocolName, outductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("Unknown outduct.", outductName);
		return 0;
	}

	expression->outductElt = vduct->outductElt;
	return 1;
}

static void	executeAdd(int tokenCount, char **tokens)
{
	DuctExpression	expression;
	int		sourceServiceNbr;
	int		sourceNodeNbr;

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		if (parseDuctExpression(tokens[3], &expression) == 0)
		{
			return;
		}

		ipn_addPlan(strtouvast(tokens[2]), &expression);
		return;
	}

	if (strcmp(tokens[1], "planrule") == 0)
	{
		if (tokenCount != 6)
		{
			SYNTAX_ERROR;
			return;
		}

		if (strcmp(tokens[3], "*") == 0)
		{
			sourceServiceNbr = -1;
		}
		else
		{
			sourceServiceNbr = strtoul(tokens[3], NULL, 0);
		}

		if (strcmp(tokens[4], "*") == 0)
		{
			sourceNodeNbr = -1;
		}
		else
		{
			sourceNodeNbr = strtouvast(tokens[4]);
		}

		if (parseDuctExpression(tokens[5], &expression) == 0)
		{
			return;
		}

		ipn_addPlanRule(strtouvast(tokens[2]), sourceServiceNbr,
				sourceNodeNbr, &expression);
		return;
	}

	if (strcmp(tokens[1], "group") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		ipn_addGroup(strtouvast(tokens[2]), strtouvast(tokens[3]),
				tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "grouprule") == 0)
	{
		if (tokenCount != 7)
		{
			SYNTAX_ERROR;
			return;
		}

		if (strcmp(tokens[4], "*") == 0)
		{
			sourceServiceNbr = -1;
		}
		else
		{
			sourceServiceNbr = strtoul(tokens[4], NULL, 0);
		}

		if (strcmp(tokens[5], "*") == 0)
		{
			sourceNodeNbr = -1;
		}
		else
		{
			sourceNodeNbr = strtouvast(tokens[5]);
		}

		ipn_addGroupRule(strtouvast(tokens[2]), strtouvast(tokens[3]),
				sourceServiceNbr, sourceNodeNbr, tokens[6]);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeChange(int tokenCount, char **tokens)
{
	DuctExpression	expression;
	int		sourceServiceNbr;
	int		sourceNodeNbr;

	if (tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		if (parseDuctExpression(tokens[3], &expression) == 0)
		{
			return;
		}

		ipn_updatePlan(strtouvast(tokens[2]), &expression);
		return;
	}

	if (strcmp(tokens[1], "planrule") == 0)
	{
		if (tokenCount != 6)
		{
			SYNTAX_ERROR;
			return;
		}

		if (strcmp(tokens[3], "*") == 0)
		{
			sourceServiceNbr = -1;
		}
		else
		{
			sourceServiceNbr = strtoul(tokens[3], NULL, 0);
		}

		if (strcmp(tokens[4], "*") == 0)
		{
			sourceNodeNbr = -1;
		}
		else
		{
			sourceNodeNbr = strtouvast(tokens[4]);
		}

		if (parseDuctExpression(tokens[5], &expression) == 0)
		{
			return;
		}

		ipn_updatePlanRule(strtouvast(tokens[2]),
				sourceServiceNbr, sourceNodeNbr, &expression);
		return;
	}

	if (strcmp(tokens[1], "group") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		ipn_updateGroup(strtouvast(tokens[2]), strtouvast(tokens[3]),
				tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "grouprule") == 0)
	{
		if (tokenCount != 7)
		{
			SYNTAX_ERROR;
			return;
		}

		if (strcmp(tokens[4], "*") == 0)
		{
			sourceServiceNbr = -1;
		}
		else
		{
			sourceServiceNbr = strtoul(tokens[4], NULL, 0);
		}

		if (strcmp(tokens[5], "*") == 0)
		{
			sourceNodeNbr = -1;
		}
		else
		{
			sourceNodeNbr = strtouvast(tokens[5]);
		}

		ipn_updateGroupRule(strtouvast(tokens[2]),
				strtouvast(tokens[3]), sourceServiceNbr,
				sourceNodeNbr, tokens[6]);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	int	sourceServiceNbr;
	int	sourceNodeNbr;

	if (tokenCount < 2)
	{
		printText("Delete what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		ipn_removePlan(strtouvast(tokens[2]));
		return;
	}

	if (strcmp(tokens[1], "planrule") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		if (strcmp(tokens[3], "*") == 0)
		{
			sourceServiceNbr = -1;
		}
		else
		{
			sourceServiceNbr = strtoul(tokens[3], NULL, 0);
		}

		if (strcmp(tokens[4], "*") == 0)
		{
			sourceNodeNbr = -1;
		}
		else
		{
			sourceNodeNbr = strtouvast(tokens[4]);
		}

		ipn_removePlanRule(strtouvast(tokens[2]), sourceServiceNbr,
				sourceNodeNbr);
		return;
	}

	if (strcmp(tokens[1], "group") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		ipn_removeGroup(strtouvast(tokens[2]), strtouvast(tokens[3]));
		return;
	}

	if (strcmp(tokens[1], "grouprule") == 0)
	{
		if (tokenCount != 6)
		{
			SYNTAX_ERROR;
			return;
		}

		if (strcmp(tokens[4], "*") == 0)
		{
			sourceServiceNbr = -1;
		}
		else
		{
			sourceServiceNbr = strtoul(tokens[4], NULL, 0);
		}

		if (strcmp(tokens[5], "*") == 0)
		{
			sourceNodeNbr = -1;
		}
		else
		{
			sourceNodeNbr = strtouvast(tokens[5]);
		}

		ipn_removeGroupRule(strtouvast(tokens[2]),
				strtouvast(tokens[3]), sourceServiceNbr,
				sourceNodeNbr);
		return;
	}

	SYNTAX_ERROR;
}

static void	printDirective(char *context, FwdDirective *dir)
{
	Sdr	sdr = getIonsdr();
	Object	ductObj;
		OBJ_POINTER(Outduct, duct);
		OBJ_POINTER(ClProtocol, clp);
	char	string[SDRSTRING_BUFSZ + 1];
	char	buffer[1024];

	switch (dir->action)
	{
	case xmit:
		ductObj = sdr_list_data(sdr, dir->outductElt);
		GET_OBJ_POINTER(sdr, Outduct, duct, ductObj);
		GET_OBJ_POINTER(sdr, ClProtocol, clp, duct->protocol);
		if (dir->destDuctName == 0)
		{
			string[0] = '\0';
		}
		else
		{
			string[0] = ',';
			if (sdr_string_read(sdr, string + 1,
					dir->destDuctName) < 0)
			{
				istrcpy(string + 1, "?", sizeof string - 1);
			}
		}

		isprintf(buffer, sizeof buffer, "%.80s x %.8s/%.128s%.128s",
				context, clp->name, duct->name, string);
		printText(buffer);
		break;

	case fwd:
		if (sdr_string_read(sdr, string, dir->eid) < 0)
		{
			istrcpy(string, "?", sizeof string);
		}

		isprintf(buffer, sizeof buffer, "%.80s f %.255s", context,
				string);
		printText(buffer);
		break;

	default:
		isprintf(buffer, sizeof buffer, "%.128s ?", context);
		printText(buffer);
	}
}

static void	printPlan(IpnPlan *plan)
{
	char	context[32];

	isprintf(context, sizeof context, UVAST_FIELDSPEC, plan->nodeNbr);
	printDirective(context, &plan->defaultDirective);
}

static void	infoPlan(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	int	nodeNbr;
	Object	planAddr;
	Object	elt;
		OBJ_POINTER(IpnPlan, plan);

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	nodeNbr = strtouvast(tokens[2]);
	CHKVOID(sdr_begin_xn(sdr));
	ipn_findPlan(nodeNbr, &planAddr, &elt);
	if (elt == 0)
	{
		printText("Unknown node.");
	}
	else
	{
		GET_OBJ_POINTER(getIonsdr(), IpnPlan, plan, planAddr);
		printPlan(plan);
	}

	sdr_exit_xn(sdr);
}

static void	printRule(IpnRule *rule)
{
	char	sourceServiceString[21];
	char	sourceNodeString[21];
	char	context[80];

	if (rule->srcServiceNbr == 0)
	{
		istrcpy(sourceServiceString, "*", sizeof sourceServiceString);
	}
	else
	{
		isprintf(sourceServiceString, sizeof sourceServiceString,
				"%ld", rule->srcServiceNbr);
	}

	if (rule->srcNodeNbr == 0)
	{
		istrcpy(sourceNodeString, "*", sizeof sourceNodeString);
	}
	else
	{
		isprintf(sourceNodeString, sizeof sourceNodeString, "%ld",
				rule->srcNodeNbr);
	}

	isprintf(context, sizeof context, "rule for service %s from node %s =",
			sourceServiceString, sourceNodeString);
	printDirective(context, &rule->directive);
}

static void	infoPlanRule(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	uvast	nodeNbr;
	Object	planAddr;
	Object	elt;
		OBJ_POINTER(IpnPlan, plan);
	int	sourceServiceNbr;
	uvast	sourceNodeNbr;
	Object	ruleAddr;
		OBJ_POINTER(IpnRule, rule);

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	nodeNbr = strtouvast(tokens[2]);
	CHKVOID(sdr_begin_xn(sdr));
	ipn_findPlan(nodeNbr, &planAddr, &elt);
	if (elt == 0)
	{
		printText("Unknown node.");
	}
	else
	{
		GET_OBJ_POINTER(sdr, IpnPlan, plan, planAddr);
		printPlan(plan);
		if (strcmp(tokens[3], "*") == 0)
		{
			sourceServiceNbr = -1;
		}
		else
		{
			sourceServiceNbr = strtoul(tokens[3], NULL, 0);
		}

		if (strcmp(tokens[4], "*") == 0)
		{
			sourceNodeNbr = -1;
		}
		else
		{
			sourceNodeNbr = strtouvast(tokens[4]);
		}

		ipn_findPlanRule(nodeNbr, sourceServiceNbr, sourceNodeNbr, plan,
				&ruleAddr, &elt);
		if (elt == 0)
		{
			printText("Unknown rule.");
		}
		else
		{
			GET_OBJ_POINTER(sdr, IpnRule, rule, ruleAddr);
			printRule(rule);
		}
	}

	sdr_exit_xn(sdr);
}

static void	printGroup(IpnGroup *group)
{
	char	eidString[SDRSTRING_BUFSZ];
	char	buffer[384];

	sdr_string_read(getIonsdr(), eidString, group->defaultDirective.eid);
	isprintf(buffer, sizeof buffer, "From " UVAST_FIELDSPEC " \
through " UVAST_FIELDSPEC ", forward via %.256s.",
			group->firstNodeNbr, group->lastNodeNbr, eidString);
	printText(buffer);
}

static void	infoGroup(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	uvast	firstNodeNbr;
	uvast	lastNodeNbr;
	Object	elt;
		OBJ_POINTER(IpnGroup, group);

	if (tokenCount != 4)
	{
		SYNTAX_ERROR;
		return;
	}

	firstNodeNbr = strtouvast(tokens[2]);
	lastNodeNbr = strtouvast(tokens[3]);
	if (lastNodeNbr < firstNodeNbr)
	{
		printText("Unknown group.");
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getIpnConstants())->groups); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnGroup, group, sdr_list_data(sdr, elt));
		if (group->firstNodeNbr == firstNodeNbr
		&& group->lastNodeNbr == lastNodeNbr)
		{
			printGroup(group);
			break;
		}
	}

	if (elt == 0)
	{
		printText("Unknown group.");
	}

	sdr_exit_xn(sdr);
}

static void	infoGroupRule(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	uvast		firstNodeNbr;
	uvast		lastNodeNbr;
	Object		groupAddr;
	Object		elt;
			OBJ_POINTER(IpnGroup, group);
	unsigned int	sourceServiceNbr;
	uvast		sourceNodeNbr;
	Object		ruleAddr;
			OBJ_POINTER(IpnRule, rule);

	if (tokenCount != 6)
	{
		SYNTAX_ERROR;
		return;
	}

	firstNodeNbr = strtouvast(tokens[2]);
	lastNodeNbr = strtouvast(tokens[3]);
	CHKVOID(sdr_begin_xn(sdr));
	ipn_findGroup(firstNodeNbr, lastNodeNbr, &groupAddr, &elt);
	if (elt == 0)
	{
		printText("Unknown node.");
	}
	else
	{
		GET_OBJ_POINTER(sdr, IpnGroup, group, groupAddr);
		printGroup(group);
		if (strcmp(tokens[4], "*") == 0)
		{
			sourceServiceNbr = -1;
		}
		else
		{
			sourceServiceNbr = strtoul(tokens[4], NULL, 0);
		}

		if (strcmp(tokens[5], "*") == 0)
		{
			sourceNodeNbr = -1;
		}
		else
		{
			sourceNodeNbr = strtouvast(tokens[5]);
		}

		ipn_findGroupRule(firstNodeNbr, lastNodeNbr, sourceServiceNbr,
				sourceNodeNbr, group, &ruleAddr, &elt);
		if (elt == 0)
		{
			printText("Unknown rule.");
		}
		else
		{
			GET_OBJ_POINTER(sdr, IpnRule, rule, ruleAddr);
			printRule(rule);
		}
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

	if (strcmp(tokens[1], "plan") == 0)
	{
		infoPlan(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "planrule") == 0)
	{
		infoPlanRule(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "group") == 0)
	{
		infoGroup(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "grouprule") == 0)
	{
		infoGroupRule(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	listPlans()
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(IpnPlan, plan);

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getIpnConstants())->plans); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnPlan, plan, sdr_list_data(sdr, elt));
		printPlan(plan);
	}

	sdr_exit_xn(sdr);
}

static void	listRules(Object rules)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(IpnRule, rule);

	for (elt = sdr_list_first(sdr, rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnRule, rule, sdr_list_data(sdr, elt));
		printRule(rule);
	}
}

static void	listGroups()
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(IpnGroup, group);

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getIpnConstants())->groups); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnGroup, group, sdr_list_data(sdr, elt));
		printGroup(group);
	}

	sdr_exit_xn(sdr);
}

static void	executeList(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	uvast	nodeNbr;
	Object	planAddr;
	Object	elt;
		OBJ_POINTER(IpnPlan, plan);
	uvast	firstNodeNbr;
	uvast	lastNodeNbr;
	Object	groupAddr;
		OBJ_POINTER(IpnGroup, group);

	if (tokenCount < 2)
	{
		printText("List what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		listPlans();
		return;
	}

	if (strcmp(tokens[1], "planrule") == 0)
	{
		if (tokenCount < 3)
		{
			printText("Must specify plan node nbr.");
			return;
		}

		nodeNbr = strtouvast(tokens[2]);
		CHKVOID(sdr_begin_xn(sdr));
		ipn_findPlan(nodeNbr, &planAddr, &elt);
		if (elt == 0)
		{
			printText("Unknown plan.");
		}
		else
		{
			GET_OBJ_POINTER(sdr, IpnPlan, plan, planAddr);
			printPlan(plan);
			listRules(plan->rules);
		}

		sdr_exit_xn(sdr);
		return;
	}

	if (strcmp(tokens[1], "group") == 0)
	{
		listGroups();
		return;
	}

	if (strcmp(tokens[1], "grouprule") == 0)
	{
		if (tokenCount < 4)
		{
			printText("Must specify group first & last node nbrs.");
			return;
		}

		firstNodeNbr = strtouvast(tokens[2]);
		lastNodeNbr = strtouvast(tokens[3]);
		CHKVOID(sdr_begin_xn(sdr));
		ipn_findGroup(firstNodeNbr, lastNodeNbr, &groupAddr, &elt);
		if (elt == 0)
		{
			printText("Unknown group.");
		}
		else
		{
			GET_OBJ_POINTER(sdr, IpnGroup, group, groupAddr);
			printGroup(group);
			listRules(group->rules);
		}

		sdr_exit_xn(sdr);
		return;
	}

	SYNTAX_ERROR;
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

static int	processLine(char *line, int lineLength)
{
	int	tokenCount;
	char	*cursor;
	int	i;
	char	*tokens[9];
	char	buffer[80];

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
			isprintf(buffer, sizeof buffer, "%s",
					IONVERSIONNUMBER);
			printText(buffer);
			return 0;

		case 'a':
			executeAdd(tokenCount, tokens);
			return 0;

		case 'c':
			executeChange(tokenCount, tokens);
			return 0;

		case 'd':
			executeDelete(tokenCount, tokens);
			return 0;

		case 'i':
			executeInfo(tokenCount, tokens);
			return 0;

		case 'l':
			executeList(tokenCount, tokens);
			return 0;

		case 'e':
			switchEcho(tokenCount, tokens);
			return 0;

		case 'q':
			return -1;	/*	End program.		*/

		default:
			printText("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

static int	run_ipnadmin(char *cmdFileName)
{
	int	cmdFile;
	char	line[256];
	int	len;

	if (bpAttach() < 0)
	{
		putErrmsg("ipnadmin can't attach to BP", NULL);
		return -1;
	}

	if (ipnInit() < 0)
	{
		putErrmsg("ipnadmin can't initialize routing database", NULL);
		return -1;
	}

	if (cmdFileName == NULL)	/*	Interactive.		*/
	{
#ifdef FSWLOGGER
		return 0;
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
	printText("Stopping ipnadmin.");
	ionDetach();
	return 0;
}

#if defined (ION_LWT)
int	ipnadmin(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = argc > 1 ? argv[1] : NULL;
#endif
	return run_ipnadmin(cmdFileName);
}
