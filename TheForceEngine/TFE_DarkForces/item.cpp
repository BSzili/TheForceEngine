#include "item.h"
#include <TFE_FileSystem/paths.h>

using namespace TFE_Jedi;

namespace TFE_DarkForces
{
	// TODO: Move into data file for TFE, rather than hardcoding here.
	static const char* c_itemResoure[ITEM_COUNT] =
	{
		"IDPLANS.WAX",  // ITEM_PLANS
		"IPHRIK.WAX",   // ITEM_PHRIK
		"INAVA.WAX",    // ITEM_NAVA
		"IDTGUN.WAX",   // ITEM_DT_WEAPON
		"IDATA.FME",    // ITEM_DATATAPE
		"IST-GUNI.FME", // ITEM_RIFLE
		"IAUTOGUN.FME", // ITEM_AUTOGUN
		"IMORTAR.FME",  // ITEM_MORTAR
		"IFUSION.FME",  // ITEM_FUSION
		"ICONCUS.FME",  // ITEM_CONCUSSION 
		"ICANNON.FME",  // ITEM_CANNON
		"IENERGY.FME",  // ITEM_ENERGY
		"IPOWER.FME",   // ITEM_POWER
		"IPLAZMA.FME",  // ITEM_PLASMA
		"IDET.FME",     // ITEM_DETONATOR
		"IDETS.FME",    // ITEM_DETONATORS 
		"ISHELL.FME",   // ITEM_SHELL
		"ISHELLS.FME",  // ITEM_SHELLS
		"IMINE.FME",    // ITEM_MINE
		"IMINES.FME",   // ITEM_MINES
		"IMSL.FME",     // ITEM_MISSILE
		"IMSLS.FME",    // ITEM_MISSILES
		"IARMOR.WAX",   // ITEM_SHIELD
		"IKEYR.FME",    // ITEM_RED_KEY
		"IKEYY.FME",    // ITEM_YELLOW_KEY 
		"IKEYB.FME",    // ITEM_BLUE_KEY
		"IGOGGLES.FME", // ITEM_GOGGLES
		"ICLEATS.FME",  // ITEM_CLEATS
		"IMASK.FME",    // ITEM_MASK
		"IBATTERY.FME", // ITEM_BATTERY
		"DET_CODE.FME", // ITEM_CODE1
		"DET_CODE.FME", // ITEM_CODE2
		"DET_CODE.FME", // ITEM_CODE3
		"DET_CODE.FME", // ITEM_CODE4
		"DET_CODE.FME", // ITEM_CODE5
		"DET_CODE.FME", // ITEM_CODE6
		"DET_CODE.FME", // ITEM_CODE7
		"DET_CODE.FME", // ITEM_CODE8
		"DET_CODE.FME", // ITEM_CODE9
		"IINVINC.WAX",  // ITEM_INVINCIBLE 
		"ICHARGE.FME",  // ITEM_SUPERCHARGE
		"IREVIVE.WAX",  // ITEM_REVIVE
		"ILIFE.WAX",    // ITEM_LIFE
		"IMEDKIT.FME",  // ITEM_MEDKIT
		"IPILE.FME",    // ITEM_PILE
	};

	SoundSourceID s_powerupPickupSnd;
	SoundSourceID s_invItemPickupSnd;
	SoundSourceID s_wpnPickupSnd;
	ItemData s_itemData[ITEM_COUNT];
	
	void loadItemData()
	{
		s_powerupPickupSnd = sound_Load("bonus.voc");
		s_invItemPickupSnd = sound_Load("complete.voc");
		s_wpnPickupSnd = sound_Load("key.voc");

		for (s32 i = 0; i < ITEM_COUNT; i++)
		{
			const char* item = c_itemResoure[i];
			if (strstr(item, ".WAX"))
			{
				s_itemData[i].wax = TFE_Sprite_Jedi::getWax(item);
				s_itemData[i].isWax = JTRUE;
			}
			else
			{
				s_itemData[i].frame = TFE_Sprite_Jedi::getFrame(item);
				s_itemData[i].isWax = JFALSE;
			}
		}
	}
}  // TFE_DarkForces