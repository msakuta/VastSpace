
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
	]
}

yieldEvent <- false;

function sequence(){
	local s = langmessages[lang == eng ? "eng" : "jpn"];

	local m = s[0].messages[0];

	local lastw = GLWmessage(m[0], m[1]);

	while(lastw.alive)
		::suspend(true);
	yieldEvent = false;

	m = s[0].messages[1];
	lastw = GLWmessage(m[0], m[1]);

	while(lastw.alive)
		::suspend(true);
	yieldEvent = false;

	m = s[0].messages[2];
	lastw = GLWmessage(m[0], m[1]);

	// Wait until a Sceptor is selected
	while(!selectSceptorCond())
		::suspend(true);
	if(lastw.alive)
		lastw.close();
	yieldEvent = false;

	m = s[0].messages[3];
	lastw = GLWmessage(m[0], m[1]);

	// Wait until the player enters attack order mode
	while(!player.attackorder)
		::suspend(true);
	if(lastw.alive)
		lastw.close();
	yieldEvent = false;

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
	yieldEvent = false;

	m = s[0].messages[5];
	lastw = GLWmessage(m[0], m[1]);

	while(lastw.alive)
		::suspend(true);

	return false;
}

sequenceIndex <- 0;
messageIndex <- 0;
lastmessage <- null;
suppressNext <- false;

function currentMessageEntry(){
	local sequences = langmessages[lang == eng ? "eng" : "jpn"];
	if(sequenceIndex < sequences.len()){
		local messages = sequences[sequenceIndex];
		if(messageIndex < messages.count)
			return messages.messages[messageIndex];
	}
	return null;
}

function showNextMessage(){
	local messageEntry = currentMessageEntry();
	if(message == null)
		return messageIndex++;
	else{
		messageIndex++;
		return lastmessage = GLWmessage(messageEntry[0], messageEntry[1], incrementMessage);
	}
}

function incrementMessage(){
	if(suppressNext){
		suppressNext = false;
		return;
	}
	local messageEntry = currentMessageEntry();
	if(messageEntry != null && messageEntry[2]())
		return showNextMessage();
}

//local mes = incrementMessage();
mes <- ::gnewthread(sequence); // We cannot use newthread for unknown reason.
mes.call();

function tutor_proceed(){
	sequenceIndex++;
	messageIndex = 0;
	if(lastmessage != null && lastmessage.alive){
		suppressNext = true;
		lastmessage.close();
	}
	incrementMessage();
};

function tutor_restart(){
	mes = ::gnewthread(sequence);
	mes.call();
/*	sequenceIndex = 0;
	messageIndex = 0;
	if(lastmessage != null && lastmessage.alive){
		suppressNext = true;
		lastmessage.close();
	}
	incrementMessage();*/
};

function tutor_end(){
	sequenceIndex = 1000;
	messageIndex = 0;
	if(lastmessage != null && lastmessage.alive){
		suppressNext = true;
		lastmessage.close();
	}
	incrementMessage();
};

checktime <- 0;

old_frameproc <- "frameproc" in this ? frameproc : null;

function frameproc(dt){
	local global_time = universe.global_time;

	local currenttime = universe.global_time + 9.;

	if(checktime + 2. < currenttime){
		checktime = currenttime;
		local ourCount = countents(targetcs, 0, "Sceptor");
		if(ourCount == 0)
			deltaFormation("Sceptor", 0, Quatd(0,0,0,1), Vec3d(0, 0.,  0.7), 0.05, 5, targetcs, null);
/*		if(sequenceIndex == 0 && messageIndex >= 2)*/{
			local targetCount = countents(targetcs, 1, "Worker");
			if(targetCount == 0)
				deltaFormation("Worker", 1, Quatd(0,0,0,1), Vec3d(0, 0.,  0.7), 0.05, 5, targetcs, null);
		}

/*		// Check only after previous message was vanished.
		if(lastmessage == null || !lastmessage.alive){
			local messageEntry = currentMessageEntry();
			if(messageEntry != null && messageEntry[2]()){
				incrementMessage();
			}
		}*/
	}

	if(mes != null && mes.getstatus() == "suspended"){
		if(!mes.wakeup())
//		if(!resume mes)
			mes = null;
	}

	if(old_frameproc != null)
		old_frameproc(dt);
}
