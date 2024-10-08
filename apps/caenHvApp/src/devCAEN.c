/*****************************************************************************\
 * File: devCEAN.c                                                           *
 *                                                                           *
 * Overview:                                                                 *
 *   This file contains the EPICS record handlers for the custom CAEN HV     *
 *   device support layer.  The functions call driver support modules which  *
 *   are in the file access.c.  This was taken from Slominski's code         *
 *                                                                           *
 * Revision History:                                                         *
 *   07/010/2003 - Initial release  Hovanes Egiyan                                          *
\*****************************************************************************/

#define ALLSET_THROUGH_ONE 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include "sy1527.h"
#include <shareLib.h>
#include <unistd.h>
epicsShareFunc int errlogPrintf(const char *pFormat, ...);

extern int mainframes[MAX_HVPS];
extern HV Measure[MAX_HVPS];

typedef int BOOL;
typedef int STATUS;

/* 
   I put these here to avoid problems 
   assiciated with migratin from vxWorks
*/
#ifndef ERROR
#define ERROR (-1)
#endif

#ifndef OK
#define OK (0)
#endif


#include <alarm.h> 
#include <dbDefs.h> 
#include <dbAccess.h>
#include <recSup.h>
#include <recGbl.h> 
#include <devSup.h>
#include <boRecord.h>
#include <biRecord.h>
#include <stringinRecord.h>
#include <waveformRecord.h>
#include <aoRecord.h>
#include <epicsExport.h> 
#include "command.h"

#include "sy1527epics1.h"

extern int is_mainframe_read[256];
extern int nmainframes;
int FLAG_BLOCK_INIT=1;

static long write_ao(struct aoRecord *);
static long init_ao(struct aoRecord *); 
static long read_bi(struct biRecord *); 
static long read_si(struct stringinRecord *); 
static long read_waveform(struct waveformRecord *); 
static long write_bo(struct boRecord *);
static long init_bo(struct boRecord *);

void block_until_fraimworks_read();
/* Thses EPICS structures associate the CAEN Dev records with the
 * initialization and processing routines defined in this file.  The
 * global structure names are required by epics because of the
 * definitions in the device application's '.dbd' file.
 */

struct
{ long number;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN get_ioint_info;
  DEVSUPFUN write_bo;
} devBoCAEN = {5, NULL, NULL, init_bo, NULL, write_bo};
epicsExportAddress(dset,devBoCAEN);

struct
{ long number;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN get_ioint_info;
  DEVSUPFUN read_bi;
} devBiCAEN = {5, NULL, NULL, NULL, NULL, read_bi};
epicsExportAddress(dset,devBiCAEN);
struct
{ long number;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN get_ioint_info;
  DEVSUPFUN read_si;
} devSiCAEN = {5, NULL, NULL, NULL, NULL, read_si};
epicsExportAddress(dset,devSiCAEN);
struct
{ long number;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN get_ioint_info;
  DEVSUPFUN read_waveform;
} devWaveformCAEN = {5, NULL, NULL, NULL, NULL, read_waveform};
epicsExportAddress(dset,devWaveformCAEN);
/*
struct
{ long number;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN read_bi;
  DEVSUPFUN get_ioint_info;
} devBiCAEN = {5, NULL, NULL, NULL, read_bi, NULL};
epicsExportAddress(dset,devBiCAEN);
*/
struct
{ long number;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN get_ioint_info;
  DEVSUPFUN write_ao;
  DEVSUPFUN special_linconv;
} devAoCAEN = {6, NULL, NULL, init_ao, NULL, write_ao, NULL};
epicsExportAddress(dset,devAoCAEN);


