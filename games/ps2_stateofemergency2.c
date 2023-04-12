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

#define SOE2_ONFOOT_GUNAIMY 0x4CFB54
#define SOE2_ONFOOT_CAMY 0x4CFB60
#define SOE2_ONFOOT_CAMX_COS 0x4CFB50
#define SOE2_ONFOOT_CAMX_SIN 0x4CFB58
#define SOE2_ONFOOT_CAMX_COS2 0x4CFB5C
#define SOE2_ONFOOT_CAMX_SIN2 0x4CFB64
#define SOE2_ONFOOT_CAMX_COS3 0x592EE4
#define SOE2_ONFOOT_CAMX_SIN3 0x592EE0

static uint8_t PS2_SOE2_Status(void);
static void PS2_SOE2_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"State of Emergency 2",
	PS2_SOE2_Status,
	PS2_SOE2_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_STATEOFEMERGENCY2 = &GAMEDRIVER_INTERFACE;

static float scale = 100.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_SOE2_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323039U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E36363BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_SOE2_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 140.f;

	float camY = PS2_MEM_ReadFloat(SOE2_ONFOOT_CAMY);
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;
	PS2_MEM_WriteFloat(SOE2_ONFOOT_CAMY, camY);
	PS2_MEM_WriteFloat(SOE2_ONFOOT_GUNAIMY, camY);

	float camXSin = PS2_MEM_ReadFloat(SOE2_ONFOOT_CAMX_SIN);
	float camXCos = PS2_MEM_ReadFloat(SOE2_ONFOOT_CAMX_COS);
	float angle = atan(camXSin / camXCos);
	angle += (float)xmouse * looksensitivity / scale;
	if (camXCos < 0)
		angle += TAU / 2.f;
	camXSin = sin(angle);
	camXCos = cos(angle);
	PS2_MEM_WriteFloat(SOE2_ONFOOT_CAMX_SIN, camXSin);
	PS2_MEM_WriteFloat(SOE2_ONFOOT_CAMX_COS, camXCos);
	PS2_MEM_WriteFloat(SOE2_ONFOOT_CAMX_SIN2, camXSin);
	PS2_MEM_WriteFloat(SOE2_ONFOOT_CAMX_COS2, camXCos);
	PS2_MEM_WriteFloat(SOE2_ONFOOT_CAMX_SIN3, camXSin);
	PS2_MEM_WriteFloat(SOE2_ONFOOT_CAMX_COS3, camXCos);

}