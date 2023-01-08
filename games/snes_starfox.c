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

#define TAU 6.2831853f // 0x40C90FDB

#define SFOX_shipx 0x000342 // signed
#define SFOX_shipy 0x000344 // signed

static uint8_t SNES_SFOX_Status(void);
static void SNES_SFOX_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Star Fox",
	SNES_SFOX_Status,
	SNES_SFOX_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_SNES_STARFOX = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SNES_SFOX_Status(void)
{
	return (SNES_MEM_ReadWord(0xE64) == 0x0E9A);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SNES_SFOX_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity;

	int16_t cursorx = SNES_MEM_ReadWord(SFOX_shipx);
	int16_t cursory = SNES_MEM_ReadWord(SFOX_shipy);

	cursorx += ((float)xmouse) * looksensitivity / 80.f;
	cursory += ((float)ymouse) * looksensitivity / 80.f;

	SNES_MEM_WriteWord(SFOX_shipx, (int16_t)cursorx);
	SNES_MEM_WriteWord(SFOX_shipy, (int16_t)cursory);
}