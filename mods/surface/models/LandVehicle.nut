local THIS_FILE = "mods/surface/models/LandVehicle.nut";

// Include guard
if(THIS_FILE in this)
	return;
this[THIS_FILE] <- true;


if(isClient()){
	function registerClass(classname){
		beginControl[classname] <- function (){
			if("print" in this)
				print(classname + "::beginControl");
			cmd("pushbind");
			cmd("r_windows 0");
		}

		endControl[classname] <- function (){
			if("print" in this)
				print(classname + "endControl");
			cmd("popbind");
			cmd("r_windows 1");
		}
	}
}
