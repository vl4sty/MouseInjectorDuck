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

#define MM64_ROTX 0x80204A04
#define MM64_CAMX 0x80204358
#define MM64_CAMY 0x8020435C

static uint8_t N64_MM64_Status(void);
static void N64_MM64_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Mega Man 64",
	N64_MM64_Status,
	N64_MM64_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_N64_MEGAMAN64 = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

static uint8_t N64_MM64_Status(void)
{
	return (N64_MEM_ReadUInt(0x80000000) == 0x3C1A800A && N64_MEM_ReadUInt(0x80000004) == 0x275AF980); // unique header in RDRAM
}

static void N64_MM64_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity;
	const float scale = 4.f;

	int16_t rotX = N64_MEM_ReadInt16(MM64_ROTX);
	int16_t camY = N64_MEM_ReadInt16(MM64_CAMY);
	float rotXF = (float)rotX;
	float camYF = (float)camY;
	
	float dx = (float)xmouse * looksensitivity / scale;
	AccumulateAddRemainder(&rotXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity / scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	float cameraX = rotXF - 2048;
	while (cameraX > 4096)
		cameraX -= 4096;
	while (cameraX < 0)
		cameraX += 4096;

	N64_MEM_WriteInt16(MM64_ROTX, (int16_t)rotXF);
	N64_MEM_WriteInt16(MM64_CAMX, (int16_t)cameraX);
	N64_MEM_WriteInt16(MM64_CAMY, (int16_t)camYF);
}