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

#define TAU 6.2831853f // 0x40C90FDB

#define TTK_CAMY 0xD72C0
#define TTK_CAMX_SIN 0xD72BC
#define TTK_CAMX_COS 0xD72C4
#define TTK_CAMX_ROT 0xD71B4
#define TTK_CAMY_ROT 0xD71B6
#define TTK_IS_UNDERWATER 0xDD7D0

#define TTK_ACTION 0xD73C4
#define TTK_ACTION_CLIMBING 0x0303
#define TTK_ACTION_FLOATING 0x0404
#define TTK_ACTION_SWIMMING 0x0505
#define TTK_ACTION_JUMPING 0x0909
#define TTK_ACTION_JETPACK 0x0A0A
//	0000 walking
//	0303 climbing (ladder, net)
//	0505 swimming
//	0808 climbing mount/dismount
//	0909 jumping/falling

#define TTK_CLIMB_MEDIUM 0xD71FD

static uint8_t PS1_TTK_Status(void);
static uint8_t PS1_TTK_CanRotate(void);
static void PS1_TTK_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Duke Nukem: Time to Kill",
	PS1_TTK_Status,
	PS1_TTK_Inject,
	1, // 1000 Hz tickrate
	0, // crosshair sway supported for driver
	"[ON] Rotation allowed during jump",
	"[OFF] Rotation allowed during jump"
};

const GAMEDRIVER *GAME_PS1_DUKETIMETOKILL = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_TTK_Status(void)
{
	return (PS1_MEM_ReadWord(0x92D4) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x92D8) == 0x5F303035U && 
			PS1_MEM_ReadWord(0x92DC) == 0x2E38333BU); // SLUS_005.83;
}

static uint8_t PS1_TTK_CanRotate(void)
{
	const uint16_t action = PS1_MEM_ReadHalfword(TTK_ACTION);

	if (action == TTK_ACTION_JUMPING) {
		if (!optionToggle)
			return 1;
		else
			return 0;
	}

	switch (action) {
		case TTK_ACTION_SWIMMING:
		case TTK_ACTION_FLOATING:
		case TTK_ACTION_JETPACK:
			return 1;
	}

	if (action == TTK_ACTION_CLIMBING)
	{
		// on chain?
		if (PS1_MEM_ReadByte(TTK_CLIMB_MEDIUM) == 0x9)
			return 1;
		else
			return 0;
	}

	if (!action)
		return 1;

	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_TTK_Inject(void)
{
	// TODO: disable 1st-person to previous 3rd-person snap

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 3.f;

	int32_t camY = PS1_MEM_ReadInt(TTK_CAMY);
	float camYF = (float)camY;
	float ym = (float)(invertpitch ? -ymouse : ymouse);
	camYF += ym * looksensitivity / 0.5f * scale;

	camYF = ClampFloat(camYF, -4070.f, 4070.f);

	int32_t camXSin = PS1_MEM_ReadInt(TTK_CAMX_SIN);
	int32_t camXCos = PS1_MEM_ReadInt(TTK_CAMX_COS);
	float camXSinF = (float)camXSin / 4096.f;
	float camXCosF = (float)camXCos / 4096.f;

	float angle = atan(camXSinF / camXCosF);
	if (camXCosF < 0)
		angle -= TAU / 2.f;

	angle += (float)xmouse * looksensitivity / 2592.f * scale;

	camXSinF = sin(angle) * 4096;
	camXCosF = cos(angle) * 4096;

	if (PS1_TTK_CanRotate())
	{
		uint16_t camXRot = PS1_MEM_ReadHalfword(TTK_CAMX_ROT);
		float camXRotF = (float)camXRot;
		float dx = (float)xmouse * looksensitivity / 4.f * scale;
		AccumulateAddRemainder(&camXRotF, &xAccumulator, (float)xmouse, dx);

		while (camXRotF > 4096)
			camXRotF -= 4096;
		while (camXRotF < 0)
			camXRotF += 4096;

		PS1_MEM_WriteHalfword(TTK_CAMX_ROT, (uint16_t)camXRotF);
	}

	// rotate Y (broken for onFoot)
	if (PS1_MEM_ReadUInt(TTK_IS_UNDERWATER))
	{
		int16_t camYRot = PS1_MEM_ReadInt16(TTK_CAMY_ROT);
		float camYRotF = (float)camYRot;
		float dy = -(float)ymouse * looksensitivity / 4.5f * scale;
		AccumulateAddRemainder(&camYRotF, &yAccumulator, -(float)ymouse, dy);

		camYRotF = ClampFloat(camYRotF, -900.f, 900.f);

		PS1_MEM_WriteInt16(TTK_CAMY_ROT, (int16_t)camYRotF);
	}

	PS1_MEM_WriteInt(TTK_CAMX_SIN, (int32_t)camXSinF);
	PS1_MEM_WriteInt(TTK_CAMX_COS, (int32_t)camXCosF);
	PS1_MEM_WriteInt(TTK_CAMY, (int32_t)camYF);
}