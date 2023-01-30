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

#define BL_CAM_X 0x5A8FA8
#define BL_CAM_Y 0x5A8FAC

static uint8_t PS2_BL_Status(void);
static void PS2_BL_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Black",
	PS2_BL_Status,
	PS2_BL_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_BLACK = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_BL_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && PS2_MEM_ReadWord(0x00093394) == 0x5F323133U) &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E37363BU;
}

static void PS2_BL_Inject(void)
{
	// TODO: gun angle drift? just for show

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	// float fov = PS2_MEM_ReadFloat(cambase + RDR_camFov);
	float looksensitivity = (float)sensitivity / 500.f;


	float camX = PS2_MEM_ReadFloat(BL_CAM_X);
	camX -= (float)xmouse * looksensitivity;
	PS2_MEM_WriteFloat(BL_CAM_X, (float)camX);

	float camY = PS2_MEM_ReadFloat(BL_CAM_Y);
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * 1.333;
	PS2_MEM_WriteFloat(BL_CAM_Y, (float)camY);

}