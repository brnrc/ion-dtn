/*
 *	bpextensions.c:	Bundle Protocol extension definitions
 *			module.
 *
 *	Copyright (c) 2008, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

/*	Add external function declarations between here...		*/

#if 0
#include "snid.h"
#endif
#include "phn.h"
#include "ecos.h"
#include "meb.h"
#include "bae.h"
#ifdef ORIGINAL_BSP
#include "extbspbab.h"
#include "extbsppcb.h"
#include "extbsppib.h"
#else
#include "bspbab.h"
#include "bspbib.h"
#include "bspbcb.h"
#endif /* ORIGINAL_BSP */

#ifdef ENABLE_BPACS
#include "cteb.h"
#endif /* ENABLE_BPACS */

/*	... and here.							*/

static ExtensionDef	extensionDefs[] =
			{
		{ "phn", EXTENSION_TYPE_PHN,
				phn_offer,
				{phn_processOnFwd,
				phn_processOnAccept,
				phn_processOnEnqueue,
				phn_processOnDequeue,
				0},
				phn_release,
				phn_copy,
				0,
				0,
				phn_parse,
				phn_check,
				phn_record,
				phn_clear
		},
#if 0
		{ "snid", EXTENSION_TYPE_SNID,
				snid_offer,
				{snid_processOnFwd,
				snid_processOnAccept,
				snid_processOnEnqueue,
				snid_processOnDequeue,
				0},
				snid_release,
				snid_copy,
				snid_acquire,
				0,
				0
				snid_check,
				snid_record,
				snid_clear
		},
#endif
#ifdef ORIGINAL_BSP
		{ "bsp_bab", BSP_BAB_TYPE,
				bsp_babOffer,
				{0,
				0,
				0,
				bsp_babProcessOnDequeue,
				bsp_babProcessOnTransmit},
				bsp_babRelease,
				bsp_babCopy,
				bsp_babAcquire,
				0,
				0,
				bsp_babCheck,
				0,
				bsp_babClear
		},
		{ "bsp_pib", BSP_PIB_TYPE,
				bsp_pibOffer,
				{0,
				0,
				0,
				bsp_pibProcessOnDequeue,
				0},
				bsp_pibRelease,
				bsp_pibCopy,
				bsp_pibAcquire,
				0,
				0,
				bsp_pibCheck,
				0,
				bsp_pibClear
		},
		{ "bsp_pcb", BSP_PCB_TYPE,
				bsp_pcbOffer,
				{0,
				0,
				0,
				bsp_pcbProcessOnDequeue,
				0},
				bsp_pcbRelease,
				bsp_pcbCopy,
				bsp_pcbAcquire,
				0,
				0,
				bsp_pcbCheck,
                                0,
				bsp_pcbClear
		},
#else
		{ "bab", EXTENSION_TYPE_BAB,
				bsp_babOffer,
				{0,
				0,
				0,
				bsp_babProcessOnDequeue,
				bsp_babProcessOnTransmit},
				bsp_babRelease,
				0,
				bsp_babAcquire,
				0,
				0,
				bsp_babCheck,
				0,
				bsp_babClear
		},
		{ "bcb", EXTENSION_TYPE_BCB,
				bsp_bcbOffer,
				{0,
				0,
				0,
				bsp_bcbProcessOnDequeue,
				0},
				bsp_bcbRelease,
				bsp_bcbCopy,
				bsp_bcbAcquire,
				bsp_bcbDecrypt,
				0,
				0,
                                0,
				bsp_bcbClear
		},
		{ "bib", EXTENSION_TYPE_BIB,
				bsp_bibOffer,
				{0,
				0,
				0,
				0,
				0},
				bsp_bibRelease,
				bsp_bibCopy,
				0,
				0,
				bsp_bibParse,
				bsp_bibCheck,
				0,
				bsp_bibClear
		},
#endif /* ORIGINAL_BSP */
		{ "ecos", EXTENSION_TYPE_ECOS,
				ecos_offer,
				{ecos_processOnFwd,
				ecos_processOnAccept,
				ecos_processOnEnqueue,
				ecos_processOnDequeue,
				0},
				ecos_release,
				ecos_copy,
				0,
				0,
				ecos_parse,
				ecos_check,
				ecos_record,
				ecos_clear
		},
		{ "meb", EXTENSION_TYPE_MEB,
				meb_offer,
				{meb_processOnFwd,
				meb_processOnAccept,
				meb_processOnEnqueue,
				meb_processOnDequeue,
				0},
				meb_release,
				meb_copy,
				meb_acquire,
				0,
				0,
				meb_check,
				meb_record,
				meb_clear
		},
		{ "bae", EXTENSION_TYPE_BAE,
				bae_offer,
				{bae_processOnFwd,
				bae_processOnAccept,
				bae_processOnEnqueue,
				bae_processOnDequeue,
				0},
				bae_release,
				bae_copy,
				0,
				0,
				bae_parse,
				bae_check,
				bae_record,
				bae_clear
		},
#ifdef ENABLE_BPACS
        	{ "cteb", EXTENSION_TYPE_CTEB,
				cteb_offer,
				{0,
				0,
				0,
				cteb_processOnDequeue,
				0},
				cteb_release,
				cteb_copy,
				0,
				0,
				cteb_parse,
				0,
				cteb_record,
				cteb_clear
       		},
#endif /* ENABLE_BPACS */
				{ "unknown",0,0,{0,0,0,0,0},0,0,0,0,0,0,0,0 }
			};

/*	NOTE: the order of appearance of extension definitions in the
 *	extensionSpecs array determines the order in which pre-payload
 *	extension blocks will be inserted into locally sourced bundles
 *	prior to the payload block and the order in which post-payload
 *	extension blocks will be inserted into locally sourced bundles
 *	after the payload block.					*/

static ExtensionSpec	extensionSpecs[] =
			{
#ifdef ORIGINAL_BSP
				{ BSP_BAB_TYPE, 0, 0, 0, 0 },
#else
				{ EXTENSION_TYPE_BAB, 0, 0, 0, 0 },
#endif /* ORIGINAL_BSP */
				{ EXTENSION_TYPE_PHN, 0, 0, 0, 0 },
				{ EXTENSION_TYPE_ECOS, 0, 0, 0, 0 },
				{ EXTENSION_TYPE_MEB, 0, 0, 0, 0 },
				{ EXTENSION_TYPE_BAE, 0, 0, 0, 0 },
#ifdef ENABLE_BPACS
        			{ EXTENSION_TYPE_CTEB, 0, 0, 0, 0 },
#endif /* ENABLE_BPACS */
#ifdef ORIGINAL_BSP
				{ BSP_PIB_TYPE, 0, 0, 0, 0 },
				{ BSP_PCB_TYPE, 0, 0, 0, 0 },
				{ BSP_BAB_TYPE, 0, 0, 1, 1 },
#else
				{ EXTENSION_TYPE_BIB, 1, 0, 0, 0 },
				{ EXTENSION_TYPE_BCB, 1, 0, 0, 0 },
				{ EXTENSION_TYPE_BAB, 0, 0, 1, 1 },
#endif /* ORIGINAL_BSP */
				{ 0,0,0,0,0 }
			};
