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

#define APS_camx 0x0005FFDC
#define APS_camy 0x00060560
#define APS_railcar_camx 0x00060562
#define APS_inrailcar 0x00060548

#define MIBC_camx 0x000EFF32
#define MIBC_camy 0x00EFFBC
#define MIBC_lookahead 0x000EFFB8
#define MIBC_playerbase 0x000EF9F0
#define MIBC_playerbase_sanity 0x000008E5 // stable value at playerbase

static uint8_t PS1_APS_Status(void);
// static uint8_t PS1_MIBC_DetectPlayer(void);
static void PS1_APS_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Armorines: Project S.W.A.R.M.",
	PS1_APS_Status,
	PS1_APS_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_ARMORINES = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_APS_Status(void)
{
	return (PS1_MEM_ReadUInt(0xB8B8) == 0x5F53554CU && PS1_MEM_ReadUInt(0xB8BC) == 0x2E303130U && PS1_MEM_ReadUInt(0xB8C0) == 0x313B3232U); // SLUS_013.87;
}
//==========================================================================
// Purpose: detects player pointer from stack address
// Changed Globals: fovbase, playerbase
//==========================================================================
// static uint8_t PS1_MIBC_DetectPlayer(void)
// {
// 	if(PS1_MEM_ReadWord(MIBC_playerbase) == MIBC_playerbase_sanity)
// 		return 1;

// 	return 0;
// }
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_APS_Inject(void)
{
	// if(!PS1_MIBC_DetectPlayer())
	// 	return;
	// PS1_MEM_WriteWord(MIBC_lookahead, 0); // disable lookahead

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	uint16_t inRailcar = PS1_MEM_ReadUInt(APS_inrailcar);

	// PS1 camx is stored in a Halfword (uint16_t)
	uint16_t camx = PS1_MEM_ReadHalfword(APS_camx);
	if (inRailcar == 5)
		camx = PS1_MEM_ReadHalfword(APS_railcar_camx);
	uint16_t camy = PS1_MEM_ReadHalfword(APS_camy);

	const float looksensitivity = (float)sensitivity / 20.f;

	camx -= (float)xmouse * looksensitivity;
	while(camx >= 4096)
		camx -= 4096;
	
	camy += (float)ymouse * looksensitivity;
	// if(camy < 0)
	// 	camy += 4096;

	if (inRailcar == 5)
		PS1_MEM_WriteHalfword(APS_railcar_camx, (uint16_t)camx);
	else
		PS1_MEM_WriteHalfword(APS_camx, (uint16_t)camx);
	PS1_MEM_WriteHalfword(APS_camy, (uint16_t)camy);
}