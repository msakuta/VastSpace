local SQRT2P2 = sqrt(2)/2;

modelScale <- 0.1;
modelFile <- "models/rstation.mqo";

hitbox <- [
	[Vec3d(0, 0, -22) * modelScale, Quatd(0,0,0,1), Vec3d(9, 0.29, 10) * modelScale],
	[Vec3d(-22, 0, 0) * modelScale, Quatd(0,SQRT2P2,0,SQRT2P2), Vec3d(9, 0.29, 10) * modelScale],
	[Vec3d(0, 0,  22) * modelScale, Quatd(1,0,0,0), Vec3d(9, 0.29, 10) * modelScale],
	[Vec3d(22, 0, 0) * modelScale, Quatd(0,-SQRT2P2,0,SQRT2P2), Vec3d(9, 0.29, 10) * modelScale],
	[Vec3d(0, 0, 0) * modelScale, Quatd(0,0,0,1), Vec3d(11, 0.29, 11) * modelScale],
	[Vec3d(0, -5, 0) * modelScale, Quatd(0,0,0,1), Vec3d(5, 5, 5) * modelScale],
	[Vec3d(0, 10, 0) * modelScale, Quatd(0,0,0,1), Vec3d(0.5, 10, 0.5) * modelScale],
];



hitRadius <- 4.;

mass <- 1.e7;
maxhealth <- 1500000.;
maxRU <- 10000.;
defaultRU <- 1000.;
maxOccupyTime <- 10.;

navlights <- [
	{pos = Vec3d(0, 20.5, 0) * modelScale, radius = 0.05, pattern = "Triangle"},
	{pos = Vec3d(4.71, 0, -33.7) * modelScale, radius = 0.05, pattern = "Triangle"},
	{pos = Vec3d(-4.71, 0, -33.7) * modelScale, radius = 0.05, pattern = "Triangle"},
	{pos = Vec3d(4.71, 0, 33.7) * modelScale, radius = 0.05, pattern = "Triangle"},
	{pos = Vec3d(-4.71, 0, 33.7) * modelScale, radius = 0.05, pattern = "Triangle"},
	{pos = Vec3d(33.7, 0, -4.71) * modelScale, radius = 0.05, pattern = "Triangle"},
	{pos = Vec3d(-33.7, 0, -4.71) * modelScale, radius = 0.05, pattern = "Triangle"},
	{pos = Vec3d(33.7, 0, 4.71) * modelScale, radius = 0.05, pattern = "Triangle"},
	{pos = Vec3d(-33.7, 0, 4.71) * modelScale, radius = 0.05, pattern = "Triangle"},
]

function drawOverlay(){
	local points = [
		10.088322,12.088322,
		-2.0956827,4.596172,
		0,4.822375,
		1.9200191,4.777724,
		0,-4.733072,
		0.1786064,-4.018646,
		2.184986,-3.256623,
		12.725711,12.725711,
		2,-2,
		-12.725711,-12.725711,
		3.6737,-2.7806688,
		3.393523,-0.9823352,
		4.554465,-0.3572134,
		-4.911678,-1.250245,
		-4.286557,0.3572131,
		-4.566732,2.8699703,
		-1.920019,-1.920019,
		-2,2,
		1.875368,1.875368,
	];
	local scale = 16.;
	local currentx = 0, currenty = 0;
	glBegin(GL_LINE_LOOP);
	for(local i = 0; i < points.len() / 2; i++){
		currentx += points[i * 2];
		currenty += points[i * 2 + 1];
		glVertex2d(currentx / scale - 1., currenty / scale - 1.);
	}
	glEnd();
}
