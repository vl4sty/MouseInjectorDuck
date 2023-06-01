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

#define SM2_CAMY 0x1BA4318
#define SM2_CAMX 0x1BA4314

static uint8_t PS2_SM2_Status(void);
static void PS2_SM2_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Spider-Man 2",
	PS2_SM2_Status,
	PS2_SM2_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_SPIDERMAN2 = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_SM2_Status(void)
{
	// SLUS_207.76
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323037U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E37363BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_SM2_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 10000.f;
	// float zoom = 1.f / PS2_MEM_ReadFloat(NOLF_ZOOM);
	float zoom = 1.f;

	float camX = PS2_MEM_ReadFloat(SM2_CAMX);
	float camY = PS2_MEM_ReadFloat(SM2_CAMY);

	camX -= (float)xmouse * looksensitivity * zoom;
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * zoom;

	while (camX > TAU / 2.f)
		camX -= TAU;
	while (camX < -TAU / 2.f)
		camX += TAU;

	camY = ClampFloat(camY, 0.3f, 2.7f);

	PS2_MEM_WriteFloat(SM2_CAMX, (float)camX);
	PS2_MEM_WriteFloat(SM2_CAMY, (float)camY);

}