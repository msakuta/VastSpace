
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

print(typeof Universe);
print(typeof universe);
print("timescale = " + universe.timescale);
print("global_time = " + universe.global_time);

