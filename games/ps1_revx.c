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

#define REVX_crosshairx 0x000FEB68
#define REVX_crosshairy 0x000FEB6A

#define CROSSHAIRXHIGH 0x1400
#define CROSSHAIRYLOW 0x2C0
#define CROSSHAIRYHIGH 0xE00

static uint8_t PS1_REVX_Status(void);
static void PS1_REVX_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"PS1 Revolution X",
	PS1_REVX_Status,
	PS1_REVX_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS1_REVOLUTIONX = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_REVX_Status(void)
{
	return (PS1_MEM_ReadWord(0xB8B8) == 0x4C55535FU && PS1_MEM_ReadWord(0xB8BC) == 0x3030302EU && PS1_MEM_ReadWord(0xB8C0) == 0x31323B31U); // LUS_000.12;1
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_REVX_Inject(void)
{
	// TODO: check if pause/on menu/aka not in-game

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	// PS1 camx is stored in a Halfword (uint16_t)
	uint16_t crosshairx = PS1_MEM_ReadHalfword(REVX_crosshairx);
	uint16_t crosshairy = PS1_MEM_ReadHalfword(REVX_crosshairy);

	const float looksensitivity = (float)sensitivity / 7.f;

	crosshairx += (float)xmouse * looksensitivity;
	crosshairx = ClampHalfword(crosshairx, 0, CROSSHAIRXHIGH);
	
	crosshairy += (float)ymouse * looksensitivity;
	crosshairy = ClampHalfword(crosshairy, CROSSHAIRYLOW, CROSSHAIRYHIGH);

	PS1_MEM_WriteHalfword(REVX_crosshairx, (uint16_t)crosshairx);
	PS1_MEM_WriteHalfword(REVX_crosshairy, (uint16_t)crosshairy);
}