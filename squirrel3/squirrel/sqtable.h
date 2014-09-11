/*	see copyright notice in squirrel.h */
#ifndef _SQTABLE_H_
#define _SQTABLE_H_
/*
* The following code is based on Lua 4.0 (Copyright 1994-2002 Tecgraf, PUC-Rio.)
* http://www.lua.org/copyright.html#4
* http://www.lua.org/source/4.0.1/src_ltable.c.html
*/

#include "sqstring.h"


#define hashptr(p)  ((SQHash)(((SQInteger)p) >> 3))

inline SQHash HashObj(const SQObjectPtr &key)
{
	switch(type(key)) {
		case OT_STRING:		return _string(key)->_hash;
		case OT_FLOAT:
		// If a floating number has the same size as a SQInteger, reinterpret the pointer.
		// Otherwise, conversion to integer (cvttsd2si or cvttss2si) would raise
		// a floating point invalid exception when the value could not be expressed by an integer.
#if defined(_WIN32)
			// If with constant expressions would be truncated by optimization.
			if(sizeof(SQHash) == sizeof(SQFloat))
				return *reinterpret_cast<const SQHash*>(&_float(key));
			else if(sizeof(SQHash) == sizeof(float)){
				float f = (float)_float(key);
				return *reinterpret_cast<const SQHash*>(&f);
			}
			else
				return (SQHash)((SQInteger)(float)_float(key));
#else
			return (SQHash)((SQInteger)(float)_float(key));
#endif
		case OT_BOOL: case OT_INTEGER:	return (SQHash)((SQInteger)_integer(key));
		default:			return hashptr(key._unVal.pRefCounted);
	}
}

struct SQTable : public SQDelegable 
{
private:
	struct _HashNode
	{
		_HashNode() { next = NULL; }
		SQObjectPtr val;
		SQObjectPtr key;
		_HashNode *next;
	};
	_HashNode *_firstfree;
	_HashNode *_nodes;
	SQInteger _numofnodes;
	SQInteger _usednodes;
	
///////////////////////////
	void AllocNodes(SQInteger nSize);
	void Rehash(bool force);
	SQTable(SQSharedState *ss, SQInteger nInitialSize);
	void _ClearNodes();
public:
	static SQTable* Create(SQSharedState *ss,SQInteger nInitialSize)
	{
		SQTable *newtable = (SQTable*)SQ_MALLOC(sizeof(SQTable));
		new (newtable) SQTable(ss, nInitialSize);
		newtable->_delegate = NULL;
		return newtable;
	}
	void Finalize();
	SQTable *Clone();
	~SQTable()
	{
		SetDelegate(NULL);
		REMOVE_FROM_CHAIN(&_sharedstate->_gc_chain, this);
		for (SQInteger i = 0; i < _numofnodes; i++) _nodes[i].~_HashNode();
		SQ_FREE(_nodes, _numofnodes * sizeof(_HashNode));
	}
#ifndef NO_GARBAGE_COLLECTOR 
	void Mark(SQCollectable **chain);
	SQObjectType GetType() {return OT_TABLE;}
#endif
	inline _HashNode *_Get(const SQObjectPtr &key,SQHash hash)
	{
		_HashNode *n = &_nodes[hash];
		do{
			if(_rawval(n->key) == _rawval(key) && type(n->key) == type(key)){
				return n;
			}
		}while((n = n->next));
		return NULL;
	}
	bool Get(const SQObjectPtr &key,SQObjectPtr &val);
	void Remove(const SQObjectPtr &key);
	bool Set(const SQObjectPtr &key, const SQObjectPtr &val);
	//returns true if a new slot has been created false if it was already present
	bool NewSlot(const SQObjectPtr &key,const SQObjectPtr &val);
	SQInteger Next(bool getweakrefs,const SQObjectPtr &refpos, SQObjectPtr &outkey, SQObjectPtr &outval);
	
	SQInteger CountUsed(){ return _usednodes;}
	void Clear();
	void Release()
	{
		sq_delete(this, SQTable);
	}
	
};

#endif //_SQTABLE_H_
