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

#define KZ_CAMX_BASE_PTR 0x12C1DA4
// offset from camXBase
#define KZ_CAMX_SIN 0x88
#define KZ_CAMX_COS 0x8C
#define KZ_ZOOM 0x44C

#define KZ_CAMY_BASE_PTR 0x12C1DA8
// offset from camYBase
#define KZ_CAMY 0x14C

#define KZ_CAN_ACT 0x13FC00

static uint8_t PS2_KZ_Status(void);
static void PS2_KZ_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Killzone",
	PS2_KZ_Status,
	PS2_KZ_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_KILLZONE = &GAMEDRIVER_INTERFACE;

static uint32_t camXBase = 0;
static uint32_t camYBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_KZ_Status(void)
{
	// SCUS_974.02
	return (PS2_MEM_ReadWord(0x55B58C) == 0x53435553U && 
			PS2_MEM_ReadWord(0x55B590) == 0x5F393734U &&
			PS2_MEM_ReadWord(0x55B594) == 0x2E30323BU);
}

static void PS2_KZ_Inject(void)
{
	// TODO: disable while
	//			paused
	//			on objective list
	//			vault
	//			loading (may be prevented by canAct value already)
	// TODO: camBase sanities
	// TODO: gun sway
	// TODO: ladder camera

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!PS2_MEM_ReadUInt(KZ_CAN_ACT))
		return;
	
	camXBase = PS2_MEM_ReadUInt(KZ_CAMX_BASE_PTR);
	camYBase = PS2_MEM_ReadUInt(KZ_CAMY_BASE_PTR);

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 250.f;
	// float fov = PS2_MEM_ReadFloat(RTCW_FOV) / 106.5f;
	// float fov = 1.f;
	float zoom = 1.f / PS2_MEM_ReadFloat(camXBase + KZ_ZOOM);

	float camXSin = PS2_MEM_ReadFloat(camXBase + KZ_CAMX_SIN);
	float camXCos = PS2_MEM_ReadFloat(camXBase + KZ_CAMX_COS);
	float angle = atan(camXSin / camXCos);
	if (camXCos < 0)
		angle += TAU / 2.f;
	
	angle -= (float)xmouse * looksensitivity / scale * zoom;
	camXSin = sin(angle);
	camXCos = cos(angle);

	PS2_MEM_WriteFloat(camXBase + KZ_CAMX_SIN, camXSin);
	PS2_MEM_WriteFloat(camXBase + KZ_CAMX_COS, camXCos);

	float camY = PS2_MEM_ReadFloat(camYBase + KZ_CAMY);
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * zoom;
	camY = ClampFloat(camY, -1.04719758f, 1.04719758f);
	PS2_MEM_WriteFloat(camYBase + KZ_CAMY, (float)camY);

}