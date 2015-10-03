#ifndef INVENTORY_FORWARD_H
#define INVENTORY_FORWARD_H
#include <list>

#ifdef _WIN32
#pragma once
#endif

class InventoryItemClass;
class InventoryItem;

typedef std::list<InventoryItem*> InventoryItemList;

#endif
