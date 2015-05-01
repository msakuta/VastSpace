
modelScale <- 10.;

hitRadius <- 2000.;

maxhealth <- 1.e5;

landOffset <- Vec3d(0,0.2,0);

local function localCoord(v){
	v = v / 128. * 1000. - Vec3d(500.,0,1000.);
	v[2] *= -1;
	return v;
}

landingSite <- localCoord(Vec3d(24, 0, 0));

hitbox <- [
	[Vec3d(0,-1000,0), Quatd(0,0,0,1), Vec3d(500., 1000., 1000.)],
];

navlights <- [];

for(local i = 0; i < 16; i++){
	navlights.append({pos = localCoord(Vec3d(21, 0.1, i * 12 + 32)), radius = 2., pattern = "Constant"});
	navlights.append({pos = localCoord(Vec3d(27, 0.1, i * 12 + 32)), radius = 2., pattern = "Constant"});
}

local lmodelScale = modelScale;

function getCoverPoints(e){
	local ret = [
		{type = "RightEdge", pos = e.pos + e.rot.trans(Vec3d(15, 0, -15) * lmodelScale), rot = e.rot * Quatd.rotation(-PI / 2., Vec3d(0,1,0))},
		{type = "LeftEdge" , pos = e.pos + e.rot.trans(Vec3d(15, 0, -15) * lmodelScale), rot = e.rot},
		{type = "RightEdge", pos = e.pos + e.rot.trans(Vec3d(25, 0, -15) * lmodelScale), rot = e.rot},
		{type = "LeftEdge" , pos = e.pos + e.rot.trans(Vec3d(25, 0, -15) * lmodelScale), rot = e.rot * Quatd.rotation(PI / 2., Vec3d(0,1,0))},
		{type = "RightEdge", pos = e.pos + e.rot.trans(Vec3d(15, 0, -25) * lmodelScale), rot = e.rot * Quatd.rotation(-PI, Vec3d(0,1,0))},
		{type = "LeftEdge" , pos = e.pos + e.rot.trans(Vec3d(15, 0, -25) * lmodelScale), rot = e.rot * Quatd.rotation(-PI / 2., Vec3d(0,1,0))},
		{type = "RightEdge", pos = e.pos + e.rot.trans(Vec3d(25, 0, -25) * lmodelScale), rot = e.rot * Quatd.rotation(PI / 2., Vec3d(0,1,0))},
		{type = "LeftEdge" , pos = e.pos + e.rot.trans(Vec3d(25, 0, -25) * lmodelScale), rot = e.rot * Quatd.rotation(PI, Vec3d(0,1,0))},
	];

	// Make copy of cover points of the first warehouse with offset for the second.
	local n = ret.len();
	for(local i = 0; i < n; i++){
		local next = clone ret[i];
		next.pos += e.rot.trans(Vec3d(0, 0, -15) * lmodelScale);
		ret.append(next);
	}

	return ret;
}

function drawOverlay(){
	glBegin(GL_LINE_STRIP);
	for(local i = 0; i < 6; i++){
		glVertex2d(0.8 * cos(i * 2 * PI / 6), 0.8 * sin(i * 2 * PI / 6));
	}
	glEnd();
}
