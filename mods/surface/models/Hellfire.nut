
modelScale <- 0.01;

mass <- 6.;

register_console_command("hellfire", function(){
	print("hellfire-proc.nut being loaded");
	dofile("mods/surface/models/Hellfire-proc.nut");
});

cmd("hellfire");
