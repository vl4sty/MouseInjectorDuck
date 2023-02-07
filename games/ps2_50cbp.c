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

#define BP_ONFOOT_CAMY 0x55B520
#define BP_ONFOOT_CAMX 0x55B524
#define BP_AUTOCENTER_BASE 0x427654
#define BP_AUTOCENTER 0x394 // offset

static uint8_t PS2_BP_Status(void);
static void PS2_BP_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"50 Cent: Bulletproof",
	PS2_BP_Status,
	PS2_BP_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_50CENTBULLETPROOF = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_BP_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && PS2_MEM_ReadWord(0x00093394) == 0x5F323133U) &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E31353BU;
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_BP_Inject(void)
{
	// TODO: fov?

	// disable auto-center after landing
	uint32_t autoCenterBase = PS2_MEM_ReadPointer(BP_AUTOCENTER_BASE);
	if (autoCenterBase != 0x0)
		PS2_MEM_WriteUInt(autoCenterBase + BP_AUTOCENTER, 0x0);

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 40.f;

	float camX = PS2_MEM_ReadFloat(BP_ONFOOT_CAMX);
	float camY = PS2_MEM_ReadFloat(BP_ONFOOT_CAMY);

	camX += (float)xmouse * looksensitivity / 200.f;
	camY += (float)ymouse * looksensitivity / 200.f;

	PS2_MEM_WriteFloat(BP_ONFOOT_CAMX, (float)camX);
	PS2_MEM_WriteFloat(BP_ONFOOT_CAMY, (float)camY);

}