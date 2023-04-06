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

// Resistance: Retribution
#define RR_CAMY 0x1C98920

static uint8_t PSP_RR_Status(void);
static void PSP_RR_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Resistance: Retribution",
	PSP_RR_Status,
	PSP_RR_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PSP_RESISTANCERETRIBUTION = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PSP_RR_Status(void)
{
	// UCUS98668
	return (PSP_MEM_ReadWord(0x10AEAC0) == 0x55435553U &&
			PSP_MEM_ReadWord(0x10AEAC4) == 0x39383636U && 
			PSP_MEM_ReadWord(0x10AEAC8) == 0x38000000U);
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PSP_RR_Inject(void)
{
	if (xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float camY = PSP_MEM_ReadFloat(RR_CAMY);

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 2000.f;
	const float zoom = 1.f;

	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale / zoom;

	// camY = ClampFloat(camY, -45.f, 45.f);

	PSP_MEM_WriteFloat(RR_CAMY, (float)camY);
}