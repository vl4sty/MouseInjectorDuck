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

#define TAU 6.2831853f // 0x40C90FDB

#define ENB_CAMY 0x9D602C
#define ENB_CAMY_LAST 0x9D6034
#define ENB_CAMX_SIN 0x9D5F20
#define ENB_CAMX_SIN2_NEG 0x9D5F08
#define ENB_CAMX_SIN3_NEG 0x9D5F48
#define ENB_CAMX_COS 0x9D5F28
#define ENB_CAMX_COS2 0x9D5F00
#define ENB_CAMX_COS3 0x9D5F40
#define ENB_CAMX_ANGLE 0x9D6030
#define ENB_CAMX_ANGLE2 0x9D6038
#define ENB_CAMX_ANGLE3 0x9D6A0C

static uint8_t PS2_ENB_Status(void);
static void PS2_ENB_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Echo Night: Beyond",
	PS2_ENB_Status,
	PS2_ENB_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_ECHONIGHTBEYOND = &GAMEDRIVER_INTERFACE;

static float scale = 100.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_ENB_Status(void)
{
	// SLUS_209.28
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323039U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E32383BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_ENB_Inject(void)
{
	// TODO: disable during
	//			pause
	//			elevator
	//			hatch open
	//			door open
	//			examine locked door
	//			item pickup
	//			conversation
	//			save

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 140.f;

	float camY = PS2_MEM_ReadFloat(ENB_CAMY);
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;
	camY = ClampFloat(camY, -0.8726645708f, 0.7853981256f);
	PS2_MEM_WriteFloat(ENB_CAMY, camY);
	PS2_MEM_WriteFloat(ENB_CAMY_LAST, camY);

	float camXSin = PS2_MEM_ReadFloat(ENB_CAMX_SIN);
	float camXCos = PS2_MEM_ReadFloat(ENB_CAMX_COS);
	float angle = atan(camXSin / camXCos);
	angle += (float)xmouse * looksensitivity / scale;
	if (camXCos < 0)
		angle += TAU / 2.f;
	camXSin = sin(angle);
	camXCos = cos(angle);
	PS2_MEM_WriteFloat(ENB_CAMX_SIN, camXSin);
	PS2_MEM_WriteFloat(ENB_CAMX_SIN2_NEG, -camXSin);
	PS2_MEM_WriteFloat(ENB_CAMX_SIN3_NEG, -camXSin);
	PS2_MEM_WriteFloat(ENB_CAMX_COS, camXCos);
	PS2_MEM_WriteFloat(ENB_CAMX_COS2, camXCos);
	PS2_MEM_WriteFloat(ENB_CAMX_COS3, camXCos);
	PS2_MEM_WriteFloat(ENB_CAMX_ANGLE, angle);

}