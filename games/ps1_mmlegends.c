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

#define MML_CAMX 0xB5206
#define MML_CAMX2 0xB520E
#define MML_CAMY 0xB52B8
#define MML_CAMY_WALKING 0xA396C

static uint8_t PS1_MML_Status(void);
static void PS1_MML_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Mega Man Legends",
	PS1_MML_Status,
	PS1_MML_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_MEGAMANLEGENDS = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_MML_Status(void)
{
	return (PS1_MEM_ReadWord(0x925C) == 0x534C5553U && PS1_MEM_ReadWord(0x9260) == 0x5F303036U && PS1_MEM_ReadWord(0x9264) == 0x2E30333BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_MML_Inject(void)
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
	// TODO: make sure text boxes that don't lock camera aren't interfered with
	// TODO: different clamps for walking and aiming
	// TODO: disable auto-lookahead when switching to aiming mode
	//			should just stay at the same angle you are already looking
	// TODO: fix jerky x-axis
	//			seems like it only happens with fast turns
	//			it will stop at one position for a moment and then continue to the final position
	//			normally turning keeps his body and the camera aligned but with mouse turning it
	//			causes him to go diagonal from the camera and then when you stop looking horizontally
	//			the camera has to catch up to the direction that he is looking
	//			when running in a circle and turning with controller, megaman's angle slowly gets off
	//				angle of camera and then snaps back when stopped
	//			seems like megaman rotates to the direction he got hit from
	// TODO: see if free aim shooting while normal walk is possible
	//			use a spider on ceiling or other high enemy to check for aim angle
	// TODO: disable auto-aim


	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	uint16_t camX = PS1_MEM_ReadHalfword(MML_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(MML_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 1.f;

	float dx = (float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	// clamp y-axis while walking
	// if (camYF > 700 && camYF < 32000)
	// 	camYF = 700;
	// if (camYF < 65510 && camYF > 32000)
	// 	camYF = 65510;

	PS1_MEM_WriteHalfword(MML_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(MML_CAMX2, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(MML_CAMY, (uint16_t)camYF);
	PS1_MEM_WriteHalfword(MML_CAMY_WALKING, (uint16_t)camYF);
}