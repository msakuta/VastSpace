
initUI();

deltaFormation("Sceptor", 0, Quatd(0,1,0,0), Vec3d(0, 0., -0.025), 0.05, 3, player.cs);
deltaFormation("Sceptor", 1, Quatd(0,0,0,1), Vec3d(0, 0.,  0.7), 0.05, 3, player.cs);

framecount <- 0;

cmd("pause 0");
//cmd("mover tactical");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
//player.setpos(lookrot.trans(Vec3d(0., 0., 0.25)));
player.viewdist = 0.25;

foreachents(player.cs, function(e){e.command("Halt");});

langmessages <- {
	eng =
	[
	"Welcome to Tutorial 1",
	"Here you can train how to control camera.",
	"You can right-drag on empty space to rotate the camera.",
	],
	jpn =
	[
	"チュートリアル1へようこそ",
	"ここではカメラの操作方法を練習することができます。",
	"何もない空間を右ボタンでドラッグすると、カメラを回転することができます。",
	]
}

cm <- 0;

function inccm(){
	local messages = langmessages[lang == eng ? "eng" : "jpn"];
	if(cm < messages.len())
		return GLWmessage(messages[cm++], 5., "inccm()");
}

local mes = inccm();

function frameproc(dt){
	framecount++;

	if(showdt)
		print("DT = " + dt + ", FPS = " + (1. / dt) + ", FC = " + framecount);
}

