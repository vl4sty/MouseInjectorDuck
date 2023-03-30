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

#define HN_CAMX 0xAD89A
#define HN_CAMY 0xADA0C

#define HN_CAN_MOVE_CAMERA 0x7E738
#define HN_IN_ROOM 0x7420C
#define HN_IS_MONSTER_CUTSCENE 0xAD86C
#define HN_IS_MAP_DISPLAYED 0x7D9CC

static uint8_t PS1_HN_Status(void);
static void PS1_HN_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Hellnight",
	PS1_HN_Status,
	PS1_HN_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_HELLNIGHT = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static uint8_t wasInRoom = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_HN_Status(void)
{
	return (PS1_MEM_ReadWord(0x9244) == 0x534C4553U && 
			PS1_MEM_ReadWord(0x9248) == 0x5F303135U && 
			PS1_MEM_ReadWord(0x924C) == 0x2E36323BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_HN_Inject(void)
{
	// TODO: Disable during
	//			map
	//			quick turn
	// TODO: reset Y to zero
	//			when leaving room
	//			during conversation
	// FIXME: moving mouse during quickturn will slow walk speed

	if (PS1_MEM_ReadUInt(HN_IN_ROOM)) {
		wasInRoom = 1;
		return;
	}
	else if (wasInRoom) {
		PS1_MEM_WriteHalfword(HN_CAMY, 0x0);
		wasInRoom = 0;
	}

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!PS1_MEM_ReadUInt(HN_CAN_MOVE_CAMERA))
		return;

	if (PS1_MEM_ReadUInt(HN_IS_MONSTER_CUTSCENE))
		return;

	if (PS1_MEM_ReadUInt(HN_IS_MAP_DISPLAYED))
		return;

	uint16_t camX = PS1_MEM_ReadHalfword(HN_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(HN_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;

	float dx = -(float)xmouse * looksensitivity;
	AccumulateAddRemainder(&camXF, &xAccumulator, -xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	// clamp y-axis
	if (camYF > 682 && camYF < 32000)
		camYF = 682;
	if (camYF < 64626 && camYF > 32000)
		camYF = 64626;

	PS1_MEM_WriteHalfword(HN_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(HN_CAMY, (uint16_t)camYF);
}