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

// old shit
#define GITS_CAMBASE 0x3AA88
#define GITS_CAMBASE2 0x1FFDAC
#define GITS_CAMBASE_SANITY_1_VALUE 0xFFFF0000
#define GITS_CAMBASE_SANITY_2_VALUE 0x01000200
// offsets from cambase
#define GITS_CAMBASE_SANITY_1 0x3D0
#define GITS_CAMBASE_SANITY_2 0x3D4
// #define GITS_ROTX 0xD8CE
#define GITS_CAMX 0xA692


// new shit
// #define GITS_CAMX_BASE_PTR 0x1FFE7C
// // offset from camXBase
// #define GITS_CAMX 0x344E

#define GITS_ROTX_BASE_PTR_A 0x3AAA4
#define GITS_ROTX_BASE_PTR_A_SANITY_VALUE 0x00020100
#define GITS_ROTX_BASE_PTR_B 0x1FFDAC
#define GITS_ROTX_BASE_PTR_B_SANITY_VALUE 0xFFFF0000
// offset from rotXBase
#define GITS_ROTX_A 0x66
#define GITS_ROTX_BASE_PTR_A_SANITY 0x0
#define GITS_ROTX_B 0xD8CE
#define GITS_ROTX_BASE_PTR_B_SANITY 0x3D0

#define GITS_IS_NOT_PAUSED 0x2DF44

#define GITS_IS_MISSION_START 0x1FFDF0
#define GITS_IS_MISSION_START_TRUE 0x00100000

// #define GITS_ROTX 0x139642
// #define GITS_CAMX 0x136406
// #define GITS_CAMY 0x139646

static uint8_t PS1_GITS_Status(void);
static uint8_t PS1_GITS_DetectCamBase(void);
static uint8_t PS1_GITS_DetectRotXBase(void);
static void PS1_GITS_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Ghost in the Shell",
	PS1_GITS_Status,
	PS1_GITS_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_GHOSTINTHESHELL = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static uint32_t camBase = 0;
static uint32_t rotXBase = 0;
static uint32_t camBaseList[2] = {GITS_CAMBASE,
								  GITS_CAMBASE2};
static uint32_t rotXBaseList[2] = {GITS_ROTX_BASE_PTR_A,
								   GITS_ROTX_BASE_PTR_B};
static uint32_t rotXOffsetList[2] = {GITS_ROTX_A,
									 GITS_ROTX_B};
static uint32_t rotXSanityOffsetList[2] = {GITS_ROTX_BASE_PTR_A_SANITY,
									   GITS_ROTX_BASE_PTR_B_SANITY};
static uint32_t rotXSanityValueList[2] = {GITS_ROTX_BASE_PTR_A_SANITY_VALUE,
									  GITS_ROTX_BASE_PTR_B_SANITY_VALUE};
static uint32_t rotXOffset = 0;
static uint32_t rotXSanityOffset = 0;
static uint32_t rotXSanityValue = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_GITS_Status(void)
{
	// SLUS_005.52
	return (PS1_MEM_ReadWord(0x9394) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x9398) == 0x5F303035U && 
			PS1_MEM_ReadWord(0x939C) == 0x2E35323BU);
}

static uint8_t PS1_GITS_DetectCamBase(void)
{
	// check if current camBase still valid
	if (camBase &&
		PS1_MEM_ReadWord(camBase + GITS_CAMBASE_SANITY_1) == GITS_CAMBASE_SANITY_1_VALUE &&
		PS1_MEM_ReadWord(camBase + GITS_CAMBASE_SANITY_2) == GITS_CAMBASE_SANITY_2_VALUE)
	{
		return 1;
	}

	// check two camBase addresses as neither is always valid
	int i = 0;
	for (i = 0; i < 2; ++i)
	{
		uint32_t tempCamBase = PS1_MEM_ReadPointer(camBaseList[i]);
		if (tempCamBase &&
			PS1_MEM_ReadWord(tempCamBase + GITS_CAMBASE_SANITY_1) == GITS_CAMBASE_SANITY_1_VALUE &&
			PS1_MEM_ReadWord(tempCamBase + GITS_CAMBASE_SANITY_2) == GITS_CAMBASE_SANITY_2_VALUE)
		{
			camBase = tempCamBase;
			return 1;
		}
	}

	return 0;
}

static uint8_t PS1_GITS_DetectRotXBase(void)
{
	if (rotXBase &&
		PS1_MEM_ReadWord(rotXBase + rotXSanityOffset) == rotXSanityValue)
	{
		return 1;
	}

	int i = 0;
	for (i = 0; i < 2; ++i)
	{
		uint32_t tempRotXBase = PS1_MEM_ReadPointer(rotXBaseList[i]);
		if (tempRotXBase &&
			PS1_MEM_ReadWord(tempRotXBase + rotXSanityOffsetList[i]) == rotXSanityValueList[i])
		{
			rotXBase = tempRotXBase;
			rotXOffset = rotXOffsetList[i];
			rotXSanityOffset = rotXSanityOffsetList[i];
			rotXSanityValue = rotXSanityValueList[i];
			return 1;
		}
	}

	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_GITS_Inject(void)
{
	// TODO: disable during
	//			mission complete
	//			mission failure
	// TODO: turn animation
	// FIXME: mission 7 boss needs a different rotXBase
	// TODO: rotXBase and camXBase sanities

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!PS1_MEM_ReadWord(GITS_IS_NOT_PAUSED))
		return;
	
	if (PS1_MEM_ReadWord(GITS_IS_MISSION_START) == GITS_IS_MISSION_START_TRUE)
		return;

	// rotXBase = PS1_MEM_ReadPointer(GITS_ROTX_BASE_PTR);
	if (!PS1_GITS_DetectRotXBase())
		return;
	
	// camBase = PS1_MEM_ReadPointer(GITS_CAMBASE);
	// if (!PS1_GITS_DetectCamBase())
	// 	return;
	PS1_GITS_DetectCamBase();
	

	// PS1_GITS_DetectRotXBase();

	uint16_t camX = PS1_MEM_ReadHalfword(rotXBase + rotXOffset);
	// uint16_t camX = PS1_MEM_ReadHalfword(rotXBase + GITS_ROTX);
	// uint16_t camX = PS1_MEM_ReadHalfword(camBase + GITS_ROTX);
	// int16_t camY = PS1_MEM_ReadInt16(GITS_CAMY);
	float camXF = (float)camX;
	// float camYF = (float)camY;

	const float scale = 10.f;
	const float looksensitivity = (float)sensitivity / 20.f / scale;

	float dx = (float)xmouse * looksensitivity;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);
	while (camXF > 256.f)
		camXF -= 256.f;
	while (camXF < 0.f)
		camXF += 256.f;
	
	// float ym = -(float)(invertpitch ? -ymouse : ymouse);
	// float dy = ym * looksensitivity / 40000.f;
	// AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);
	// camYF = ClampFloat(camYF, -340.f, 340.f);

	PS1_MEM_WriteHalfword(rotXBase + rotXOffset, (uint16_t)camXF);
	// PS1_MEM_WriteHalfword(rotXBase + GITS_ROTX, (uint16_t)camXF);
	// PS1_MEM_WriteHalfword(camBase + GITS_ROTX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(camBase + GITS_CAMX, (uint16_t)camXF);
	// PS1_MEM_WriteInt16(GITS_CAMY, (int16_t)camYF);
}