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

#define TAU 6.2831853f // 0x40C90FDB

#define SHSM_CAMY 0x14E2B9C
#define SHSM_CAMY_ANGLE 0x14E2090
#define SHSM_CAMY_LOW 0x14E2574
#define SHSM_CAMXSIN 0x1270310
#define SHSM_CAMXCOS 0x1270318
#define SHSM_ROTXSIN 0x126F1F0
#define SHSM_CAMXSIN 0x1270310
#define SHSM_CAMXCOS 0x1270318
#define SHSM_ROTXSIN 0x126F1F0
#define SHSM_ROTXCOS 0x126F1F8
#define SHSM_CAMX_ZOOM_SIN 0x1270320
#define SHSM_CAMX_ZOOM_COS 0x1270328

static uint8_t PS2_SHSM_Status(void);
static void PS2_SHSM_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Silent Hill: Shattered Memories",
	PS2_SHSM_Status,
	PS2_SHSM_Inject,
	1,
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_SHSHATTEREDMEMORIES = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_SHSM_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323138U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E39393BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_SHSM_Inject(void)
{
	// FIXME: jittery flashlight while rotating
	// TODO: set rotating animation on rotX change
	// TODO: disable during
	//			pause
	//			actions (door open, fence hop)
	//				can fall out of bounds if rotated during door opening
	// TODO: mouse cursor mini-games
	// TODO: lock camY in first person cellphone view


	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 20.f;
	float scale = 400.f;

	float camY = PS2_MEM_ReadFloat(SHSM_CAMY);
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / (scale / 2);
	camY = ClampFloat(camY, -1.f, 1.f);
	PS2_MEM_WriteFloat(SHSM_CAMY, camY);

	float camYLow = PS2_MEM_ReadFloat(SHSM_CAMY_LOW);
	float scaleValue = 1.f;
	if (camY > 0) {
		scaleValue = camYLow;
	}
	else {
		if (camYLow == 40.f)
			scaleValue = 40.f;
		else
			scaleValue = 35.f;
	}

	PS2_MEM_WriteFloat(SHSM_CAMY_ANGLE, camY * scaleValue);

	// camX
	float camXSin = PS2_MEM_ReadFloat(SHSM_CAMXSIN);
	float camXCos = PS2_MEM_ReadFloat(SHSM_CAMXCOS);

	float angle = atan(camXSin / camXCos);
	if (camXCos < 0)
		angle += TAU / 2;

	angle -= (float)xmouse * looksensitivity / scale;

	camXSin = sin(angle);
	camXCos = cos(angle);

	PS2_MEM_WriteFloat(SHSM_CAMXSIN, camXSin);
	PS2_MEM_WriteFloat(SHSM_CAMXCOS, camXCos);
	PS2_MEM_WriteFloat(SHSM_CAMX_ZOOM_SIN, camXSin);
	PS2_MEM_WriteFloat(SHSM_CAMX_ZOOM_COS, camXCos);

	// rotX
	camXSin = PS2_MEM_ReadFloat(SHSM_ROTXSIN);
	camXCos = PS2_MEM_ReadFloat(SHSM_ROTXCOS);

	angle = atan(camXSin / camXCos);
	if (camXCos < 0)
		angle += TAU / 2;

	angle -= (float)xmouse * looksensitivity / scale;

	camXSin = sin(angle);
	camXCos = cos(angle);

	PS2_MEM_WriteFloat(SHSM_ROTXSIN, camXSin);
	PS2_MEM_WriteFloat(SHSM_ROTXCOS, camXCos);
}