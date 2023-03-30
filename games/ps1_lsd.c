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

#define LSD_CAMX 0x91EC2
#define LSD_CAMY_BASE 0x9158C
// offset from camY base
#define LSD_CAMY 0x24

#define LSD_IS_ON_STAIRS 0x91E38
#define LSD_IS_TRANSITION 0x915A0
#define LSD_IS_UNPAUSED 0x8DFC8

static uint8_t PS1_LSD_Status(void);
static uint8_t PS1_LSD_DetectCamYBase(void);
static void PS1_LSD_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"LSD Dream Emulator",
	PS1_LSD_Status,
	PS1_LSD_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_LSDDREAMEMULATOR = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static float scale = 20.f;
static uint32_t camBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_LSD_Status(void)
{
	return (PS1_MEM_ReadWord(0x9244) == 0x534C5053U && PS1_MEM_ReadWord(0x9248) == 0x5F303135U && PS1_MEM_ReadWord(0x924C) == 0x2E35363BU);
}

static uint8_t PS1_LSD_DetectCamYBase(void)
{
	uint32_t tempCamBase = PS1_MEM_ReadUInt(LSD_CAMY_BASE);
	if (tempCamBase)
	{
		camBase = tempCamBase - 0x80000000;
		return 1;
	}
	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_LSD_Inject(void)
{
	// TODO: disable during
	//			FMV
	//			stairs
	//			results
	//			main menu

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!PS1_LSD_DetectCamYBase())
		return;
	
	if (PS1_MEM_ReadUInt(LSD_IS_TRANSITION))
		return;

	if (!PS1_MEM_ReadUInt(LSD_IS_UNPAUSED))
		return;

	const float looksensitivity = (float)sensitivity;

	if (!PS1_MEM_ReadUInt(LSD_IS_ON_STAIRS))
	{
		uint16_t camX = PS1_MEM_ReadHalfword(LSD_CAMX);
		float camXF = (float)camX;

		float dx = (float)xmouse * looksensitivity / scale;
		AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

		while (camXF > 4096)
			camXF -= 4096;	
		while (camXF < 0)
			camXF += 4096;
		

		PS1_MEM_WriteHalfword(LSD_CAMX, (uint16_t)camXF);
	}

	int32_t camY = PS1_MEM_ReadInt(camBase + LSD_CAMY);
	float camYF = (float)camY;

	float dy = (float)ymouse * looksensitivity / scale * 10.f;
	AccumulateAddRemainder(&camYF, &yAccumulator, ymouse, dy);

	if (camYF > 16000)
		camYF = 16000;
	if (camYF < -11300)
		camYF = -11300;

	PS1_MEM_WriteInt(camBase + LSD_CAMY, (int32_t)camYF);
}