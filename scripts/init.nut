
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
	for(local i = 0; i < 11; i++){
		local vec;
		player.getcs().addent("Assault", vec = Vec3d(i * 0.1, 0., -(i <= 5 ? i * 0.1 : 1.0 - i * 0.1)));
		print(vec);
	}
}

ae();

function printtree(cs){
	local child;
	for(child = cs.child(); child != null; child = child.next()){
		print(child.getpath());
		printtree(child);
	}
}


