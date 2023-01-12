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

#define RDR_camy 0x19B0AEC 
#define RDR_camx 0x1A25714

// #define RDR_camSanity1 0xF6C5D03F
// #define RDR_camSanity1Offset 0x10
#define RDR_camFov 0x10 // offset from RDR_cambase
#define RDR_camSanity 0x4F0180BF
#define RDR_camSanityOffset 0x38

#define RDR_onLevel6MachineGunFlag 0x13E7C48 // may require finding a ptr to the current level as this may not be static, refer to someLevelPointer
#define RDR_level6MachineGunCamX 0x181E82C
#define RDR_level6MachineGunCamY 0x181E830

#define RDR_camy_base 0x73C170
#define RDR_cambase 0x7A00CC

#define RDR_someLevelPointer 0x55CFF8
#define RDR_canMoveCameraFlag 0x4F71F0

#define AUF_camy 0x005064D0
#define AUF_camx 0x005064D4
#define AUF_car_camx 0x003CD6C0
#define AUF_car_camy 0x003CD858
#define AUF_health_lvl2 0x010C1C84

#define CAMYPLUS 0.6999999988f // 0x3FAB1DD6
#define CAMYMINUS -0.9973310232f // 0xBF7F5116
#define SNIPERCAMYPLUS 1.396263392f
#define SNIPERCAMYMINUS -0.8726648532f
// TM ADDRESSES - OFFSET ADDRESSES BELOW (REQUIRES PLAYERBASE TO USE)
// 6 cameras or angles get cycled through
#define TM_camx_enum_gap 0x8014E3F4 - 0x8014E244 // gap between each camera 1B0
#define TM_camy 0x8100D260 - 0x8100B980
#define TM_sanity 0x8100B982 - 0x8100B980 // value in player object that is always the same
#define TM_health 0x8100B9BC - 0x8100B980
#define TM_weaponInstancePointerOffset 0x8100BF88 - 0x8100B980
#define TM_weaponClassPointerOffset 0x811C3514 - 0x811C3500
// STATIC ADDRESSES BELOW
#define TM_playerbase 0x802178D4 // random stack address, commonly holds player pointer - requires sanity checks before using!
#define TM_sniperY 0x80173480
#define TM_sniperX 0x80173490
#define TM_camx 0x8014E244 // first camx
#define TM_currentCam 0x804763D4 // current enumerated camera

static uint8_t PS2_RDR_Status(void);
static uint8_t PS2_RDR_DetectCamera(void);
static void PS2_RDR_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Red Dead Revolver",
	PS2_RDR_Status,
	PS2_RDR_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_REDDEADREVOLVER = &GAMEDRIVER_INTERFACE;

