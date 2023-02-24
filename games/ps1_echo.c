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

#define ECHO_CAMY 0x1A3DA0
#define ECHO_CAMX 0x1A3DA2
#define ECHO_CAN_MOVE_CAMERA 0x9FA84
#define ECHO_STANCE_CHANGE_AUTO_CENTER_1 0x1D2820
#define ECHO_STANCE_CHANGE_AUTO_CENTER_2 0x1D2822
#define ECHO_IN_THE_CLOCK 0x1A6CC0
#define ECHO_IN_INVENTORY 0x9F880
// #define ECHO_IN_CUTSCENE 0x1C0BFE
#define ECHO_IN_CUTSCENE 0x1FFF94
#define ECHO_IN_DRAWER 0x1D27D8

static uint8_t PS1_ECHO_Status(void);
static void PS1_ECHO_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Echo Night",
	PS1_ECHO_Status,
	PS1_ECHO_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_ECHONIGHT = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_ECHO_Status(void)
{
	return (PS1_MEM_ReadWord(0x92A4) == 0x534C5553U && PS1_MEM_ReadWord(0x92A8) == 0x5F303038U && PS1_MEM_ReadWord(0x92AC) == 0x2E32303BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_ECHO_Inject(void)
{
	// TODO: disable while in the clock

	// disable stance change auto-center
	PS1_MEM_WriteHalfword(ECHO_STANCE_CHANGE_AUTO_CENTER_1, 0x1);
	PS1_MEM_WriteHalfword(ECHO_STANCE_CHANGE_AUTO_CENTER_2, 0x0);

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// don't move camera during dialogue/inspection, in menu, during room transition
	if (!PS1_MEM_ReadHalfword(ECHO_CAN_MOVE_CAMERA))
		return;
	
	if (PS1_MEM_ReadHalfword(ECHO_IN_INVENTORY))
		return;
	
	// don't move camera little puzzles
	if (PS1_MEM_ReadHalfword(ECHO_IN_THE_CLOCK))
		return;

	if (PS1_MEM_ReadHalfword(ECHO_IN_CUTSCENE) == 0x1)
		return;

	if (PS1_MEM_ReadHalfword(ECHO_IN_DRAWER))
		return;
	
	uint16_t camX = PS1_MEM_ReadHalfword(ECHO_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(ECHO_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;

	float dx = -(float)xmouse * looksensitivity;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	// clamp y-axis
	if (camYF > 640 && camYF < 32000)
		camYF = 640;
	if (camYF < 64896 && camYF > 32000)
		camYF = 64896;

	PS1_MEM_WriteHalfword(ECHO_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(ECHO_CAMY, (uint16_t)camYF);
}