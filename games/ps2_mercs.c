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

#define MERC_VEHICLE_ANGLE_UNSET -99.f

#define MERC_CAMBASE 0x501BF0 // changes to zoom base when zoomed
#define MERC_CAMBASE_SANITY_1_VALUE 0x90054C00
#define MERC_CAMBASE_SANITY_2_VALUE 0xCDCC4C3F
// offsets from cambase
#define MERC_CAMBASE_SANITY_1 0x40
#define MERC_CAMBASE_SANITY_2 0xB8
#define MERC_CAMY 0xE0
#define MERC_CAMX 0xEC
#define MERC_ZOOMY 0x50
#define MERC_ZOOMX 0x54
#define MERC_ZOOM_MULT 0x60
// #define MERC_FOV 0xA5C
#define MERC_VEHICLE_CAMX_SIN 0xFC
#define MERC_VEHICLE_CAMX_COS 0x104

#define MERC_IS_ZOOMED 0x559EFC
#define MERC_IS_DRIVING 0x558B14

static uint8_t PS2_MERC_Status(void);
static uint8_t PS2_MERC_DetectCambase(void);
static void PS2_MERC_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Mercenaries: Playground of Destruction",
	PS2_MERC_Status,
	PS2_MERC_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_MERCENARIES = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;
static float vehicleAngle = MERC_VEHICLE_ANGLE_UNSET;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_MERC_Status(void)
{
	// SLUS_209.32
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && PS2_MEM_ReadWord(0x00093394) == 0x5F323039U) &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E33323BU;
}

static uint8_t PS2_MERC_DetectCambase(void)
{
	uint32_t tempCamBase = PS2_MEM_ReadUInt(MERC_CAMBASE);
	// if (tempCamBase != 0)
	// {
		if (PS2_MEM_ReadWord(tempCamBase + MERC_CAMBASE_SANITY_1) == MERC_CAMBASE_SANITY_1_VALUE &&
			PS2_MEM_ReadWord(tempCamBase + MERC_CAMBASE_SANITY_2) == MERC_CAMBASE_SANITY_2_VALUE)
		{
			camBase = tempCamBase;
			return 1;
		}
	// }

	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_MERC_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	// if (!PS2_MERC_DetectCambase())
	// 	return;

	float looksensitivity = (float)sensitivity / 10000.f;

	if (PS2_MEM_ReadUInt(MERC_IS_ZOOMED))
	{
		// zoomed aiming
		camBase = PS2_MEM_ReadUInt(MERC_CAMBASE); // zoom base
		float fov = 1.f / PS2_MEM_ReadFloat(camBase + MERC_ZOOM_MULT);

		float camY = PS2_MEM_ReadFloat(camBase + MERC_ZOOMY);
		float camX = PS2_MEM_ReadFloat(camBase + MERC_ZOOMX);

		camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * fov;
		camX += (float)xmouse * looksensitivity * fov;

		PS2_MEM_WriteFloat(camBase + MERC_ZOOMY, (float)camY);
		PS2_MEM_WriteFloat(camBase + MERC_ZOOMX, (float)camX);
	}
	else if (PS2_MEM_ReadUInt(MERC_IS_DRIVING))
	{
		// TODO: turn off auto-faceforward

		// zoomed aiming
		camBase = PS2_MEM_ReadUInt(MERC_CAMBASE); // zoom base

		float camY = PS2_MEM_ReadFloat(camBase + MERC_CAMY);
		float camXSin = PS2_MEM_ReadFloat(camBase + MERC_VEHICLE_CAMX_SIN);
		float camXCos = PS2_MEM_ReadFloat(camBase + MERC_VEHICLE_CAMX_COS);

		// sin and cosine for the vehicle camX are stored in a strange format
		// 	they must be normalized to a -1 to 1 range
		float sinNorm = (camXSin - 1542.f) / 10.f;
		float cosNorm = (camXCos - 1848.f) / 10.f;

		if (vehicleAngle == MERC_VEHICLE_ANGLE_UNSET)
		{
			vehicleAngle = (float)atan(sinNorm / cosNorm);
		}

		float angleChange = (float)xmouse * looksensitivity;
		vehicleAngle -= angleChange;

		camXSin = (sin(vehicleAngle) * 10.f) + 1542.f;
		camXCos = (cos(vehicleAngle) * 10.f) + 1848.f;

		camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;
		// camX += (float)xmouse * looksensitivity * 8.f;

		PS2_MEM_WriteFloat(camBase + MERC_CAMY, (float)camY);
		PS2_MEM_WriteFloat(camBase + MERC_VEHICLE_CAMX_SIN, (float)camXSin);
		PS2_MEM_WriteFloat(camBase + MERC_VEHICLE_CAMX_COS, (float)camXCos);
	}
	else
	{
		camBase = PS2_MEM_ReadUInt(MERC_CAMBASE);

		// float fov = PS2_MEM_ReadFloat(camBase + CW_FOV);
		float fov = 1.f;

		float camY = PS2_MEM_ReadFloat(camBase + MERC_CAMY);
		float camX = PS2_MEM_ReadFloat(camBase + MERC_CAMX);
		// float camX = PS2_MEM_ReadFloat(camBase + MERC_VEHICLE_CAMX_SIN);

		camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * fov;
		camX -= (float)xmouse * looksensitivity * fov;

		PS2_MEM_WriteFloat(camBase + MERC_CAMY, (float)camY);
		PS2_MEM_WriteFloat(camBase + MERC_CAMX, (float)camX);
		// PS2_MEM_WriteFloat(camBase + MERC_VEHICLE_CAMX_SIN, camX);
	}

}