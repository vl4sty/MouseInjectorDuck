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

#define BHC_CAMY 0x31DC90
#define BHC_CAMX_BASE_PTR 0x31D050
// offset from camX base
#define BHC_CAMX 0xE38

#define BHC_CAN_ACT 0x3BD4B8

static uint8_t PS2_BHC_Status(void);
static void PS2_BHC_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Beverly Hills Cop",
	PS2_BHC_Status,
	PS2_BHC_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_BEVERLYHILLSCOP = &GAMEDRIVER_INTERFACE;

uint32_t camXBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_BHC_Status(void)
{
	// SLES_544.56
	return (PS2_MEM_ReadWord(0x93390) == 0x534C4553U && 
			PS2_MEM_ReadWord(0x93394) == 0x5F353434U &&
			PS2_MEM_ReadWord(0x93398) == 0x2E35363BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_BHC_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!PS2_MEM_ReadUInt(BHC_CAN_ACT))
		return;
	
	camXBase = PS2_MEM_ReadUInt(BHC_CAMX_BASE_PTR);
	if (!camXBase)
		return;

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 250.f;

	float camX = PS2_MEM_ReadFloat(camXBase + BHC_CAMX);
	float camY = PS2_MEM_ReadFloat(BHC_CAMY);

	camX += (float)xmouse * looksensitivity / scale;
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;

	camY = ClampFloat(camY, -1.221730471f, 1.221730471f);

	PS2_MEM_WriteFloat(camXBase + BHC_CAMX, (float)camX);
	PS2_MEM_WriteFloat(BHC_CAMY, (float)camY);

}