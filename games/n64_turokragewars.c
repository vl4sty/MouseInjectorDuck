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

#define TRW_CAMX 0x8069260C
#define TRW_CAMY 0x80692C2C
#define TRW_CAMY_ANGLE 0x80692CC4

static uint8_t N64_TRW_Status(void);
static void N64_TRW_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Turok: Rage Wars",
	N64_TRW_Status,
	N64_TRW_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_N64_TUROKRAGEWARS = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

static uint8_t N64_TRW_Status(void)
{
	return (N64_MEM_ReadUInt(0x80000000) == 0x3C1A002C && N64_MEM_ReadUInt(0x80000004) == 0x275A0D70); // unique header in RDRAM
}

static void N64_TRW_Inject(void)
{
	// TODO: find different cheat to disable auto-level
	//			current one breaks CPUs
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity;
	const float scale = 9000.f;
	// const float zoom = 256.f / N64_MEM_ReadInt16(DNZH_ZOOM);
	const float zoom = 1.f;

	float camX = N64_MEM_ReadFloat(TRW_CAMX);
	// float camY = N64_MEM_ReadFloat(TRW_CAMY);
	float camY = N64_MEM_ReadFloat(TRW_CAMY_ANGLE);

	camX += (float)xmouse * looksensitivity / scale * zoom * 2.f;

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	camY -= ym * looksensitivity / scale * zoom;

	N64_MEM_WriteFloat(TRW_CAMX, (float)camX);
	// N64_MEM_WriteFloat(TRW_CAMY, (float)camY);
	N64_MEM_WriteFloat(TRW_CAMY_ANGLE, (float)camY);
	// N64_MEM_WriteFloat(TRW_CAMY_ANGLE, (float)camY * 1.570796251f);
}