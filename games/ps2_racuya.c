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

#define UYA_CAMX 0x1A6160
#define UYA_CAMY 0x1A6180

#define UYA_IN_TURRET 0x1A71C4
#define UYA_TURRET_CAMX 0x1D73678
#define UYA_TURRET_CAMY 0x1E3DBC0

#define UYA_IS_BUSY 0x1D668C

static uint8_t PS2_UYA_Status(void);
static void PS2_UYA_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Ratchet & Clank: Up Your Arsenal",
	PS2_UYA_Status,
	PS2_UYA_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_RACUPYOURARSENAL = &GAMEDRIVER_INTERFACE;

float yInvert = 1;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_UYA_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x53435553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F393733U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E353300U);
}

static void PS2_UYA_Inject(void)
{
	// TODO: disable aim-assist
	// TODO: disable while
	//			paused
	//			weapon wheel
	// 			map
	//			in-game cutscene
	//			locked camera sections
	//				level 1: skydiving
	//				level 2: wall jumping
	//				level 2: sliding on vine
	//			big bolt collect
	// TODO: find different inTurret value
	// TODO: make savestates

	if (PS2_MEM_ReadUInt(UYA_IS_BUSY))
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 300.f;
	float camXAddress, camYAddress;

	if (PS2_MEM_ReadUInt(UYA_IN_TURRET))
	// if (0)
	{
		camXAddress = UYA_TURRET_CAMX;
		camYAddress = UYA_TURRET_CAMY;
		yInvert = -1.f;
	}
	else
	{
		camXAddress = UYA_CAMX;
		camYAddress = UYA_CAMY;
		yInvert = 1.f;
	}

	float camX = PS2_MEM_ReadFloat(camXAddress);
	camX -= (float)xmouse * looksensitivity / scale;
	PS2_MEM_WriteFloat(camXAddress, (float)camX);

	float camY = PS2_MEM_ReadFloat(camYAddress);
	camY += (float)(invertpitch ? -ymouse: ymouse) * yInvert * looksensitivity / scale;
	PS2_MEM_WriteFloat(camYAddress, (float)camY);

}