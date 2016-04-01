
/// Mixin is a way to simulate multiple inheritance in a single inheritance language.
function mixin(target,src){
	foreach(k,v in src){
		target[k] <- v;
	}
	return target;
}

class A{
	function good(){
		return "Good";
	}
}

class B extends A{
	function good(){
		return "Guten";
	}
}

class C{
	function morning(){
		return "Morning";
	}
}

class D{
	function morning(){
		return "Morgen";
	}
}

class M extends D{
	function goodMorning(){
		return good() + " " + morning() + "!";
	}
}

mixin(M, B);

local m = M();

print(m.goodMorning());
