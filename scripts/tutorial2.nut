
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

class MessageSet{
	count = 0;
	messages = null;
	constructor(){
		messages = [];
	}
	function append(m, time = 5.){
		messages.append([m, time]);
		count++;
		return this;
	}
}

langmessages <- {
	eng = [
	MessageSet()
	.append("Welcome to Tutorial 2")
	.append("Here you can learn combat control.")
	.append("Now, select the Interceptors.")
	]
	,
	jpn = [
	MessageSet()
	.append("チュートリアル2へようこそ")
	.append("ここでは戦闘の操作方法を学習することができます。")
	.append("Interceptorを選択してください。")
	]
}

sequenceIndex <- 0;
messageIndex <- 0;
lastmessage <- null;
suppressNext <- false;

function inccm(){
	if(suppressNext){
		suppressNext = false;
		return;
	}
	local sequences = langmessages[lang == eng ? "eng" : "jpn"];
	if(sequenceIndex < sequences.len()){
		local messages = sequences[sequenceIndex];
		if(messageIndex < messages.count){
			local message = messages.messages[messageIndex][0];
			if(message == "")
				return messageIndex++;
			else{
				messageIndex++;
				return lastmessage = GLWmessage(message, messages.messages[messageIndex][1], inccm);
			}
		}
		else{
			messageIndex = 0;
		}
	}
}

local mes = inccm();

function tutor_proceed(){
	sequenceIndex++;
	messageIndex = 0;
	if(lastmessage != null && lastmessage.alive){
		suppressNext = true;
		lastmessage.close();
	}
	inccm();
};

function tutor_restart(){
	sequenceIndex = 0;
	messageIndex = 0;
	if(lastmessage != null && lastmessage.alive){
		suppressNext = true;
		lastmessage.close();
	}
	inccm();
};

function tutor_end(){
	sequenceIndex = 1000;
	messageIndex = 0;
	if(lastmessage != null && lastmessage.alive){
		suppressNext = true;
		lastmessage.close();
	}
	inccm();
};

checktime <- 0;
targetcs <- player.cs;

old_frameproc <- "frameproc" in this ? frameproc : null;

function frameproc(dt){
	local global_time = universe.global_time;

	local currenttime = universe.global_time + 9.;

	if(checktime + 2. < currenttime){
		checktime = currenttime;
		local ourCount = countents(targetcs, 0, "Sceptor");
		if(ourCount == 0)
			deltaFormation("Sceptor", 0, Quatd(0,0,0,1), Vec3d(0, 0.,  0.7), 0.05, 5, targetcs, null);
		if(sequenceIndex == 0 && messageIndex >= 2){
			local targetCount = countents(targetcs, 1, "Worker");
			if(targetCount == 0)
				deltaFormation("Worker", 1, Quatd(0,0,0,1), Vec3d(0, 0.,  0.7), 0.05, 5, targetcs, null);
		}
	}

	if(old_frameproc != null)
		old_frameproc(dt);
}
