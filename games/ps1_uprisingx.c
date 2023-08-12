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

#define UX_CAM_BASE_PTR 0x1905E0
// offsets from camBase
#define UX_CAMY 0x228
#define UX_CAMX 0x23C
// #define UX_CAMX_2 0x224

#define UX_IS_PAUSED 0x1FFA72

// #define UX_CAMY 0x19078C
// #define UX_CAMX 0x1907A0

static uint8_t PS1_UX_Status(void);
static void PS1_UX_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Uprising X",
	PS1_UX_Status,
	PS1_UX_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_UPRISINGX = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_UX_Status(void)
{
	// SLUS_006.86
	return (PS1_MEM_ReadWord(0x92EC) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x92F0) == 0x5F303036U && 
			PS1_MEM_ReadWord(0x92F4) == 0x2E38363BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_UX_Inject(void)
{
	// TODO: test camBase and isPaused after level 3

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// if (PS1_MEM_ReadInt16(UX_IS_PAUSED))
	// 	return;
	
	uint32_t camBase = PS1_MEM_ReadPointer(UX_CAM_BASE_PTR);
	if (!camBase)
		return;

	int32_t camX = PS1_MEM_ReadInt(camBase + UX_CAMX);
	int32_t camY = PS1_MEM_ReadInt(camBase + UX_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 700.f;

	float dx = (float)xmouse * looksensitivity * scale / 1.6f;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = -ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);

	camYF = ClampFloat(camYF, -184320.f, 184320.f);

	PS1_MEM_WriteInt(camBase + UX_CAMX, (int32_t)camXF);
	PS1_MEM_WriteInt(camBase + UX_CAMY, (int32_t)camYF);
}