// Aerial.nut
// Common definitions for Aerial vehicles.

if(isClient()){
	register_console_command("gear", function(){
		if(player.controlled)
			player.controlled.gear = !player.controlled.gear;
	});

	register_console_command("brake", function(){
		if(player.controlled)
			player.controlled.brake = !player.controlled.brake;
	});

	register_console_command("spoiler", function(){
		if(player.controlled)
			player.controlled.spoiler = !player.controlled.spoiler;
	});

	register_console_command("showILS", function(){
		if(player.controlled)
			player.controlled.showILS = !player.controlled.showILS;
	});

	function registerControls(classname){
		beginControl[classname] <- function (){
			if("print" in this)
				print("A10::beginControl");
			cmd("pushbind");
			cmd("bind g gear");
			cmd("bind b brake");
			cmd("bind h spoiler");
			cmd("bind i showILS");
			cmd("r_windows 0");
		}

		endControl[classname] <- function (){
			if("print" in this)
				print("A10::endControl");
			cmd("popbind");
			cmd("r_windows 1");
		}
	}
}
