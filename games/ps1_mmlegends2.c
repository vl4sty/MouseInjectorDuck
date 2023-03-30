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

#define MML2_CAMY 0x7D0F2
#define MML2_CAMX 0x8C0DA

static uint8_t PS1_MML2_Status(void);
static void PS1_MML2_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Mega Man Legends 2",
	PS1_MML2_Status,
	PS1_MML2_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_MEGAMANLEGENDS2 = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_MML2_Status(void)
{
	return (PS1_MEM_ReadWord(0x925C) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x9260) == 0x5F303131U && 
			PS1_MEM_ReadWord(0x9264) == 0x2E34303BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_MML2_Inject(void)
{
	// TODO: disable during...
	//			dialogue/text box
	//				reading signs
	//				conversations
	//			pause/menu/status screen
	//			map screen
	//			in-game cutscenes (walking through doors, boss intros/outros)
	//			hanging on ledges
	//			door fade to black transition (market)
	//			mission complete


	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	uint16_t camX = PS1_MEM_ReadHalfword(MML2_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(MML2_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 1.f;

	float dx = (float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	while (camXF > 4096)
		camXF -= 4096;
	while (camXF < 0)
		camXF += 4096;

	// // clamp y-axis while walking
	if (camYF > 512 && camYF < 32000)
		camYF = 512;
	if (camYF < 65024 && camYF > 32000)
		camYF = 65024;

	PS1_MEM_WriteHalfword(MML2_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(MML2_CAMY, (uint16_t)camYF);
}