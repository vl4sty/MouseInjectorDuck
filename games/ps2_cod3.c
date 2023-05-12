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

#define COD3_CAMY 0x48FC64
#define COD3_CAMX 0x48FC68
#define COD3_AIM_ASSIST 0x47CB74
#define COD3_FOV 0x4C0430
#define COD3_CAN_MOVE_JEEP_3RD_PERSON_CAM 0x4C0474

#define COD3_IS_IN_GAME_CUTSCENE 0x49D858
#define COD3_IS_PAUSED 0x4A5CD0

static uint8_t PS2_COD3_Status(void);
static void PS2_COD3_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Call of Duty 3",
	PS2_COD3_Status,
	PS2_COD3_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_CALLOFDUTY3 = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_COD3_Status(void)
{
	// SLUS_214.26
	return (PS2_MEM_ReadWord(0x49C918) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x49C91C) == 0x5F323134U &&
			PS2_MEM_ReadWord(0x49C920) == 0x2E32363BU);
}

// static uint8_t PS2_COD3_DetectCam(void)
// {
// 	uint32_t tempCamBase = PS2_MEM_ReadUInt(RTCW_ACTUAL_CAMY_BASE_PTR);
// 	if (tempCamBase)
// 	{
// 		actualCamYBase = tempCamBase;
// 		return 1;
// 	}
// 	return 0;
// }

static void PS2_COD3_Inject(void)
{
	// TODO: 3rd person jeep camera
	//			does move but only moved manually with controller button
	//			works for a bit and then it rebounds to center and locks 
	// TODO: charge plant mini-game with mouse

	// disable aim-assist
	PS2_MEM_WriteUInt(COD3_AIM_ASSIST, 0x0);

	// PS2_MEM_WriteUInt(COD3_CAN_MOVE_JEEP_3RD_PERSON_CAM, 0x1);

	if (PS2_MEM_ReadUInt(COD3_IS_IN_GAME_CUTSCENE))
		return;

	if (PS2_MEM_ReadUInt(COD3_IS_PAUSED))
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// if (!PS2_RTCW_DetectCam())
	// 	return;

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 6.f;
	float fov = PS2_MEM_ReadFloat(COD3_FOV) / 65.f;

	float camX = PS2_MEM_ReadFloat(COD3_CAMX);
	camX -= (float)xmouse * looksensitivity / scale * fov;
	PS2_MEM_WriteFloat(COD3_CAMX, (float)camX);

	float camY = PS2_MEM_ReadFloat(COD3_CAMY);
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * fov;
	// game clamps internally to actual camY
	PS2_MEM_WriteFloat(COD3_CAMY, (float)camY);

}