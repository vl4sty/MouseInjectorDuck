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

#define ER_CAMY 0x1FF350
#define ER_CAMX 0x1FF354
#define ER_IS_BUSY 0x1FFAA4
#define ER_IS_PAUSED 0x202B74

static uint8_t PS2_ER_Status(void);
static void PS2_ER_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Eternal Ring",
	PS2_ER_Status,
	PS2_ER_Inject,
	1,
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_ETERNALRING = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_ER_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323030U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E31353BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_ER_Inject(void)
{
	// opening doors, cutscenes, item pickup, reading message, conversation
	if (PS2_MEM_ReadUInt16(ER_IS_BUSY))
		return;

	if (PS2_MEM_ReadUInt16(ER_IS_PAUSED))
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 500.f;

	float camX = PS2_MEM_ReadFloat(ER_CAMX);
	float camY = PS2_MEM_ReadFloat(ER_CAMY);

	camX += (float)xmouse * looksensitivity / scale;
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;

	PS2_MEM_WriteFloat(ER_CAMX, camX);
	PS2_MEM_WriteFloat(ER_CAMY, camY);

}