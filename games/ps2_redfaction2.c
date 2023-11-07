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

#define TAU 6.2831853f // 0x40C90FDB

#define RF2_CAMBASEPTR 0x334878
#define RF2_CAMBASE_SANITY_1_VALUE 0x19FCFFFF
#define RF2_CAMBASE_SANITY_2_VALUE 0xFFFFFFFF
// offsets from camBase
#define RF2_CAMBASE_SANITY_1 0x8
#define RF2_CAMBASE_SANITY_2 0x14
#define RF2_CAMX 0x288
#define RF2_CAMY 0x29C

#define RF2_IS_PAUSED 0x334A10

#define RF2_IS_TURRET 0x1D7C1CC
#define RF2_TURRET_Y 0xC4E7A8
#define RF2_TURRET_X 0xC4E7AC

static uint8_t PS2_RF2_Status(void);
static uint8_t PS2_RF2_DetectCambase(void);
static void PS2_RF2_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Red Faction II",
	PS2_RF2_Status,
	PS2_RF2_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_REDFACTION2 = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_RF2_Status(void)
{
	// SLUS_204.42
	return (PS2_MEM_ReadWord(0x93390) == 0x534C5553 &&
			PS2_MEM_ReadWord(0x93394) == 0x5F323034 &&
			PS2_MEM_ReadWord(0x93398) == 0x2E34323B);
}

static uint8_t PS2_RF2_DetectCambase(void)
{
	uint32_t tempCamBase = PS2_MEM_ReadPointer(RF2_CAMBASEPTR);
	if (tempCamBase &&
		PS2_MEM_ReadWord(tempCamBase + RF2_CAMBASE_SANITY_1) == RF2_CAMBASE_SANITY_1_VALUE &&
		PS2_MEM_ReadWord(tempCamBase + RF2_CAMBASE_SANITY_2) == RF2_CAMBASE_SANITY_2_VALUE)
	{
		camBase = tempCamBase;
		return 1;
	}

	return 0;
}

static void PS2_RF2_Inject(void)
{
	// TODO: test isTurret after reset
	// TODO: fix aim-assist cheat not working until camera moved manually with controller
	//			or find aim-assist option in game
	// TODO: test entire game

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (PS2_MEM_ReadUInt(RF2_IS_PAUSED) == 0x1)
		return;
	
	float looksensitivity = (float)sensitivity;
	float scale = 10000.f;

	if (PS2_MEM_ReadUInt(RF2_IS_TURRET))
	{
		float turretX = PS2_MEM_ReadFloat(RF2_TURRET_X);
		float turretY = PS2_MEM_ReadFloat(RF2_TURRET_Y);

		turretX -= (float)xmouse * looksensitivity / scale;
		turretY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;

		PS2_MEM_WriteFloat(RF2_TURRET_X, turretX);
		PS2_MEM_WriteFloat(RF2_TURRET_Y, turretY);
	}
	else
	{
		if (!PS2_RF2_DetectCambase())
			return;

		float camX = PS2_MEM_ReadFloat(camBase + RF2_CAMX);
		float camY = PS2_MEM_ReadFloat(camBase + RF2_CAMY);

		camX += (float)xmouse * looksensitivity / scale;
		while (camX > TAU)
			camX -= TAU;
		while (camX < -TAU)
			camX += TAU;

		camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;
		camY = ClampFloat(camY, -1.570796371f, 1.570796371f);

		PS2_MEM_WriteFloat(camBase + RF2_CAMX, camX);
		PS2_MEM_WriteFloat(camBase + RF2_CAMY, camY);
	}
}