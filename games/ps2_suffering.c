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

#define SUFF_CAMX_BASE 0x4DA758
// offsets from camXBase
#define SUFF_CAMX_BASE_SANITY_1 0x140
#define SUFF_CAMX_BASE_SANITY_PRE -0x120
#define SUFF_CAMX_SIN 0x150
#define SUFF_CAMX_COS 0x158
#define SUFF_CAMX_COS_NEG 0x170
#define SUFF_CAMX_SIN2 0x178
#define SUFF_CAMY_BASE 0x1A0
// // offsets from camYBase
#define SUFF_CAMY 0x5B0

// #define SUFF_CAMY_BASE 0xEB888C
// // offsets from camYBase
// #define SUFF_CAMY 0x264

// #define SUFF_CAMX_BASE 0x4DA758
// // offsets from camXBase
// #define SUFF_CAMX_SIN 0x150
// #define SUFF_CAMX_COS 0x158
// #define SUFF_CAMX_COS_NEG 0x170
// #define SUFF_CAMX_SIN2 0x178

// #define SUFF_IS_FIRST_PERSON 0x93FF88

// #define SUFF_CAMX_SIN 0x908F90
// #define SUFF_CAMX_COS 0x908F98
// #define SUFF_CAMX_COS_NEG 0x908FB0
// #define SUFF_CAMX_SIN2 0x908FB8
// #define SUFF_CAMY 0x9404D0
// #define SUFF_IS_FIRST_PERSON 0x93FF88

#define SUFF_IS_NOT_INTERACTING 0x94028C
#define SUFF_IS_IN_GAME_CUTSCENE 0xE0E3B8
#define SUFF_IS_PAUSED 0x4DB774

static uint8_t PS2_SUFF_Status(void);
static uint8_t PS2_SUFF_DetectCamBase(void);
static void PS2_SUFF_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"The Suffering",
	PS2_SUFF_Status,
	PS2_SUFF_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_THESUFFERING = &GAMEDRIVER_INTERFACE;

static uint32_t camYBase = 0;
static uint32_t camXBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_SUFF_Status(void)
{
	// SLUS_206.36
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323036U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E33363BU);
}

static uint8_t PS2_SUFF_DetectCamBase(void)
{
	uint32_t tempCamBase = PS2_MEM_ReadUInt(SUFF_CAMX_BASE);
	if (tempCamBase && 
		PS2_MEM_ReadUInt(tempCamBase + SUFF_CAMX_BASE_SANITY_1) == 0xA) // camX/camYBase below
	{
		camXBase = tempCamBase;
		return 1;
	}
	else if (tempCamBase &&
			 PS2_MEM_ReadUInt(tempCamBase + SUFF_CAMX_BASE_SANITY_PRE) == 0xA) // camX/camYBase above
	{
		camXBase = tempCamBase + SUFF_CAMX_BASE_SANITY_PRE - 0x140;
		return 1;
	}
	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_SUFF_Inject(void)
{
	// TODO: disable during
	//			conext interaction animation
	//			pause
	//			map
	//			in-game cutscene
	// TODO: update weapon position in first-person
	// TODO: rotation animation when turning
	// TODO: object drag/move camX is different
	// TODO: interaction camera
	// TODO: find player base
	//			look for interacting/cutscene as offsets

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// if (PS2_MEM_ReadUInt(SUFF_IS_NOT_INTERACTING) != 3)
	// 	return;

	// if (PS2_MEM_ReadUInt(SUFF_IS_IN_GAME_CUTSCENE))
	// 	return;

	if (PS2_MEM_ReadUInt(SUFF_IS_PAUSED))
		return;

	// TODO: sanity tests for both cam bases
	// camYBase = PS2_MEM_ReadUInt(SUFF_CAMY_BASE);

	if (!PS2_SUFF_DetectCamBase())
		return;

	camYBase = PS2_MEM_ReadUInt(camXBase + SUFF_CAMY_BASE);

	float looksensitivity = (float)sensitivity / 40.f;
	float zoom = 1.f;

	float camY = PS2_MEM_ReadFloat(camYBase + SUFF_CAMY);
	camY += (float)ymouse * looksensitivity / 200.f * (360.f / TAU) * zoom;
	// if (PS2_MEM_ReadUInt(SUFF_IS_FIRST_PERSON))
	// 	camY = ClampFloat(camY, -65.f, 75.f);
	// else
	// 	camY = ClampFloat(camY, -50.f, 50.f);

	float camXSin = PS2_MEM_ReadFloat(camXBase + SUFF_CAMX_SIN);
	float camXCos = PS2_MEM_ReadFloat(camXBase + SUFF_CAMX_COS);

	float angle = atan(camXSin / camXCos);
	if (camXCos < 0)
		angle += TAU / 2.f;

	angle -= (float)xmouse * looksensitivity / 200.f * zoom;
	// if (camXCos < 0)
	// 	angle += TAU / 2;
	// if other quadrants add pi

	camXSin = sin(angle);
	camXCos = cos(angle);

	PS2_MEM_WriteFloat(camXBase + SUFF_CAMX_SIN, (float)camXSin);
	PS2_MEM_WriteFloat(camXBase + SUFF_CAMX_SIN2, (float)camXSin);
	PS2_MEM_WriteFloat(camXBase + SUFF_CAMX_COS, (float)camXCos);
	PS2_MEM_WriteFloat(camXBase + SUFF_CAMX_COS_NEG, -(float)camXCos);
	PS2_MEM_WriteFloat(camYBase + SUFF_CAMY, (float)camY);

}