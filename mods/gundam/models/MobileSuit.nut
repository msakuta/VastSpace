// MobileSuit.nut
// Common definitions for Mobile Suits.


if(isClient()){
	::reloadCommand <- function(){
		player.controlled.command("Reload");
	}

	::getCoverCommand <- function(){
		player.controlled.command("GetCover", 2);
	}

	::transformCommand <- function(){
		player.controlled.waverider = !player.controlled.waverider;
	}

	::stabilizerCommand <- function(){
		player.controlled.stabilizer = !player.controlled.stabilizer;
	}

	::setWeaponCommand <- function(w){
		player.controlled.weapon = w;
	}

	function registerControls(classname){
		beginControl[classname] <- function (){
			if("print" in this)
				print("ZetaGundam::beginControl");
			cmd("pushbind");
			cmd("bind r sq \"reloadCommand()\"");
			cmd("bind t sq \"getCoverCommand()\"");
			cmd("bind b sq \"transformCommand()\"");
			cmd("bind g sq \"stabilizerCommand()\"");
			cmd("bind 1 sq \"setWeaponCommand(0)\"");
			cmd("bind 2 sq \"setWeaponCommand(1)\"");
			cmd("bind 3 sq \"setWeaponCommand(2)\"");
			cmd("bind 4 sq \"setWeaponCommand(3)\"");
			cmd("r_windows 0");
		}

		endControl[classname] <- function (){
			if("print" in this)
				print("ZetaGundam::endControl");
			cmd("popbind");
			cmd("r_windows 1");
		}
	}
}