/* Name: init_bo                                                       *\
 | Parameters: pbo - Bo record pointer                                 |
 | Return: Error status, zero is normal return                         |
 | Remarks:                                                            |
 |   This function is called at startup of the IOC to perform any      |
 |   initialization required by the record handler.  In this case,     |
 |   the HVcmd records are intialized to the status of the chassis.    |
\*                                                                     */
//===============================================================================================
static long init_bo(struct boRecord  *pbo)
{
  float value;

  /* Get the card & signal numbers from the record, pointed to by pbo.
     These data items are not used as card/signal, but are defined as
     {card 0-7: chassis, 8-15: slot;  signal 0-7:channel, 8-15:command}.
   */

  struct vmeio *pvmeio = (struct vmeio *) &(pbo->out.value);  

  unsigned short* card    = (unsigned short*) &pvmeio->card;
  unsigned short* signal  = (unsigned short*) &pvmeio->signal;

  unsigned slot = (*card)>>8;
  unsigned chassis = (*card) - ((slot)<<8) ;

  unsigned command = (*signal)>>8;
  unsigned channel = (*signal) - ((command)<<8);

  block_until_fraimworks_read();

  //char tmp[81]; /// temporal
  //int first_channel=channel;
  //int chs_number=command;
  if (strstr(pbo->desc,"smi")) {
    //strncpy(tmp, pbo->name, strlen(pbo->name)-strlen("_BO"));
    //tmp[strlen(pbo->name)-strlen("_BO")]=0;
    ////retv=sy1527BoardSmiMonitor(tmp, chassis, slot, first_channel, chs_number);
    //sy1527BoardSmiMonitor(tmp, chassis, slot, first_channel, chs_number);
    //pbo->rval = 0; /// means nothing
  }
  else {
   if (command == S_CE) {
     if (CAEN_GetProperty(chassis, slot, channel, "CE", &value) == ERROR) {
       char alert[128];
       sprintf(alert, "CAEN init_bo - %s(%d): Card=%d Signal=%d",
               __FILE__, __LINE__, (*(unsigned short*)card), (*(unsigned short*)signal) );
       recGblRecordError(S_db_badField, (void *) pbo, alert);
       return(S_db_badField);
     }
     pbo->rval = value;
   }
   else if (command == S_HV) {
     int onoff;
     //retv = CAEN_GetHv(chassis, &onoff);
     CAEN_GetHv(chassis, &onoff);
     pbo->rval = onoff;
   }
   else if (command == S_CHHV) {
     int st=sy1527GetChannelStatus(chassis, slot,channel);
     pbo->rval = st & (1<<0); ///only1527
   }
   else if (command == S_BDHV) {
     pbo->rval=0; /// just init somehow
   }
   else if (command == S_INTLK) {
     pbo->rval = sy1527GetChannelInterlock(chassis, slot,channel);
   }
   else if (command == S_RANGE) {
     pbo->rval = sy1527GetChannelRange(chassis, slot,channel);
   }
   else if (command == S_CLR) {
     pbo->rval = 0;
   }
  }
  return 0;
}


