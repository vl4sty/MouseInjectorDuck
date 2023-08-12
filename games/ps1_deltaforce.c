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

#define PI 3.1415926f // 0x40490FDB
#define TAU 6.28319f // 0x40C90FDB

#define DFUW_CAMY_BASE_PTR 0xC1380
// offset from camYBase
#define DFUW_CAMY 0x178

#define DFUW_CAMX_BASE_PTR 0xC138C
// offsets from camXBase
#define DFUW_MINIMAP_DIR 0x4
#define DFUW_CAMX_SIN 0x10
#define DFUW_CAMX_COS 0x18
#define DFUW_CAMX_COS_NEG 0x28
#define DFUW_CAMX_SIN_2 0x30
#define DFUW_SNIPER_X 0x9C0
#define DFUW_SNIPER_Y 0x9C4
#define DFUW_SNIPER_SANITY 0x9D8
#define DFUW_SNIPER_SANITY_VALUE 0xAF0CFFFF

#define DFUW_FOV_BASE_PTR 0xE50A8
// offset from FOVBase
#define DFUW_FOV 0x3C

#define DFUW_IS_MID_LVL_LOADING 0xC1D5C

#define DFUW_IS_BUSY 0xC0ACC
#define DFUW_NOT_BUSY 0x2530

// #define DFUW_CAMY 0x10276C
// #define DFUW_CAMX_SIN 0x101050
// #define DFUW_CAMX_COS 0x101058
// #define DFUW_CAMX_COS_NEG 0x101068
// #define DFUW_CAMX_SIN_2 0x101070
// #define DFUW_FOV 0x10FF18
// #define DFUW_MINIMAP_DIR 0x1025F8

#define DFUW_IS_PAUSED 0xABCA4

static uint8_t PS1_DFUW_Status(void);
static void PS1_DFUW_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Delta Force: Urban Warfare",
	PS1_DFUW_Status,
	PS1_DFUW_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_DELTAFORCEURBANWARFARE = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static uint32_t camYBase = 0;
static uint32_t camXBase = 0;
static uint32_t fovBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_DFUW_Status(void)
{
	// SLUS_014.29
	return (PS1_MEM_ReadWord(0x9274) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x9278) == 0x5F303134U && 
			PS1_MEM_ReadWord(0x927C) == 0x2E32393BU);

}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_DFUW_Inject(void)
{
	// TODO: test isBusy flag
	//			continue at isBusyTestContinueHERE.sav

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (PS1_MEM_ReadUInt(DFUW_IS_PAUSED))
		return;

	if (PS1_MEM_ReadHalfword(DFUW_IS_BUSY) != DFUW_NOT_BUSY)
		return;
	
	// if (PS1_MEM_ReadUInt(DFUW_IS_MID_LVL_LOADING) != 1)
	// 	return;
	
	camYBase = PS1_MEM_ReadPointer(DFUW_CAMY_BASE_PTR);
	camXBase = PS1_MEM_ReadPointer(DFUW_CAMX_BASE_PTR);
	fovBase = PS1_MEM_ReadPointer(DFUW_FOV_BASE_PTR);

	const float scale = 5.f;
	const float looksensitivity = (float)sensitivity / 200.f;
	float fov = (float)PS1_MEM_ReadInt(fovBase + DFUW_FOV) / 853.f;

	if (PS1_MEM_ReadWord(camXBase + DFUW_SNIPER_SANITY) == DFUW_SNIPER_SANITY_VALUE)
	{
		const float sniperScale = 200.f;
		// do the sniper	
		int32_t sniperX = PS1_MEM_ReadInt(camXBase + DFUW_SNIPER_X);
		int32_t sniperY = PS1_MEM_ReadInt(camXBase + DFUW_SNIPER_Y);
		float sniperXF = (float)sniperX;
		float sniperYF = (float)sniperY;

		float dx = (float)xmouse * sniperScale / 8.f * fov;
		AccumulateAddRemainder(&sniperXF, &xAccumulator, xmouse, dx);

		float ym = (float)(invertpitch ? -ymouse : ymouse);
		float dy = ym * looksensitivity * sniperScale * fov;
		AccumulateAddRemainder(&sniperYF, &yAccumulator, ym, dy);

		PS1_MEM_WriteInt(camXBase + DFUW_SNIPER_X, (int32_t)sniperXF);
		PS1_MEM_WriteInt(camXBase + DFUW_SNIPER_Y, (int32_t)sniperYF);
	}

	int32_t camXSin = PS1_MEM_ReadInt(camXBase + DFUW_CAMX_SIN);
	int32_t camXCos = PS1_MEM_ReadInt(camXBase + DFUW_CAMX_COS);
	float camXSinF = (float)(floor(camXSin)) / 65535.f;
	float camXCosF = (float)(floor(camXCos)) / 65535.f;

	float angle = (float)atan((float)camXSinF / (float)camXCosF);

	if (camXCos < 0)
		angle += PI;

	angle += (float)xmouse * looksensitivity / 20.f / scale * fov;

	while (angle > TAU)
		angle -= TAU;
	while (angle < 0)
		angle += TAU;

	PS1_MEM_WriteInt(camXBase + DFUW_CAMX_SIN, (int32_t)((float)sin(angle) * 65535.f));
	PS1_MEM_WriteInt(camXBase + DFUW_CAMX_SIN_2, (int32_t)((float)sin(angle) * 65535.f));
	PS1_MEM_WriteInt(camXBase + DFUW_CAMX_COS, (int32_t)((float)cos(angle) * 65535.f));
	PS1_MEM_WriteInt(camXBase + DFUW_CAMX_COS_NEG, -(int32_t)((float)cos(angle) * 65535.f));
	PS1_MEM_WriteInt(camYBase + DFUW_MINIMAP_DIR, (int32_t)((angle / TAU) * 4096.f) + 3072);

	int32_t camY = PS1_MEM_ReadInt(camYBase + DFUW_CAMY);
	float camYF = (float)camY;

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = -ym * looksensitivity * 1900000.f / scale * fov;
	AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);

	PS1_MEM_WriteInt(camYBase + DFUW_CAMY, (int32_t)camYF);
}