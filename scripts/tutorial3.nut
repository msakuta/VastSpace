
squirrelShare.tutorial = "true";

local e = player.cs.addent("Shipyard", Vec3d(-0.0, 0, 0));
bluebase <- e;
e.setrot(Quatd.rotation(-PI/2., Vec3d(0,1,0)));
e.race = 0;

cmd("pause 0");
//cmd("mover tactical");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
//player.setpos(lookrot.trans(Vec3d(0., 0., 0.25)));
player.viewdist = 2.0;

function sequence(){
	local lastw = GLWmessage(lang == eng ? "Welcome to tutorial 3." : "チュートリアル3へようこそ。", 5.);

	while(lastw.alive)
		yield true;

	lastw = GLWmessage(lang == eng ? "Here you can learn how to build units." : "ここではユニットを建造する方法を学びます。", 5.);

	while(lastw.alive)
		yield true;

	lastw = GLWmessage(lang == eng ? "Select the Shipyard." : "Shipyardを選択してください。", 5.);

	while(lastw.alive)
		yield true;

	local function selectShipyardCond(){
		foreach(e in player.selected){
			if(e.race == player.race && e.classname == "Shipyard")
				return true;
		}
		return false;
	}

	// Wait until the Shipyard is selected
	while(!selectShipyardCond())
		yield true;
	if(lastw.alive)
		lastw.close();

	lastw = GLWmessage(lang == eng ? "Press the Build Manager button." : "ビルドマネージャボタンを押してください。", 5.);
	buildManButton.flashTime = 5.;

	// Wait until the player enters attack order mode
	while(!player.attackorder)
		yield true;
	if(lastw.alive)
		lastw.close();

	return false;
}

messageCoroutine <- sequence();

function tutor_restart(){
	messageCoroutine = sequence();
};

frameProcs.append(function(dt){
	if(messageCoroutine != null && messageCoroutine.getstatus() == "suspended"){
		if(!resume messageCoroutine)
			messageCoroutine = null;
	}
})

