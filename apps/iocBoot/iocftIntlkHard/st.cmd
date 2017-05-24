#!../../bin/linux-x86_64/ftIntlk

## You may have to change ftIntlk to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/ftIntlk.dbd"
ftIntlk_registerRecordDeviceDriver pdbbase

dbLoadRecords("db/save_restoreStatus.db", "P=${IOC}:")
dbLoadRecords("db/iocAdminSoft.db","IOC=${IOC}")
dbLoadRecords("db/ftIntlkHard.db")

cd ${TOP}/iocBoot/${IOC}

< save_restore.cmd

iocInit();

makeAutosaveFiles()
create_monitor_set("info_positions.req", 5, "P=${IOC}:")
create_monitor_set("info_settings.req", 30, "P=${IOC}:")

dbl > pv.list

