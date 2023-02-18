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

#define RES_CAMX 0xA8606
#define RES_CAMY 0xA865C
#define RES_TEXT_ON_SCREEN 0x62DEC
#define RES_CURSORX 0x62DDC
#define RES_CURSORY 0x62DE0

static uint8_t PS1_RES_Status(void);
static void PS1_RES_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Resident Evil: Survivor",
	PS1_RES_Status,
	PS1_RES_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_RESIDENTEVILSURVIVOR = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_RES_Status(void)
{
	return (PS1_MEM_ReadWord(0x9424) == 0x534C5553U && PS1_MEM_ReadWord(0x9428) == 0x5F303130U && PS1_MEM_ReadWord(0x942C) == 0x2E38373BU); // SLUS_010.87;
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_RES_Inject(void)
{
	// disable aim-lock on targets
	PS1_MEM_WriteWord(0xA85B8, 0x0); // aim-lock active?
	PS1_MEM_WriteWord(0xA8670, 0x0); // last aim-lock target?
	PS1_MEM_WriteWord(0xBB018, 0x0); // aim-lock target?

	// disable auto-center
	PS1_MEM_WriteWord(0x78A14, 0x0); // disable -pitch adjust
	PS1_MEM_WriteWord(0x78A30, 0x0); // disable +pitch adjust
	// PS1_MEM_WriteWord(0x78A48, 0x0); // disable snap to zero? requires testing
	// PS1_MEM_WriteWord(0x78A4C, 0x0); // disable snap to zero? requires testing

	// lock cursor to center of screen
	PS1_MEM_WriteWord(RES_CURSORX, 0xA0); // x middle: 160
	PS1_MEM_WriteWord(RES_CURSORY, 0x78); // y middle: 120

	// if text prompt on screen, don't move camera
	if (PS1_MEM_ReadWord(RES_TEXT_ON_SCREEN) == 0x0)
		return;

	// TODO: add strafe
	// TODO: two modes of play
	// 			one just uses mouse to move cursor
	//			the other controls like a standard FPS with strafing
	// TODO: mouse snaps to zero when walking? might need an extra auto-center code that disable some snap opcode
	//			could be also due to wrapping or type cast
	// TODO: find 60FPS speed modifier

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	uint16_t camX = PS1_MEM_ReadHalfword(RES_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(RES_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 30.f;

	float dx = (float)xmouse * looksensitivity;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	// clamp y-axis
	if (camYF > 800 && camYF < 32000)
		camYF = 800;
	if (camYF < 64735 && camYF > 32000)
		camYF = 64735;

	PS1_MEM_WriteHalfword(RES_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(RES_CAMY, (uint16_t)camYF);
}