/* Name: write_bo                                                      *\
 | Parameters: pbo - Bo record pointer                                 |
 | Return: Error status, zero is normal return                         |
 | Remarks:                                                            |
 |   This function handles EPICS Bo record requests to the CAEN HV     |
 |   device support.  The only binary outputs are the commanding of    |
 |   high voltage on/off for a chassis and the enable/disable of a     |
 |   channel.                                                          |
\*                                                                     */
//===============================================================================================
static long write_bo(struct boRecord *pbo)
{
  STATUS status;

  /* Get the card & signal numbers from the record, pointed to by pbo.
     These data items are not used as card/signal, but are defined as
     {card 0-7: chassis, 8-15: slot;  signal 0-7:channel, 8-15:command}.
   */
  struct vmeio *pvmeio = (struct vmeio *) &(pbo->out.value);  
  unsigned short* card    = (unsigned short*) &pvmeio->card;
  unsigned short* signal  = (unsigned short*) &pvmeio->signal;

  unsigned slot = (*card)>>8;
  unsigned chassis = (*card) - ((slot)<<8) ;

  unsigned command = (*signal)>>8;
  unsigned channel = (*signal) - ((command)<<8);

  printf("WRITE_BO ======================================== name=%s %d %d\n",pbo->name, pbo->val, pbo->rval);

  // identify the type of Bo and issue a command to the desired chassis.
  //int first_channel=channel, chs_number=command;
  if (strstr(pbo->desc,"smi")) {
  //  //retv=sy1527BoardSmiControl(pbo->name, chassis, slot,  
  //  sy1527BoardSmiControl(pbo->name, chassis, slot,  
  //  first_channel, chs_number, (unsigned char)pbo->rval);
  }
  else if (strstr(pbo->desc,"crate_fsm_init")) { /// for any order of launch of ioc and fsm
  //  //retv=sy1527CrateSmiInit(pbo->name, chassis);
  //  sy1527CrateSmiInit(pbo->name, chassis);
  }
  else{
    if (command == S_CE)
      status = CAEN_HVload(chassis, slot, channel, "CE", (float)pbo->rval );
    else if (command == S_HV)
      status = CAEN_SetHV(chassis, (unsigned char)pbo->rval  );
    else if (command == S_CHHV)
      status = CAEN_HVload(chassis, slot, channel, "CHONOFF", (float)pbo->rval );
    else if (command == S_BDHV)
      //status = CAEN_HVload(chassis, slot, channel, "CHONOFF", (float)pbo->rval ); 
      status = sy1527SetBoardOnOff(chassis, slot, (unsigned char)pbo->rval );
    else if (command == S_INTLK)
      status = sy1527SetChannelInterlock(chassis, slot, channel, (unsigned int)pbo->rval );
    else if (command == S_RANGE)
      status = sy1527SetChannelRange(chassis, slot, channel, (unsigned int)pbo->rval );
    else if (command == S_CLR)
      status = sy1527ExecComm(chassis,"ClearAlarm");
    else status = ERROR;

    // Alert if an error occures processing the request.
    if (status == ERROR) {
      char alert[128];
      sprintf(alert, "CAEN Bo - %s(%d): Card=%d Signal=%d",
              __FILE__, __LINE__, (*(unsigned short*)card), (*(unsigned short*)signal) );
      recGblRecordError(S_db_badField, (void *) pbo, alert);
      return(S_db_badField);
    }
  }
  return 0;  
}


/* Name: read_bi                                                       *\
 | Parameters: pbi - Bi record pointer                                 |
 | Return: Error status, zero is normal return                         |
 | Remarks:                                                            |
 |   This function handles EPICS Bi record requests to access the      |
 |   state of a CAEN   HV chassis.  The three types of binary accesses |
 |   are chassis validity, alarm status,  and high voltage on.         |
\*

                                                              */
#include "smiuirtl.h"
//===============================================================================================
static long read_bi(struct biRecord *pbi)
{
  int result;

  /* Get the card & signal numbers from the record, pointed to by pbo.
     These data items are not used as card/signal, but are defined as
     {card 0-7: chassis, 8-15: slot;  signal 0-7:channel, 8-15:command}.
     Note that slot and channel numbers are not used in this situation.
   */
  struct vmeio *pvmeio = (struct vmeio *) &(pbi->inp.value);  

  unsigned short* card    = (unsigned short*) &pvmeio->card;
  unsigned short* signal  = (unsigned short*) &pvmeio->signal;

  unsigned slot = (*card)>>8;
  unsigned chassis = (*card) - ((slot)<<8) ;

  unsigned command = (*signal)>>8;
  //unsigned channel = (*signal) - ((command)<<8);

  /* Access the requested chassis's database, depending on which of the two
   * bi commands was sent.  Show an error if the request is not recognized.
   */
  int onoff;
  //int retv;
  if (strstr(pbi->desc,"smi")) {
    //retv=sy1527BoardSmiMonitor(pbi->name, chassis, slot, channel, command);
    //pbi->rval = retv; /// means nothing
  }
  else {
    if      (command == G_Valid) result = CAEN_GetValidity(chassis);
    else if (command == G_HV)    result = CAEN_GetHv(chassis, &onoff);
    else if (command == G_Alarm) result = CAEN_GetAlarm(chassis);
    else {
      char alert[128];
      sprintf(alert, "%s(%d): Card=%d Signal=%d", __FILE__, __LINE__,
        (*(unsigned short*)card), (*(unsigned short*)signal) );
      recGblRecordError(S_db_badField, (void *) pbi, alert);
      return(S_db_badField);
    }
    // Show error if the requested chassis does not exist.
    if (result == -1) {
      recGblRecordError(S_db_badField, (void *) pbi, "No such chassis");
      return(S_db_badField);
    }
    else pbi->rval = onoff;
  }

  return 0;  
}


