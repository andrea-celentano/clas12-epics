importPackage(Packages.org.csstudio.opibuilder.scriptUtil);

var prefix = "B_DET_FTC_HV";

var onoff  = widget.getMacroValue("ONOFF");

for (var qq=1; qq<=4; qq++)
{
  for (var gg=1; gg<=9; gg++)
  {
  
      pv = prefix + "Q" + qq + "G" + gg + ":pwonoff";
      PVUtil.writePV(pv,onoff);
  }
}
