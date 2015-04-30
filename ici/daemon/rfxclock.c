/*
	rfxclock.c:	scheduled-event management daemon for ION.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "rfx.h"

#ifndef	MAX_SECONDS_UNCLAIMED
#define	MAX_SECONDS_UNCLAIMED	(3)
#endif

extern void	ionShred(ReqTicket ticket);

static long	_running(long *newValue)
{
	void	*value;
	long	state;
	
	if (newValue)			/*	Changing state.		*/
	{
		value = (void *) (*newValue);
		state = (long) sm_TaskVar(&value);
	}
	else				/*	Just check.		*/
	{
		state = (long) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown()	/*	Commands rfxclock termination.	*/
{
	long	stop = 0;

	oK(_running(&stop));	/*	Terminates rfxclock.		*/
}

static int	setProbeIsDue(unsigned long destNodeNbr,
			unsigned long neighborNodeNbr)
{
	IonNode		*node;
	PsmAddress	nextElt;
	PsmPartition	ionwm;
	PsmAddress	elt;
	Embargo		*embargo;

	node = findNode(getIonVdb(), destNodeNbr, &nextElt);
	if (node == NULL)
	{
		return 0;	/*	Weird, but let it go.		*/
	}

	ionwm = getIonwm();
	for (elt = sm_list_first(ionwm, node->embargoes); elt;
			elt = sm_list_next(ionwm, elt))
	{
		embargo = (Embargo *) psp(ionwm, sm_list_data(ionwm, elt));
		CHKERR(embargo);
		if (embargo->nodeNbr == neighborNodeNbr)
		{
			embargo->probeIsDue = 1;
			if (postProbeEvent(node, embargo) == 0)
			{
				putErrmsg("Failed setting probeIsDue.", NULL);
				return -1;
			}

			return 0;
		}
	}

	return 0;	/*	Embargo no longer exists; no problem.	*/
}

static IonNeighbor	*getNeighbor(IonVdb *vdb, unsigned long nodeNbr)
{
	IonNeighbor	*neighbor;
	PsmAddress	next;

	neighbor = findNeighbor(vdb, nodeNbr, &next);
	if (neighbor == NULL)
	{
		neighbor = addNeighbor(vdb, nodeNbr);
	}

	return neighbor;
}

static int	dispatchEvent(IonVdb *vdb, IonEvent *event, int *forecastNeeded)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	addr;
	IonRXref	*rxref;
	IonNeighbor	*neighbor;
	IonCXref	*cxref;
	PsmAddress	ref;
	Object		iondbObj;
	IonDB		iondb;

	switch (event->type)
	{
	case IonStopImputedRange:
	case IonStopAssertedRange:
		rxref = (IonRXref *) psp(ionwm, event->ref);
		return rfx_remove_range(rxref->fromTime,
				rxref->fromNode, rxref->toNode);

	case IonStopXmit:
		cxref = (IonCXref *) psp(ionwm, event->ref);
		if (cxref->fromNode == getOwnNodeNbr())
		{
			neighbor = getNeighbor(vdb, cxref->toNode);
			if (neighbor)
			{
				neighbor->xmitRate = 0;
				*forecastNeeded = 1;
			}
		}

		return 0;

	case IonStopFire:
		cxref = (IonCXref *) psp(ionwm, event->ref);
		if (cxref->toNode == getOwnNodeNbr())
		{
			neighbor = getNeighbor(vdb, cxref->fromNode);
			if (neighbor)
			{
				neighbor->fireRate = 0;
			}
		}

		return 0;

	case IonStopRecv:
		cxref = (IonCXref *) psp(ionwm, event->ref);
		if (cxref->toNode == getOwnNodeNbr())
		{
			neighbor = getNeighbor(vdb, cxref->fromNode);
			if (neighbor)
			{
				neighbor->recvRate = 0;
				*forecastNeeded = 1;
			}
		}

		return 0;

	case IonStartImputedRange:
	case IonStartAssertedRange:
		rxref = (IonRXref *) psp(ionwm, event->ref);
		if (rxref->fromNode == getOwnNodeNbr())
		{
			neighbor = getNeighbor(vdb, rxref->toNode);
			if (neighbor)
			{
				neighbor->owltOutbound = rxref->owlt;
			}
		}

		if (rxref->toNode == getOwnNodeNbr())
		{
			neighbor = getNeighbor(vdb, rxref->fromNode);
			if (neighbor)
			{
				neighbor->owltInbound = rxref->owlt;
			}
		}

		return 0;

	case IonStartXmit:
		cxref = (IonCXref *) psp(ionwm, event->ref);
		if (cxref->fromNode == getOwnNodeNbr())
		{
			neighbor = getNeighbor(vdb, cxref->toNode);
			if (neighbor)
			{
				neighbor->xmitRate = cxref->xmitRate;
				*forecastNeeded = 1;
			}
		}

		return 0;

	case IonStartFire:
		cxref = (IonCXref *) psp(ionwm, event->ref);
		if (cxref->toNode == getOwnNodeNbr())
		{
			neighbor = getNeighbor(vdb, cxref->fromNode);
			if (neighbor)
			{
				neighbor->fireRate = cxref->xmitRate;

				/*	Only now can we post the
				 *	events for start/stop of
				 *	reception and purging of
				 *	contact: we need to know the
				 *	range (one-way light time)
				 *	from this neighbor.		*/

				ref = event->ref;
				iondbObj = getIonDbObject();
				sdr_read(getIonsdr(), (char *) &iondb, iondbObj,
						sizeof(IonDB));

				/*	Be a little quick to start
				 *	accepting segments, and a
				 *	little slow to stop, to
				 *	minimize the chance of
				 *	discarding legitimate input.	*/

				addr = psm_zalloc(ionwm, sizeof(IonEvent));
				if (addr == 0)
				{
					return -1;
				}

				event = (IonEvent *) psp(ionwm, addr);
				cxref->startRecv = event->time =
					(cxref->fromTime - iondb.maxClockError)
						+ neighbor->owltInbound;
				event->type = IonStartRecv;
				event->ref = ref;
				if (sm_rbt_insert(ionwm, vdb->timeline, addr,
						rfx_order_events, event) == 0)
				{
					psm_free(ionwm, addr);
					return -1;
				}

				addr = psm_zalloc(ionwm, sizeof(IonEvent));
				if (addr == 0)
				{
					return -1;
				}

				event = (IonEvent *) psp(ionwm, addr);
				cxref->stopRecv = event->time =
					(cxref->toTime + iondb.maxClockError)
						+ neighbor->owltInbound;
				event->type = IonStopRecv;
				event->ref = ref;
				if (sm_rbt_insert(ionwm, vdb->timeline, addr,
						rfx_order_events, event) == 0)
				{
					psm_free(ionwm, addr);
					return -1;
				}

				/*	Purge contact when reception
				 *	is expected to stop.		*/

				addr = psm_zalloc(ionwm, sizeof(IonEvent));
				if (addr == 0)
				{
					return -1;
				}

				event = (IonEvent *) psp(ionwm, addr);
				cxref->purgeTime = event->time =
					(cxref->toTime + iondb.maxClockError)
						+ neighbor->owltInbound;
				event->type = IonPurgeContact;
				event->ref = ref;
				if (sm_rbt_insert(ionwm, vdb->timeline, addr,
						rfx_order_events, event) == 0)
				{
					psm_free(ionwm, addr);
					return -1;
				}
			}
		}

		return 0;

	case IonStartRecv:
		cxref = (IonCXref *) psp(ionwm, event->ref);
		if (cxref->toNode == getOwnNodeNbr())
		{
			neighbor = getNeighbor(vdb, cxref->fromNode);
			if (neighbor)
			{
				neighbor->recvRate = cxref->xmitRate;
				*forecastNeeded = 1;
			}
		}

		return 0;

	case IonPurgeContact:
		cxref = (IonCXref *) psp(ionwm, event->ref);
		return rfx_remove_contact(cxref->fromTime,
				cxref->fromNode, cxref->toNode);

	default:
		putErrmsg("Invalid RFX timeline event.", itoa(event->type));
		return -1;

	}
}