static long read_si(struct stringinRecord *pbi)
{
  struct vmeio *pvmeio = (struct vmeio *) &(pbi->inp.value);  

  unsigned short* card    = (unsigned short*) &pvmeio->card;
  unsigned short* signal  = (unsigned short*) &pvmeio->signal;

  unsigned slot = (*card)>>8;
  unsigned chassis = (*card) - ((slot)<<8) ;

  unsigned command = (*signal);//>>8;

  if (command == G_HVFS)
      sy1527GetMainframeHVFanStat(chassis,pbi->val); 
  else if (command == G_PWFS)
      sy1527GetMainframePWFanStat(chassis,pbi->val); 
  else if (command == G_PWV)
      sy1527GetMainframePWVoltage(chassis,pbi->val); 
  else if (command == G_HVPW)
      sy1527GetMainframeHvPwSM(chassis,pbi->val); 
  else if (command == G_MOD)
      sy1527GetMainframeModelName(chassis,pbi->val); 
  else if (command == G_SWR)
      sy1527GetMainframeSwRelease(chassis,pbi->val);
  else if (command == G_BDMOD)
      sy1527GetBoardModelName(chassis,slot,pbi->val);
  else {
      char alert[128];
      sprintf(alert, "%s(%d): Chassis%d Command%d", __FILE__, __LINE__,
              chassis,command);
      recGblRecordError(S_db_badField, (void *) pbi, alert);
      return(S_db_badField);
  }
  return 0;  
}

static long read_waveform(struct waveformRecord *pbi)
{
  struct vmeio *pvmeio = (struct vmeio *) &(pbi->inp.value);  

  unsigned short* card    = (unsigned short*) &pvmeio->card;
  unsigned short* signal  = (unsigned short*) &pvmeio->signal;

  unsigned slot = (*card)>>8;
  unsigned chassis = (*card) - ((slot)<<8) ;

  unsigned command = (*signal);//>>8;

  float values[MAX_SYSPROPS];

  if (command == G_HVFS)
      sy1527GetMainframeHVFanStats(chassis,values); 
  else if (command == G_PWFS)
      sy1527GetMainframePWFanStats(chassis,values); 
  else if (command == G_PWV)
      sy1527GetMainframePWVoltages(chassis,values); 
  else {
      char alert[128];
      sprintf(alert, "%s(%d): Chassis%d Command%d", __FILE__, __LINE__,
              chassis,command);
      recGblRecordError(S_db_badField, (void *) pbi, alert);
      return(S_db_badField);
  }

  for (pbi->nord=0; pbi->nord<MAX_SYSPROPS; pbi->nord++)
      ((float*)pbi->bptr)[pbi->nord] = values[pbi->nord];

  return 0;  
}

