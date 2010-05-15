
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

cmd("echo echo from Squirrel");

local timescale = cvar.g_timescale;

cmd("echo timescale = " + timescale);

function fact(n){
	if(1 < n)
		return n * fact(n-1);
	else
		return 1;
}

print(universe.addent);

function ae(){
	for(local i = 0; i < 11; i++){
		player.getcs().addent("Assault", Vec3(i * 0.1, 0., -(i <= 5 ? i * 0.1 : 1.0 - i * 0.1)));
	}
//		player.getcs().addent("Assault", Vec3(i * .1 - .5, 0., i <= 5 ? i * .1 : .5 - i * -.1));
}

ae();

function printtree(cs){
	local child;
	for(child = cs.child(); child != null; child = child.next()){
		print(child.getpath());
		printtree(child);
	}
}

/*{
local v = Vec3();
v.a[0] = 1;
v.a[1] = 2;
print(v.a);
print(v.a.len());
print("[" + v.a[0] + ", " + v.a[1] + ", " + v.a[2] + "]");
//universe.addent("Assault", v);
}*/

