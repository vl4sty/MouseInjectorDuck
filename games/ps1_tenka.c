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
#include <stdio.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

// OFFSET addresses, requires cam_base to use
#define TENKA_camx_offset 0x00176AAA - 0x00176A84
#define TENKA_camy_offset 0x00176B50 - 0x00176A84
// STATIC addresses
#define TENKA_cam_base 0x000565F4
#define TENKA_cam_sanity 0x000565A0
#define TENKA_controlsbyte 0x00019EFC // 1 byte, first half of controller button pressed bit flags (0000 0000) ([] X O ^ R1 L1 R2 L2)

static uint8_t PS1_TENKA_Status(void);
static uint8_t PS1_TENKA_CameraExists(void);
static void PS1_TENKA_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"PS1 Codename: Tenka",
	PS1_TENKA_Status,
	PS1_TENKA_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS1_CODENAMETENKA = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_TENKA_Status(void)
{
	return (PS1_MEM_ReadWord(0x925C) == 0x53435553U && PS1_MEM_ReadWord(0x9260) == 0x5F393434U && PS1_MEM_ReadWord(0x9264) == 0x2E30393BU); // SCUS_944.09;
}
//==========================================================================
// Purpose: determines if there is a camera
//==========================================================================
static uint8_t PS1_TENKA_CameraExists(void)
{
	if(PS1_MEM_ReadWord(TENKA_cam_sanity) == 0xE6001800 ||
	   PS1_MEM_ReadWord(TENKA_cam_sanity) == 0xE6000800) // value near static camera pointer that is unchanging while a camera exists
		return 1;

	return 0;
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_TENKA_Inject(void)
{
	if(!PS1_TENKA_CameraExists())
		return;

	// read in controls byte
	uint8_t controlflags = PS1_MEM_ReadByte(TENKA_controlsbyte);
	// set R2 bit always on to strafe
	controlflags |= 1UL << 1;
	// toggle strafe always on (R2 would normally need to be held to strafe)
	PS1_MEM_WriteByte(TENKA_controlsbyte, controlflags);

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const uint32_t cam_base = PS1_MEM_ReadPointer(TENKA_cam_base);

	uint16_t camX = PS1_MEM_ReadHalfword(cam_base + TENKA_camx_offset);
	uint16_t camY = PS1_MEM_ReadHalfword(cam_base + TENKA_camy_offset);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 40.f;
	const float scale = 1.f;

	float dx = -(float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, -xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	// camx -= (float)xmouse * looksensitivity;
	// while(camx >= 4096)
	// 	camx -= 4096;
	
	// camy += (float)ymouse * looksensitivity;
	// if(camy < 0)
	// 	camy += 4096;

	PS1_MEM_WriteHalfword(cam_base + TENKA_camx_offset, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(cam_base + TENKA_camy_offset, (uint16_t)camYF);
}