#!../../bin/linux-x86_64/agilent

epicsEnvSet("GPIB","hallb-gpib15")
epicsEnvSet("PORT","S3R2")
epicsEnvSet("SECREG","SEC3_R2")
epicsEnvSet("ADDR","1")
epicsEnvSet("SCAN","2 second")

< st-generic.cmd

