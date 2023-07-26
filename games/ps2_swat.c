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

#define TAU 6.2831853f // 0x40C90FDB

#define SWAT_CAM_BASE_PTR 0x4DA1B8
// offsets from camBase
#define SWAT_CAMX_SIN 0xC4
#define SWAT_CAMX_COS 0xCC
#define SWAT_CAMY 0x6EC
#define SWAT_FOV 0x4D0
#define SWAT_IS_ZOOMED 0x4CC

#define SWAT_IS_PAUSED 0x524F49

static uint8_t PS2_SWAT_Status(void);
static void PS2_SWAT_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"SWAT: Global Strike Team",
	PS2_SWAT_Status,
	PS2_SWAT_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_SWAT = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_SWAT_Status(void)
{
	// SLUS_204.33
	return (PS2_MEM_ReadWord(0x00093252) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093256) == 0x5F323034U &&
			PS2_MEM_ReadWord(0x0009325A) == 0x2E333300U);
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_SWAT_Inject(void)
{
	// TODO: camBase sanity

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (PS2_MEM_ReadUInt8(SWAT_IS_PAUSED))
		return;

	float looksensitivity = (float)sensitivity / 110.f;
	float zoom = 1.f ;

	camBase = PS2_MEM_ReadPointer(SWAT_CAM_BASE_PTR);
	if (!camBase)
		return;

	if (PS2_MEM_ReadUInt(camBase + SWAT_IS_ZOOMED))
		zoom = PS2_MEM_ReadFloat(camBase + SWAT_FOV) / 53.f;

	float camY = PS2_MEM_ReadFloat(camBase + SWAT_CAMY);
	camY -= (float)ymouse * looksensitivity / 160.f * zoom;
	camY = ClampFloat(camY, -1.308997035f, 1.308997035f);

	float camXSin = PS2_MEM_ReadFloat(camBase + SWAT_CAMX_SIN);
	float camXCos = PS2_MEM_ReadFloat(camBase + SWAT_CAMX_COS);

	float angle = atan(camXSin / camXCos);
	if (camXCos < 0)
		angle += TAU / 2.f;

	angle += (float)xmouse * looksensitivity / 200.f * zoom;

	camXSin = sin(angle);
	camXCos = cos(angle);

	PS2_MEM_WriteFloat(camBase + SWAT_CAMX_SIN, (float)camXSin);
	PS2_MEM_WriteFloat(camBase + SWAT_CAMX_COS, (float)camXCos);

	PS2_MEM_WriteFloat(camBase + SWAT_CAMY, (float)camY);

}