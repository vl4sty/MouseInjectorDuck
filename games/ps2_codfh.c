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

#define CODFH_CURRENT_LEVEL 0x113E10
#define CODFH_IS_PAUSED 0x3E291C // 0x6=true

#define CODFH_ONFOOT_CAMBASE 0x4968DC // same as player base
#define CODFH_ONFOOT_SANITY_VALUE 0xE7FF2F03
// ---- offsets from onFoot cambase ----
#define CODFH_ONFOOT_SANITY 0x20
#define CODFH_ONFOOT_CAMY 0x520
#define CODFH_ONFOOT_CAMX_BASE 0x330 // pointer
#define CODFH_ONFOOT_FOV 0x524
#define CODFH_ONRAILS_CAMX 0xE4
// #define CODFH_ONRAILS_SANITY 0xC0 // old, not working
#define CODFH_ONRAILS_SANITY 0x420 // 0x1=onRails, 0x2=not
#define CODFH_HEALTH 0x19BC
// ---- offsets from camX base ----
#define CODFH_ONFOOT_CAMX_COS_1 0x10 
#define CODFH_ONFOOT_CAMX_SIN_1 0X18
#define CODFH_ONFOOT_CAMX_SIN_2 0X30
#define CODFH_ONFOOT_CAMX_COS_2 0X38
#define CODFH_ONFOOT_CAMX_SIN_SKEW 0x14
#define CODFH_ONFOOT_CAMX_COS_SKEW 0x34

#define CODFH_ONFOOT_TOTAL_ANGLE_UNSET -99.f // internal angle

#define CODFH_TURRET_BASE 0x3E32C0 // stack value? that blips though different pointers, eventually leading to current turret?
#define CODFH_ON_TURRET 0x843E64 // player on turret?
#define CODFH_TURRET_BASE_SANITY_1_VALUE 0x3
#define CODFH_TURRET_BASE_SANITY_2_VALUE 0x13
// -- offsets from turret base
#define CODFH_TURRET_BASE_SANITY_1 0x10
#define CODFH_TURRET_BASE_SANITY_2 0x20
#define CODFH_TURRET_OCCUPIED 0x5B0
#define CODFH_TURRET_CAMY 0x3A8
#define CODFH_TURRET_CAMX 0x3AC


// tank base is generally held in one of these locations
#define CODFH_TANK_BASE_1 0x496910
#define CODFH_TANK_BASE_2 0x49694C
#define CODFH_TANK_BASE_3 0x5048DC
#define CODFH_WHICH_TANK_BASE -0x1C
#define CODFH_TANK_IS_HAND_AIMING 0x831B70
#define CODFH_TANK_SANITY_VALUE 0xB791D542 // sanity not valid for american tank
#define CODFH_TANK_SANITY_VALUE_2 0x7309D342
// offsets from tank base
#define CODFH_TANK_SANITY 0x30
// #define CODFH_TANK_IS_HAND_AIMING 0x5CC // non-zero equals true, not working?
#define CODFH_IN_TANK 0x6E0
#define CODFH_TANK_CAMX 0x2848
#define CODFH_TANK_CAMY 0x2850
#define CODFH_TANK_HAND_CAMX 0x2870
#define CODFH_TANK_HAND_CAMY 0x287C

static uint8_t PS2_CODFH_Status(void);
static uint8_t PS2_CODFH_DetectCambase(void);
static void PS2_CODFH_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Call of Duty: Finest Hour",
	PS2_CODFH_Status,
	PS2_CODFH_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_CODFINESTHOUR = &GAMEDRIVER_INTERFACE;

static uint32_t playerBase = 0;
static uint32_t onFootCamBase = 0;
static float onFootTotalAngle = CODFH_ONFOOT_TOTAL_ANGLE_UNSET;
static uint32_t turretBase;
static uint32_t tankBase;
static int tankBaseArraySize = 3;
static uint32_t tankBaseArray[] =  {CODFH_TANK_BASE_1,
									CODFH_TANK_BASE_2,
									CODFH_TANK_BASE_3};
static uint32_t currentLevel = 0x9999;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_CODFH_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && PS2_MEM_ReadWord(0x00093394) == 0x5F323037U) &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E32353BU;
}

