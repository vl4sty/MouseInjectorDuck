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

#define AUF_CARGUN_TOTAL_ANGLE_UNSET -99.f
#define AUF_TANKGUN_TOTAL_ANGLE_UNSET -99.f

#define AUF_ONFOOT_CAMY 0x005064D0
#define AUF_ONFOOT_CAMX 0x005064D4
// #define AUF_ONFOOT_SANITY 0x005064F0
// #define AUF_ONFOOT_SANITY_VALUE 0x0000803F // not true when zoomed with sniper, holds a normalized FOV value?
#define AUF_ONFOOT_SANITY 0x005064D8
#define AUF_ONFOOT_SANITY_VALUE 0x0
#define AUF_CARGUN_CAMX_SIN 0x003CD6C0
// #define AUF_CARGUN_CAMX_COS 0x003CD6A0
#define AUF_CARGUN_CAMX_COS 0x003CD6C8
#define AUF_CARGUN_CAMY 0x003CD858
#define AUF_CARGUN_SANITY_1 0x3CD670
#define AUF_CARGUN_SANITY_1_VALUE 0x4D420000
#define AUF_CARGUN_SANITY_2 0x3CD674
#define AUF_CARGUN_SANITY_2_VALUE 0xA0030000 // not always A0, was 40 one time
#define AUF_CARGUN_CROSSHAIR_X 0x3CD840
#define AUF_CARGUN_CROSSHAIR_Y 0x3CD844
// #define AUF_CARGUN_AIMLOCK_1 0x3CD840
// #define AUF_CARGUN_AIMLOCK_2 0x3CD844
// #define AUF_CARGUN_AIMLOCK_3 0x3CD850
// #define AUF_CARGUN_AIMLOCK_4 0x3CD854
// #define AUF_CARGUN_AIMLOCK_5 0x3CD85C
// #define AUF_CARGUN_AIMLOCK_6 0x3CD860
// #define AUF_CARGUN_AIMLOCK_7 0x3CD864
// #define AUF_CARGUN_AIMLOCK_8 0x3CD858
#define AUF_health_lvl2 0x010C1C84

#define AUF_TANKGUN_CAMX_SIN 0x3BD600
#define AUF_TANKGUN_CAMX_COS 0x3BD608
#define AUF_TANKGUN_CAMY 0x3BD798
#define AUF_TANKGUN_SANITY_1 0x3BD5B0
#define AUF_TANKGUN_SANITY_1_VALUE 0x4D420000
#define AUF_TANKGUN_SANITY_2 0x3BD5B4
#define AUF_TANKGUN_SANITY_2_VALUE 0xA0030000

#define AUF_AIMLOCK_BASE 0x11D9A0 // requires sanity check as it blips values
#define AUF_AIMLOCK_SANITY 0x4
#define AUF_AIMLOCK_SANITY_VALUE 0xB8BD3F00
#define AUF_AIMLOCK 0x7B8
// #define AUF_AIMLOCK_1 0x7D4
// #define AUF_AIMLOCK_2 0x7D8
// #define AUF_AIMLOCK_3 0x7DC

#define AUF_FOV 0x385330

#define AUF_CROSSHAIR 0xC2815C
#define AUF_CROSSHAIR2 0x385300

// #define AUF_AIMLOCK_1 0x1A33264
// #define AUF_AIMLOCK_2 0x1A33268
// #define AUF_AIMLOCK_3 0x1A3326C


static uint8_t PS2_AUF_Status(void);
static void PS2_AUF_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"007: Agent Under Fire",
	PS2_AUF_Status,
	PS2_AUF_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_007AGENTUNDERFIRE = &GAMEDRIVER_INTERFACE;