/* Name: init_ao                                                       *\
 | Parameters: pao - Ao record pointer                                 |
 | Return: Error status, zero is normal return                         |
 | Remarks:                                                            |
 |   This function is called at startup of the IOC to perform any      |
 |   initialization required by the record handler.  In this case,     |
 |   the numeric control records are intialized to the status of the   |
 |   chassis.                                                          |
\*                                                                     */
//===============================================================================================
static long init_ao(struct aoRecord  *pao)
{

  if(ALLSET_THROUGH_ONE)return 0;

  STATUS status;
  float value;

  /* Get the card & signal numbers from the record, pointed to by pbo.
     These data items are not used as card/signal, but are defined as
     {card 0-7: chassis, 8-15: slot;  signal 0-7:channel, 8-15:command}.
   */
  struct vmeio *pvmeio = (struct vmeio *) &(pao->out.value);  

  unsigned short* card    = (unsigned short*) &pvmeio->card;
  unsigned short* signal  = (unsigned short*) &pvmeio->signal;

  unsigned slot = (*card)>>8;
  unsigned chassis = (*card) - ((slot)<<8) ;

  unsigned command = (*signal)>>8;
  unsigned channel = (*signal) - ((command)<<8);

  // Initialize the record depending on the passed parameters.

  block_until_fraimworks_read();

  switch (command)
  { case S_DV:
      status = CAEN_GetProperty(chassis, slot, channel, "DV", &value); break;
    case S_RDN:
      status = CAEN_GetProperty(chassis, slot, channel, "RDN", &value); break;
    case S_RUP:
      status = CAEN_GetProperty(chassis, slot, channel, "RUP", &value); break;
    case S_TC:
      status = CAEN_GetProperty(chassis, slot, channel, "TC", &value); break;
    case S_MVDZ:
      status = CAEN_GetProperty(chassis, slot, channel, "MVDZ", &value); break;
    case S_MCDZ:
      status = CAEN_GetProperty(chassis, slot, channel, "MCDZ", &value); break;
    case S_SOT:
      status = CAEN_GetProperty(chassis, slot, channel, "SOT", &value); break;
    case S_VMAX:
      status = CAEN_GetProperty(chassis, slot, channel, "HVL", &value); break;
    case S_PRD:
      status = CAEN_GetProperty(chassis, slot, channel, "PRD", &value); break;
    case S_UNVT:
      status = CAEN_GetProperty(chassis, slot, channel, "UNVT", &value); break;
    case S_OVVT:
      status = CAEN_GetProperty(chassis, slot, channel, "OVVT", &value); break;
    default: status = ERROR; break;
  }

  // these result in setpoint initted to zero (and do not propogate to hw):
  //if (status ==OK) pao->val = value;
  //if (status ==OK) pao->oval = value;

  // this results in setpoint initted "correctly" but truncated to an integer 
  // since rval is an integer (and also does not propogate to hw):
  if (status ==OK) pao->rval = value;

  else {
    char alert[128];
    sprintf(alert, "init_ao - %s(%d): Card=%d Signal=%dx",
	    __FILE__, __LINE__, (*(unsigned short*)card), (*(unsigned short*)signal) );
    recGblRecordError(S_db_badField, (void *) pao, alert);
    return(S_db_badField);
  }

  return 0;  
}


/* Name: write_ao                                                      *\
 | Parameters: pao - Ao record pointer                                 |
 | Return: Error status, zero is normal return                         |
 | Remarks:                                                            |
 |   This function handles EPICS Ao record requests for the CAEN       |
 |   HV device support.  A command string is generated, depending on   |
 |   the type record received. The HVload function is called to issue  |
 |   commands to a chassis.                                            |
\*                                                                     */
//===============================================================================================
static long write_ao(struct aoRecord *pao)
{
  STATUS status = OK;
  char *property; /// value[8]

  /* Get the card & signal numbers from the record, pointed to by pbo.
     These data items are not used as card/signal, but are defined as
     {card 0-7: chassis, 8-15: slot;  signal 0-7:channel, 8-15:command}.
   */

  struct vmeio *pvmeio = (struct vmeio *) &(pao->out.value);  
  unsigned short* card    = (unsigned short*) &pvmeio->card;
  unsigned short* signal  = (unsigned short*) &pvmeio->signal;

  unsigned slot = (*card)>>8;
  unsigned chassis = (*card) - ((slot)<<8) ;

  unsigned command = (*signal)>>8;
  unsigned channel = (*signal) - ((command)<<8);

  float value_f;
  if (ALLSET_THROUGH_ONE) {
   unsigned long long t=pao->val;
   chassis= 0xff & t; t = t >> 8;
   slot= 0xff & t; t = t >> 8;
   channel= 0xff & t; t = t >> 8;
   command= 0xff & t; t = t >> 8; 
   value_f=(((double) t)/100.);
  }

  // Convert the record's assignment value to ASCII for passing to HVload.
printf("WRITE_AO ======================================== name=%s %f %d\n",pao->name, pao->val, pao->rval);

  // my: not much meaning but to save in EPICS not a huge number, but real value
  if (ALLSET_THROUGH_ONE) pao->val = value_f; 
  else value_f = pao->val;

printf("********************************** %d %d %d %d %f\n",chassis, slot, channel, command, value_f);
 
  // Set the property name for HVload, depending on the passed command code.
  switch (command)
  { case S_DV:   property = "DV"; break;
    case S_RDN:  property = "RDN"; break;
    case S_RUP:  property = "RUP"; break;
    case S_TC:   property = "TC"; break;
    case S_MVDZ: property = "MVDZ"; break;
    case S_MCDZ: property = "MCDZ"; break;
    case S_SOT:  property = "SOT"; break;
    case S_VMAX:  property = "HVL"; break;
    case S_PRD:  property = "PRD"; break;
    case S_UNVT: property = "UNVT"; break;
    case S_OVVT: property = "OVVT"; break;
    default: status = ERROR; break;
  }

  /* Command the property of one channel on a HV module in a slot of the
   * chassis to the desired value.
   */
  if (status == OK) {
    /*    status = CAEN_HVload(chassis, slot, channel, property, value);  */
    status = CAEN_HVload(chassis, slot, channel, property, value_f );

  }
  else if (ALLSET_THROUGH_ONE) {
   if (command == S_CE)
     status = CAEN_HVload(chassis, slot, channel, "CE", (float)pao->val );
   else if (command == S_HV) {
     printf("=================== ***************** %d %d\n", chassis, (unsigned char)pao->val);
     status = CAEN_SetHV(chassis, (unsigned char)pao->val  );
   }
   else status = ERROR;
  }

  if (status == ERROR) {
    char alert[128];
    sprintf(alert, "Ao - %s(%d): Card=%d Signal=%d",
            __FILE__, __LINE__, (*(unsigned short*)card), (*(unsigned short*)signal));
    recGblRecordError(S_db_badField, (void *) pao, alert);
    return(S_db_badField);
  }

  return 0;  
}

