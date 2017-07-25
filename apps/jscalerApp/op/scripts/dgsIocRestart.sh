#!/bin/sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &&  pwd )"

# companion script:
DGSRESTARTSCRIPT=$DIR/dgsRestart.sh

USAGE='dgsIocRestart.sh [sector# OR crateName]'

CRATES=( adcecal tdcecal adcpcal tdcpcal adcftof tdcftof )

# reboot ioc for given sector:
function iocReboot
{
    sector=$1
    echo Rebooting jscaler IOC for sector $sector ...
    softioc_console -R iocjscalers$sector
}

# reboot diagguiserver for given crate:
function dgsReboot
{
    crate=$1
    echo Rebooting DiagGuiServer on $crate ...
    ssh $crate $DGSRESTARTSCRIPT
}

# reboot ioc corresponding to given crate:
function iocRebootByCrate
{
    crate=$1
    ii=$((${#crate}-1))
    sector=${crate:$ii:1}
    iocReboot $sector
}

# reboot diagguiserver and ioc for given crate:
function dgsAndIocRebootByCrate
{
    crate=$1
    dgsReboot $crate
    sleep 2
    iocRebootByCrate $crate
}

# reboot all diagguiservers and ioc for given sector:
function sectorReboot
{
    sector=$1
    for crate in ${CRATES[@]}
    do
        dgsReboot $crate$sector
    done
    sleep 2
    iocReboot $sector
}

##################################################

if [ -z $1 ] || ! [ -z $2 ]
then
    echo $USAGE
    exit
fi

if [[ $1 == "adcft1" || $1 == "adcft2" ]]
then
    dgsReboot $1
    sleep 2
    softioc_console -R iocjscalersC

elif [[ $1 == "ft" ]]
then
    dgsReboot adcft1
    dgsReboot adcft2
    sleep 2
    softioc_console -R iocjscalersC

elif [[ $1 == "ctof" || $1 == "adcctof1" ]]
then
    dgsReboot adcctof1
    sleep 2
    softioc_console -R iocjscalersC

elif [[ $1 == 1 || $1 == 2 || $1 == 3 || $1 == 4 || $1 == 5 || $1 == 6 ]]
then
    sectorReboot $1
else
    crate=$1
    ii=$((${#crate}-1))
    sec=${crate:$ii:1}
    cratestub=${crate:0:$ii}
    found=0
    for crate in ${CRATES[@]}
    do
        if [ "$crate" = "$cratestub" ]
        then
            found=1
            break
        fi
    done
    if [ $found != 1 ]
    then
        echo Invalid Crate:  $1
        echo $USAGE
        exit
    fi
    if [[ $sec != 1 && $sec != 2 && $sec != 3 && $sec != 4 && $sec != 5 && $sec != 6 ]]
    then
        echo Invalid Sector:  $1
        echo $USAGE
        exit
    fi

    dgsAndIocRebootByCrate $crate$sec
fi

