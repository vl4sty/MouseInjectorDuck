//===========================================================
// Mouse Injector for Dolphin
//==========================================================================
// Copyright (C) 2019-2020 Carnivorous
// All rights reserved.
//
// Mouse Injector is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, visit http://www.gnu.org/licenses/gpl-2.0.html
//==========================================================================
#include <stdint.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define TAU 6.2831853f // 0x40C90FDB
#define CAMYPLUS 0.6999999988f // 0x3FAB1DD6
#define CAMYMINUS -0.9973310232f // 0xBF7F5116
#define SNIPERCAMYPLUS 1.396263392f
#define SNIPERCAMYMINUS -0.8726648532f
// TM ADDRESSES - OFFSET ADDRESSES BELOW (REQUIRES PLAYERBASE TO USE)
// 6 cameras or angles get cycled through
#define TM_camx_enum_gap 0x8014E3F4 - 0x8014E244 // gap between each camera 1B0
#define TM_camy 0x8100D260 - 0x8100B980
#define TM_sanity 0x8100B982 - 0x8100B980 // value in player object that is always the same
#define TM_health 0x8100B9BC - 0x8100B980
#define TM_weaponInstancePointerOffset 0x8100BF88 - 0x8100B980
#define TM_weaponClassPointerOffset 0x811C3514 - 0x811C3500
// STATIC ADDRESSES BELOW
#define TM_playerbase 0x802178D4 // random stack address, commonly holds player pointer - requires sanity checks before using!
#define TM_sniperY 0x80173480
#define TM_sniperX 0x80173490
#define TM_camx 0x8014E244 // first camx
#define TM_currentCam 0x804763D4 // current enumerated camera

static uint8_t TM_Status(void);
static uint8_t TM_DetectPlayer(void);
static void TM_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Trigger Man",
	TM_Status,
	TM_Inject,
	1, // 1000 Hz tickrate
	1 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_TRIGGERMAN = &GAMEDRIVER_INTERFACE;

static uint32_t playerbase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t TM_Status(void)
{
	return (MEM_ReadUInt(0x80000000) == 0x47473245U && MEM_ReadUInt(0x80000004) == 0x345A0000U); // check game header to see if it matches Trigger Man (GG2E4Z)
}
//==========================================================================
// Purpose: detects player pointer from stack address
// Changed Globals: fovbase, playerbase
//==========================================================================
static uint8_t TM_DetectPlayer(void)
{
	const uint32_t tempplayerbase = MEM_ReadUInt(TM_playerbase);
	if(WITHINMEMRANGE(tempplayerbase) && tempplayerbase != playerbase) // if pointer is valid, sanity check pointer
	{
		const uint32_t tempsanity = MEM_ReadUInt(tempplayerbase + TM_sanity);
		const uint32_t temphealth = MEM_ReadUInt(tempplayerbase + TM_health);
		if(temphealth > 0 && temphealth <= 0x43C80000 && tempsanity == 0x41000080U) // if player base is valid, use player pointer for level
		{
			playerbase = tempplayerbase;
			return 1;
		}
	}
	return WITHINMEMRANGE(playerbase);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void TM_Inject(void)
{
	if(!TM_DetectPlayer()) // if player pointer was not found
		return;
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
    const uint32_t currentCam = MEM_ReadUInt(TM_currentCam);
    const uint32_t camAddressOffset = currentCam * (TM_camx_enum_gap);
	const uint32_t pWeaponInstance = MEM_ReadUInt(playerbase + TM_weaponInstancePointerOffset);
	const uint32_t weaponClass = MEM_ReadUInt(pWeaponInstance + TM_weaponClassPointerOffset);

	float camx = MEM_ReadFloat(TM_camx + camAddressOffset);
	int bWeaponIsSniper = 0;
	if (weaponClass == 0x80A499C4 || weaponClass == 0x80A49AA0)
		bWeaponIsSniper = 1;

	float camy;
	// snipers use a different camY
	// sniper camY is on a scale from 0 to PI, 0 being straight up and PI being straight down
	if (bWeaponIsSniper == 1) {
		camy = MEM_ReadFloat(TM_sniperY);
		camy = camy - TAU / 4; // offset sniper camy to match scale of normal cam, -PI/2 to PI/2
	}
	else {
		camy = MEM_ReadFloat(playerbase + TM_camy);
	}

	const float fov = 0.8f; // just an arbitrary  fov value
	const float looksensitivity = (float)sensitivity / 40.f;

	camx += (float)xmouse / 10.f * looksensitivity / (360.f / TAU) / (1.2f / fov); // normal calculation method for X
	while(camx >= TAU)
		camx -= TAU;
	if (bWeaponIsSniper == 1)
		// invert the invert since sniper camy positive and negative are opposite the normal camy
		camy += (float)(invertpitch ? -ymouse : ymouse) / 10.f * looksensitivity / (360.f / TAU) / (1.2f / fov); // normal calculation method for Y
	else
		camy += (float)(invertpitch ? ymouse : -ymouse) / 10.f * looksensitivity / (360.f / TAU) / (1.2f / fov); // normal calculation method for Y

	if (bWeaponIsSniper == 1) {
		camy = ClampFloat(camy, SNIPERCAMYMINUS, SNIPERCAMYPLUS);
		camy = camy + TAU / 4;

		MEM_WriteFloat(TM_sniperX, camx);
		MEM_WriteFloat(TM_sniperY, camy);
	}
	else {
		camy = ClampFloat(camy, CAMYMINUS, CAMYPLUS);
		MEM_WriteFloat(TM_camx + camAddressOffset, camx);
		MEM_WriteFloat(playerbase + TM_camy, camy);
	}
}