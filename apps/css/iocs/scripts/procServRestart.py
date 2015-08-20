from org.csstudio.opibuilder.scriptUtil import PVUtil
from org.csstudio.opibuilder.scriptUtil import ConsoleUtil, FileUtil
import os,sys
import subprocess
from java.lang import Thread, Runnable

# Reboot the IOC specified by the LABEL of the action menu widget
# This script will execute the script in EPICS $APP directory for 
# rebooting IOC using the macro ni the widget parent to specify the name of the IOC. 
# Therefore, the macro for the parent of the action button widget should match the name 
# of the IOC in the reboting script.  
class MyTask_procServAction(Runnable):
	def run(self):

            appDirectory = os.getenv( "APP" )
            cmd = appDirectory + "/scripts/IOC/procServMgr.py"

            arguments = " -i " + widget.getParent().getMacroValue( "ioc" ) + " " + widget.getParent().getMacroValue("action")
	    #print "yyy " + sys.argv[0] + " xxx " + cmd + arguments
	    #ConsoleUtil.writeInfo( sys.argv[0] + "Will execute: " + cmd + arguments)
            process = subprocess.Popen([cmd + arguments, ], stdout=subprocess.PIPE, shell=True)
            (out, err) = process.communicate()

            #widget.setPropertyValue("text", out)

thread = Thread(MyTask_procServAction())
thread.start()
