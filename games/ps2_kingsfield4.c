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

#define KF4_CAMY 0x413F90
#define KF4_CAMX 0x413F94

#define KF4_CAMY2 0x414290
#define KF4_CAMX2 0x414294

static uint8_t PS2_KF4_Status(void);
static void PS2_KF4_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"King's Field IV: The Ancient City",
	PS2_KF4_Status,
	PS2_KF4_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_KINGSFIELD4 = &GAMEDRIVER_INTERFACE;


//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_KF4_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && PS2_MEM_ReadWord(0x00093394) == 0x5F323033U) &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E31383BU;
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_KF4_Inject(void)
{
	// TODO: disable during..
	//			pause
	//			status screen
	//			conversation
	//			item pickup	
	//			main menu

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 14000.f;

	float camX = PS2_MEM_ReadFloat(KF4_CAMX);
	float camY = PS2_MEM_ReadFloat(KF4_CAMY);

	camX += (float)xmouse * looksensitivity;
	camY -= (float)ymouse * looksensitivity;

	PS2_MEM_WriteFloat(KF4_CAMX, (float)camX);
	PS2_MEM_WriteFloat(KF4_CAMY, (float)camY);
	PS2_MEM_WriteFloat(KF4_CAMX2, (float)camX);
	PS2_MEM_WriteFloat(KF4_CAMY2, (float)camY);

}