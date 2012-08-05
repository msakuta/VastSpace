local scale = 0.0010;

modelScale <- scale;

hitbox <- [
	[Vec3d(0,0,0), Quatd(0,0,0,1), Vec3d(0.3, 0.2, 0.500)],

	// Exterior scaffolds
	[Vec3d(-400,170,400) * scale, Quatd(0,0,0,1), Vec3d(200, 10, 10) * scale],
	[Vec3d(-400,170,200) * scale, Quatd(0,0,0,1), Vec3d(200, 10, 10) * scale],
	[Vec3d(-400,170,000) * scale, Quatd(0,0,0,1), Vec3d(200, 10, 10) * scale],
	[Vec3d(-400,170,-200) * scale, Quatd(0,0,0,1), Vec3d(200, 10, 10) * scale],
	[Vec3d(-400,170,-400) * scale, Quatd(0,0,0,1), Vec3d(200, 10, 10) * scale],

	[Vec3d(-400,-170,400) * scale, Quatd(0,0,0,1), Vec3d(200, 10, 10) * scale],
	[Vec3d(-400,-170,200) * scale, Quatd(0,0,0,1), Vec3d(200, 10, 10) * scale],
	[Vec3d(-400,-170,000) * scale, Quatd(0,0,0,1), Vec3d(200, 10, 10) * scale],
	[Vec3d(-400,-170,-200) * scale, Quatd(0,0,0,1), Vec3d(200, 10, 10) * scale],
	[Vec3d(-400,-170,-400) * scale, Quatd(0,0,0,1), Vec3d(200, 10, 10) * scale],

	[Vec3d(-610,0,400) * scale, Quatd(0,0,0,1), Vec3d(10, 160, 10) * scale],
	[Vec3d(-610,0,200) * scale, Quatd(0,0,0,1), Vec3d(10, 160, 10) * scale],
	[Vec3d(-610,0,000) * scale, Quatd(0,0,0,1), Vec3d(10, 160, 10) * scale],
	[Vec3d(-610,0,-200) * scale, Quatd(0,0,0,1), Vec3d(10, 160, 10) * scale],
	[Vec3d(-610,0,-400) * scale, Quatd(0,0,0,1), Vec3d(10, 160, 10) * scale],

	[Vec3d(-610,170,0) * scale, Quatd(0,0,0,1), Vec3d(10, 10, 410) * scale],
	[Vec3d(-610,-170,0) * scale, Quatd(0,0,0,1), Vec3d(10, 10, 410) * scale],
]

mass <- 5.e9;

navlights <- [
	{pos = Vec3d(0, 520 * scale, 220 * scale), period = 2},
	{pos = Vec3d(0, -520 * scale, 220 * scale), period = 2},
	{pos = Vec3d(140 * scale, 370 * scale, 220 * scale), period = 2},
	{pos = Vec3d(-140 * scale, 370 * scale, 220 * scale), period = 2},
	{pos = Vec3d(140 * scale, -370 * scale, 220 * scale), period = 2},
	{pos = Vec3d(-140 * scale, -370 * scale, 220 * scale), period = 2},
	{pos = Vec3d(-200 * scale, -180 * scale,  500 * scale), period = 2},
	{pos = Vec3d( 200 * scale, -180 * scale,  500 * scale), period = 2},
	{pos = Vec3d(-200 * scale,  180 * scale,  500 * scale), period = 2},
	{pos = Vec3d( 200 * scale,  180 * scale,  500 * scale), period = 2},
	{pos = Vec3d(-200 * scale, -180 * scale, -500 * scale), period = 2},
	{pos = Vec3d( 200 * scale, -180 * scale, -500 * scale), period = 2},
	{pos = Vec3d(-200 * scale,  180 * scale, -500 * scale), period = 2},
	{pos = Vec3d( 200 * scale,  180 * scale, -500 * scale), period = 2},
]

if(false){
	local cuts = 32;
	for(local i = 0; i < cuts; i++)
		navlights.append({
			pos = Vec3d(0.50 * cos(i * 2 * PI / cuts), 0.50 * sin(i * 2 * PI / cuts)),
			color = [i.tofloat() / cuts, 1.0, 0, 1],
			radius = 0.2,
			period = 1,
			phase = i.tofloat() / cuts,
		});
}

function drawOverlay(){
	local cuts = 12;

	glBegin(GL_LINE_LOOP);
	for(local i = 0; i < cuts; i++)
		glVertex2d( 0.00 + 0.20 * cos(i * 2 * PI / 12),  -0.80 + 0.20 * sin(i * 2 * PI / 12));
	glEnd();

	glBegin(GL_LINES);
	glVertex2d( 0.00,  0.75);
	glVertex2d( 0.00, -0.60);
	glVertex2d(-0.30, -0.45);
	glVertex2d( 0.30, -0.45);
	glEnd();

	glBegin(GL_LINE_STRIP);
	glVertex2d(-0.80,  0.20);
	glVertex2d(-0.60,  0.45);
	glVertex2d(-0.40,  0.61);
	glVertex2d(-0.20,  0.75);
	glVertex2d(-0.00,  0.85);
	glVertex2d( 0.20,  0.75);
	glVertex2d( 0.40,  0.61);
	glVertex2d( 0.60,  0.45);
	glVertex2d( 0.80,  0.20);
	glEnd();
}
