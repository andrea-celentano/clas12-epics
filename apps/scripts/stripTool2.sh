#!/bin/sh

cfg=""
cfgDir=$EPICS/tools/stripCharts

if [ -d $cfgDir/$1 ]
then
    cd $cfgDir/$1

    exec StripTool >& /dev/null &

else
    cd $cfgDir

    tmpdir=/tmp/StripTool
    if [[ ! -e $tmpdir ]]; then
        mkdir -p $tmpdir -m 777
    fi

    scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    cfgTemplate=$scriptDir/stripToolTemplate.stp

    cfg=`mktemp -p $tmpdir $1.XXXXXXXX`

    # same for all PVs:
    grep -v Strip.Curve $cfgTemplate >> $cfg

    # PV-specific:
    npv=0
    for pv in $@
    do
        grep Strip.Curve.0 $cfgTemplate | \
            sed "s/_____PV_____/$pv/" | \
            sed "s/Strip.Curve.0/Strip.Curve.$npv/" \
            >> $cfg
        let npv=npv+1
    done
    
    exec StripTool $cfg >& /dev/null &
fi


