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
#include <math.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define TAU 6.2831853f // 0x40C90FDB

#define SSNE_IS_BUSY 0x36B868

#define SSNE_CAMBASE 0x66FD08
#define SSNE_CAMBASE_SANITY_1_VALUE 0x8988883C
#define SSNE_CAMBASE_SANITY_2_VALUE 0x31040B00
// offsets from cambase
#define SSNE_CAMBASE_SANITY_1 0xC
#define SSNE_CAMBASE_SANITY_2 0x18
#define SSNE_CAMX 0x2CC
#define SSNE_CAMY 0x2D0

#define SSNE_ZOOM 0x61B750

static uint8_t PS2_SSNE_Status(void);
static uint8_t PS2_SSNE_DetectCamBase(void);
static void PS2_SSNE_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Serious Sam: Next Encounter",
	PS2_SSNE_Status,
	PS2_SSNE_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_SERIOUSSAMNE = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;
static float scale = 600.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_SSNE_Status(void)
{
	return (PS2_MEM_ReadWord(0x003C0900) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x003C0904) == 0x5F323039U &&
			PS2_MEM_ReadWord(0x003C0908) == 0x2E30373BU);
}

static uint8_t PS2_SSNE_DetectCamBase(void)
{
	uint32_t tempCamBase = PS2_MEM_ReadUInt(SSNE_CAMBASE);
	if (tempCamBase &&
		PS2_MEM_ReadWord(tempCamBase + SSNE_CAMBASE_SANITY_1) == SSNE_CAMBASE_SANITY_1_VALUE &&
		PS2_MEM_ReadWord(tempCamBase + SSNE_CAMBASE_SANITY_2) == SSNE_CAMBASE_SANITY_2_VALUE)
	{
		camBase = tempCamBase;
		return 1;
	}
	return 0;
}

static void PS2_SSNE_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (PS2_MEM_ReadUInt(SSNE_IS_BUSY))
		return;

	if (!PS2_SSNE_DetectCamBase())
		return;

	float zoom = 1.153978825f / PS2_MEM_ReadFloat(SSNE_ZOOM);
	// float fov = 1.f;
	float looksensitivity = (float)sensitivity / 20.f;

	float camX = PS2_MEM_ReadFloat(camBase + SSNE_CAMX);
	camX += (float)xmouse * looksensitivity / scale * zoom;
	PS2_MEM_WriteFloat(camBase + SSNE_CAMX, (float)camX);

	float camY = PS2_MEM_ReadFloat(camBase + SSNE_CAMY);
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * zoom;
	camY = ClampFloat(camY, -1.308996916, 1.308996916);
	PS2_MEM_WriteFloat(camBase + SSNE_CAMY, (float)camY);

}