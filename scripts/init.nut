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

local cvar = Cvar();
cvar.pause = "0";
//set("pause","0");

cmd("echo echo from Squirrel");

local timescale = cvar.g_timescale;

cmd("echo timescale = " + timescale);

