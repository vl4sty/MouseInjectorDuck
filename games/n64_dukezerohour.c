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

#define DNZH_ROTX 0x80117F12
#define DNZH_ROTY 0x80117F14
#define DNZH_ZOOM 0x80117F44
#define DNZH_CAMX 0x801A27A4
#define DNZH_CAMY 0x801A27AA

static uint8_t N64_DNZH_Status(void);
static void N64_DNZH_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Duke Nukem: Zero Hour",
	N64_DNZH_Status,
	N64_DNZH_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_N64_DUKEZEROHOUR = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

static uint8_t N64_DNZH_Status(void)
{
	return (N64_MEM_ReadUInt(0x80000000) == 0x3C1A800B && N64_MEM_ReadUInt(0x80000004) == 0x275A44A0); // unique header in RDRAM
}

static void N64_DNZH_Inject(void)
{
	// TODO: disable during
	//			pause
	//			climbing
	//			in-game cutscene
	// TODO: disable idle camera spin
	// TODO: check out in-game debug menu cheat
	// TODO: disable auto-aim
	// 			lock red dot to center?
	// 			lvl2 save with 2 enemies and invincibility on, turn
	//			  auto aim level up and down, red dot moves but camera
	//			  will stay the same so you can look for red dot position
	//			look for different values between first and third person
	//				maybe you can find an auto-aim value between the two
	// TODO: try also writting cam values

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity;
	const float scale = 32.f;
	const float zoom = 256.f / N64_MEM_ReadInt16(DNZH_ZOOM);

	int16_t rotX = N64_MEM_ReadInt16(DNZH_ROTX);
	int16_t rotY = N64_MEM_ReadInt16(DNZH_ROTY);
	float rotXF = (float)rotX;
	float rotYF = (float)rotY;
	
	float dx = (float)xmouse * looksensitivity / scale * zoom;
	AccumulateAddRemainder(&rotXF, &xAccumulator, xmouse, dx);
	// cursorXF = ClampFloat(cursorXF, 8.f, 312.f);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity / scale * zoom;
	AccumulateAddRemainder(&rotYF, &yAccumulator, ym, dy);
	rotYF = ClampFloat(rotYF, -301.f, 301.f);

	// rotXF -= (float)xmouse * looksensitivity * scale * zoom;
	// rotYF -= (float)ymouse * looksensitivity * scale * zoom;

	N64_MEM_WriteInt16(DNZH_ROTX, (int16_t)rotXF);
	N64_MEM_WriteInt16(DNZH_CAMX, (int16_t)(rotXF + 515.f));
	N64_MEM_WriteInt16(DNZH_ROTY, (int16_t)rotYF);
	N64_MEM_WriteInt16(DNZH_CAMY, (int16_t)rotYF);
}