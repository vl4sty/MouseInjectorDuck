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

#define UC_CAMBASE 0x779188
#define UC_CAM_Y 0x674 // offset
#define UC_CAM_X 0x678 // offset
#define UC_ON_LADDER 0x2E0 // offset
#define UC_FOV 0x370

static uint8_t PS2_UC_Status(void);
static void PS2_UC_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Urban Chaos - Riot Response",
	PS2_UC_Status,
	PS2_UC_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_URBANCHAOS = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_UC_Status(void)
{
	// BASLUS-21390
	return (PS2_MEM_ReadWord(0x006AC730) == 0x4241534C && PS2_MEM_ReadWord(0x006AC734) == 0x55532D32 &&
			PS2_MEM_ReadWord(0x006AC738) == 0x31333930);
}

static void PS2_UC_Inject(void)
{
	// TODO: no mouse when paused
	// TODO: adjust sensitivity based on FOV

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	uint32_t camBase = PS2_MEM_ReadPointer(UC_CAMBASE);

	if (camBase == 0x0) // no cambase, not in game
		return;

	if (PS2_MEM_ReadUInt(camBase + UC_ON_LADDER) == 0x1) // don't move camera when on ladder | NEEDS TESTING
		return;

	// float fov = PS2_MEM_ReadFloat(cambase + RDR_camFov);
	float fov = PS2_MEM_ReadFloat(camBase + UC_FOV); // default FOV = 53
	float looksensitivity = (float)sensitivity / 13000.f * ((int)fov / 53.f);


	float camX = PS2_MEM_ReadFloat(camBase + UC_CAM_X);
	camX -= (float)xmouse * looksensitivity;
	PS2_MEM_WriteFloat(camBase + UC_CAM_X, (float)camX);

	float camY = PS2_MEM_ReadFloat(camBase + UC_CAM_Y);
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;
	camY = ClampFloat(camY, -1.256637096f, 1.256637096f);
	PS2_MEM_WriteFloat(camBase + UC_CAM_Y, (float)camY);

}