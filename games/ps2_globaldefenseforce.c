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

#define GDF_NOT_IN_TANK 0xAC0578
#define GDF_JAPAN_IS_PAUSED 0x278800
#define GDF_PAL_IS_PAUSED 0x279C80
#define GDF_PAUSED 0x00000101

#define GDF_PLAYER_BASE_JAPAN 0x278718
#define GDF_PLAYER_BASE_PAL 0x279B98
// offsets from playerBase
#define GDF_CAM_ZOOM 0x400
#define GDF_CAMY 0x470
#define GDF_CAMX 0x474
#define GDF_CAMY_LAST 0x460
#define GDF_CAMX_LAST 0x464
#define GDF_VEHICLE_BASE 0xCC
// offsets from vehicle base
#define GDF_VEHICLE_IS_TANK 0xD8
#define GDF_VEHICLE_IS_BIKE 0xEC
#define GDF_VEHICLE_TYPE 0x3FC
#define GDF_TANK_CANNON_X 0x614
#define GDF_TANK_CANNON_Y 0x618
#define GDF_TANK_CANNON_X_SPEED 0x60C
#define GDF_TANK_CANNON_Y_SPEED 0x610
#define GDF_BIKE_ROT_X_SPEED 0x690
#define GDF_BIKE_ROT_X 0x694
#define GDF_HELI_VEL_Y 0x5B0
#define GDF_HELI_VEL_Z 0x5B4
#define GDF_HELI_ROLL 0x5C0
#define GDF_HELI_PITCH 0x5C8
#define GDF_HELI_YAW_SPEED 0x5E0

// vehicle id values
#define GDF_VEHICLE_HELI 0x3F800000 // also onFoot?
#define GDF_VEHICLE_BIKE 0x3E4CCCCD
#define GDF_VEHICLE_TANK 0x3DCCCCCD

static uint8_t PS2_GDF_Status(void);
static uint8_t PS2_GDF_DetectPlayerBase(void);
static void PS2_GDF_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Global Defense Force",
	PS2_GDF_Status,
	PS2_GDF_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_GLOBALDEFENSEFORCE = &GAMEDRIVER_INTERFACE;

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
static uint8_t PS2_GDF_Status(void)
{
	// SLPM_626.52
	if (PS2_MEM_ReadWord(0x0093390) == 0x534C504DU && 
		PS2_MEM_ReadWord(0x0093394) == 0x5F363236U &&
		PS2_MEM_ReadWord(0x0093398) == 0x2E35323BU)
	{
		regionalPlayerBaseAddress = GDF_PLAYER_BASE_JAPAN;
		isJapan = 1;
		isPAL = 0;
		return 1;
	}

	// SLES_544.64
	if (PS2_MEM_ReadWord(0x0093390) == 0x534C4553U && 
		PS2_MEM_ReadWord(0x0093394) == 0x5F353434U &&
		PS2_MEM_ReadWord(0x0093398) == 0x2E36343BU)
	{
		regionalPlayerBaseAddress = GDF_PLAYER_BASE_PAL;
		isPAL = 1;
		isJapan = 0;
		return 1;
	}
	
	return 0;
}

static uint8_t PS2_GDF_DetectPlayerBase(void)
{
	uint32_t tempPlayerBase = PS2_MEM_ReadUInt(regionalPlayerBaseAddress);
	if (tempPlayerBase)
	{
		playerBase = tempPlayerBase;
		return 1;
	}
	return 0;
}

