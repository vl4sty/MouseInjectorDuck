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

#define PI 3.14159265f // 0x40490FDB
#define TAU 6.2831853f // 0x40C90FDB

#define MRFH_CAMY 0x344FA8
#define MRFH_CAMX 0x342CFC
#define MRFH_CAMX2 0x344F90
#define MRFH_ZOOM 0x344FF4
#define MRFH_IS_FOCUSED 0x345028

#define MRFH_IS_PEEKING 0x2A48C8
#define MRFH_PEEKING_CAMY 0x32E6C0
#define MRFH_PEEKING_CAMX 0x32E6C4

#define MRFH_IS_NOT_BUSY 0x32C01C
#define MRFH_IS_STATIC_SCREEN 0x2A50B4
#define MRFH_IS_PAUSED 0x257234

#define MRFH_CAMY_MAX 1.04719758f

static uint8_t PS2_MRFH_Status(void);
static void PS2_MRFH_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Michigan: Report from Hell",
	PS2_MRFH_Status,
	PS2_MRFH_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_MICHIGAN = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_MRFH_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C4553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F353330U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E37333BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_MRFH_Inject(void)
{
	// TODO: clampX
	// TODO: peek camera
	//			disable snapback
	// TODO: disable when
	//			paused
	//			reporter opening closing something (closet)
	// 			loading
	//			during static

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// reporter opening closet
	if (!PS2_MEM_ReadUInt(MRFH_IS_NOT_BUSY))
		return;

	if (PS2_MEM_ReadUInt(MRFH_IS_STATIC_SCREEN))
		return;

	if (PS2_MEM_ReadUInt(MRFH_IS_PAUSED))
		return;
	
	uint32_t isFocused = PS2_MEM_ReadUInt(MRFH_IS_FOCUSED);
	if (isFocused != 0x1 && isFocused != 0xB)
		return;

	float fov = PS2_MEM_ReadFloat(MRFH_ZOOM);
	float looksensitivity = (float)sensitivity / 8000.f * (fov / 0.8726646304);

	float camX = PS2_MEM_ReadFloat(MRFH_CAMX);
	float camY = PS2_MEM_ReadFloat(MRFH_CAMY);

	camX -= (float)xmouse * looksensitivity;
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;

	if (camY > MRFH_CAMY_MAX)
		camY = MRFH_CAMY_MAX;
	if (camY < -MRFH_CAMY_MAX)
		camY = -MRFH_CAMY_MAX;
	
	while (camX > PI)
		camX -= TAU;
	while (camX < -PI)
		camX += TAU;

	PS2_MEM_WriteFloat(MRFH_CAMX, (float)camX);
	PS2_MEM_WriteFloat(MRFH_CAMX2, (float)camX);
	PS2_MEM_WriteFloat(MRFH_CAMY, (float)camY);
}