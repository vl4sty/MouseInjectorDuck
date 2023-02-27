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

#define JF_CAMBASE 0x1FFE7C
#define JF_CAMBASE_SANITY_1_VALUE 0x450B0010
// offsets from cambase
#define JF_CAMX 0x74
#define JF_CAMY 0x78
#define JF_CAMY_SIGN 0x7A
#define JF_CAMBASE_SANITY_1 -0x30
#define JF_DISABLE_CAMY 0xDD760

// #define JF_CAMX 0x138144
// #define JF_CAMY 0x138148
// #define JF_CAMY_SIGN 0x13814A
#define JF_AUTO_LOOK_BELOW 0x102724
#define JF_IS_STAGE_CLEAR 0x1FDBE0

static uint8_t PS1_JF_Status(void);
static uint8_t PS1_JF_IsCambaseValid(void);
static uint8_t PS1_JF_DetectCambase(void);
static void PS1_JF_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Jumping Flash",
	PS1_JF_Status,
	PS1_JF_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_JUMPINGFLASH = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;
static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_JF_Status(void)
{
	return (PS1_MEM_ReadWord(0x943C) == 0x53435553U && PS1_MEM_ReadWord(0x9440) == 0x5F393431U && PS1_MEM_ReadWord(0x9444) == 0x2E30333BU);
}

static uint8_t PS1_JF_IsCambaseValid(void)
{
	if (PS1_MEM_ReadWord(camBase + JF_CAMBASE_SANITY_1) == JF_CAMBASE_SANITY_1_VALUE){
		return 1;
	}
	return 0;
}

static uint8_t PS1_JF_DetectCambase(void)
{
	uint32_t tempCamBase = PS1_MEM_ReadPointer(JF_CAMBASE);
	if (PS1_MEM_ReadWord(tempCamBase + JF_CAMBASE_SANITY_1) == JF_CAMBASE_SANITY_1_VALUE)
	{
		camBase = tempCamBase;
		return 1;
	}

	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_JF_Inject(void)
{
	// TODO: find auto-center cheat/solution that doesn't wholly disable camY movement with controller
	// TODO: disable mouse movement on stage clear, on platform, boss defeated
	// TODO: disable mouse movement in menu/paused
	// TODO: find cheat for Extra World 2

	// PS1_MEM_WriteWord(JF_AUTO_LOOK_BELOW, 0x0);

	// PS1_MEM_WriteWord(JF_DISABLE_CAMY, 0x0);

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!PS1_JF_IsCambaseValid())
	{
		if (!PS1_JF_DetectCambase()){
			return;
		}
	}
	
	if (PS1_MEM_ReadWord(JF_IS_STAGE_CLEAR))
		return;
	
	uint16_t camX = PS1_MEM_ReadHalfword(camBase + JF_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(camBase + JF_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 2.f;

	float dx = (float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = -ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	camX = (uint16_t)camXF;
	camY = (uint16_t)camYF;

	// if (camYF < 0 || camY > 32000)
	if (camY > 32000)
		PS1_MEM_WriteHalfword(camBase + JF_CAMY_SIGN, 0xFFFF);
	else
		PS1_MEM_WriteHalfword(camBase + JF_CAMY_SIGN, 0x0);


	// clamp y-axis
	if (camY > 776 && camY < 32000)
		camY = 776;
	if (camY < 64640 && camY > 32000)
		camY = 64640;

	PS1_MEM_WriteHalfword(camBase + JF_CAMX, (uint16_t)camX);
	PS1_MEM_WriteHalfword(camBase + JF_CAMY, (uint16_t)camY);

	// PS1_MEM_WriteHalfword(JF_CAMX, (uint16_t)camXF);
	// PS1_MEM_WriteHalfword(JF_CAMY, (uint16_t)camYF);
	// PS1_MEM_WriteWord(JF_CAMY, (uint32_t)camYF);
}