static void PS2_GDF_Inject(void)
{
	// don't update if game paused
	if (isJapan) {
		if (PS2_MEM_ReadUInt(GDF_JAPAN_IS_PAUSED) == GDF_PAUSED)
			return;
	}
	else if(isPAL) {
		if (PS2_MEM_ReadWord(GDF_PAL_IS_PAUSED) == GDF_PAUSED)
			return;
	}

	if (!PS2_GDF_DetectPlayerBase())
		return;

	uint32_t vehicleType = PS2_MEM_ReadUInt(playerBase + GDF_VEHICLE_TYPE);
	vehicleBase = PS2_MEM_ReadUInt(playerBase + GDF_VEHICLE_BASE);

	if (xmouse == 0 && vehicleBase)
	{
		// automatically move tank barrel horizontally to avoid constantly moving the mouse
		if (vehicleType == GDF_VEHICLE_TANK)
		{
			float cannonSpeedX = PS2_MEM_ReadFloat(vehicleBase + GDF_TANK_CANNON_X_SPEED);
			if (fabs(lastTankCannonSpeedX) > 0.0035f &&
				((lastTankCannonSpeedX < 0 && cannonSpeedX <= 0) ||
				(lastTankCannonSpeedX > 0 && cannonSpeedX >= 0)))
			{
				PS2_MEM_WriteFloat(vehicleBase + GDF_TANK_CANNON_X_SPEED, (float)lastTankCannonSpeedX);
			}
		}
	}
	if (ymouse == 0 && vehicleBase)
	{
		// keep helicopter pitch set
		if (vehicleType == GDF_VEHICLE_HELI)
		{
			if (PS2_MEM_ReadFloat(vehicleBase + GDF_HELI_VEL_Y) != 0.f &&
				PS2_MEM_ReadFloat(vehicleBase + GDF_HELI_VEL_Z) != 0.f)
			{
				if (lastHeliPitch != 0)
					PS2_MEM_WriteFloat(vehicleBase + GDF_HELI_PITCH, lastHeliPitch);
			}

		}
	}


	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 20.f;

	if (vehicleBase && vehicleType == GDF_VEHICLE_HELI) {
		if (PS2_MEM_ReadFloat(vehicleBase + GDF_HELI_VEL_Y) == 0.f &&
			PS2_MEM_ReadFloat(vehicleBase + GDF_HELI_VEL_Z) == 0.f)
		{
			return;
		}
		
		float heliYawSpeedScale = 3000.f;
		float heliYawSpeed = PS2_MEM_ReadFloat(vehicleBase + GDF_HELI_YAW_SPEED);
		heliYawSpeed += (float)xmouse * looksensitivity / heliYawSpeedScale;
		PS2_MEM_WriteFloat(vehicleBase + GDF_HELI_YAW_SPEED, (float)heliYawSpeed);

		float heliPitchScale = 120000.f;
		float heliPitch = PS2_MEM_ReadFloat(vehicleBase + GDF_HELI_PITCH);
		heliPitch -= (float)ymouse * looksensitivity / heliPitchScale;
		heliPitch = ClampFloat(heliPitch, -0.01, 0.01);
		PS2_MEM_WriteFloat(vehicleBase + GDF_HELI_PITCH, (float)heliPitch);
		lastHeliPitch = heliPitch;

		float heliRollScale = 120000.f;
		float heliRoll = PS2_MEM_ReadFloat(vehicleBase + GDF_HELI_ROLL);
		heliRoll += (float)xmouse * looksensitivity / heliRollScale;
		heliRoll = ClampFloat(heliRoll, -0.01, 0.01);
		PS2_MEM_WriteFloat(vehicleBase + GDF_HELI_ROLL, (float)heliRoll);
		// lastHeliPitch = heliPitch;
	}
	if (vehicleBase && vehicleType == GDF_VEHICLE_BIKE) {
	// if (bikeBase) {
		// TODO: clamp rotation max based on speed?
		float bikeRotSpeedScale = 20000.f;
		float bikeRotSpeedX = PS2_MEM_ReadFloat(vehicleBase + GDF_BIKE_ROT_X_SPEED);
		bikeRotSpeedX += (float)xmouse * looksensitivity / bikeRotSpeedScale;
		PS2_MEM_WriteFloat(vehicleBase + GDF_BIKE_ROT_X_SPEED, (float)bikeRotSpeedX);
	}
	else if (vehicleBase && vehicleType == GDF_VEHICLE_TANK) {
		// === direct control of barrel ===
		// float camX = PS2_MEM_ReadFloat(GDF_TANK_CANNON_X);
		// camX += (float)xmouse * looksensitivity / scale * zoom;
		// PS2_MEM_WriteFloat(GDF_TANK_CANNON_X, (float)camX);

		// float camY = PS2_MEM_ReadFloat(GDF_TANK_CANNON_Y);
		// camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * zoom;
		// // camY = ClampFloat(camY, -0.9424778223f, 0.9424778223);
		// PS2_MEM_WriteFloat(GDF_TANK_CANNON_Y, (float)camY);

		// if (xmouse)
		// 	PS2_MEM_WriteFloat(GDF_TANK_CANNON_X_SPEED, 0.001f);
		// if (ymouse)
		// 	PS2_MEM_WriteFloat(GDF_TANK_CANNON_Y_SPEED, 0.001f);

		
		// === indirect control, adjusts speed ===
		float cannonRotationScale = 20000.f;
		// float cannonSpeedX = PS2_MEM_ReadFloat(tankBase + GDF_TANK_CANNON_X_SPEED);
		float cannonSpeedX = PS2_MEM_ReadFloat(vehicleBase + GDF_TANK_CANNON_X_SPEED);
		cannonSpeedX += (float)xmouse * looksensitivity / cannonRotationScale;
		cannonSpeedX = ClampFloat(cannonSpeedX, -0.0045f, 0.0045);
		// PS2_MEM_WriteFloat(tankBase + GDF_TANK_CANNON_X_SPEED, (float)cannonSpeedX);
		PS2_MEM_WriteFloat(vehicleBase + GDF_TANK_CANNON_X_SPEED, (float)cannonSpeedX);
		lastTankCannonSpeedX = cannonSpeedX;

		// float cannonSpeedY = PS2_MEM_ReadFloat(tankBase + GDF_TANK_CANNON_Y_SPEED);
		float cannonSpeedY = PS2_MEM_ReadFloat(vehicleBase + GDF_TANK_CANNON_Y_SPEED);
		cannonSpeedY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / cannonRotationScale;
		cannonSpeedY = ClampFloat(cannonSpeedY, -0.0045f, 0.0045);
		// PS2_MEM_WriteFloat(tankBase + GDF_TANK_CANNON_Y_SPEED, (float)cannonSpeedY);
		PS2_MEM_WriteFloat(vehicleBase + GDF_TANK_CANNON_Y_SPEED, (float)cannonSpeedY);
	}
	else {
		// on foot

		float zoom = PS2_MEM_ReadFloat(playerBase + GDF_CAM_ZOOM);

		float camX = PS2_MEM_ReadFloat(playerBase + GDF_CAMX);
		camX += (float)xmouse * looksensitivity / scale / zoom;
		PS2_MEM_WriteFloat(playerBase + GDF_CAMX, (float)camX);
		PS2_MEM_WriteFloat(playerBase + GDF_CAMX_LAST, (float)camX);

		float camY = PS2_MEM_ReadFloat(playerBase + GDF_CAMY);
		camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale / zoom;
		camY = ClampFloat(camY, -0.9424778223f, 0.9424778223);
		PS2_MEM_WriteFloat(playerBase + GDF_CAMY, (float)camY);
		PS2_MEM_WriteFloat(playerBase + GDF_CAMY_LAST, (float)camY);
	}

}