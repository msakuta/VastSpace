#ifndef INVENTORY_FORWARD_H
#define INVENTORY_FORWARD_H
#include <vector>

#ifdef _WIN32
#pragma once
#endif

class InventoryItemClass;
class InventoryItem;

typedef std::vector<InventoryItem*> InventoryItemList;

#endif
