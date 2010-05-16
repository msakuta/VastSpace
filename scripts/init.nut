
class Cvar{
	function _set(idx,val){
		::set_cvar(idx,val);
	}
	function _get(idx){
		return ::get_cvar(idx);
	}
}

print("Squirrel script initialized!"); 

cvar <- Cvar();
cvar.pause = "1";

function fact(n){
	if(1 < n)
		return n * fact(n-1);
	else
		return 1;
}

function ae(){
	function delta(team, rot){
		for(local i = 0; i < 11; i++){
/*			local e = player.getcs().addent("Assault", Vec3d((i - 5) * 0.2, 0.,
				(team * 2 - 1) * (2 -(i <= 5 ? i * 0.1 : 1.0 - i * 0.1))));
			e.race = team;
			e.rot = rot;*/
			local e = player.getcs().addent("Sceptor", Vec3d((i - 5) * 0.2, 1.,
				(team * 2 - 1) * (2 -(i <= 5 ? i * 0.1 : 1.0 - i * 0.1))));
			e.race = team;
			e.rot = rot;
			print(e.classname() + ": " + e.race + ", " + e.pos);
		}
	}

	delta(0, Quatd(0,1,0,0));
	delta(1, Quatd(0,0,0,1));
}

ae();

function printtree(cs){
	local child;
	for(child = cs.child(); child != null; child = child.next()){
		print(child.getpath());
		printtree(child);
	}
}


