
/* sy1527_test.c - generic test for sy1527 driver */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "sy1527.h"

int main(int argc,char** argv)
{
  int i, id = 0, board = 0, channel = 2;
  char ip_address[100]="129.57.86.162";
  float u, uset, uget;
  unsigned int l, lget, active, onoff, alarm, itmp;
  char parname[MAX_CAEN_NAME];
  strcpy(parname, "IMon(0)");

  if (argc>2) {
      board = atoi(argv[1]);
      channel = atoi(argv[2]);
  }

  printf("\n\n=============== CAEN mainframe SY1527/SY4527 test ===============\n");
  printf("===== ip_address= %s =====\n\n\n",ip_address);

  sy1527Start(id, ip_address);

  printf("\n\n======= sy1527PrintParams =======\n");
  sy1527PrintParams(id);

  sy1527PrintMap(id);

  printf("\n\n======= sy1527PrintSysProps =======\n");
  sy1527PrintSysProps(id);

  printf("\n\n======= sy1527PrintExecCommList =======\n");
  sy1527PrintExecCommList(id);
 
  printf("\n\n======= sy1527PrintBoardProps =======\n");
  sy1527PrintBoardProps(id,0);
  sy1527PrintBoardProps(id,1);
  sy1527PrintBoardProps(id,2);
  sy1527PrintBoardProps(id,3);
  sy1527PrintBoardProps(id,4);
  sy1527PrintBoardProps(id,5);
  sy1527PrintBoardProps(id,6);
  sy1527PrintBoardProps(id,7);
  sy1527PrintBoardProps(id,8);
  sy1527PrintBoardProps(id,9);
  sy1527PrintBoardProps(id,10);
  sy1527PrintBoardProps(id,11);
  sy1527PrintBoardProps(id,12);
  sy1527PrintBoardProps(id,13);
  sy1527PrintBoardProps(id,14);
  sy1527PrintBoardProps(id,15);

  return(0);

  sy1527BoardClearAlarm(id,0);

  printf("\n\n======= sy1527ExecComm(ClearAlarm) =======\n");
  sy1527ExecComm(id,"ClearAlarm");

  sy1527GetBoard(id, board);

  sy1527Measure2Demand(id, board);

  sy1527SetBoard(id, board);
  
  printf("sleep1 ..\n");fflush(stdout);
  sleep(3);

  printf("==> 0x%08x\n",sy1527GetChannelStatus(id, board, channel));

  u = 9.8;
  l = 1;

  printf("--> id=%d board=%d channel=%d\n",id,board,channel);

  sy1527SetChannelEnableDisable(id, board, channel, 1);
  sleep(5);

  itmp=sy1527GetChannelEnableDisable(id, board, channel);
  printf("1: itmp=%d\n",itmp);

  sleep(5);
  exit(0);

  sy1527SetChannelEnableDisable(id, board, channel, 1);
  itmp=sy1527GetChannelEnableDisable(id, board, channel);
  printf("1: itmp=%d\n",itmp);

  sy1527SetChannelEnableDisable(id, board, channel, 0);
  itmp=sy1527GetChannelEnableDisable(id, board, channel);
  printf("2: itmp=%d\n",itmp);

  sy1527SetChannelEnableDisable(id, board, channel, 1);
  itmp=sy1527GetChannelEnableDisable(id, board, channel);
  printf("3: itmp=%d\n",itmp);

  sy1527SetChannelDemandVoltage(id, board, channel, u);
  sy1527SetChannelOnOff(id, board, channel, l);
  printf("==> 0x%08x\n",sy1527GetChannelStatus(id, board, channel));
  printf("set U=%f OnOff=%u\n",u,l);
  printf("==> 0x%08x\n",sy1527GetChannelStatus(id, board, channel));
  sleep(1);
  printf("==> 0x%08x\n",sy1527GetChannelStatus(id, board, channel));
  sleep(1);
  printf("==> 0x%08x\n",sy1527GetChannelStatus(id, board, channel));
  sleep(1);
  printf("==> 0x%08x\n",sy1527GetChannelStatus(id, board, channel));
  sleep(3);
  
  for(i=0; i<10; i++)
  {
    sleep(1);
    if(i==3) sy1527SetChannelOnOff(id, board, channel, 1);
    uset = sy1527GetChannelDemandVoltage(id, board, channel);
    uget = sy1527GetChannelMeasuredVoltage(id, board, channel);
    lget = sy1527GetChannelOnOff(id, board, channel);
    printf("got U(set)=%f, U(get)=%f OnOff=%u\n",uset,uget,lget);
  }
  sleep(3);
  
  sy1527GetMainframeStatus((unsigned int) id,  (int *) &(active), (int *) &(onoff), (int *) &alarm);
  printf("3status: %d %d %d\n",active, onoff, alarm);
  sleep(1);
  sy1527SetMainframeOnOff(id, 0);
  sleep(3);
  sy1527GetMainframeStatus((unsigned int) id, (int *) &active, (int *) &onoff, (int *) &alarm);
  printf("3status: %d %d %d\n",active, onoff, alarm);
  sleep(1);

  printf("STATUS=[0]=0x%08x\n",sy1527GetChannelStatus(id, 2, 0));
  printf("STATUS=[1]=0x%08x\n",sy1527GetChannelStatus(id, 2, 1));
  printf("STATUS=[2]=0x%08x\n",sy1527GetChannelStatus(id, 2, 2));
  sleep(1);
  sy1527Stop(id);

  printf("sleep2 ..\n");fflush(stdout);
  sleep(3);
  printf(".. done2\n");fflush(stdout);

  exit(0);
} 
