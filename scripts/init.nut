function foo(i, f, s) 
{ 
	print("Called foo(), i="+i+", f="+f+", s='"+s+"'\n"); 
	print(f + " + " + f + " = " + (f + f) + "\n"); 
}

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

function printfact(n){
	print(fact(n));
}

function return100(){
	return 100;
}
