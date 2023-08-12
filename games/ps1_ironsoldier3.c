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

#define IS3_CAMY 0xA82DC
#define IS3_ROTX 0xA82E4
#define IS3_SANITY1 0xA803C

#define IS3_MISSILE_BASE_PTR 0xA8238
#define IS3_MISSILE_SANITY_VALUE 0x07000200
// offsets from missile base
#define IS3_MISSILE_SANITY 0x0
#define IS3_MISSILE_PITCH 0x20
#define IS3_MISSILE_ROLL 0x24
#define IS3_MISSILE_STATUS 0x07

#define IS3_MISSILE_IS_ALIVE 0x84

#define IS3_IS_NOT_PAUSED 0xA8B54

static uint8_t PS1_IS3_Status(void);
static void PS1_IS3_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Iron Soldier 3",
	PS1_IS3_Status,
	PS1_IS3_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_IRONSOLDIER3 = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static float pitchAccumulator = 0.f;
static float rollAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_IS3_Status(void)
{
	return (PS1_MEM_ReadWord(0x92A4) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x92A8) == 0x5F303130U && 
			PS1_MEM_ReadWord(0x92AC) == 0x2E36313BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_IS3_Inject(void)
{
	// TODO: advanced controls rotation
	//			move rotX when triangle is held, otherwise move camX
	// TODO: mouse control cruise missile
	//			cruise missile base
	//			sanity - [base]+0x0 = 0x07000200
	//				   - [base]+0x7	= 0x84 when alive

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!PS1_MEM_ReadByte(IS3_IS_NOT_PAUSED))
		return;
	
	if (PS1_MEM_ReadWord(IS3_SANITY1) != 0x41325C53)
		return;

	const float looksensitivity = (float)sensitivity / 20.f;
	
	uint32_t missileBase = PS1_MEM_ReadPointer(IS3_MISSILE_BASE_PTR);	
	if (missileBase &&
		PS1_MEM_ReadWord(missileBase + IS3_MISSILE_SANITY) == IS3_MISSILE_SANITY_VALUE &&
		PS1_MEM_ReadByte(missileBase + IS3_MISSILE_STATUS) == IS3_MISSILE_IS_ALIVE)
	{
		// control missile with mouse
		int16_t pitch = PS1_MEM_ReadInt16(missileBase + IS3_MISSILE_PITCH);
		int16_t roll = PS1_MEM_ReadInt16(missileBase + IS3_MISSILE_ROLL);
		float pitchF = (float)pitch;
		float rollF = (float)roll;

		float ym = (float)(invertpitch ? -ymouse : ymouse);
		float dPitch = ym * looksensitivity;
		AccumulateAddRemainder(&pitchF, &pitchAccumulator, ym, dPitch);

		float dRoll = -(float)xmouse * looksensitivity;
		AccumulateAddRemainder(&rollF, &rollAccumulator, -xmouse, dRoll);

		PS1_MEM_WriteInt16(missileBase + IS3_MISSILE_PITCH, (int16_t)pitchF);
		PS1_MEM_WriteInt16(missileBase + IS3_MISSILE_ROLL, (int16_t)rollF);
	}
	else
	{
		// control mech
		int32_t rotX = PS1_MEM_ReadInt(IS3_ROTX); // advanced mode rotate, normal mode camX
		int32_t camY = PS1_MEM_ReadInt(IS3_CAMY);
		float rotXF = (float)rotX;
		float camYF = (float)camY;

		float dx = -(float)xmouse * looksensitivity;
		AccumulateAddRemainder(&rotXF, &xAccumulator, -xmouse, dx);

		float ym = (float)(invertpitch ? -ymouse : ymouse);
		float dy = -ym * looksensitivity;
		AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);
		camYF = ClampFloat(camYF, -850.f, 850.f);

		PS1_MEM_WriteInt(IS3_ROTX, (int32_t)rotXF);
		PS1_MEM_WriteInt(IS3_CAMY, (int32_t)camYF);
	}
}