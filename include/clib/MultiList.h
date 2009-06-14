#ifndef MULTILIST_H
#define MULTILIST_H

// class template that describes list of arbitrary type
// which can be dynamically inserted or deleted without cost of reallocation.
template<class T, unsigned size = 128>
class MultiList{
	struct Node{
		Node *next;
		char t[sizeof(T)];
	} pool[size], *l1, *l2;
public:
	MultiList(){
		unsigned i;
		for(i = 0; i < size-1; i++){
			pool[i].next = &pool[i+1];
		}
		pool[size-1].next = NULL;
		l2 = pool;
		l1 = NULL;
	}
	~MultiList(){
		Node *p = l1;
		while(p){
			reinterpret_cast<T*>(p->t)->~T();
			p = p->next;
		}
	}
	T *get(const T &t){
		Node *p = l1;
		while(p){
			if(*reinterpret_cast<T*>(p->t) == t){
				return reinterpret_cast<T*>(p->t);
			}
			p = p->next;
		}
		return NULL;
	}
	T *get(bool criteria(T&, void*), void *privdata = NULL){
		Node *p = l1;
		while(p){
			if(criteria(*reinterpret_cast<T*>(p->t), privdata)){
				return reinterpret_cast<T*>(p->t);
			}
			p = p->next;
		}
		return NULL;
	}
	class iterator{
		Node *cur;
	public:
		iterator(Node *cur) : cur(cur){}
		T *operator->(){ return reinterpret_cast<T*>(cur->t); }
		T &operator*(){ return *reinterpret_cast<T*>(cur->t); }
		operator bool()const{return cur != NULL;}
		bool operator++(){ if(!cur) return false; return NULL != (cur = cur->next); }
	};
	iterator active(){
		return iterator(l1);
	}
	bool add(const T &t){
		if(!l2) return false;
		Node *l = l2;
		l2 = l->next;
		l->next = l1;
		l1 = l;
		*reinterpret_cast<T*>(l->t) = t;
		return true;
	}
	void del(Node **pp){
		Node *p = *pp;
		*pp = p->next;
		p->next = l2;
		l2 = p;
		(*reinterpret_cast<T*>(p->t)).~T();
	}
	bool del(const T &t){
		Node **pp = &l1;
		while(*pp){
			if(*reinterpret_cast<T*>((*pp)->t) == t){
				del(pp);
				return true;
			}
			pp = &(*pp)->next;
		}
		return false;
	}
	bool del(bool criteria(T&, void*), void *privdata = NULL){
		Node **pp = &l1;
		while(*pp){
			if(criteria(*reinterpret_cast<T*>((*pp)->t), privdata)){
				del(pp);
				return true;
			}
			pp = &(*pp)->next;
		}
		return false;
	}
};

#endif
