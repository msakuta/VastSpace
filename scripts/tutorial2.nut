
squirrelShare.tutorial = "true";

deltaFormation("Sceptor", 0, Quatd(0,1,0,0), Vec3d(0, 0., -0.025), 0.05, 5, player.cs, @(e) e.command("SetPassive"));

cmd("pause 0");
//cmd("mover tactical");
player.setrot(Quatd(0,0,0,1)); // Reset rotation for freelook
player.setmover("tactical");

local lookrot = Quatd(0., 1., 0., 0.) * Quatd(-sin(PI/8.), 0, 0, cos(PI/8.)) * Quatd(0, sin(PI/8.), 0, cos(PI/8.));
player.setrot(lookrot);
//player.setpos(lookrot.trans(Vec3d(0., 0., 0.25)));
player.viewdist = 0.25;

targetcs <- player.cs;

class MessageSet{
	count = 0;
	messages = null;
	constructor(){
		messages = [];
	}
	/// \param m The message string.
	/// \param time Time for displaying the message in seconds.
	/// \param proceedCond Closure that defines the condition to proceed to next message.
	function append(m, time = 5., proceedCond = @() true){
		messages.append([m, time, proceedCond]);
		count++;
		return this;
	}
}

function selectSceptorCond(){
	local selectCount = 0;
	foreach(e in player.selected){
		if(e.race == player.race)
			selectCount++;
	};
	return 0 < selectCount;
}

langmessages <- {
	eng = [
	MessageSet()
	.append("Welcome to Tutorial 2")
	.append("Here you can learn combat control.")
	.append("Now, select the Interceptors.", 5.0, selectSceptorCond)
	.append("Now, press attack order button.")
	.append("Now, click or drag around an enemy unit.")
	.append("You can order an unit to stop attacking by pressing " + tlate("Halt") + " button.")
	.append("Tutorial 2 is complete.")
	]
	,
	jpn = [
	MessageSet()
	.append("チュートリアル2へようこそ")
	.append("ここでは戦闘の操作方法を学習することができます。")
	.append("Interceptorを選択してください。", 5.0, selectSceptorCond)
	.append("攻撃命令ボタンを押してください。")
	.append("敵ユニットをクリックするか周囲をドラッグしてください。")
	.append("ユニットに攻撃を止めるように命令するには、" + tlate("Halt") + "ボタンをクリックするか周囲をドラッグしてください。")
	.append("チュートリアル2は完了です。")
	]
}

function sequence(){
	local s = langmessages[lang == eng ? "eng" : "jpn"];

	local m = s[0].messages[0];

	local lastw = GLWmessage(m[0], m[1]);

	while(lastw.alive)
		::suspend(true);

	m = s[0].messages[1];
	lastw = GLWmessage(m[0], m[1]);

	while(lastw.alive)
		::suspend(true);

	m = s[0].messages[2];
	lastw = GLWmessage(m[0], m[1]);

	// Wait until a Sceptor is selected
	while(!selectSceptorCond())
		::suspend(true);
	if(lastw.alive)
		lastw.close();

	m = s[0].messages[3];
	lastw = GLWmessage(m[0], m[1]);
	attackOrderButton.flashTime = 5.;

	// Wait until the player enters attack order mode
	while(!player.attackorder)
		::suspend(true);
	if(lastw.alive)
		lastw.close();

	m = s[0].messages[4];
	lastw = GLWmessage(m[0], m[1]);

	local function isAnyoneAttacking(){
		foreach(e in targetcs.entlist)
			if(e.race == player.race && e.enemy != null)
				return true;
		return false;
	}

	// Wait until the player issues attack order
	while(!isAnyoneAttacking())
		::suspend(true);
	if(lastw.alive)
		lastw.close();

	m = s[0].messages[5];
	lastw = GLWmessage(m[0], m[1]);

	while(lastw.alive)
		::suspend(true);

	m = s[0].messages[6];
	lastw = GLWmessage(m[0], m[1]);

	while(lastw.alive)
		::suspend(true);

	return false;
}

sequenceIndex <- 0;
messageIndex <- 0;
lastmessage <- null;
suppressNext <- false;


//mes <- sequence(); // We cannot use newthread for unknown reason.
mes <- ::gnewthread(sequence);
mes.call();

function tutor_proceed(){
};

function tutor_restart(){
	mes = ::gnewthread(sequence);
	mes.call();
};

function tutor_end(){
};

checktime <- 0;

frameProcs.append(function(dt){
	local global_time = universe.global_time;

	local currenttime = universe.global_time + 9.;

	if(checktime + 2. < currenttime){
		checktime = currenttime;

		// Re-generate Sceptors
		local ourCount = countents(targetcs, 0, "Sceptor");
		if(ourCount == 0)
			deltaFormation("Sceptor", 0, Quatd(0,0,0,1), Vec3d(0, 0.,  0.7), 0.05, 5, targetcs, null);

		// Re-generate Workers
		local targetCount = countents(targetcs, 1, "Worker");
		if(targetCount == 0)
			deltaFormation("Worker", 1, Quatd(0,0,0,1), Vec3d(0, 0.,  0.7), 0.05, 5, targetcs, null);
	}

	if(mes != null && mes.getstatus() == "suspended"){
		if(!mes.wakeup())
//		if(!resume mes)
			mes = null;
	}
})

