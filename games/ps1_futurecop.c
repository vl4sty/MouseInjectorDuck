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

#define FC_ROTBASE 0x9FA58
// offset from rotbase
#define FC_ROTX 0xB0

#define FC_CAMBASE 0x9F588
// offsets from cambase
#define FC_CAMX 0x20
#define FC_CAMERA_TYPE -0x288
#define FC_IS_PAUSED -0x1C

// #define FC_ROTX 0xC0C70
// #define FC_CAMX 0x1D6EB4
// #define FC_CAMERA_TYPE 0x1D6C0C
// #define FC_IS_PAUSED 0x1D6E78

static uint8_t PS1_FC_Status(void);
static void PS1_FC_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Future Cop: L.A.P.D.",
	PS1_FC_Status,
	PS1_FC_Inject,
	1, // 1000 Hz tickrate
	0, // crosshair sway supported for driver
	"[ON] Lock camera to rotation",  // control type option
	"[OFF] Lock camera to rotation",  // control type option
};

const GAMEDRIVER *GAME_PS1_FUTURECOP = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static float scale = 24.f;
static uint32_t camBase = 0;
static uint32_t rotBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_FC_Status(void)
{
	return (PS1_MEM_ReadWord(0x92A4) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x92A8) == 0x5F303037U && 
			PS1_MEM_ReadWord(0x92AC) == 0x2E33393BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_FC_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	camBase = PS1_MEM_ReadPointer(FC_CAMBASE);
	if (camBase == 0x0)
		return;
	rotBase = PS1_MEM_ReadPointer(FC_ROTBASE);

	if (PS1_MEM_ReadHalfword(camBase + FC_IS_PAUSED))
		return;
	
	uint8_t camType = PS1_MEM_ReadByte(camBase + FC_CAMERA_TYPE);

	uint16_t rotX = PS1_MEM_ReadHalfword(rotBase + FC_ROTX);
	float rotXF = (float)rotX;

	const float looksensitivity = (float)sensitivity;

	float dx = (float)xmouse * looksensitivity / scale;
	AccumulateAddRemainder(&rotXF, &xAccumulator, xmouse, dx);

	while (rotXF > 4096)
		rotXF -= 4096;
	while (rotXF < 0)
		rotXF += 4096;
	
	if (!optionToggle && (camType > 0x3 || camType < 0x2))
	{
		float camXF = rotXF;
		if (camXF >= 2048)
			camXF -= 4096;
		
		PS1_MEM_WriteInt(camBase + FC_CAMX, (int32_t)camXF);
	}

	PS1_MEM_WriteHalfword(rotBase + FC_ROTX, (uint16_t)rotXF);
}