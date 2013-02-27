
squirrelShare.tutorial = "true";

//initUI();
/*
local tutorialbut = GLWbuttonMatrix(3, 1);
tutorialbut.title = tlate("Tutorial");
tutorialbut.addButton("tutor_restart", "textures/tutor_restart.png", tlate("Restart"));
tutorialbut.addButton("tutor_proceed", "textures/tutor_proceed.png", tlate("Proceed"));
tutorialbut.addButton("tutor_end", "textures/tutor_end.png", tlate("End"));
tutorialbut.x = screenwidth() - tutorialbut.width;
tutorialbut.y = sysbut.y - tutorialbut.height;
tutorialbut.pinned = true;
*/
//deltaFormation("Defender", 0, Quatd(0,1,0,0), Vec3d(0, 0., -0.025), 0.05, 5, player.cs, null);
deltaFormation("Sceptor", 0, Quatd(0,1,0,0), Vec3d(0, 0., -0.025), 0.05, 5, player.cs, null);
deltaFormation("Sceptor", 1, Quatd(0,0,0,1), Vec3d(0, 0.,  0.7), 0.05, 5, player.cs, null);

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

foreachents(player.cs, function(e){
	if(e.race == 0)
		player.chase = e;
});

class MessageSet{
	count = 0;
	messages = null;
	constructor(){
		messages = [];
	}
	function append(m){
		messages.append(m);
		count++;
		return this;
	}
}

langmessages <- {
	eng = [
	MessageSet()
	.append("Welcome to Tutorial 1")
	.append("Here you can learn how to control camera.")
	.append("You can right-drag on empty space to rotate the camera.")
	.append("Press proceed button when you are ready.")
	,
	MessageSet()
	.append("You can use mouse wheels or PageUp/PageDown keys to zoom in/out camera.")
	.append("Press proceed button when you are ready.")
	,
	MessageSet()
	.append("You can select units by left-clicking on it or dragging around units.")
	.append("A bar indicating status of vehicle damages is shown over selected units.")
	.append("You can see selected units at Entity list window.")
	.append("Press proceed button when you are ready.")
	,
	MessageSet()
	.append("You can issue move order by left clicking on Move Order button and then clicking on destination.")
	.append("Holding on Shift key while specifying move order destination lets you control height.")
	]
	,
	jpn = [
	MessageSet()
	.append("チュートリアル1へようこそ")
	.append("ここではカメラの操作方法を練習することができます。")
	.append("何もない空間を右ボタンでドラッグすると、カメラを回転することができます。")
	.append("用意ができたら「次へ」ボタンを押してください。")
	,
	MessageSet()
	.append("マウスホイールかPageUp/PageDownキーでカメラをズームすることができます。")
	.append("用意ができたら「次へ」ボタンを押してください。")
	,
	MessageSet()
	.append("ユニットを選択するには、ユニットを左クリックするか、周囲をドラッグします。")
	.append("選択されたユニットの上部には、機体のダメージ状況を示すバーが表示されます。")
	.append("実体リストウィンドウで選択されたユニットを確認することもできます。")
	.append("用意ができたら「次へ」ボタンを押してください。")
	,
	MessageSet()
	.append("移動命令ボタンを左クリックしてから目的地をクリックすることで、移動命令を実行することができます。")
	.append("移動命令中はShiftキーを押すことで高さ方向の指定ができます。")
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
			local message = messages.messages[messageIndex];
			if(message == "")
				return messageIndex++;
			else{
				messageIndex++;
				return lastmessage = GLWmessage(message, 5., inccm);
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


/*
function frameproc(dt){
	framecount++;

	if(showdt)
		print("DT = " + dt + ", FPS = " + (1. / dt) + ", FC = " + framecount);
}
*/