static uint32_t aimLockBase = 0;
static float carGunTotalAngle = AUF_CARGUN_TOTAL_ANGLE_UNSET;
static float tankGunTotalAngle = AUF_TANKGUN_TOTAL_ANGLE_UNSET;
static uint8_t inGunCar = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_AUF_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && PS2_MEM_ReadWord(0x00093394) == 0x5F323032U) &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E36353BU;
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_AUF_Inject(void)
{
	// TODO: disable crosshair bob that happens when walking
	// TODO: disable carGun camera during in-game cutscenes and pause menu, will warp camera
	// TODO: unset tankgun total angle when switching weapons, causes next weapon to pop to location of previous

	// always small crosshair
	// PS2_MEM_WriteUInt(AUF_CROSSHAIR, 0x1);
	// PS2_MEM_WriteUInt(AUF_CROSSHAIR2, 0x1);

	if (aimLockBase == 0)
		aimLockBase = PS2_MEM_ReadPointer(AUF_AIMLOCK_BASE);
	
	if (PS2_MEM_ReadWord(aimLockBase + AUF_AIMLOCK_SANITY) == AUF_AIMLOCK_SANITY_VALUE)
	{
		// disable aimlock
		PS2_MEM_WriteUInt(aimLockBase + AUF_AIMLOCK, 0x0);
		// PS2_MEM_WriteUInt(aimLockBase + AUF_AIMLOCK_1, 0xFFFFFFFF);
		// PS2_MEM_WriteUInt(aimLockBase + AUF_AIMLOCK_2, 0x2);
		// PS2_MEM_WriteUInt(aimLockBase + AUF_AIMLOCK_3, 0x0);
	}
	else {
		aimLockBase = PS2_MEM_ReadPointer(AUF_AIMLOCK_BASE);
	}

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 180.f;


	if (PS2_MEM_ReadWord(AUF_TANKGUN_SANITY_1) == AUF_TANKGUN_SANITY_1_VALUE && PS2_MEM_ReadWord(AUF_TANKGUN_SANITY_2) == AUF_TANKGUN_SANITY_2_VALUE) { // in tank gun
		// TODO: if weapon swtiched, unset tankGunTotalAngle

		float camY = PS2_MEM_ReadFloat(AUF_TANKGUN_CAMY);
		float camXSin = PS2_MEM_ReadFloat(AUF_TANKGUN_CAMX_SIN);
		float camXCos = PS2_MEM_ReadFloat(AUF_TANKGUN_CAMX_COS);

		float angle = atan(camXSin / camXCos);

		if (tankGunTotalAngle == AUF_TANKGUN_TOTAL_ANGLE_UNSET) {
			tankGunTotalAngle = angle;
		}

		float angleChange = (float)xmouse * looksensitivity / 100.f;

		angle += angleChange;
		tankGunTotalAngle += angleChange;
		// if (abs(angle - tankGunTotalAngle) > TAU / 4)
		// 	tankGunTotalAngle = angle;

		while (tankGunTotalAngle > (TAU))
			tankGunTotalAngle -= TAU;
		while (tankGunTotalAngle < -(TAU))
			tankGunTotalAngle += TAU;

		camXSin = sin(tankGunTotalAngle);
		camXCos = cos(tankGunTotalAngle);

		PS2_MEM_WriteFloat(AUF_TANKGUN_CAMX_SIN, (float)camXSin);
		PS2_MEM_WriteFloat(AUF_TANKGUN_CAMX_COS, (float)camXCos);

		camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / 600.f;
		PS2_MEM_WriteFloat(AUF_TANKGUN_CAMY, (float)camY);
	}
	// else if (PS2_MEM_ReadWord(AUF_CARGUN_SANITY_1) == AUF_CARGUN_SANITY_1_VALUE && PS2_MEM_ReadWord(AUF_CARGUN_SANITY_2) == AUF_CARGUN_SANITY_2_VALUE) { // in gun car
	else if (PS2_MEM_ReadWord(AUF_CARGUN_SANITY_1) == AUF_CARGUN_SANITY_1_VALUE && PS2_MEM_ReadWord(AUF_CARGUN_SANITY_2) != 0x0) { // in gun car
		float camY = PS2_MEM_ReadFloat(AUF_CARGUN_CAMY);
		float camXSin = PS2_MEM_ReadFloat(AUF_CARGUN_CAMX_SIN);
		float camXCos = PS2_MEM_ReadFloat(AUF_CARGUN_CAMX_COS);

		float angle = atan(camXSin / camXCos);
		if (carGunTotalAngle == AUF_CARGUN_TOTAL_ANGLE_UNSET) {
			carGunTotalAngle = angle;
		}

		float angleChange = (float)xmouse * looksensitivity / 100.f;

		angle += angleChange;
		carGunTotalAngle += angleChange;

		camXSin = sin(carGunTotalAngle);
		camXCos = cos(carGunTotalAngle);

		PS2_MEM_WriteFloat(AUF_CARGUN_CAMX_SIN, (float)camXSin);
		PS2_MEM_WriteFloat(AUF_CARGUN_CAMX_COS, (float)camXCos);

		camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / 600.f;
		PS2_MEM_WriteFloat(AUF_CARGUN_CAMY, (float)camY);
	}
	else if (PS2_MEM_ReadWord(AUF_ONFOOT_SANITY) == AUF_ONFOOT_SANITY_VALUE) { // on foot
		tankGunTotalAngle = AUF_TANKGUN_TOTAL_ANGLE_UNSET;
		carGunTotalAngle = AUF_CARGUN_TOTAL_ANGLE_UNSET;

		float fov = PS2_MEM_ReadFloat(AUF_FOV);
		float camX = PS2_MEM_ReadFloat(AUF_ONFOOT_CAMX);
		float camY = PS2_MEM_ReadFloat(AUF_ONFOOT_CAMY);

		camX -= (float)xmouse * looksensitivity / 2.f * (fov / 60);
		camY += (float)ymouse * looksensitivity / 2.f * (fov / 60);

		PS2_MEM_WriteFloat(AUF_ONFOOT_CAMX, (float)camX);
		PS2_MEM_WriteFloat(AUF_ONFOOT_CAMY, (float)camY);
	}

}