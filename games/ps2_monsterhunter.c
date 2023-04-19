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

#define MH_CAMX 0x499998
#define MH_CAMX_LAST 0x499928
#define MH_CAMY 0x499934
#define MH_CAMY_LOW 0x499964

static uint8_t PS2_MH_Status(void);
static void PS2_MH_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Monster Hunter",
	PS2_MH_Status,
	PS2_MH_Inject,
	1,
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_MONSTERHUNTER = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_MH_Status(void)
{
	// SLUS_208.96
	return (PS2_MEM_ReadWord(0x0047874C) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00478750) == 0x5F323038U &&
			PS2_MEM_ReadWord(0x00478754) == 0x2E39363BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_MH_Inject(void)
{
	// TODO: camY base?
	// TODO: clamp camY based on ground?
	// TODO: disable during
	//			in town
	//			pause
	//			item selection
	//			in-game cutscenes

	// clamp camY when moved due to walking on slope
	float camY = PS2_MEM_ReadFloat(MH_CAMY);
	float camYLow = PS2_MEM_ReadFloat(MH_CAMY_LOW);
	camY = ClampFloat(camY, camYLow, camYLow + 380.f);
	PS2_MEM_WriteFloat(MH_CAMY, (float)camY);

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 40.f;

	uint16_t camX = PS2_MEM_ReadUInt(MH_CAMX);
	float camXF = (float)camX;
	float dx = -(float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, -xmouse, dx);
	while (camXF > 65535)
		camXF -= 65535;
	while (camXF < 0)
		camXF += 65535;

	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * (scale / 30.f);
	camY = ClampFloat(camY, camYLow, camYLow + 380.f);

	PS2_MEM_WriteUInt16(MH_CAMX, (uint16_t)camXF);
	PS2_MEM_WriteUInt16(MH_CAMX_LAST, (uint16_t)camXF);
	PS2_MEM_WriteFloat(MH_CAMY, (float)camY);

}