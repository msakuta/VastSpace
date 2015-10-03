#ifndef INVENTORY_H
#define INVENTORY_H
#include "Inventory-forward.h"

#ifdef _WIN32
#pragma once
#endif

class InventoryItemClass{
public:
	gltestp::dstring typeString;
	bool countable;
	bool stackable;
	double specificWeight;
	GLuint texture;
	InventoryItemClass(bool countable = false, bool stackable = false, double specificWeight = 1.) :
		countable(countable), stackable(stackable), specificWeight(specificWeight) {}
};

typedef std::map<gltestp::dstring, InventoryItemClass> InventoryItemClassMap;

extern InventoryItemClassMap inventoryItemDef;

class InventoryItem{
public:
	gltestp::dstring typeString()const{return type->typeString;}
	virtual double getVolume()const{return amount;}
	virtual double getMass()const{return amount * type->specificWeight;}
	virtual bool stack(const InventoryItem &o){
		if(type->stackable && type == o.type){
			amount += o.amount;
			return true;
		}
		return false;
	}
protected:
	const InventoryItemClass *type; // We won't modify the type descriptor from item, hence const
	double amount;
};

#endif