static uint8_t PS2_CODFH_DetectCambase(void)
{
	uint32_t tempcambase = PS2_MEM_ReadPointer(CODFH_ONFOOT_CAMBASE);
	if (PS2_MEM_ReadWord(tempcambase + CODFH_ONFOOT_SANITY) == CODFH_ONFOOT_SANITY_VALUE)
	{
		onFootCamBase = tempcambase;
		return 1;
	}

	onFootTotalAngle = CODFH_ONFOOT_TOTAL_ANGLE_UNSET;
	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_CODFH_Inject(void)
{
	// TODO: fix hand aiming popping after full auto kickback
	//			usually the aim would lerp back to the original aiming direction but with mouse the angle
	//			is not overridden by the gun kickback

	// reset variables when dead or level changed
	if (PS2_MEM_ReadUInt(onFootCamBase + CODFH_HEALTH) == 0x0 || PS2_MEM_ReadUInt(CODFH_CURRENT_LEVEL) != currentLevel)
	{
		onFootTotalAngle = CODFH_ONFOOT_TOTAL_ANGLE_UNSET;
	}	

	currentLevel = PS2_MEM_ReadUInt(CODFH_CURRENT_LEVEL);

	if (PS2_MEM_ReadUInt(CODFH_IS_PAUSED) == 0x6) // don't update if paused
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!PS2_CODFH_DetectCambase()) // check for valid cambase
		return;

	float fov = PS2_MEM_ReadFloat(onFootCamBase + CODFH_ONFOOT_FOV) / 0.7f; // 0.7 is walking around FOV
	float looksensitivity = (float)sensitivity / 140.f;

	if (turretBase == 0x0 || PS2_MEM_ReadUInt(turretBase + CODFH_TURRET_OCCUPIED) == 0x0)
	{
		turretBase = PS2_MEM_ReadPointer(CODFH_TURRET_BASE);

		if (PS2_MEM_ReadUInt(turretBase + CODFH_TURRET_BASE_SANITY_1) != CODFH_TURRET_BASE_SANITY_1_VALUE ||
			PS2_MEM_ReadUInt(turretBase + CODFH_TURRET_BASE_SANITY_2) != CODFH_TURRET_BASE_SANITY_2_VALUE ||
			PS2_MEM_ReadUInt(turretBase + CODFH_TURRET_OCCUPIED) == 0x0)
			turretBase = 0;
	}

	tankBase = 0;
	int i;
	for (i = 0; i < tankBaseArraySize; ++i) // loop through addresses where valid tankBases may be held
	{
		tankBase = PS2_MEM_ReadPointer(tankBaseArray[i]);
		if (PS2_MEM_ReadWord(tankBase + 0x44) == 0xFFFFFFFF && PS2_MEM_ReadWord(tankBase) != 0x0) // old tank base on lvl 16
			continue;
		if (PS2_MEM_ReadUInt(tankBase + CODFH_WHICH_TANK_BASE) != 0xFFFFFFFF && PS2_MEM_ReadUInt(tankBase + CODFH_WHICH_TANK_BASE) != 0x1) // check if base is not valid
			break;
	}

	// check tankBase sanity for one of two values
	uint8_t tankIsValid = 0;
	if (PS2_MEM_ReadWord(tankBase + CODFH_TANK_SANITY) == CODFH_TANK_SANITY_VALUE)
		tankIsValid = 1;
	if (PS2_MEM_ReadWord(tankBase + CODFH_TANK_SANITY) == CODFH_TANK_SANITY_VALUE_2)
		tankIsValid = 1;

	if (tankIsValid && PS2_MEM_ReadUInt(tankBase + CODFH_IN_TANK) == 0x1) {
		float tankCamX, tankCamY;
		uint32_t isHandAiming = PS2_MEM_ReadUInt(CODFH_TANK_IS_HAND_AIMING);
		uint32_t camXAddress, camYAddress;
		if (isHandAiming == 0x1) { // aiming gun from top of tank
			camXAddress = tankBase + CODFH_TANK_HAND_CAMX;
			camYAddress = tankBase + CODFH_TANK_HAND_CAMY;
		}
		else { // aiming barrel, 1st and 3rd person
			camXAddress = tankBase + CODFH_TANK_CAMX;
			camYAddress = tankBase + CODFH_TANK_CAMY;
		}

		tankCamX = PS2_MEM_ReadFloat(camXAddress);
		tankCamY = PS2_MEM_ReadFloat(camYAddress);

		tankCamX -= (float)xmouse * looksensitivity / 2.f * fov;
		tankCamY += (float)ymouse * looksensitivity / 2.f * fov;

		PS2_MEM_WriteFloat(camXAddress, tankCamX);
		PS2_MEM_WriteFloat(camYAddress, tankCamY);
	}
	else if (PS2_MEM_ReadUInt(CODFH_ON_TURRET) == 0x2) {
		onFootTotalAngle = CODFH_ONFOOT_TOTAL_ANGLE_UNSET;

		if (PS2_MEM_ReadUInt(turretBase + CODFH_TURRET_BASE_SANITY_1) != CODFH_TURRET_BASE_SANITY_1_VALUE ||
			PS2_MEM_ReadUInt(turretBase + CODFH_TURRET_BASE_SANITY_2) != CODFH_TURRET_BASE_SANITY_2_VALUE ||
			PS2_MEM_ReadUInt(turretBase + CODFH_TURRET_OCCUPIED) == 0x0)
			return;

		float turretCamX = PS2_MEM_ReadFloat(turretBase + CODFH_TURRET_CAMX);
		float turretCamY = PS2_MEM_ReadFloat(turretBase + CODFH_TURRET_CAMY);

		turretCamX -= (float)xmouse * looksensitivity / 2.f * fov;
		turretCamY += (float)ymouse * looksensitivity / 2.f * fov;

		PS2_MEM_WriteFloat(turretBase + CODFH_TURRET_CAMX, turretCamX);
		PS2_MEM_WriteFloat(turretBase + CODFH_TURRET_CAMY, turretCamY);
	}
	else {
		turretBase = 0; // player not in turret, reset turret base

		if (PS2_MEM_ReadUInt(onFootCamBase + CODFH_ONRAILS_SANITY) == 0x1) { // onRails camX, level 1 boat ride has different camX
			float camX = PS2_MEM_ReadFloat(onFootCamBase + CODFH_ONRAILS_CAMX);
			camX -= (float)xmouse * looksensitivity / 2.f;
			PS2_MEM_WriteFloat(onFootCamBase + CODFH_ONRAILS_CAMX, (float)camX);
		}
		else { // onFoot camX
			uint32_t camXBase = PS2_MEM_ReadPointer(onFootCamBase + CODFH_ONFOOT_CAMX_BASE);
			float camXSin = PS2_MEM_ReadFloat(camXBase + CODFH_ONFOOT_CAMX_SIN_1);
			float camXCos = PS2_MEM_ReadFloat(camXBase + CODFH_ONFOOT_CAMX_COS_1);

			if (camXSin == 0 && camXCos == 0) // reset on level switch? when values are zeroed
			{
				// NOTE: will only happen when mouse is moved
				onFootTotalAngle = CODFH_ONFOOT_TOTAL_ANGLE_UNSET;
				return;
			}

			float angleChange = (float)xmouse * looksensitivity / 100.f * fov;

			float angle = atan(camXSin / camXCos);
			// if (onFootTotalAngle == CODFH_ONFOOT_TOTAL_ANGLE_UNSET) {
				onFootTotalAngle = angle;
				if (camXCos < 0) // if in opposite 2 quadrants
					onFootTotalAngle = angle - (TAU / 2.f); // turn angle 180 degrees
			// }

			// angle += angleChange;
			onFootTotalAngle += angleChange;

			camXSin = sin(onFootTotalAngle);
			camXCos = cos(onFootTotalAngle);

			// just setting one set of these will kind of move the camera  while also squeezing or stretching the character's hand/gun
			// both sets need to be set and the second's sin is opposite the first in memory so it must be negated
			PS2_MEM_WriteFloat(camXBase + CODFH_ONFOOT_CAMX_SIN_1, (float)camXSin);
			PS2_MEM_WriteFloat(camXBase + CODFH_ONFOOT_CAMX_COS_1, (float)camXCos);
			PS2_MEM_WriteFloat(camXBase + CODFH_ONFOOT_CAMX_SIN_2, -(float)camXSin);
			PS2_MEM_WriteFloat(camXBase + CODFH_ONFOOT_CAMX_COS_2, (float)camXCos);

			// if lvl 11 only, beginning onFoot part needs extra rotation values set
			if (currentLevel == 0x11)
			{
				float skew = 0.17447f;
				PS2_MEM_WriteFloat(camXBase + CODFH_ONFOOT_CAMX_SIN_SKEW, -(float)camXSin * skew);
				PS2_MEM_WriteFloat(camXBase + CODFH_ONFOOT_CAMX_COS_SKEW, -(float)camXCos * skew);
			}
		}

		float camY = PS2_MEM_ReadFloat(onFootCamBase + CODFH_ONFOOT_CAMY);
		camY += (float)ymouse * looksensitivity / 2.f * fov; 
		PS2_MEM_WriteFloat(onFootCamBase + CODFH_ONFOOT_CAMY, camY);
	}
}