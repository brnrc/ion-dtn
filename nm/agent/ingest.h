/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: ingest.h
 **
 ** Description: This implements the data ingest thread to receive DTNMP msgs.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/10/13  E. Birrane     Initial Implementation
 *****************************************************************************/

#ifndef _INGEST_H_
#define _INGEST_H_


/* Validation function */
int rx_validate_mid_mc(Lyst mids, int passEmpty);
int rx_validate_rule(rule_time_prod_t *rule);


void *rx_thread(void *threadId);

/* Message Handling Functions. */
void rx_handle_rpt_def(pdu_metadata_t *meta, uint8_t *cursor, uint32_t size, uint32_t *bytes_used);
void rx_handle_exec(pdu_metadata_t *meta, uint8_t *cursor, uint32_t size, uint32_t *bytes_used);
void rx_handle_time_prod(pdu_metadata_t *meta, uint8_t *cursor, uint32_t size, uint32_t *bytes_used);
void rx_handle_macro_def(pdu_metadata_t *meta, uint8_t *cursor, uint32_t size, uint32_t *bytes_used);


#endif /* _INGEST_H_ */
