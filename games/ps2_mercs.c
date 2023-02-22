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

#define MERC_PLAYERBASE 0x501A70
#define MERC_PLAYERBASE_SANITY_1_VALUE 0x0A5B616E
#define MERC_PLAYERBASE_SANITY_2_VALUE 0xC8F44B00
// offsets from playerbase
#define MERC_PLAYERBASE_SANITY_1 0x0
#define MERC_PLAYERBASE_SANITY_2 0xB0
#define MERC_STANCE_CHANGE_TIMER 0xD78
#define MERC_IS_SNIPER_ZOOMED 0xD0C

#define MERC_CAMBASE 0x501BF0 // changes to zoom base when zoomed
#define MERC_CAMBASE_SANITY_1_VALUE 0x90054C00
#define MERC_CAMBASE_SANITY_2_VALUE 0xCDCC4C3F
// offsets from cambase
#define MERC_CAMBASE_SANITY_1 0x40
#define MERC_CAMBASE_SANITY_2 0xB8
#define MERC_CAMY 0xE0
#define MERC_CAMX 0xEC
#define MERC_ZOOMED_Y 0x50
#define MERC_ZOOMED_X 0x54
#define MERC_ZOOM_MULT 0x60
#define MERC_CAM_TYPE 0x138
#define MERC_CAM_AUTO_CENTER 0x140 // must be 0'd with CAM_TYPE to disable auto-center
#define MERC_Y_ZOOM 0xA0 // 2.5
#define MERC_X_ZOOM 0xA4 // 9.7
#define MERC_X_PIVOT 0xC4 // -0.5
#define MERC_Y_PIVOT 0xC8 // -8.5
#define MERC_AUTO_CENTER 0x18C
#define MERC_AUTO_CENTER_IMPULSE 0x18C
#define MERC_AUTO_CENTER_FLAG 0x13C
// #define MERC_FOV 0xA5C
#define MERC_VEHICLE_CAMX_ANGLE 0xEC
#define MERC_VEHICLE_CAMX_SIN 0xFC
#define MERC_VEHICLE_CAMX_COS 0x104
#define MERC_VEHICLE_CAMX_SIN_MID 0x194
#define MERC_VEHICLE_CAMX_COS_MID 0x19C

#define MERC_IS_ZOOMED 0x559EFC
#define MERC_IS_DRIVING 0x558B14

// #define MERC_STANCE_CHANGE_TIMER 0x19404F8

#define MERC_VEHICLEGUN_BASE_PTR 0x4A482C
#define MERC_VEHICLEGUN_BASE 0xD00 // offset from vehicle gun base ptr
// #define MERC_VEHICLEGUN_BASE 0x1940480
#define MERC_VEHICLEGUN_SANITY_1_VALUE 0xF86B4B00
#define MERC_VEHICLEGUN_SANITY_2_VALUE 0xE88B4B00
// offsets from vehicleGunBase
#define MERC_VEHICLEGUN_SANITY_1 0x8
#define MERC_VEHICLEGUN_SANITY_2 0x90
#define MERC_VEHICLEGUN_SANITY_3 0x2A0
#define MERC_VEHICLEGUN_CAMY 0x1F8
#define MERC_VEHICLEGUN_CAMX 0x214

// #define MERC_TANK_BASE_PTR 0x5B6090
#define MERC_TANK_BASE_PTR_2 0x55B058
#define MERC_TANK_BASE_PTR_1 0x2C // offset from *MERC_TANK_BASE_PTR_2
#define MERC_TANK_BASE_PTR_0 0x190 // offset from *MERC_TANK_BASE_PTR_1
// #define MERC_TANK_BASE_PTR 0x1A76E20
#define MERC_TANK_BASE 0x190 // offset from tank base pointer
#define MERC_TANK_SANITY_1_VALUE 0xF86B4B00
#define MERC_TANK_SANITY_2_VALUE 0xE88B4B00
// offsets from tank base
#define MERC_TANK_CAMY 0x1F8
#define MERC_TANK_CAMX 0x214
#define MERC_TANK_SANITY_1 0x8
#define MERC_TANK_SANITY_2 0x90
#define MERC_TANK_SANITY_3 0x2A0

