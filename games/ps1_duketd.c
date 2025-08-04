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
#include <math.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define DN3D_CAMX 0xEC92C
#define DN3D_CAMY 0xEC8C8
#define DN3D_CAMY2 0xEC8CC
#define DN3D_IS_PAUSED_FLAG 0xEC9E8
#define DN3D_IS_CAMERA_FLAG 0xEC964
#define DN3D_IS_DUKEKO_FLAG1 0xEC9EA
#define DN3D_IS_DUKEKO_FLAG2 0xEC936
#define DN3D_FORCE_CONTROLLER_PRESET 0xB5402

static uint8_t PS1_DN3D_Status(void);
static void PS1_DN3D_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Duke Nukem: Total Meltdown",
	PS1_DN3D_Status,
	PS1_DN3D_Inject,
	1, // 1000 Hz tickrate
	0, // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_DUKE_3D = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static uint16_t Pause_Flag = 0;
static uint16_t Camera_Flag = 0;
static uint16_t DKO_Flag1 = 0;
static uint16_t DKO_Flag2 = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_DN3D_Status(void)
{	
	return (PS1_MEM_ReadWord(0x9394) == 0x534C5553U &&
			PS1_MEM_ReadWord(0x9398) == 0x5F303033U &&
			PS1_MEM_ReadWord(0x939C) == 0x2E35353BU); // SLUS_003.55;
}

static void PS1_DN3D_Inject(void)
{
	Pause_Flag = PS1_MEM_ReadByte(DN3D_IS_PAUSED_FLAG); //0x01 RIP, 0x05 Pause menu, 0x00 Demo mode, 0x04 ingame
	Camera_Flag = PS1_MEM_ReadHalfword(DN3D_IS_CAMERA_FLAG);
	DKO_Flag1 = PS1_MEM_ReadHalfword(DN3D_IS_DUKEKO_FLAG1); //logs other dukes states
	DKO_Flag2 = PS1_MEM_ReadHalfword(DN3D_IS_DUKEKO_FLAG2); //dukes health
	PS1_MEM_WriteByte(DN3D_FORCE_CONTROLLER_PRESET, 0x02); //set DOOMED preset

	if (xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	else if (Pause_Flag != 0x04 || Camera_Flag < 0xFFFF || DKO_Flag2 == 0x0000)
	{
		return;
	}
	else
	{
		uint16_t camX = PS1_MEM_ReadHalfword(DN3D_CAMX);
		int16_t camY = PS1_MEM_ReadHalfword(DN3D_CAMY);

		float camXF = (float)camX;
		float camYF = (float)camY;

		const float looksensitivityX = (float)sensitivity / 35.f;
		const float looksensitivityY = (float)sensitivity / 70.f;
		const float scale = 1.f;
		float ym = (float)(invertpitch ? -ymouse : ymouse);
		float dy = -ym * looksensitivityY * scale;
		float dx = (float)xmouse * looksensitivityX * scale;
		AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);
		AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);

		// Range X
		while (camXF > 2048)
			camXF -= 2048;
		while (camXF < 0)
			camXF += 2048;

		// Clamp Y
		if (camYF < -99)
			camYF = -99;
		if (camYF > 299)
			camYF = 299;

		PS1_MEM_WriteHalfword(DN3D_CAMX, (uint16_t)camXF);
		PS1_MEM_WriteInt(DN3D_CAMY, (int32_t)camYF);
		PS1_MEM_WriteInt(DN3D_CAMY2, (int32_t)camYF);
	}
}