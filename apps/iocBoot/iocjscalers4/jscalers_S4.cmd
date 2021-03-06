#!../../bin/linux-x86_64/iocscalers
< envPaths
cd ${TOP}

Init_SCALERS()

Start_SCALERS_CRATE("0","ADCECAL4")
Start_SCALERS_CRATE("1","ADCPCAL4")
Start_SCALERS_CRATE("2","ADCFTOF4")
Start_SCALERS_CRATE("3","TDCECAL4")
Start_SCALERS_CRATE("4","TDCPCAL4")
Start_SCALERS_CRATE("5","TDCFTOF4")


## Register all support components
dbLoadDatabase("dbd/iocscalers.dbd")
iocscalers_registerRecordDeviceDriver(pdbbase)

callbackSetQueueSize(5000)
scanOnceSetQueueSize(5000)

## Load record instances
dbLoadRecords("db/iocAdminSoft.db", "IOC=${IOC}")

dbLoadTemplate("db/jscalers_S4_ECAL_FADC.substitutions")
dbLoadTemplate("db/jscalers_S4_PCAL_FADC.substitutions")
dbLoadTemplate("db/jscalers_S4_FTOF_FADC.substitutions")
dbLoadTemplate("db/jscalers_S4_LTCC_FADC.substitutions")
dbLoadTemplate("db/jscalers_S4_ECAL_DISC.substitutions")
dbLoadTemplate("db/jscalers_S4_PCAL_DISC.substitutions")
dbLoadTemplate("db/jscalers_S4_FTOF_DISC.substitutions")
dbLoadTemplate("db/jscalers_S4_LTCC_DISC.substitutions")

dbLoadRecords("db/jscalers_ECAL_sums.db","TYPE=FADC,SEC=4,CH=1")
dbLoadRecords("db/jscalers_ECAL_sums.db","TYPE=DISC,SEC=4,CH=3")
dbLoadRecords("db/jscalers_PCAL_sums.db","TYPE=FADC,SEC=4,CH=1")
dbLoadRecords("db/jscalers_PCAL_sums.db","TYPE=DISC,SEC=4,CH=3")
dbLoadRecords("db/jscalers_FTOF_sums.db","TYPE=FADC,SEC=4,CH=1")
dbLoadRecords("db/jscalers_FTOF_sums.db","TYPE=DISC,SEC=4,CH=3")
dbLoadRecords("db/jscalers_LTCC_sums.db","TYPE=FADC,SEC=4,CH=1")
dbLoadRecords("db/jscalers_LTCC_sums.db","TYPE=DISC,SEC=4,CH=3")

dbLoadTemplate("db/jscalers_TDCFTOF4_RF.substitutions")
dbLoadTemplate("db/jscalers_TDCPCAL4_TRIG.substitutions")
dbLoadTemplate("db/jscalers_TDCFTOF4_TRIG.substitutions")

cd ${TOP}/iocBoot/${IOC}

iocInit

#seq seqJscalersFC, "S=4"

dbl > pv.list

