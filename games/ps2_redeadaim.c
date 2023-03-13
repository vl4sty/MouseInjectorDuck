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

#define TAU 6.2831853f // 0x40C90FDB

#define REDA_cursorx 0x4B0E54
#define REDA_snapx 0x4B21A4
#define REDA_CURSORY 0x2D092C

#define REDA_TPS_CAMX 0x4B0E54
#define REDA_TPS_CAMY 0x4B0E50
#define REDA_TPS_CAMX_2 0x2DC7A4
#define REDA_FPS_CAMY 0x4B21A0
#define REDA_FPS_CAMX 0x4B21A4
#define REDA_IS_AIMING 0x4B0DD8
#define REDA_ROTX 0x4AF9E4
// #define REDA_MODEL_PITCH 0x4AF9E0
#define REDA_MODEL_PITCH 0x4B0E80

#define REDA_ON_STAIRS 0x2DE184
#define REDA_IS_EXAMINE 0x4B0CB8
#define REDA_IS_NOT_PAUSED 0x31C6CC

static uint8_t PS2_REDA_Status(void);
static void PS2_REDA_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"PS2 Resident Evil Dead Aim",
	PS2_REDA_Status,
	PS2_REDA_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_REDEADAIM = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_REDA_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323036U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E36393BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_REDA_Inject(void)
{
	// lock cursorY to middle
	PS2_MEM_WriteUInt16(REDA_CURSORY, 215);

	if (PS2_MEM_ReadUInt16(REDA_ON_STAIRS))
		return;
	
	// when game pauses to exmaine something, some in-game cutscenes, saving
	if (PS2_MEM_ReadUInt16(REDA_IS_EXAMINE))
		return;
	
	// pause menu, item pickup
	if (!PS2_MEM_ReadUInt16(REDA_IS_NOT_PAUSED))
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity / 40.f;
	const float scale = 600.f;

	if (PS2_MEM_ReadUInt16(REDA_IS_AIMING))	 {
		float camX = PS2_MEM_ReadFloat(REDA_FPS_CAMX);
		float camY = PS2_MEM_ReadFloat(REDA_FPS_CAMY);
		camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;
		camX += (float)xmouse * looksensitivity / scale;

		if (camY > 1)
			camY = 1;
		if (camY < -1)
			camY = -1;

		PS2_MEM_WriteFloat(REDA_FPS_CAMY, (float)camY);
		PS2_MEM_WriteFloat(REDA_FPS_CAMX, (float)camX);
		PS2_MEM_WriteFloat(REDA_TPS_CAMX, (float)camX);
	}
	else {
		// float camY = PS2_MEM_ReadFloat(REDA_TPS_CAMY);	
		float camX = PS2_MEM_ReadFloat(REDA_TPS_CAMX);	
		// camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;
		camX += (float)xmouse * looksensitivity / scale;

		while (camX > TAU / 2)
			camX -= TAU;
		while (camX < -TAU / 2)
			camX += TAU;
		
		// if (camY > 1)
		// 	camY = 1;
		// if (camY < -1)
		// 	camY = -1;

		// PS2_MEM_WriteFloat(REDA_TPS_CAMY, (float)camY);
		PS2_MEM_WriteFloat(REDA_TPS_CAMX, (float)camX);
	}
}