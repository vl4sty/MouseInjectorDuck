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

#define GITS_CAMBASE 0x343CC0
// offsets from cambase
#define GITS_CAMX_COS 0x360
#define GITS_CAMX_SIN 0x368
#define GITS_CAM_TRANSFORM_C 0x370
#define GITS_CAM_TRANSFORM_D 0x374
#define GITS_CAM_TRANSFORM_E 0x378
#define GITS_CAM_TRANSFORM_F 0x380
#define GITS_CAMY 0x384
#define GITS_CAM_TRANSFORM_H 0x388

#define GITS_CAM_ZOOM 0x80F0

#define GITS_CAN_MOVE_CAMERA 0x472D84

// #define GITS_CAMY 0xDF8044
// #define GITS_CAMX_COS 0xDF8020
// #define GITS_CAMX_SIN 0xDF8028

static uint8_t PS2_GITS_Status(void);
static void PS2_GITS_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Ghost in the Shell: Stand Alone Complex",
	PS2_GITS_Status,
	PS2_GITS_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_GHOSTINTHESHELL = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;
static float lastCos = 0;
static float lastSin = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_GITS_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323130U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E30363BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_GITS_Inject(void)
{
	// TODO: camBase sanity
	// TODO: hacked entity cam base
	//			enemy soldiers
	//			camera
	//			based on playerBase?
	// TODO: disable during
	//			tutorial message
	//			hacking
	// 			calls
	// TODO: set turn animation for camX movement
	// TODO: clamp camX on ladders and ledges

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!PS2_MEM_ReadUInt(GITS_CAN_MOVE_CAMERA))
		return;

	camBase = PS2_MEM_ReadPointer(GITS_CAMBASE);

	float looksensitivity = (float)sensitivity / 40.f;
	float zoom = 1 / PS2_MEM_ReadFloat(camBase + GITS_CAM_ZOOM);

	float camY = PS2_MEM_ReadFloat(camBase + GITS_CAMY);
	camY += (float)ymouse * looksensitivity / 200.f * zoom;
	camY = ClampFloat(camY, -0.94f, 0.98f);

	float camXSin = PS2_MEM_ReadFloat(camBase + GITS_CAMX_SIN);
	float camXCos = PS2_MEM_ReadFloat(camBase + GITS_CAMX_COS);
	if (lastSin == 0)
	{
		lastSin = camXSin;
		lastCos = camXCos;
	}

	float angle = atan(camXSin / camXCos);

	angle -= (float)xmouse * looksensitivity / 200.f * zoom;
	if (camXCos < 0)
		angle += TAU / 2;
	// if other quadrants add pi

	camXSin = sin(angle);
	camXCos = cos(angle);

	float D = PS2_MEM_ReadFloat(camBase + GITS_CAM_TRANSFORM_D);

	PS2_MEM_WriteFloat(camBase + GITS_CAMX_SIN, (float)camXSin);
	PS2_MEM_WriteFloat(camBase + GITS_CAM_TRANSFORM_C, (float)camXSin * (float)camY);
	PS2_MEM_WriteFloat(camBase + GITS_CAM_TRANSFORM_E, (float)camXCos * -(float)camY);
	PS2_MEM_WriteFloat(camBase + GITS_CAM_TRANSFORM_F, -(float)camXSin * D);
	PS2_MEM_WriteFloat(camBase + GITS_CAMX_COS, (float)camXCos);
	PS2_MEM_WriteFloat(camBase + GITS_CAM_TRANSFORM_H, (float)camXCos * D);
	PS2_MEM_WriteFloat(camBase + GITS_CAMY, (float)camY);

	lastSin = camXSin;
	lastCos = camXCos;

}