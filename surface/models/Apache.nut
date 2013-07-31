
modelScale <- 0.00001;

mass <- 5000.;

maxhealth <- 100.;

rotorAxisSpeed <- 0.1 * PI;

mainRotorLiftFactor <- 1.1;
tailRotorLiftFactor <- 0.1;

featherSpeed <- 1.0;

cockpitOfs <- Vec3d(0., 0.0008, -0.0022);

hitbox <- [
	[Vec3d(0, 0, 0), Quatd(0,0,0,1), Vec3d(140, 230, 700) * modelScale],
];

if(isClient()){

	beginControl["Apache"] <- function (){
		if("print" in this)
			print("Apache::beginControl");
		cmd("r_windows 0");
	}

	endControl["Apache"] <- function (){
		if("print" in this)
			print("Apache::endControl");
		cmd("r_windows 1");
	}
}