#define MERC_IN_HELI 0x4A4800

#define MERC_HELI_BASE_PTR_1_ALT 0x59E8B8
#define MERC_HELI_BASE_PTR_1 0x59E890
#define MERC_HELI_BASE_PTR_0 0x3C // offset from *MERC_HELI_BASE_PTR_1
#define MERC_HELI_CAMX 0x3C
#define MERC_HELI_CAMX_IMPULSE 0x54
#define MERC_HELI_SANITY_1 0x4
#define MERC_HELI_SANITY_2 0x18

#define MERC_HELI_SANITY_1_VALUE 0x70B74C00
#define MERC_HELI_SANITY_2_VALUE 0xCDCC4C3D

// #define MERC_AIM_ASSIST 0x194015C
// #define MERC_AIM_ASSIST_2 0x19404D4
// #define MERC_AIM_ASSIST_3 0x19404D8

static uint8_t PS2_MERC_Status(void);
static uint8_t PS2_MERC_DetectPlayerBase(void);
static uint8_t PS2_MERC_DetectCambase(void);
static uint8_t PS2_MERC_DetectVehicleGun(void);
static uint8_t PS2_MERC_DetectTank(void);
static uint8_t PS2_MERC_CurrentTankBaseValid(void);
static uint8_t PS2_MERC_DetectHeli(void);
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

static uint32_t playerBase = 0;
static uint32_t camBase = 0;
static uint32_t vehicleGunBase = 0;
static uint32_t tankBase = 0;
static uint32_t heliBase = 0;
static float vehicleAngle = MERC_VEHICLE_ANGLE_UNSET;
// static uint8_t vehicleCamAdjusted = 0;
// static float lastCamX = 0;
// static uint8_t wasOnFoot = 0;
// static uint8_t wasInVehicle = 0;
// static uint8_t customVehicleCam = 0;
// static float lastVehicleCamXSinMid = 0.f;
// static float lastVehicleCamXCosMid = 0.f;
// static float lastCamXAngle = 0.f;
// static float lastCamXSin = 0.f;
// static float lastCamXCos = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_MERC_Status(void)
{
	// SLUS_209.32
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && PS2_MEM_ReadWord(0x00093394) == 0x5F323039U) &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E33323BU;
}

