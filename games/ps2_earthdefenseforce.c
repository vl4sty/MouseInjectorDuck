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

#define EDF_JAPAN_IS_PAUSED 0x21CA9C
#define EDF_PAL_IS_PAUSED 0x21A61C
#define EDF_PAUSED 0x00010000

#define EDF_PLAYER_BASE_JAPAN 0x1F4A98
#define EDF_PLAYER_BASE_PAL 0x1F1F98
// offsets from playerBase
// same as GDF -0x120
#define EDF_CAM_ZOOM 0x2B0
#define EDF_CAMY 0x350
#define EDF_CAMX 0x354
#define EDF_CAMY_LAST 0x340
#define EDF_CAMX_LAST 0x344
#define EDF_VEHICLE_BASE 0x68
// // offsets from vehicle base
#define EDF_VEHICLE_TYPE 0x68
#define EDF_BIKE_ROT_X_SPEED 0x640
#define EDF_BIKE_ROT_X 0x644

#define EDF_TANK_CANNON_X_SPEED 0x5BC
#define EDF_TANK_CANNON_Y_SPEED 0x5C0
#define EDF_HELI_VEL_Y 0x560 // 0x5B0
#define EDF_HELI_VEL_Z 0x564 // 0x5B4
#define EDF_HELI_ROLL 0x570 // 0x5C0
#define EDF_HELI_PITCH 0x578 // 0x5C8
#define EDF_HELI_YAW_SPEED 0x590 // 0x5E0

// vehicle id values
#define EDF_VEHICLE_HELI 0x494C4548 // ILEH
#define EDF_VEHICLE_BIKE 0x454B4942 // EKIB
#define EDF_VEHICLE_TANK 0x4B4E4154 // KNAT

static uint8_t PS2_EDF_Status(void);
static uint8_t PS2_EDF_DetectPlayerBase(void);
static void PS2_EDF_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Earth Defense Force",
	PS2_EDF_Status,
	PS2_EDF_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_EARTHDEFENSEFORCE = &GAMEDRIVER_INTERFACE;

static uint32_t regionalPlayerBaseAddress = 0;
static uint32_t playerBase = 0;
static float scale = 600.f;
static uint32_t tankBase = 0;
static uint32_t bikeBase = 0;
static uint32_t vehicleBase = 0;
static float lastTankCannonSpeedX = 0.f;
static int lastMouseXDir = 0;
static uint8_t isJapan = 0;
static uint8_t isPAL = 0;
static float lastHeliPitch = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_EDF_Status(void)
{
	// SLPM_623.44
	if (PS2_MEM_ReadWord(0x0093390) == 0x534C504DU && 
		PS2_MEM_ReadWord(0x0093394) == 0x5F363233U &&
		PS2_MEM_ReadWord(0x0093398) == 0x2E34343BU)
	{
		regionalPlayerBaseAddress = EDF_PLAYER_BASE_JAPAN;
		isJapan = 1;
		isPAL = 0;
		return 1;
	}

	// SLES_518.56
	if (PS2_MEM_ReadWord(0x0093390) == 0x534C4553U && 
		PS2_MEM_ReadWord(0x0093394) == 0x5F353138U &&
		PS2_MEM_ReadWord(0x0093398) == 0x2E35363BU)
	{
		regionalPlayerBaseAddress = EDF_PLAYER_BASE_PAL;
		isPAL = 1;
		isJapan = 0;
		return 1;
	}
	
	return 0;
}

static uint8_t PS2_EDF_DetectPlayerBase(void)
{
	uint32_t tempPlayerBase = PS2_MEM_ReadUInt(regionalPlayerBaseAddress);
	if (tempPlayerBase)
	{
		playerBase = tempPlayerBase;
		return 1;
	}
	return 0;
}