static uint32_t playerbase = 0;
static uint32_t cambase = 0;
static uint32_t camXOffsets[] = {0x26C, 0x1BC, -0x17234};
		//	0x26C
		//	0x1BC
		//	-0x17234

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_RDR_Status(void)
{
	// return (MEM_ReadUInt(0x80000000) == 0x47473245U && MEM_ReadUInt(0x80000004) == 0x345A0000U); // check game header to see if it matches Trigger Man (GG2E4Z)
	// return (PS2_MEM_ReadUInt(0x00093390) == 0x534C5553U);
	return (PS2_MEM_ReadUInt(0x00093390) == 0x534C5553U && PS2_MEM_ReadUInt(0x00093394) == 0x5F323035U) &&
			PS2_MEM_ReadUInt(0x00093398) == 0x2E30303BU;
}
//==========================================================================
// Purpose: detects player pointer from stack address
// Changed Globals: fovbase, playerbase
//==========================================================================
static uint8_t PS2_RDR_DetectCamera(void)
{
	uint32_t tempcambase = PS2_MEM_ReadPointer(RDR_cambase);
	if (PS2WITHINMEMRANGE(tempcambase))
	{
		if (PS2_MEM_ReadUInt(tempcambase + RDR_camSanityOffset) == RDR_camSanity)
		{
			cambase = tempcambase;
			return 1;
		}
	}

	return 0;

	// const uint32_t tempplayerbase = MEM_ReadUInt(TM_playerbase);
	// if(WITHINMEMRANGE(tempplayerbase) && tempplayerbase != playerbase) // if pointer is valid, sanity check pointer
	// {
	// 	const uint32_t tempsanity = MEM_ReadUInt(tempplayerbase + TM_sanity);
	// 	const uint32_t temphealth = MEM_ReadUInt(tempplayerbase + TM_health);
	// 	if(temphealth > 0 && temphealth <= 0x43C80000 && tempsanity == 0x41000080U) // if player base is valid, use player pointer for level
	// 	{
	// 		playerbase = tempplayerbase;
	// 		return 1;
	// 	}
	// }
	// return WITHINMEMRANGE(playerbase);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_RDR_Inject(void)
{
	// PS2_MEM_WriteFloat(AUF_health_lvl2, 2000.f);

	// TODO: *DONE* sanity check when getting cam bases and early termination if not found
	// TODO: *DONE* handle different camxbase on lvl 4 (train)
	// TODO: *DONE* prevent mouse input in shops, menu, etc...
	// TODO: prevent mousey on chat train
	// TODO: *DONE* turret machine gun, lvl 5
	// TODO: camy max and min to prevent warping at extremes
	// TODO: prevent mouse when dead
	// TODO: *DONE* cover camx/y
	// TODO: *DONE* zoom camx/y
	// TODO: duel cursor? and draw with mouse? (down then up motion)
	// TODO: prevent mouse on pause menu
	// TODO: *DONE* camera not working on chapter 16 jailbreak
	// TODO: fix: widescreen breaks it
	// TODO: disable aim lock?
	// TODO: *DONE* find fov
	// TODO: machinegun in alamo lvl

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	if (!PS2_RDR_DetectCamera())
		return;
	
	// TODO: check if in town as just this flag breaks ch.16 jailbreak
	// if (PS2_MEM_ReadUInt(RDR_canMoveCameraFlag) == 0x01010000U) // return if in an interior where camera movement is disabled
	// 	return;

	// 1.631041288, FOV while walking
	float fov = PS2_MEM_ReadFloat(cambase + RDR_camFov);
	float looksensitivity = (float)sensitivity / 180.f / 40.f / (fov / 1.6f);

	// float camx = PS2_MEM_ReadFloat(AUF_car_camx);
	// float pre = camx;
	// camx -= (float)xmouse * looksensitivity / 20.f; // normal calculation method for X
	// float camx = PS2_MEM_ReadFloat(RDR_camx);
	// camx -= (float)xmouse * looksensitivity; // normal calculation method for X

	// float camy = PS2_MEM_ReadFloat(RDR_camy);
	// camy += (float)ymouse * looksensitivity / 360.f; // normal calculation method for X
	// float camy = PS2_MEM_ReadFloat(RDR_camy);
	// camy += (float)ymouse * looksensitivity; // normal calculation method for X


	uint32_t camxOffset = 0;
	uint32_t camxbase = 0;

	int i;
	for (i = 0; i < 3; i++) // loop through the known camXOffsets from cambase
	{
		camxOffset = camXOffsets[i];
		camxbase = PS2_MEM_ReadPointer(cambase + camxOffset);

		if (PS2_MEM_ReadUInt(camxbase - 0x250) == 0xC8145100) // check that pointer points to a camx object
			break;
	}


	if (PS2_MEM_ReadUInt(RDR_onLevel6MachineGunFlag) == 0x01000000U && PS2_MEM_ReadUInt(RDR_onLevel6MachineGunFlag + 0x4) == 0x50548A01U) // onMachineGun and sanity check
	{
		// machine gun
		float camx = PS2_MEM_ReadFloat(RDR_level6MachineGunCamX);
		camx -= (float)xmouse * looksensitivity; // normal calculation method for X

		float camy = PS2_MEM_ReadFloat(RDR_level6MachineGunCamY);
		camy -= (float)ymouse * looksensitivity; // normal calculation method for X

		PS2_MEM_WriteFloat(RDR_level6MachineGunCamX, camx);
		PS2_MEM_WriteFloat(RDR_level6MachineGunCamY, camy);
	} 
	else if (PS2_MEM_ReadUInt(camxbase - 0x25C + 0xCE0) == 0x00000000) // aiming from cover
	{
		// &camx + 0xCE0 = isCoverAiming
		uint32_t coverbase = camxbase - 0x25C - 0x124;

		float coverX = PS2_MEM_ReadFloat(coverbase + 0x4);
		coverX -= (float)xmouse * looksensitivity; // normal calculation method for X
		PS2_MEM_WriteFloat(coverbase + 0x4, coverX);

		float coverY = PS2_MEM_ReadFloat(coverbase + 0x24);
		coverY -= (float)ymouse * looksensitivity; // normal calculation method for X
		PS2_MEM_WriteFloat(coverbase + 0x24, coverY);
	}
	else {
		// uint32_t camybase = PS2_MEM_ReadPointer(RDR_camy_base) - 0xB4;
		uint32_t cameraBase = PS2_MEM_ReadPointer(RDR_camy_base);
		uint32_t camYAddress = PS2_MEM_ReadPointer(cameraBase + 0x30) + 0x1C;
		// uint32_t camybase = PS2_MEM_ReadPointer(PS2_MEM_ReadPointer(RDR_camy_base) + 0x30) + 0x1C;
		float camy = PS2_MEM_ReadFloat(camYAddress);
		camy += (float)ymouse * looksensitivity; // normal calculation method for X
		PS2_MEM_WriteFloat(camYAddress, camy);

		// uint32_t ptr = PS2_MEM_ReadPointer(cameraBase + 0x30);	
		// ptr = PS2_MEM_ReadPointer(ptr + 0x50);
		// ptr = PS2_MEM_ReadPointer(ptr + 0x9C);
		// ptr = PS2_MEM_ReadPointer(ptr + 0x20);
		// uint32_t camXAddress = ptr + 0x1A4;
		// float camx = PS2_MEM_ReadFloat(camXAddress);
		// camx -= (float)xmouse * looksensitivity; // normal calculation method for X
		// PS2_MEM_WriteFloat(camXAddress, camx);


		// possible cam x offsets
		//	0x26C
		//	0x1BC
		//	-0x17234

		// uint32_t camxOffset = 0;
		// uint32_t camxbase = 0;

		// int i;
		// for (i = 0; i < 3; i++) // loop through the known camXOffsets from cambase
		// {
		// 	camxOffset = camXOffsets[i];
		// 	camxbase = PS2_MEM_ReadPointer(cambase + camxOffset);

		// 	if (PS2_MEM_ReadUInt(camxbase - 0x250) == 0xC8145100) // check that pointer points to a camx object
		// 		break;
		// }

		float camx = PS2_MEM_ReadFloat(camxbase - 0x25C);
		camx -= (float)xmouse * looksensitivity; // normal calculation method for X
		PS2_MEM_WriteFloat(camxbase - 0x25C, camx);


		// only do if covered
		// uint32_t coverbase = camxbase - 0x25C - 0x124;

		// float coverX = PS2_MEM_ReadFloat(coverbase + 0x4);
		// coverX -= (float)xmouse * looksensitivity; // normal calculation method for X
		// PS2_MEM_WriteFloat(coverbase + 0x4, coverX);

		// float coverY = PS2_MEM_ReadFloat(coverbase + 0x24);
		// coverY -= (float)ymouse * looksensitivity; // normal calculation method for X
		// PS2_MEM_WriteFloat(coverbase + 0x24, coverY);

		// // uint32_t cambase = PS2_MEM_ReadPointer(RDR_cambase);
		// uint32_t camxOffset = 0x26C;
		// // levels with different camxOffset
		// // ch.16 jailbreak, 0x1BC
		// // train level, 0x1BC
		// // 0x13A47E0U needs 0x1BC offset in beginning of ch17 and 0x26C on final fight against Ted and Tony
		// //   value remained til next level so maybe not an indicator on current level
		// 	//  PS2_MEM_ReadPointer(RDR_someLevelPointer) == 0x1400DA0U) // && PS2_MEM_ReadUInt(RDR_someLevelPointer - 0x8) == 6 ) // train action level has different camxOffset
		// if ((PS2_MEM_ReadPointer(RDR_someLevelPointer) == 0x1385C00U && PS2_MEM_ReadUInt(RDR_someLevelPointer - 0x8) == 0) ||
		// 	 PS2_MEM_ReadPointer(RDR_someLevelPointer) == 0x1400DA0U) // || PS2_MEM_ReadPointer(RDR_someLevelPointer) == 0x13A47E0U) // both pointers to same level on different boots, need a different solution
		// {
		// 	camxOffset = 0x1BC;
		// // } else if (PS2_MEM_ReadPointer(RDR_someLevelPointer) == 0x13A47E0) // 0x14060D0U) // ch.18 buffalo soldier section 2, changes from section 1
		// } else if (PS2_MEM_ReadPointer(RDR_someLevelPointer) == 0x14060D0U)
		// {
		// 	camxOffset = -0x17234;
		// }
		// // uint32_t camxbase = PS2_MEM_ReadPointer(cambase + 0x26C);
		// uint32_t camxbase = PS2_MEM_ReadPointer(cambase + camxOffset);
		// // if level == train level, camxbase = cambase + 0x1BC
		// float camx = PS2_MEM_ReadFloat(camxbase - 0x25C);
		// camx -= (float)xmouse * looksensitivity; // normal calculation method for X

		// // PS2_MEM_WriteFloat(AUF_car_camx, camx);
		// // PS2_MEM_WriteFloat(AUF_car_camy, camy);
		// // PS2_MEM_WriteFloat(RDR_camx, camx);
		// PS2_MEM_WriteFloat(camxbase - 0x25C, camx);
		// // PS2_MEM_WriteFloat(RDR_camy, camy);
		// PS2_MEM_WriteFloat(camYAddress, camy);
		// PS2_MEM_WriteFloat(AUF_camx, 0.f);
	}

}