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

#define GB_CAMX_BASE_PTR 0x6201E0
// offsets from camXBase
#define GB_CAMX_SIN 0xE0
#define GB_CAMX_COS 0xE8
#define GB_CAMX_COS_NEG 0xF8
#define GB_CAMX_SIN2 0x100

#define GB_CAMX2_BASE_PTR 0x1810 // offset from camXBase
// offsets from camX2Base
#define GB_CAMX_SIN3 0x30
#define GB_CAMX_COS2 0x38
#define GB_CAMX_COS_NEG2 0x50
#define GB_CAMX_SIN4 0x58

#define GB_CAMY_BASE_PTR 0x22D4 // offset from camXBase
// offset from camYBase
#define GB_CAMY 0x158

// #define GB_CAMY 0x17E5ED8

// #define GB_CAMX_SIN 0x14B3300
// #define GB_CAMX_COS 0x14B3308
// #define GB_CAMX_COS_NEG 0x14B3318
// #define GB_CAMX_SIN2 0x14B3320
// #define GB_CAMX_SIN3 0x1767750
// #define GB_CAMX_COS2 0x1767758
// #define GB_CAMX_COS_NEG2 0x1767770
// #define GB_CAMX_SIN4 0x1767778



static uint8_t PS2_GB_Status(void);
static void PS2_GB_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Ghostbusters: The Video Game",
	PS2_GB_Status,
	PS2_GB_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_GHOSTBUSTERS = &GAMEDRIVER_INTERFACE;

static uint32_t camXBase = 0;
static uint32_t camX2Base = 0;
static uint32_t camYBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_GB_Status(void)
{
	// SLUS_218.82
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323138U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E38323BU);
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_GB_Inject(void)
{
	// TODO: camXBase sanity
	// TODO: 60 FPS cheat

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 40.f;
	float zoom = 1.f;

	camXBase = PS2_MEM_ReadPointer(GB_CAMX_BASE_PTR);
	camX2Base = PS2_MEM_ReadPointer(camXBase + GB_CAMX2_BASE_PTR);
	camYBase = PS2_MEM_ReadPointer(camXBase + GB_CAMY_BASE_PTR);

	float camY = PS2_MEM_ReadFloat(camYBase + GB_CAMY);
	camY -= (float)ymouse * looksensitivity / 200.f * zoom;
	camY = ClampFloat(camY, -1.299999952f, 1.299999952f);

	float camXSin = PS2_MEM_ReadFloat(camXBase + GB_CAMX_SIN);
	float camXCos = PS2_MEM_ReadFloat(camXBase + GB_CAMX_COS);

	float angle = atan(camXSin / camXCos);
	if (camXCos < 0)
		angle += TAU / 2.f;

	angle -= (float)xmouse * looksensitivity / 200.f * zoom;

	camXSin = sin(angle);
	camXCos = cos(angle);

	PS2_MEM_WriteFloat(camXBase + GB_CAMX_SIN, (float)camXSin);
	PS2_MEM_WriteFloat(camXBase + GB_CAMX_COS, (float)camXCos);
	PS2_MEM_WriteFloat(camXBase + GB_CAMX_COS_NEG, -(float)camXCos);
	PS2_MEM_WriteFloat(camXBase + GB_CAMX_SIN2, (float)camXSin);

	PS2_MEM_WriteFloat(camX2Base + GB_CAMX_SIN3, (float)camXSin);
	PS2_MEM_WriteFloat(camX2Base + GB_CAMX_COS2, (float)camXCos);
	PS2_MEM_WriteFloat(camX2Base + GB_CAMX_COS_NEG2, -(float)camXCos);
	PS2_MEM_WriteFloat(camX2Base + GB_CAMX_SIN4, (float)camXSin);

	PS2_MEM_WriteFloat(camYBase + GB_CAMY, (float)camY);

}