//=====================================================================================

void block_until_fraimworks_read(){
  int i;
  if(FLAG_BLOCK_INIT){
    while(1) {  
      int allmfsareread=1;
      for(i=0; i<nmainframes; i++) {
        if (is_mainframe_read[i] == 0 && mainframes[i]!=-1) {
          allmfsareread=0;
          printf("is_mainframe_read wait %d %d\n",i,mainframes[i]);
        }
      }
      if (allmfsareread) {
        FLAG_BLOCK_INIT=0;
        break;
      }
      sleep(1);
      printf("is_mainframe_read wait\n");
    }
  }
}

///=======================================================================================================
///=======================================================================================================

/* $Header: HVCAENx527/HVCAENx527App/src/HVCAENx527chMBBio.c 1.14 2007/06/01 13:32:58CST Ru Igarashi (igarasr) Exp Ru Igarashi (igarasr)(2007/06/01 13:32:58CST) $ 
 *
 * Copyright Canadian Light Source, Inc.  All rights reserved.
 *    - see licence.txt and licence_CAEN.txt for limitations on use.
 */
/*
 * HVCAENx527chMBBio.c:
 * MultiBit Binary input record and output record device support routines.
 */
///#include "HVCAENx527.h"

#include <mbbiRecord.h>

/*
 * devCAENx527chMBBi
 */

void setMbbiField (char* field, unsigned int id) {
	size_t n;
	mbbiRecord dummy;
	n = sizeof(dummy.zrst);
	if (id < MAX_HVPS) {
		// if (Crate[id].connected == 1) {
        if (Measure[id].id != -1) {
			strncpy(field, Measure[id].name, n);
		}
	} else {
		errlogPrintf("Possible Create IDes are: 0 - %d, got %d\n", MAX_HVPS-1, id);
	}
	return;
}

static long
init_record_mbbi_mf( mbbiRecord *pior)
{
	char *str;

/*	if( pior->inp.type != CONSTANT)
	{
		errlogPrintf( "%s(%d): mbbi INP field type should be CONSTANT(0)\n", pior->name, pior->inp.type);
		return( S_db_badField);
	}*/

	/* parse device dependent option string and set data pointer */
	str = pior->inp.value.constantStr;
	if (strcmp(str,"crateList") != 0) {
		errlogPrintf( "ERROR: Unsupported INP field %s for PV %s . Should be crates instead.", str, pior->name);
		return(-1);
	}

	setMbbiField( pior->zrst,  0 );
	setMbbiField( pior->onst,  1 );
	setMbbiField( pior->twst,  2 );
	setMbbiField( pior->thst,  3 );
	setMbbiField( pior->frst,  4 );
	setMbbiField( pior->fvst,  5 );
	setMbbiField( pior->sxst,  6 );
	setMbbiField( pior->svst,  7 );
	setMbbiField( pior->eist,  8 );
	setMbbiField( pior->nist,  9 );
	setMbbiField( pior->test, 10 );
	setMbbiField( pior->elst, 11 );
	setMbbiField( pior->tvst, 12 );
	setMbbiField( pior->ttst, 13 );
	setMbbiField( pior->ftst, 14 );
	setMbbiField( pior->ffst, 15 );

	return( 0);
}

