
modelScale <- 1.e-4;

mass <- 4.e3;

maxhealth <- 200.; // Show some guts for demonstrating shooting effect in the client.

maxfuel <- 120.; // seconds for full thrust

hitbox <- [
	[Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(0.005, 0.002, 0.003)],
];
 
enginePos <- [
	{pos = Vec3d(0,0,30.) * modelScale, rot = Quatd(0,0,0,1)},
	{pos = Vec3d(34.5,0,40.) * modelScale, rot = Quatd(0,0,0,1)},
	{pos = Vec3d(-34.5,0,40.) * modelScale, rot = Quatd(0,0,0,1)},
];

local ofs = 32.5;
local upangle = Quatd(sin(PI*5./6./2.),0.,0.,cos(PI*5./6./2.));
local dnangle = Quatd(sin(-PI*5./6./2.),0.,0.,cos(-PI*5./6./2.));

enginePosRev <- [
	{pos = Vec3d( 34.5, 0, ofs) * modelScale, rot = upangle},
	{pos = Vec3d( 34.5, 0, ofs) * modelScale, rot = dnangle},
	{pos = Vec3d(-34.5, 0, ofs) * modelScale, rot = upangle},
	{pos = Vec3d(-34.5, 0, ofs) * modelScale, rot = dnangle},
];

function drawOverlay(){
	glBegin(GL_LINE_LOOP);
	glVertex2d(-1.0, -1.0);
	glVertex2d(-0.5,  0.0);
	glVertex2d(-1.0,  1.0);
	glVertex2d( 0.0,  0.5);
	glVertex2d( 1.0,  1.0);
	glVertex2d( 0.5,  0.0);
	glVertex2d( 1.0, -1.0);
	glVertex2d( 0.0, -0.5);
	glEnd();
}
