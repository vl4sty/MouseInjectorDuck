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

// #define CA_CAMY 0xF886A4
// #define CA_CAMX 0xF8869C
#define CA_CAMBASE 0x1BC9074
#define CA_CAMBASE_SANITY_1_VALUE 0xFFFFFFFF
#define CA_CAMBASE_SANITY_2_VALUE 0x9476A808 // might just be a pointer so may need something different
// offsets from cambase
#define CA_CAMBASE_SANITY_1 0x6C
#define CA_CAMBASE_SANITY_2 0xC0
#define CA_CAMY 0x524
#define CA_CAMX 0x51C
// #define CA_ZOOM 0x23BC

#define CA_ZOOM 0xA8ADD0

#define CA_IS_BUSY 0xF0D90A
#define CA_IS_BUSY2 0x1BF0B88
// #define CA_IS_BUSY 0x1BF0B8C
// #define CA_IS_MAP_DISPLAYED 0xA8C710
#define CA_MENU_TYPE 0xF0D8FC
// #define CA_CAN_AIM 0x010C0108


static uint8_t PSP_CA_Status(void);
static uint8_t PSP_CA_DetectCam(void);
static void PSP_CA_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Coded Arms",
	PSP_CA_Status,
	PSP_CA_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PSP_CODEDARMS = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PSP_CA_Status(void)
{
	return (PSP_MEM_ReadWord(0xA5D7C4) == 0x554C5553U &&
			PSP_MEM_ReadWord(0xA5D7C8) == 0x31303031U && 
			PSP_MEM_ReadWord(0xA5D7CC) == 0x39000000U);
}

static uint8_t PSP_CA_DetectCam(void)
{
	uint32_t tempCamBase = PSP_MEM_ReadUInt(CA_CAMBASE) - 0x8000000;
	if (PSP_MEM_ReadWord(tempCamBase + CA_CAMBASE_SANITY_1) == CA_CAMBASE_SANITY_1_VALUE &&
		PSP_MEM_ReadWord(tempCamBase + CA_CAMBASE_SANITY_2) == CA_CAMBASE_SANITY_2_VALUE)
	{
		camBase = tempCamBase;
		return 1;
	}

	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PSP_CA_Inject(void)
{
	// TODO: look for 2-byte isBusy value

	if (xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// paused, message, confirmation, plugin pickup, etc..
	uint32_t isBusy = PSP_MEM_ReadUInt16(CA_IS_BUSY);
	if (isBusy != 0x4120 && isBusy != 0x3F80)
		return;
	if (PSP_MEM_ReadUInt(CA_IS_BUSY2) == 0x10)
		return;
	
	// if (PSP_MEM_ReadUInt(CA_MENU_TYPE) != 0x010C0108)
	// 	return;
	
	if (!PSP_CA_DetectCam())
		return;

	float camX = PSP_MEM_ReadFloat(camBase + CA_CAMX);
	float camY = PSP_MEM_ReadFloat(camBase + CA_CAMY);

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 600.f;
	const float zoom = PSP_MEM_ReadFloat(CA_ZOOM) / 1.08f;
	// const float zoom = 1.f;

	camX -= (float)xmouse * looksensitivity / scale / zoom;
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale / zoom;

	while (camX > TAU / 2)
		camX -= TAU;
	while (camX < -TAU / 2)
		camX += TAU;
	camY = ClampFloat(camY, -1.308997035, 1.308997035);

	PSP_MEM_WriteFloat(camBase + CA_CAMX, (float)camX);
	PSP_MEM_WriteFloat(camBase + CA_CAMY, (float)camY);
}