static void PS2_EDF_Inject(void)
{
	// don't update if game paused
	if (isJapan) {
		if (PS2_MEM_ReadUInt(EDF_JAPAN_IS_PAUSED) == EDF_PAUSED)
			return;
	}
	else if(isPAL) {
		if (PS2_MEM_ReadUInt(EDF_PAL_IS_PAUSED) == EDF_PAUSED)
			return;
	}

	if (!PS2_EDF_DetectPlayerBase())
		return;

	vehicleBase = PS2_MEM_ReadUInt(playerBase + EDF_VEHICLE_BASE);
	uint32_t vehicleType = PS2_MEM_ReadUInt(vehicleBase + EDF_VEHICLE_TYPE);

	if (xmouse == 0 && vehicleBase)
	{
		// automatically move tank barrel horizontally to avoid constantly moving the mouse
		if (vehicleType == EDF_VEHICLE_TANK)
		{
			float cannonSpeedX = PS2_MEM_ReadFloat(vehicleBase + EDF_TANK_CANNON_X_SPEED);
			if (fabs(lastTankCannonSpeedX) > 0.0035f &&
				((lastTankCannonSpeedX < 0 && cannonSpeedX <= 0) ||
				(lastTankCannonSpeedX > 0 && cannonSpeedX >= 0)))
			{
				PS2_MEM_WriteFloat(vehicleBase + EDF_TANK_CANNON_X_SPEED, (float)lastTankCannonSpeedX);
			}
		}
	}
	if (ymouse == 0 && vehicleBase)
	{
		// keep helicopter pitch set
		if (vehicleType == EDF_VEHICLE_HELI)
		{
			if (PS2_MEM_ReadFloat(vehicleBase + EDF_HELI_VEL_Y) != 0.f &&
				PS2_MEM_ReadFloat(vehicleBase + EDF_HELI_VEL_Z) != 0.f)
			{
				if (lastHeliPitch != 0)
					PS2_MEM_WriteFloat(vehicleBase + EDF_HELI_PITCH, lastHeliPitch);
			}

		}
	}


	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 20.f;

	if (vehicleBase && vehicleType == EDF_VEHICLE_HELI) {
		if (PS2_MEM_ReadFloat(vehicleBase + EDF_HELI_VEL_Y) == 0.f &&
			PS2_MEM_ReadFloat(vehicleBase + EDF_HELI_VEL_Z) == 0.f)
		{
			return;
		}
		
		float heliYawSpeedScale = 3000.f;
		float heliYawSpeed = PS2_MEM_ReadFloat(vehicleBase + EDF_HELI_YAW_SPEED);
		heliYawSpeed += (float)xmouse * looksensitivity / heliYawSpeedScale;
		PS2_MEM_WriteFloat(vehicleBase + EDF_HELI_YAW_SPEED, (float)heliYawSpeed);

		float heliPitchScale = 120000.f;
		float heliPitch = PS2_MEM_ReadFloat(vehicleBase + EDF_HELI_PITCH);
		heliPitch -= (float)ymouse * looksensitivity / heliPitchScale;
		heliPitch = ClampFloat(heliPitch, -0.01, 0.01);
		PS2_MEM_WriteFloat(vehicleBase + EDF_HELI_PITCH, (float)heliPitch);
		lastHeliPitch = heliPitch;

		float heliRollScale = 120000.f;
		float heliRoll = PS2_MEM_ReadFloat(vehicleBase + EDF_HELI_ROLL);
		heliRoll += (float)xmouse * looksensitivity / heliRollScale;
		heliRoll = ClampFloat(heliRoll, -0.01, 0.01);
		PS2_MEM_WriteFloat(vehicleBase + EDF_HELI_ROLL, (float)heliRoll);
		// lastHeliPitch = heliPitch;
	}
	else if (vehicleBase && vehicleType == EDF_VEHICLE_BIKE) {
	// if (bikeBase) {
		// TODO: clamp rotation max based on speed?
		float bikeRotSpeedScale = 20000.f;
		float bikeRotSpeedX = PS2_MEM_ReadFloat(vehicleBase + EDF_BIKE_ROT_X_SPEED);
		bikeRotSpeedX += (float)xmouse * looksensitivity / bikeRotSpeedScale;
		PS2_MEM_WriteFloat(vehicleBase + EDF_BIKE_ROT_X_SPEED, (float)bikeRotSpeedX);
	}
	else if (vehicleBase && vehicleType == EDF_VEHICLE_TANK) {
		// === direct control of barrel ===
		// float camX = PS2_MEM_ReadFloat(EDF_TANK_CANNON_X);
		// camX += (float)xmouse * looksensitivity / scale * zoom;
		// PS2_MEM_WriteFloat(EDF_TANK_CANNON_X, (float)camX);

		// float camY = PS2_MEM_ReadFloat(EDF_TANK_CANNON_Y);
		// camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * zoom;
		// // camY = ClampFloat(camY, -0.9424778223f, 0.9424778223);
		// PS2_MEM_WriteFloat(EDF_TANK_CANNON_Y, (float)camY);

		// if (xmouse)
		// 	PS2_MEM_WriteFloat(EDF_TANK_CANNON_X_SPEED, 0.001f);
		// if (ymouse)
		// 	PS2_MEM_WriteFloat(EDF_TANK_CANNON_Y_SPEED, 0.001f);

		
		// === indirect control, adjusts speed ===
		float cannonRotationScale = 20000.f;
		// float cannonSpeedX = PS2_MEM_ReadFloat(tankBase + EDF_TANK_CANNON_X_SPEED);
		float cannonSpeedX = PS2_MEM_ReadFloat(vehicleBase + EDF_TANK_CANNON_X_SPEED);
		cannonSpeedX += (float)xmouse * looksensitivity / cannonRotationScale;
		cannonSpeedX = ClampFloat(cannonSpeedX, -0.0045f, 0.0045);
		// PS2_MEM_WriteFloat(tankBase + EDF_TANK_CANNON_X_SPEED, (float)cannonSpeedX);
		PS2_MEM_WriteFloat(vehicleBase + EDF_TANK_CANNON_X_SPEED, (float)cannonSpeedX);
		lastTankCannonSpeedX = cannonSpeedX;

		// float cannonSpeedY = PS2_MEM_ReadFloat(tankBase + EDF_TANK_CANNON_Y_SPEED);
		float cannonSpeedY = PS2_MEM_ReadFloat(vehicleBase + EDF_TANK_CANNON_Y_SPEED);
		cannonSpeedY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / cannonRotationScale;
		cannonSpeedY = ClampFloat(cannonSpeedY, -0.0045f, 0.0045);
		// PS2_MEM_WriteFloat(tankBase + EDF_TANK_CANNON_Y_SPEED, (float)cannonSpeedY);
		PS2_MEM_WriteFloat(vehicleBase + EDF_TANK_CANNON_Y_SPEED, (float)cannonSpeedY);
	}
	else {
		// on foot
		float zoom = PS2_MEM_ReadFloat(playerBase + EDF_CAM_ZOOM);

		float camX = PS2_MEM_ReadFloat(playerBase + EDF_CAMX);
		camX += (float)xmouse * looksensitivity / scale / zoom;
		PS2_MEM_WriteFloat(playerBase + EDF_CAMX, (float)camX);
		PS2_MEM_WriteFloat(playerBase + EDF_CAMX_LAST, (float)camX);

		float camY = PS2_MEM_ReadFloat(playerBase + EDF_CAMY);
		camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale / zoom;
		camY = ClampFloat(camY, -0.9424778223f, 0.9424778223);
		PS2_MEM_WriteFloat(playerBase + EDF_CAMY, (float)camY);
		PS2_MEM_WriteFloat(playerBase + EDF_CAMY_LAST, (float)camY);
	}

}