static long
read_mbbi_mf( mbbiRecord *pior)
{
	/*pior->val = 0;*/
	pior->udf = FALSE;
/*PDEBUG(10) printf( "DEBUG: get %s = %o %hd\n", pp->pname, (short)(pp->pval.l), pior->val);*/

	return(0);
}

struct
{
        long number;
        DEVSUPFUN       report;
        DEVSUPFUN       init;
        DEVSUPFUN       init_record;
        DEVSUPFUN       get_ioint_info;
        DEVSUPFUN       read_mbbi;
} devCAENx527MBBi =
        {
                5,
                NULL,
                NULL,
                init_record_mbbi_mf,
                NULL,
                read_mbbi_mf
        };


#include <epicsExport.h>
epicsExportAddress(dset,devCAENx527MBBi);

/*
 *  $Log: HVCAENx527/HVCAENx527App/src/HVCAENx527chMBBio.c  $
 *  Revision 1.14 2007/06/01 13:32:58CST Ru Igarashi (igarasr) 
 *  Member moved from EPICS/HVCAENx527App/src/HVCAENx527chMBBio.c in project e:/MKS_Home/archive/cs/epics_local/drivers/CAENx527HV/project.pj to HVCAENx527/HVCAENx527App/src/HVCAENx527chMBBio.c in project e:/MKS_Home/archive/cs/epics_local/drivers/CAENx527HV/project.pj.
 */


///=======================================================================================================
///=======================================================================================================

///#include "HVCAENx527.h"

/*#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "epicsTime.h"
#include "recGbl.h"
#include "devSup.h"
#include "link.h"*/
//#include "stringinRecord.h"
///#include "epicsExport.h"*

/* Create the dset for devSiSoft */
static long init_record_crate_stringin(stringinRecord *prec);
static long read_crate_stringin(stringinRecord *prec);

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_stringin;
} devCAENx527Stringin = {
    5,
    NULL,
    NULL,
    init_record_crate_stringin,
    NULL,
    read_crate_stringin
};
epicsExportAddress(dset, devCAENx527Stringin);

static long init_record_crate_stringin(stringinRecord *prec)
{
    if (prec->inp.type != INST_IO) {
        recGblRecordError(S_db_badField, (void *)prec, "devCAENx527Stringin (init_record_crate_stringin) Illegal INP field");
        return S_db_badField;
    }
    return 0;
}

static long read_crate_stringin(stringinRecord *prec)
{
    struct instio *pinstio;
	char mf[64];
	int slot;

	/* parse device dependent option string and set data pointer */
	pinstio = &(prec->inp.value.instio);
	sscanf(pinstio->string, "%s %d", mf, &slot);
	if( mf[0] == '\0' || slot<-1 || slot>MAX_SLOT ) {
		printf( "%s: Invalid device parameters: \"%s\"\n", prec->name, pinstio->string);
		return(-1);
	}

	int i;
	i = 0;
	while ( i < MAX_HVPS && strcmp(/** Crate[i].name*/Measure[i].name, mf) != 0) i++;

	if (i>=MAX_HVPS) return(-1);
	if (slot == -1) {
		snprintf(prec->val, sizeof(prec->val),"%s", Measure[i].IPADDR /** Crate[i].IPaddr */);
	}
    else if (slot>=0) {
		///if (Crate[i].hvchmap[slot].nchan) {
        if (Measure[i].board[slot].nchannels) {
			///snprintf(prec->val, sizeof(prec->val),"%s (%d ch)", Crate[i].hvchmap[slot].slname, Crate[i].hvchmap[slot].nchan);
			snprintf(prec->val, sizeof(prec->val),"%s (%d ch)", Measure[i].board[slot].modelname, Measure[i].board[slot].nchannels);
		} else {
			snprintf(prec->val, sizeof(prec->val),"N/A");
		}
	}
	return 0;
}


