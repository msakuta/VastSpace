/// \file Warpable.nut

function popupMenu(e){
	return [
		{separator = true},
		{title = translate("Inventory"), cmd = "inventory"},
		{separator = true},
		{title = translate("Warp to..."), cmd = "warpmenu"},
	];
}