#if defined (ION_LWT)
int	rfxclock(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	Sdr		sdr;
	PsmPartition	ionwm;
	IonVdb		*vdb;
	long		start = 1;
	time_t		currentTime;
	PsmAddress	elt;
	PsmAddress	addr;
	IonProbe	*probe;
	int		destNodeNbr;
	int		neighborNodeNbr;
	int		forecastNeeded;
	IonEvent	*event;
	int		i;
	PsmAddress	nextElt;
	PsmAddress	reqAddr;
	Requisition	*req;

	if (ionAttach() < 0)
	{
		putErrmsg("rfxclock can't attach to ION.", NULL);
		return -1;
	}

	sdr = getIonsdr();
	ionwm = getIonwm();
	vdb = getIonVdb();
	isignal(SIGTERM, shutDown);

	/*	Main loop: wait for event occurrence time, then
	 *	execute applicable events.				*/

	oK(_running(&start));
	writeMemo("[i] rfxclock is running.");
	while (_running(NULL))
	{
		/*	Sleep for 1 second, then dispatch all events
		 *	whose execution times have now been reached.	*/

		snooze(1);
		currentTime = getUTCTime();
		if (!sdr_begin_xn(sdr))
		{
			putErrmsg("rfxclock failed to begin new transaction.",
					NULL);
			break;
		}

		/*	First enable probes.				*/

		for (elt = sm_list_first(ionwm, vdb->probes); elt;
				elt = sm_list_next(ionwm, elt))
		{
			addr = sm_list_data(ionwm, elt);
			probe = (IonProbe *) psp(ionwm, addr);
			if (probe == NULL)
			{
				putErrmsg("List of probes is corrupt.", NULL);
				sdr_exit_xn(sdr);
				break;
			}

			if (probe->time > currentTime)
			{
				sdr_exit_xn(sdr);
				break;	/*	No more for now.	*/
			}

			/*	Destroy this probe and post the next.	*/

			oK(sm_list_delete(ionwm, elt, NULL, NULL));
			destNodeNbr = probe->destNodeNbr;
			neighborNodeNbr = probe->neighborNodeNbr;
			psm_free(ionwm, addr);
			if (setProbeIsDue(destNodeNbr, neighborNodeNbr) < 0)
			{
				putErrmsg("Can't enable probes.", NULL);
				sdr_cancel_xn(sdr);
				return -1;
			}
		}

		/*	Now dispatch all events that are scheduled
		 *	for any time that is not in the future.  In
		 *	so doing, adjust the volatile contact and
		 *	range state of the local node.			*/

		forecastNeeded = 0;
		while (1)
		{
			elt = sm_rbt_first(ionwm, vdb->timeline);
			if (elt == 0)
			{
				break;
			}

			addr = sm_rbt_data(ionwm, elt);
			event = (IonEvent *) psp(ionwm, addr);
			if (event->time > currentTime)
			{
				break;
			}

			if (dispatchEvent(vdb, event, &forecastNeeded) < 0)
			{
				putErrmsg("Failed handling event.", NULL);
				break;
			}

			oK(sm_rbt_delete(ionwm, vdb->timeline, rfx_order_events,
					event, NULL, NULL));
		}

		/*	Finally, clean up any ZCO space requisitions
		 *	that have been serviced but that applicants
		 *	have not yet claimed; let other applicants
		 *	get access to ZCO space.			*/

		for (i = 0; i < 2; i++)
		{
			for (elt = sm_list_first(ionwm, vdb->requisitions[i]);
					elt; elt = nextElt)
			{
				nextElt = sm_list_next(ionwm, elt);
				reqAddr = sm_list_data(ionwm, elt);
				req = (Requisition *) psp(ionwm, reqAddr);
				switch (req->secondsUnclaimed)
				{
				case -1:	/*	Not serviced.	*/
					break;	/*	Out of switch.	*/

				case MAX_SECONDS_UNCLAIMED:

					/*	Requisition expired.	*/

					sm_SemEnd(req->semaphore);
					ionShred(elt);
					continue;

				default:	/*	Still waiting.	*/
					req->secondsUnclaimed++;
					continue;
				}

				/*	No more serviced requisitions.	*/

				break;		/*	Out of loop.	*/
			}
		}

		/*	Finished all ION 1Hz processing.		*/

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't do ION 1Hz updates.", NULL);
			return -1;
		}

		if (forecastNeeded)
		{
			oK(pseudoshell("ionwarn"));
		}
	}

	writeErrmsgMemos();
	writeMemo("[i] rfxclock has ended.");
	ionDetach();
	return 0;
}
