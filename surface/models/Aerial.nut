// Aerial.nut
// Common definitions for Aerial vehicles.

// Prevent multiple definition when this file is evaluated more than once.
if(!("fireLauncher" in root)){
	// Define the method in the root table, because initialization scripts
	// use temporary tables as "this" which vaporize after evaluation.
	/// \param e The base Entity that owns the launcher.
	/// \param dt The delta-time of this frame.
	/// \param launcherType A string designating the class of launchers of interest.
	/// \param proc A procedure hook evaluated after each launch.
	::fireLauncher <- function(e,dt,launcherType, proc = @(e,arm,fired) null){
		while(e.cooldown < dt){
			local arms = e.arms;
			local launchers = [];
			local max_launcher = null;
			local max_ammo = 0;

			// Ammunition count is saved in each launchers, so we must scan them
			// to find which launcher is capable of firing right now.
			foreach(it in arms){
				if(it.classname == launcherType && 0 < it.ammo){
					launchers.append(it);
					if(it.cooldown == 0. && max_ammo < it.ammo){
						max_ammo = it.ammo;
						max_launcher = it;
					}
				}
			}

			// If there's no rockets left in the launchers, exit
			if(max_launcher == null)
				return;

			local arm = max_launcher;
			local fired = arm.fire(dt);
			if(fired != null && fired.alive){
				e.cooldown += arm.cooldownTime / launchers.len();
				e.lastMissile = fired;
				proc(e, arm, fired);
			}
		}
	}
	// This message should never be printed twice.
	print("fireLauncher defined");
}

// Load aerialLanding function definition in an external file
dofile("surface/models/aerialLanding.nut");

if(isClient()){
	register_console_command("navlight", function(){
		if(player.controlled)
			player.controlled.navlight = !player.controlled.navlight;
	});

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

	register_console_command("afterburner", function(){
		if(player.controlled)
			player.controlled.afterburner = !player.controlled.afterburner;
	});

	function registerControls(classname){
		beginControl[classname] <- function (){
			if("print" in this)
				print("A10::beginControl");
			cmd("pushbind");
			cmd("bind n navlight");
			cmd("bind g gear");
			cmd("bind b brake");
			cmd("bind h spoiler");
			cmd("bind i showILS");
			cmd("bind e afterburner");
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
