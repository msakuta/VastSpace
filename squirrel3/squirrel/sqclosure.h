/*	see copyright notice in squirrel.h */
#ifndef _SQCLOSURE_H_
#define _SQCLOSURE_H_


#define _CALC_CLOSURE_SIZE(func) (sizeof(SQClosure) + (func->_noutervalues*sizeof(SQObjectPtr)) + (func->_ndefaultparams*sizeof(SQObjectPtr)))

struct SQFunctionProto;
struct SQClass;
struct SQClosure : public CHAINABLE_OBJ
{
private:
	SQClosure(SQSharedState *ss,SQFunctionProto *func){_function = func; __ObjAddRef(_function); _base = NULL; INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this); _env = NULL;}
public:
	static SQClosure *Create(SQSharedState *ss,SQFunctionProto *func){
		SQInteger size = _CALC_CLOSURE_SIZE(func);
		SQClosure *nc=(SQClosure*)SQ_MALLOC(size);
		new (nc) SQClosure(ss,func);
		nc->_outervalues = (SQObjectPtr *)(nc + 1);
		nc->_defaultparams = &nc->_outervalues[func->_noutervalues];
		_CONSTRUCT_VECTOR(SQObjectPtr,func->_noutervalues,nc->_outervalues);
		_CONSTRUCT_VECTOR(SQObjectPtr,func->_ndefaultparams,nc->_defaultparams);
		return nc;
	}
	void Release(){
		SQFunctionProto *f = _function;
		SQInteger size = _CALC_CLOSURE_SIZE(f);
		_DESTRUCT_VECTOR(SQObjectPtr,f->_noutervalues,_outervalues);
		_DESTRUCT_VECTOR(SQObjectPtr,f->_ndefaultparams,_defaultparams);
		__ObjRelease(_function);
		this->~SQClosure();
		sq_vm_free(this,size);
	}
	SQClosure *Clone()
	{
		SQFunctionProto *f = _function;
		SQClosure * ret = SQClosure::Create(_opt_ss(this),f);
		ret->_env = _env;
		if(ret->_env) __ObjAddRef(ret->_env);
		for(SQInteger i = 0; i < f->_noutervalues; i++)
			ret->_outervalues[i] = _outervalues[i];
		for(SQInteger j = 0; j < f->_ndefaultparams; j++)
			ret->_defaultparams[j] = _defaultparams[j];
		return ret;
	}
	~SQClosure();
	
	bool Save(SQVM *v,SQUserPointer up,SQWRITEFUNC write);
	static bool Load(SQVM *v,SQUserPointer up,SQREADFUNC read,SQObjectPtr &ret);
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
	void Finalize(){
		SQFunctionProto *f = _function;
		for(SQInteger i = 0; i < f-> _noutervalues; i++)
			_outervalues[i].Null();
		for(SQInteger j = 0; j < f-> _ndefaultparams; j++)
			_defaultparams[j].Null();
	}
#endif
	SQWeakRef *_env;
	SQClass *_base;
	SQFunctionProto *_function;
	SQObjectPtr *_outervalues;
	SQObjectPtr *_defaultparams;
};

//////////////////////////////////////////////
struct SQOuter : public CHAINABLE_OBJ
{

private:
	SQOuter(SQSharedState *ss, SQObjectPtr *outer){_valptr = outer; _value  = _null_; _next   = NULL; INIT_CHAIN(); ADD_TO_CHAIN(&_ss(this)->_gc_chain,this); }

public:
	static SQOuter *Create(SQSharedState *ss, SQObjectPtr *outer)
	{
		SQOuter *nc  = (SQOuter*)SQ_MALLOC(sizeof(SQOuter));
		new (nc) SQOuter(ss, outer);
		return nc;
	}
	~SQOuter() { REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this); }

	void Release()
	{
		this->~SQOuter();
		sq_vm_free(this,sizeof(SQOuter));
	}

#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
	void Finalize() { _value.Null(); }
#endif

	SQObjectPtr *_valptr;  /* pointer to value on stack, or _value below */
	SQInteger    _idx;     /* idx in stack array, for relocation */
	SQObjectPtr  _value;   /* value of outer after stack frame is closed */
	SQOuter     *_next;    /* pointer to next outer when frame is open   */
};

//////////////////////////////////////////////
struct SQGenerator : public CHAINABLE_OBJ 
{
	enum SQGeneratorState{eRunning,eSuspended,eDead};
private:
	SQGenerator(SQSharedState *ss,SQClosure *closure){_closure=closure;_state=eRunning;_ci._generator=NULL;INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);}
public:
	static SQGenerator *Create(SQSharedState *ss,SQClosure *closure){
		SQGenerator *nc=(SQGenerator*)SQ_MALLOC(sizeof(SQGenerator));
		new (nc) SQGenerator(ss,closure);
		return nc;
	}
	~SQGenerator()
	{
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
	}
    void Kill(){
		_state=eDead;
		_stack.resize(0);
		_closure.Null();}
	void Release(){
		sq_delete(this,SQGenerator);
	}
	bool Yield(SQVM *v,SQInteger target);
	bool Resume(SQVM *v,SQObjectPtr &dest);
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
	void Finalize(){_stack.resize(0);_closure=_null_;}
#endif
	SQObjectPtr _closure;
	SQObjectPtrVec _stack;
	//SQObjectPtrVec _vargsstack;
	SQVM::CallInfo _ci;
	ExceptionsTraps _etraps;
	SQGeneratorState _state;
};

struct SQNativeClosure : public CHAINABLE_OBJ
{
private:
	SQNativeClosure(SQSharedState *ss,SQFUNCTION func){_function=func;INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this); _env = NULL;}
public:
	static SQNativeClosure *Create(SQSharedState *ss,SQFUNCTION func)
	{
		SQNativeClosure *nc=(SQNativeClosure*)SQ_MALLOC(sizeof(SQNativeClosure));
		new (nc) SQNativeClosure(ss,func);
		return nc;
	}
	SQNativeClosure *Clone()
	{
		SQNativeClosure * ret = SQNativeClosure::Create(_opt_ss(this),_function);
		ret->_env = _env;
		if(ret->_env) __ObjAddRef(ret->_env);
		ret->_name = _name;
		ret->_outervalues.copy(_outervalues);
		ret->_typecheck.copy(_typecheck);
		ret->_nparamscheck = _nparamscheck;
		return ret;
	}
	~SQNativeClosure()
	{
		__ObjRelease(_env);
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
	}
	void Release(){
		sq_delete(this,SQNativeClosure);
	}
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
	void Finalize(){_outervalues.resize(0);}
#endif
	SQInteger _nparamscheck;
	SQIntVec _typecheck;
	SQObjectPtrVec _outervalues;
	SQWeakRef *_env;
	SQFUNCTION _function;
	SQObjectPtr _name;
};



#endif //_SQCLOSURE_H_
