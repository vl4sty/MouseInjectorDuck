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

#define Q3_ONFOOT_CAMY 0x33D56C
#define Q3_ONFOOT_CAMX 0x33D570
#define Q3_ONFOOT_SANITY 0x33D57C
#define Q3_ONFOOT_FOV 0x6D4020

static uint8_t PS2_Q3_Status(void);
static void PS2_Q3_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Quake III: Revolution",
	PS2_Q3_Status,
	PS2_Q3_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_QUAKE3 = &GAMEDRIVER_INTERFACE;


//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_Q3_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && PS2_MEM_ReadWord(0x00093394) == 0x5F323031U) &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E36373BU;
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_Q3_Inject(void)
{
	// TODO: disable mouse during level intro and possibly boss cutscenes

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 140.f;

	if (PS2_MEM_ReadWord(Q3_ONFOOT_SANITY) != 0x0) { // in-game

		float fov = PS2_MEM_ReadFloat(Q3_ONFOOT_FOV);
		float camX = PS2_MEM_ReadFloat(Q3_ONFOOT_CAMX);
		float camY = PS2_MEM_ReadFloat(Q3_ONFOOT_CAMY);

		camX -= (float)xmouse * looksensitivity * (fov / 90.f);
		camY += (float)ymouse * looksensitivity * (fov / 90.f);

		PS2_MEM_WriteFloat(Q3_ONFOOT_CAMX, (float)camX);
		PS2_MEM_WriteFloat(Q3_ONFOOT_CAMY, (float)camY);
	}

}