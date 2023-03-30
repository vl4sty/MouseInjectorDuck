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

#define AH_CAMX 0x13E7AE
#define AH_CAMY 0x13E84C
#define AH_ROT_SPEED_X 0x13E854
#define AH_ROT_SPEED_Y 0x13E844

#define AH_MAX_ROT_SPEED_X 4840.f
#define AH_MAX_ROT_SPEED_Y 4096.f

#define AH_CONTROL_TYPE_DIRECT 1
#define AH_CONTROL_TYPE_INDIRECT 2

static uint8_t PS1_AH_Status(void);
static void PS1_AH_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Aquanaut's Holiday",
	PS1_AH_Status,
	PS1_AH_Inject,
	1, // 1000 Hz tickrate
	0, // crosshair sway supported for driver
	"Camera Control Type: Direct",  // control type option
	"Camera Control Type: Indirect",  // control type option
};

const GAMEDRIVER *GAME_PS1_AQUANAUTSHOLIDAY = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static uint8_t controlType = AH_CONTROL_TYPE_DIRECT;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_AH_Status(void)
{
	return (PS1_MEM_ReadWord(0x925C) == 0x53435553U && 
			PS1_MEM_ReadWord(0x9260) == 0x5F393436U && 
			PS1_MEM_ReadWord(0x9264) == 0x2E30333BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_AH_Inject(void)
{
	// TODO: seperate cheats for each control type

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (optionToggle)
		controlType = AH_CONTROL_TYPE_INDIRECT;
	else
		controlType = AH_CONTROL_TYPE_DIRECT;

	
	if (controlType == AH_CONTROL_TYPE_DIRECT)
	{
		// TODO: don't disable zero
		// TODO: clampY

		// set rotation speeds to 0 to avoid drifting
		PS1_MEM_WriteInt(AH_ROT_SPEED_X, 0);
		PS1_MEM_WriteInt(AH_ROT_SPEED_Y, 0);

		uint16_t camX = PS1_MEM_ReadHalfword(AH_CAMX);
		int32_t camY = PS1_MEM_ReadInt(AH_CAMY);
		float camXF = (float)camX;
		float camYF = (float)camY;

		const float looksensitivity = (float)sensitivity / 20.f;

		float dx = (float)xmouse * looksensitivity;
		AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

		float ym = (float)(invertpitch ? ymouse : -ymouse);
		float dy = ym * looksensitivity * 20.f;
		AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

		// clamp y-axis
		camYF = ClampFloat(camYF, -6000, 6000);

		PS1_MEM_WriteHalfword(AH_CAMX, (uint16_t)camXF);
		PS1_MEM_WriteInt(AH_CAMY, (uint32_t)camYF);
	}
	else if (controlType == AH_CONTROL_TYPE_INDIRECT)
	{
		int32_t rotSpeedX = PS1_MEM_ReadInt(AH_ROT_SPEED_X);
		int32_t rotSpeedY = PS1_MEM_ReadInt(AH_ROT_SPEED_Y);
		float rotSpeedXF = (float)rotSpeedX;
		float rotSpeedYF = (float)rotSpeedY;

		const float looksensitivity = (float)sensitivity;

		float dx = (float)xmouse * looksensitivity;
		AccumulateAddRemainder(&rotSpeedXF, &xAccumulator, xmouse, dx);

		float ym = (float)(invertpitch ? ymouse : -ymouse);
		float dy = ym * looksensitivity;
		AccumulateAddRemainder(&rotSpeedYF, &yAccumulator, ym, dy);

		if (rotSpeedXF > AH_MAX_ROT_SPEED_X)
			rotSpeedXF = AH_MAX_ROT_SPEED_X;
		if (rotSpeedXF < -AH_MAX_ROT_SPEED_X)
			rotSpeedXF = -AH_MAX_ROT_SPEED_X;
		if (rotSpeedYF > AH_MAX_ROT_SPEED_Y)
			rotSpeedYF = AH_MAX_ROT_SPEED_Y;
		if (rotSpeedYF < -AH_MAX_ROT_SPEED_Y)
			rotSpeedYF = -AH_MAX_ROT_SPEED_Y;

		PS1_MEM_WriteInt(AH_ROT_SPEED_X, (int32_t)rotSpeedXF);
		PS1_MEM_WriteInt(AH_ROT_SPEED_Y, (int32_t)rotSpeedYF);
	}
}