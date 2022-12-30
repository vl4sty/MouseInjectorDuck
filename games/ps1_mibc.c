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

#define MIBC_camx 0x000EFF32
#define MIBC_camy 0x00EFFBC
#define MIBC_lookahead 0x000EFFB8
#define MIBC_playerbase 0x000EF9F0
#define MIBC_playerbase_sanity 0x000008E5 // stable value at playerbase

static uint8_t PS1_MIBC_Status(void);
static uint8_t PS1_MIBC_DetectPlayer(void);
static void PS1_MIBC_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"PS1 Men in Black - The Series - Crashdown",
	PS1_MIBC_Status,
	PS1_MIBC_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_MENINBLACKCRASHDOWN = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_MIBC_Status(void)
{
	return (PS1_MEM_ReadWord(0x9304) == 0x534C5553U && PS1_MEM_ReadWord(0x9308) == 0x5F303133U && PS1_MEM_ReadWord(0x930C) == 0x2E38373BU); // SLUS_013.87;
}
//==========================================================================
// Purpose: detects player pointer from stack address
// Changed Globals: fovbase, playerbase
//==========================================================================
static uint8_t PS1_MIBC_DetectPlayer(void)
{
	if(PS1_MEM_ReadWord(MIBC_playerbase) == MIBC_playerbase_sanity)
		return 1;

	return 0;
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_MIBC_Inject(void)
{
	if(!PS1_MIBC_DetectPlayer())
		return;
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	// disable look ahead
	PS1_MEM_WriteWord(MIBC_lookahead, 0);

	// PS1 camx is stored in a Halfword (uint16_t)
	uint16_t camx = PS1_MEM_ReadHalfword(MIBC_camx);
	uint16_t camy = PS1_MEM_ReadHalfword(MIBC_camy);

	const float looksensitivity = (float)sensitivity / 40.f;

	camx += (float)xmouse * looksensitivity;
	while(camx >= 4096)
		camx -= 4096;
	
	camy -= (float)ymouse * looksensitivity;
	// if(camy < 0)
	// 	camy += 4096;

	PS1_MEM_WriteHalfword(MIBC_camx, (uint16_t)camx);
	PS1_MEM_WriteHalfword(MIBC_camy, (uint16_t)camy);
}