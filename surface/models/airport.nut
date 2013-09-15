
modelScale <- 0.01;

hitRadius <- 2.;

maxhealth <- 1.e5;

landOffset <- Vec3d(0,0.2,0);

hitbox <- [
	[Vec3d(0,-1,0), Quatd(0,0,0,1), Vec3d(0.5, 1., 1.)],
];

navlights <- [];

local function localCoord(v){
	v = v / 128. - Vec3d(0.5,0,1);
	v[2] *= -1;
	return v;
}

for(local i = 0; i < 16; i++){
	navlights.append({pos = localCoord(Vec3d(21, 0.1, i * 12 + 32)), radius = 0.002, pattern = "Constant"});
	navlights.append({pos = localCoord(Vec3d(27, 0.1, i * 12 + 32)), radius = 0.002, pattern = "Constant"});
}

function drawOverlay(){
	glBegin(GL_LINE_STRIP);
	for(local i = 0; i < 6; i++){
		glVertex2d(0.8 * cos(i * 2 * PI / 6), 0.8 * sin(i * 2 * PI / 6));
	}
	glEnd();
}
