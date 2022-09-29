#!/bin/bash
# quick hack to cycle RICH LV and reinitialize FPGAs, and output to log file

logfile="/usr/clas12/DATA/logs/rich-lvcycle.logtmp"
log="tee -a $logfile"

# this was probably because ssh keys are required:
me=`whoami`
if ! [ $me == "clasrun" ]
then
    echo "You must be clasrun, but you are $me" | $log
    exit
fi

function date_msg {
    date | $log
}

function start_msg {
    date_msg
    echo "#####################################################" | $log
    echo "#                                                   #" | $log
    echo "# NOTE: the DAQ will need to be reinitialized after #" | $log
    echo "#  ***AFTER*** this script is complete:             #" | $log
    echo "#                                                   #" | $log
    echo "#   Cancel->Reset->Configure->Download->Prestart    #" | $log
    echo "#                                                   #" | $log
    echo "#####################################################" | $log
    echo  | $log
}

function failure_msg {
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" | $log
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" | $log
    echo "!                                                  !" | $log
    echo "!  $@" | $log
    echo "!                                                  !" | $log
    echo "!            !!  Contact RICH Expert !!            !" | $log
    echo "!                                                  !" | $log
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" | $log
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" | $log
    date_msg
    echo "press Return to continue, which will close this window!"
    read
    exit 1
}

function success_msg {
    echo  | $log
    echo "#####################################################" | $log
    echo "#                                                   #" | $log
    echo "#           rich-lvcycle.sh COMPLETE                #" | $log
    echo "#                                                   #" | $log
    echo "# !!!!!!!!!!!   WARNING, SEE BELOW   !!!!!!!!!!!!!  #" | $log
    echo "#                                                   #" | $log
    echo "# NOTE1: RICH temperatures and scalers can take up  #" | $log
    echo "#     to one minute to update after this recovery.  #" | $log
    echo "#                                                   #" | $log
    echo "# NOTE2: the DAQ will now need to be reinitialized: #" | $log
    echo "#   Cancel->Reset->Configure->Download->Prestar t   #" | $log
    echo "#                                                   #" | $log
    echo "#####################################################" | $log
    date_msg
    echo "press Return to continue, which will close this window!"
    read
    exit 0
}

function lvcycle {
  pvGo=$1
  pvCheck=$2
  cnt=0
  cnterr=0
  caput $pvGo 1 | $log
  sleep 5
  while [ 1 ]
  do
      sleep 1
      echo -n '.'
      stat=`caget -t $pvCheck`
      if [ $stat -eq 1 ]
      then
          sleep 1
          break
      fi
      let cnt=$cnt+1
      if [ $cnt -gt 20 ]
      then
          let cnterr=$cnterr+1
          if [ $cnterr -lt 3 ]
          then
              echo "Trying Again to Clear Errors ..." | $log
              caput B_HW_HVRICH1:ClearAlarm 1 | $log
              caput B_HW_HVRICH2:ClearAlarm 1 | $log
              sleep 5
              caput $pvGo 1 | $log
              sleep 5
              let cnt=$cnt-15
          else
              failure_msg ERROR on $pvCheck or ClearAlarm
          fi
      fi
  done
}

function checkssh {
  hostname=$1
  maxtries=$2
  delay=$3
  tries=0
  echo -e "\nWaiting $delay seconds before trying to connect to rich4 ..." | $log
  for xx in `seq $delay`
  do
      echo -n '.' | $log
      sleep 1
  done
  while [ 1 ]
  do
    let tries=$tries+1
    echo && echo -n -e "Attempting ssh ... " | $log
    ssh -q $1 exit
    if [ $? -eq 0 ]
    then
        echo " SUCCESS." | $log
        break
    else
        echo " failed.  Retrying ..." | $log
    fi
    if [ $tries -gt $maxtries ]
    then
        failure_msg ERROR on SSH to $hostname
    fi
    sleep 1
  done
}

############################################################

start_msg

maxattempts=4
nattempts=0

while [ 1 ]
do

    let nattempts=$nattempts+1

    echo -e "\n!!!!   RICH RECOVERY   !!!!\n\nTurning RICH LV OFF ...\n" | $log

    lvcycle B_DET_RICH_ALL_LV:OFF B_DET_RICH_ALL_LV:isOff

    echo -e "\nRICH LV OFF succecsfull.\n\nTurning RICH LV ON ...\n" | $log

    lvcycle B_DET_RICH_ALL_LV:ON B_DET_RICH_ALL_LV:isOn

    echo -e "\nRICH LV ON succesfull.\n" | $log

    let odd=$nattempts%2

    if [ $odd -eq 0 ]
    then
        # need to change the BIOS for this to do what Ben wants:
        #ssh rich4 'vme_sysreset && tiinit && rich_init' | $log
        # meanwhile, stick to this:
        echo -e "\nRebooting rich4 ...\n" | $log
        roc_reboot rich4 | $log
        checkssh rich4 60 70
        sleep 10
    fi

    echo -e "\nrunning rich_init ..." | $log
    ssh rich4 rich_init | $log
    ntiles=`tail -100 $logfile | grep 'Total Tiles' | awk '{print$4}'`
    echo $ntiles
    
    if [ $ntiles -eq 276 ]
    then
        success_msg
        break
    elif [ $nattempts -gt $maxattempts ]
    then
        failure_msg TERMINAL FAILURE, $maxattempts TIMES, CANNOT SUCCEED
        break
    else
        echo -e "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" | $log
        echo -e "FAILURE on attempt #$nattempts !!!!!!!!!!!!!!!!!!!!!!!!!!!!" | $log
        echo -e "RETRYING ...           !!!!!!!!!!!!!!!!!!!!!!!!!!!!" | $log
        date_msg
        echo -e "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" | $log
    fi
done