static uint8_t PS2_MERC_DetectPlayerBase(void)
{
	uint32_t tempPlayerBase = PS2_MEM_ReadPointer(MERC_PLAYERBASE);
	if (tempPlayerBase && 
		PS2_MEM_ReadWord(tempPlayerBase + MERC_PLAYERBASE_SANITY_1) == MERC_PLAYERBASE_SANITY_1_VALUE &&
		PS2_MEM_ReadWord(tempPlayerBase + MERC_PLAYERBASE_SANITY_2) == MERC_PLAYERBASE_SANITY_2_VALUE)
	{
		playerBase = tempPlayerBase;
		return 1;
	}

	return 0;
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

static uint8_t PS2_MERC_DetectVehicleGun(void)
{
	uint32_t vehicleGunBasePtr = PS2_MEM_ReadPointer(MERC_VEHICLEGUN_BASE_PTR) + MERC_VEHICLEGUN_BASE;
	uint32_t tempVehicleGunBase = PS2_MEM_ReadUInt(vehicleGunBasePtr);
	if (tempVehicleGunBase)
	{
		if (PS2_MEM_ReadWord(tempVehicleGunBase + MERC_VEHICLEGUN_SANITY_1) == MERC_VEHICLEGUN_SANITY_1_VALUE &&
			PS2_MEM_ReadWord(tempVehicleGunBase + MERC_VEHICLEGUN_SANITY_2) == MERC_VEHICLEGUN_SANITY_2_VALUE &&
			PS2_MEM_ReadWord(tempVehicleGunBase + MERC_VEHICLEGUN_SANITY_3) != 0x0)
		{
			vehicleGunBase = tempVehicleGunBase;
			return 1;
		}
	}

	return 0;
}

static uint8_t PS2_MERC_CurrentTankBaseValid(void)
{
	if (PS2_MEM_ReadWord(tankBase + MERC_TANK_SANITY_1) == MERC_TANK_SANITY_1_VALUE &&
		PS2_MEM_ReadWord(tankBase + MERC_TANK_SANITY_2) == MERC_TANK_SANITY_2_VALUE &&
		PS2_MEM_ReadWord(tankBase + MERC_TANK_SANITY_3) != 0x0)
	{
		return 1;
	}

	return 0;
}

static uint8_t PS2_MERC_DetectTank(void)
{
	uint32_t tankBasePtr2 = PS2_MEM_ReadPointer(MERC_TANK_BASE_PTR_2); // 0x1971B74
	uint32_t tankBasePtr1 = PS2_MEM_ReadPointer(tankBasePtr2 + MERC_TANK_BASE_PTR_1); // 0x1C96BC0
	uint32_t tempTankBase = PS2_MEM_ReadPointer(tankBasePtr1 + MERC_TANK_BASE_PTR_0); // 0x1B8F3C0

	if (tempTankBase)
	{
		if (PS2_MEM_ReadWord(tempTankBase + MERC_TANK_SANITY_1) == MERC_TANK_SANITY_1_VALUE &&
			PS2_MEM_ReadWord(tempTankBase + MERC_TANK_SANITY_2) == MERC_TANK_SANITY_2_VALUE &&
			PS2_MEM_ReadWord(tempTankBase + MERC_TANK_SANITY_3) != 0x0)
		{
			tankBase = tempTankBase;
			return 1;
		}
	}

	return 0;
}

static uint8_t PS2_MERC_DetectHeli(void)
{
	uint32_t heliBasePtr1 = PS2_MEM_ReadPointer(MERC_HELI_BASE_PTR_1);
	uint32_t tempHeliBase = PS2_MEM_ReadPointer(heliBasePtr1 + MERC_HELI_BASE_PTR_0);
	if (tempHeliBase)
	{
		if (PS2_MEM_ReadWord(tempHeliBase + MERC_HELI_SANITY_1) == MERC_HELI_SANITY_1_VALUE &&
			PS2_MEM_ReadWord(tempHeliBase + MERC_HELI_SANITY_2) == MERC_HELI_SANITY_2_VALUE)
		{
			heliBase = tempHeliBase;
			return 1;
		}

		// try again with different starting point since other base isn't stable
		//	not a great solution but it prevents heliBase from breaking?
		heliBasePtr1 = PS2_MEM_ReadPointer(MERC_HELI_BASE_PTR_1_ALT);
		tempHeliBase = PS2_MEM_ReadPointer(heliBasePtr1 + MERC_HELI_BASE_PTR_0);
		if (PS2_MEM_ReadWord(tempHeliBase + MERC_HELI_SANITY_1) == MERC_HELI_SANITY_1_VALUE &&
			PS2_MEM_ReadWord(tempHeliBase + MERC_HELI_SANITY_2) == MERC_HELI_SANITY_2_VALUE)
		{
			heliBase = tempHeliBase;
			return 1;
		}
	}
	return 0;
}

static uint8_t PS2_MERC_CurrentHeliValid(void)
{
	if (PS2_MEM_ReadWord(heliBase + MERC_HELI_SANITY_1) == MERC_HELI_SANITY_1_VALUE &&
		PS2_MEM_ReadWord(heliBase + MERC_HELI_SANITY_2) == MERC_HELI_SANITY_2_VALUE)
	{
		return 1;
	}

	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_MERC_Inject(void)
{
	// TODO: character animation while in vehicleGun should change to have their hand on the gun
	//			right now they will do the idle animations like they're not shooting
	// TODO: find a different starting point for heliBase to make it more stable

	// if (customVehicleCam)
	// {
	// 	if (PS2_MEM_ReadUInt(MERC_IS_DRIVING))
	// 	{
	// 		// TODO: zoom out while in vehicle since changing camera type sets to onFoot zoom
	// 		// disable auto-center
	// 		PS2_MEM_WriteUInt(camBase + MERC_CAM_TYPE, 0x0);
	// 		PS2_MEM_WriteUInt(camBase + MERC_CAM_AUTO_CENTER, 0x0);

	// 		// if (!vehicleCamAdjusted)
	// 		// {
	// 		PS2_MEM_WriteFloat(camBase + MERC_Y_ZOOM, 10.f);
	// 		PS2_MEM_WriteFloat(camBase + MERC_X_ZOOM, 20.f);
	// 		PS2_MEM_WriteFloat(camBase + MERC_X_PIVOT, 0.f);
	// 		PS2_MEM_WriteFloat(camBase + MERC_Y_PIVOT, -12.f);
	// 		// 	vehicleCamAdjusted = 1;
	// 		// }

	// 		// if (wasOnFoot)
	// 		// {
	// 		// 	// PS2_MEM_WriteFloat(camBase + MERC_CAMX, lastCamX);
	// 		// 	PS2_MEM_WriteFloat(camBase + MERC_Y_ZOOM, 2.5f);
	// 		// 	PS2_MEM_WriteFloat(camBase + MERC_X_ZOOM, 5.f);
	// 		// 	wasOnFoot = 0;
	// 		// }
	// 	}
	// 	else if (wasInVehicle)
	// 	{
	// 		PS2_MEM_WriteFloat(camBase + MERC_Y_ZOOM, 2.f);
	// 		PS2_MEM_WriteFloat(camBase + MERC_X_ZOOM, 5.f);
	// 		PS2_MEM_WriteFloat(camBase + MERC_X_PIVOT, 1.899999976f);
	// 		PS2_MEM_WriteFloat(camBase + MERC_Y_PIVOT, -1.25f);
	// 	}
	// }

	// if (PS2_MEM_ReadUInt(MERC_IS_DRIVING))
	// {
		// // tried to just override the new values that are adjusted by auto-center, kind of works, not great

	// 	// float camXSin = PS2_MEM_ReadFloat(camBase + MERC_VEHICLE_CAMX_SIN);
	// 	// float camXCos = PS2_MEM_ReadFloat(camBase + MERC_VEHICLE_CAMX_COS);
	// 	float camXSinMid = PS2_MEM_ReadFloat(camBase + MERC_VEHICLE_CAMX_SIN_MID);
	// 	float camXCosMid = PS2_MEM_ReadFloat(camBase + MERC_VEHICLE_CAMX_COS_MID); // sin and cosine for the vehicle camX are stored in a strange format
	// 	// float sinNorm = (camXSin - camXSinMid) / 10.f;
	// 	// float cosNorm = (camXCos - camXCosMid) / 10.f;

	// 	// float angle = (float)atan(sinNorm / cosNorm);

	// 	float camXSin = (sin(vehicleAngle) * 10.f) + camXSinMid;
	// 	float camXCos = (cos(vehicleAngle) * 10.f) + camXCosMid;
	// 	// camXSin = (sin(vehicleAngle) * 10.f) + lastVehicleCamXSinMid;
	// 	// camXCos = (cos(vehicleAngle) * 10.f) + lastVehicleCamXCosMid;

	// 	PS2_MEM_WriteFloat(camBase + MERC_VEHICLE_CAMX_SIN, (float)camXSin);
	// 	PS2_MEM_WriteFloat(camBase + MERC_VEHICLE_CAMX_COS, (float)camXCos);
	// 	// PS2_MEM_WriteFloat(camBase + MERC_VEHICLE_CAMX_ANGLE, (float)vehicleAngle);
	// }

	if (PS2_MERC_DetectPlayerBase()) {
		// shortcut stance change timer to allow camY movement when going from crouch to standing and vice versa
		PS2_MEM_WriteFloat(playerBase + MERC_STANCE_CHANGE_TIMER, 0.f);
	}
	else {
		return;
	}

	// if (!PS2_MERC_DetectCambase()) {
	// 	return;
	// }

	// detect helicopter base when mouse isn't moving
	PS2_MERC_DetectHeli();

	// detect cam base to avoid it breaking sometimes
	PS2_MERC_DetectCambase();

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 10000.f;

	if (PS2_MEM_ReadUInt(MERC_IN_HELI) && (PS2_MERC_DetectHeli() || PS2_MERC_CurrentHeliValid()))
	{
		camBase = PS2_MEM_ReadUInt(MERC_CAMBASE);

		float camY = PS2_MEM_ReadFloat(camBase + MERC_CAMY);
		float camXImpulse = PS2_MEM_ReadFloat(heliBase + MERC_HELI_CAMX_IMPULSE);

		camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;
		camXImpulse -= (float)xmouse * looksensitivity * 2.f;

		PS2_MEM_WriteFloat(camBase + MERC_CAMY, (float)camY);
		PS2_MEM_WriteFloat(heliBase + MERC_HELI_CAMX_IMPULSE, (float)camXImpulse);
	}
	else if (PS2_MEM_ReadUInt(MERC_IS_ZOOMED) || PS2_MEM_ReadUInt(playerBase + MERC_IS_SNIPER_ZOOMED))
	{
		// zoomed aiming
		camBase = PS2_MEM_ReadUInt(MERC_CAMBASE); // zoom base
		float fov = 1.f / PS2_MEM_ReadFloat(camBase + MERC_ZOOM_MULT);

		float camY = PS2_MEM_ReadFloat(camBase + MERC_ZOOMED_Y);
		float camX = PS2_MEM_ReadFloat(camBase + MERC_ZOOMED_X);

		camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * fov;
		camX += (float)xmouse * looksensitivity * fov;

		PS2_MEM_WriteFloat(camBase + MERC_ZOOMED_Y, (float)camY);
		PS2_MEM_WriteFloat(camBase + MERC_ZOOMED_X, (float)camX);
	}
	else if (PS2_MERC_CurrentTankBaseValid() || PS2_MERC_DetectTank())
	{
		float tankCamY = PS2_MEM_ReadFloat(tankBase + MERC_TANK_CAMY);
		float tankCamX = PS2_MEM_ReadFloat(tankBase + MERC_TANK_CAMX);

		tankCamY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;
		tankCamX += (float)(xmouse) * looksensitivity;
		
		PS2_MEM_WriteFloat(tankBase + MERC_TANK_CAMY, tankCamY);
		PS2_MEM_WriteFloat(tankBase + MERC_TANK_CAMX, tankCamX);
	}
	else if (PS2_MERC_DetectVehicleGun())
	{
		// TODO: look for countdown timer for idle animation
		//		 should be reset if camera is moved to keep character's hands on the gun

		float vehicleGunCamY = PS2_MEM_ReadFloat(vehicleGunBase + MERC_VEHICLEGUN_CAMY);
		float vehicleGunCamX = PS2_MEM_ReadFloat(vehicleGunBase + MERC_VEHICLEGUN_CAMX);

		vehicleGunCamY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;
		vehicleGunCamX += (float)(xmouse) * looksensitivity;
		
		PS2_MEM_WriteFloat(vehicleGunBase + MERC_VEHICLEGUN_CAMY, vehicleGunCamY);
		PS2_MEM_WriteFloat(vehicleGunBase + MERC_VEHICLEGUN_CAMX, vehicleGunCamX);
	}
	// else if (customVehicleCam && PS2_MEM_ReadUInt(MERC_IS_DRIVING))
	// else if (PS2_MEM_ReadUInt(MERC_IS_DRIVING))
	// {
	// 	// zoomed aiming
	// 	camBase = PS2_MEM_ReadUInt(MERC_CAMBASE); // zoom base

	// 	float camY = PS2_MEM_ReadFloat(camBase + MERC_CAMY);
	// 	// float camX = PS2_MEM_ReadFloat(camBase + MERC_CAMX);
	// 	float camXSin = PS2_MEM_ReadFloat(camBase + MERC_VEHICLE_CAMX_SIN);
	// 	float camXCos = PS2_MEM_ReadFloat(camBase + MERC_VEHICLE_CAMX_COS);
	// 	float camXSinMid = PS2_MEM_ReadFloat(camBase + MERC_VEHICLE_CAMX_SIN_MID);
	// 	float camXCosMid = PS2_MEM_ReadFloat(camBase + MERC_VEHICLE_CAMX_COS_MID); // sin and cosine for the vehicle camX are stored in a strange format
	// 	// 	they must be normalized to a -1 to 1 range
	// 	//  the middle value used to normalize also changes but is stored in the camera object
	// 	float sinNorm = (camXSin - camXSinMid) / 10.f;
	// 	float cosNorm = (camXCos - camXCosMid) / 10.f;

	// 	if (vehicleAngle == MERC_VEHICLE_ANGLE_UNSET)
	// 	{
	// 		vehicleAngle = (float)atan(sinNorm / cosNorm);
	// 	}

	// 	float angleChange = (float)xmouse * looksensitivity;
	// 	vehicleAngle -= angleChange;

	// 	camXSin = (sin(vehicleAngle) * 10.f) + camXSinMid;
	// 	camXCos = (cos(vehicleAngle) * 10.f) + camXCosMid;

	// 	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;
	// 	// camX -= (float)xmouse * looksensitivity;

	// 	// clamp Y value since different camera range is too high
	// 	// camY = ClampFloat(camY, 0.2f, 0.8f);

	// 	PS2_MEM_WriteFloat(camBase + MERC_CAMY, (float)camY);
	// 	// PS2_MEM_WriteFloat(camBase + MERC_CAMX, (float)camX);
	// 	PS2_MEM_WriteFloat(camBase + MERC_VEHICLE_CAMX_SIN, (float)camXSin);
	// 	PS2_MEM_WriteFloat(camBase + MERC_VEHICLE_CAMX_COS, (float)camXCos);

	// 	wasInVehicle = 1;
	// }
	else
	{
		heliBase = 0;
		tankBase = 0;
		// vehicleCamAdjusted = 0;
		// wasInVehicle = 0;

		camBase = PS2_MEM_ReadUInt(MERC_CAMBASE);

		// if (customVehicleCam)
		// {
		// 	PS2_MEM_WriteFloat(camBase + MERC_Y_ZOOM, 2.f);
		// 	PS2_MEM_WriteFloat(camBase + MERC_X_ZOOM, 5.f);
		// 	PS2_MEM_WriteFloat(camBase + MERC_X_PIVOT, 1.899999976f);
		// 	PS2_MEM_WriteFloat(camBase + MERC_Y_PIVOT, -1.25f);
		// }

		float fov = 1.f;

		float camY = PS2_MEM_ReadFloat(camBase + MERC_CAMY);
		float camX = PS2_MEM_ReadFloat(camBase + MERC_CAMX);

		camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * fov;
		camX -= (float)xmouse * looksensitivity * fov;

		PS2_MEM_WriteFloat(camBase + MERC_CAMY, (float)camY);
		PS2_MEM_WriteFloat(camBase + MERC_CAMX, (float)camX);
		PS2_MEM_WriteFloat(camBase + MERC_CAMX + 0x4, (float)camX);

		// lastCamX = (float)camX;
		// wasOnFoot = 1;
		// PS2_MEM_WriteFloat(camBase + MERC_VEHICLE_CAMX_SIN, camX);
	}

}