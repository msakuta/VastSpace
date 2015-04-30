
modelScale <- 2.e-2;

mass <- 2.e3;

maxhealth <- 100.; // We are not designed for combat

maxfuel <- 120.; // seconds for full thrust

hitbox <- [
	[Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(3., 3., 4.)],
];

enginePos <- [];

for(local ix = -1; ix <= 1; ix += 2) for(local iy = -1; iy <= 1; iy += 2)
	enginePos.append({pos = Vec3d(ix * 73.28, iy * 127.59, 63.10) * modelScale, rot = Quatd(0,0,0,1)});

function drawOverlay(){
	local xp = 0.5, yp = -1.5;
	local rmove = @(x,y) (xp += x, yp += y, glVertex2d(xp, yp));
	glPushMatrix();
	glScaled(1. / 4., 1. / 4., 1. / 4.);
	glBegin(GL_LINE_LOOP);
	rmove(0,-1);
	rmove(1,-1);
	rmove(1,0);
	rmove(-1,1);
	rmove(0,1);
	rmove(1,0);
	rmove(1,-1);
	rmove(0,1);
	rmove(-1,1);
	rmove(-1,0);
	rmove(-2,2);
	rmove(0,1);
	rmove(-1,1);
	rmove(-1,0);
	rmove(1,-1);
	rmove(0,-1);
	rmove(-1,0);
	rmove(-1,1);
	rmove(0,-1);
	rmove(1,-1);
	rmove(1,0);
	rmove(2,-2);
	glEnd();
	glPopMatrix();
}
