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

#define RDR_camy 0x19B0AEC 
#define RDR_camx 0x1A25714

// #define RDR_camSanity1 0xF6C5D03F
// #define RDR_camSanity1Offset 0x10
#define RDR_camFov 0x10 // offset from RDR_cambase
#define RDR_camSanity 0x4F0180BF
#define RDR_camSanityOffset 0x38

// #define RDR_onLevel6MachineGunFlag 0x13E7C48 
// #define RDR_level6MachineGunCamX 0x181E82C
// #define RDR_level6MachineGunCamY 0x181E830

#define RDR_ch6GatlingX 0x181E82C
#define RDR_ch6GatlingY 0x181E830
#define RDR_ch6GatlingSanity 0x50548A01
#define RDR_ch6GatlingFlag 0x13E7C48 // may require finding a ptr to the current level as this may not be static, refer to someLevelPointer

#define RDR_ch20GatlingX 0x19E17CC
#define RDR_ch20GatlingY 0x19E17D0
#define RDR_ch20GatlingSanity 0x4049A901
#define RDR_ch20GatlingFlag 0x14667A8 

#define RDR_ch21GatlingX 0x1A099FC
#define RDR_ch21GatlingY 0x1A09A00
#define RDR_ch21GatlingSanity 0xA03CA601
#define RDR_ch21GatlingFlag 0x14B1D58 
#define RDR_ch21Gatling2Flag 0x14B3648

#define RDR_ch22GatlingX 0x165270C
#define RDR_ch22GatlingY 0x1652710
#define RDR_ch22GatlingSanity 0x70E77001
#define RDR_ch22GatlingFlag 0x1381A68

#define RDR_playerDueling 0x73CC88
#define RDR_duelingBase 0x751330
#define RDR_duelX 0x2E8
#define RDR_duelY 0x2E4

#define RDR_camy_base 0x73C170
#define RDR_cambase 0x7A00CC

#define RDR_someLevelPointer 0x55CFF8
#define RDR_canMoveCameraFlag 0x4F71F0

#define RDR_currentLevel 0x552B68
#define RDR_ch26rooftopcamx 0x17B8814
#define RDR_ch26rooftopcamy 0x17C682C
#define RDR_ch26rooftopCoverAiming 0x17A7D64

// #define RDR_aimLockTarget1 0x1B85B60
// #define RDR_aimLockTarget2 0x1B85B90
// #define RDR_aimLockFlag 0x1B85B94

#define RDR_aimLockBase 0x7608F0
#define RDR_aimLockTarget1 0x690
#define RDR_aimLockTarget2 0x6C0
#define RDR_aimLockFlag 0x6C4

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
static uint32_t camXOffsets[] = {0x278, 0x26C, 0x1BC, -0x17234};

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_RDR_Status(void)
{
	// return (MEM_ReadUInt(0x80000000) == 0x47473245U && MEM_ReadUInt(0x80000004) == 0x345A0000U); // check game header to see if it matches Trigger Man (GG2E4Z)
	// return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U);
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && PS2_MEM_ReadWord(0x00093394) == 0x5F323035U) &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E30303BU;
}

static uint8_t PS2_RDR_DetectCamera(void)
{
	uint32_t tempcambase = PS2_MEM_ReadPointer(RDR_cambase);
	if (PS2WITHINMEMRANGE(tempcambase))
	{
		if (PS2_MEM_ReadWord(tempcambase + RDR_camSanityOffset) == RDR_camSanity)
		{
			cambase = tempcambase;
			return 1;
		}
	}

	return 0;
}

static void PS2_RDR_Inject(void)
{
	// PS2_MEM_WriteFloat(AUF_health_lvl2, 2000.f);

	// TODO: prevent mousey on chat train
	// TODO: camy max and min to prevent warping at extremes
	// TODO: prevent mouse when dead
	// TODO: duel cursor? and draw with mouse? (down then up motion)
	// TODO: prevent mouse on pause menu
	// TODO: disable aim lock?
	// TODO: prevent mouse during in-game cutscenes
	// TODO: find a less brute force solution for gatling
	// TODO: prevent mouse in shops
	// TODO: fix weird 'popping' when circling mouse, might just be the animation?
	//  		doesn't feel like it effects aiming but looks weird
	//			seems like camera goes off center for 1 frame and pops back?	

	// disable aim lock
	uint32_t aimLockBase = PS2_MEM_ReadPointer(RDR_aimLockBase);
	if (PS2_MEM_ReadWord(aimLockBase) == 0x68945000)
	{
		PS2_MEM_WriteWord(aimLockBase + RDR_aimLockTarget1, 0x0);
		PS2_MEM_WriteWord(aimLockBase + RDR_aimLockTarget2, 0x0);
		PS2_MEM_WriteWord(aimLockBase + RDR_aimLockFlag, 0x0);
	}

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	uint32_t currentLevel = PS2_MEM_ReadWord(RDR_currentLevel);
	if (currentLevel == 0x13000000 || currentLevel == 0x00000000 || currentLevel == 0x03000000) // on main menu or loading
		return;

	if (!PS2_RDR_DetectCamera())
		return;
	
	// TODO: check if in town as just this flag breaks ch.16 jailbreak
	// if (PS2_MEM_ReadWord(RDR_canMoveCameraFlag) == 0x01010000U) // return if in an interior where camera movement is disabled
	// 	return;

	// uint8_t preventCamY = 0;
	// if (currentLevel == 0x15000000) // Ch.4 The Traincar, talk and duel
	// 	preventCamY = 1;

	// 1.631041288, FOV while walking
	float fov = PS2_MEM_ReadFloat(cambase + RDR_camFov);
	float looksensitivity = (float)sensitivity / 12000.f / (fov / 1.6f) / 1.4f;

	uint32_t camxOffset = 0;
	uint32_t camxbase = 0;
	uint8_t camXSet = 0;

	// cambase + someOffset gets a pointer to (&camX + 0x25C), &camX is the pointer - 0x25C
	int i;
	for (i = 0; i < 4; i++) // loop through the known camXOffsets from cambase
	{
		camxOffset = camXOffsets[i];
		if (currentLevel == 0x1D000000)
			camxOffset = 0x280;
		camxbase = PS2_MEM_ReadPointer(cambase + camxOffset);

		if (PS2_MEM_ReadWord(camxbase - 0x250) == 0xC8145100) // check that pointer points to a camx object
		{
			camXSet = 1; // a proper camx was found
			break;
		}
	}

	float gatlingX = 0;
	float gatlingY = 0;
	uint32_t gatlingSanity = 0;
	uint32_t gatlingFlag = 0;
	uint8_t onGatling = 0;

	// check for a level with a gatling gun
	switch(currentLevel) {
		case 0x39000000 : // chapter 6, Carnival Life
			gatlingX = RDR_ch6GatlingX;
			gatlingY = RDR_ch6GatlingY;
			gatlingSanity = RDR_ch6GatlingSanity;
			gatlingFlag = RDR_ch6GatlingFlag;
			onGatling = 1;
			break;
		case 0x70000000 : // chapter 20, Fort Diego
			gatlingX = RDR_ch20GatlingX;
			gatlingY = RDR_ch20GatlingY;
			gatlingSanity = RDR_ch20GatlingSanity;
			gatlingFlag = RDR_ch20GatlingFlag;
			onGatling = 1;
			break;
		case 0x68000000 : // chapter 21, End of the Line
			gatlingX = RDR_ch21GatlingX;
			gatlingY = RDR_ch21GatlingY;
			gatlingSanity = RDR_ch21GatlingSanity;
			gatlingFlag = RDR_ch21GatlingFlag;
			onGatling = 1;
			break;
		case 0x66000000 : // chapter 21, End of the Line
			gatlingX = RDR_ch21GatlingX;
			gatlingY = RDR_ch21GatlingY;
			gatlingSanity = RDR_ch21GatlingSanity;
			gatlingFlag = RDR_ch21Gatling2Flag;
			onGatling = 1;
			break;
		case 0x40000000 : // chapter 22, Angels and Devils
			gatlingX = RDR_ch22GatlingX;
			gatlingY = RDR_ch22GatlingY;
			gatlingSanity = RDR_ch22GatlingSanity;
			gatlingFlag = RDR_ch22GatlingFlag;
			onGatling = 1;
			break;
	}	

	if (PS2_MEM_ReadWord(RDR_playerDueling) == 0x40020000)	
	{
		uint32_t duelBase = PS2_MEM_ReadPointer(RDR_duelingBase);
		float duelX = PS2_MEM_ReadFloat(duelBase - RDR_duelX);
		duelX += (float)xmouse * looksensitivity / 2.f;

		float duelY = PS2_MEM_ReadFloat(duelBase - RDR_duelY);
		duelY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / 2.f;

		PS2_MEM_WriteFloat(duelBase - RDR_duelX, duelX);
		PS2_MEM_WriteFloat(duelBase - RDR_duelY, duelY);
	}
	// if (currentLevel == 0x39000000 && PS2_MEM_ReadWord(RDR_onLevel6MachineGunFlag) == 0x01000000U && PS2_MEM_ReadWord(RDR_onLevel6MachineGunFlag + 0x4) == 0x50548A01U) // onMachineGun and sanity check
	else if (onGatling == 1 && PS2_MEM_ReadWord(gatlingFlag) == 0x01000000 && PS2_MEM_ReadWord(gatlingFlag + 0x4) == gatlingSanity)
	{
		// on gatling gun
		float camx = PS2_MEM_ReadFloat(gatlingX);
		camx -= (float)xmouse * looksensitivity;

		float camy = PS2_MEM_ReadFloat(gatlingY);
		camy -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;

		PS2_MEM_WriteFloat(gatlingX, camx);
		PS2_MEM_WriteFloat(gatlingY, camy);

	} 
	else if (camXSet == 1 && PS2_MEM_ReadWord(camxbase - 0x25C + 0xCE0) == 0x00000000) // aiming from cover
	{
		// &camx + 0xCE0 = isCoverAiming
		uint32_t coverbase = camxbase - 0x25C - 0x124;

		float coverX = PS2_MEM_ReadFloat(coverbase + 0x4);
		coverX -= (float)xmouse * looksensitivity;
		PS2_MEM_WriteFloat(coverbase + 0x4, coverX);

		float coverY = PS2_MEM_ReadFloat(coverbase + 0x24);
		coverY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;
		PS2_MEM_WriteFloat(coverbase + 0x24, coverY);
	}
	else if (currentLevel == 0x1D000000) // chapter 26, final rooftop scene has different cam offsets?
	{
		if (PS2_MEM_ReadWord(RDR_ch26rooftopCoverAiming) == 0x00000000) // if aiming from cover
		{
			uint32_t coverbase = RDR_ch26rooftopcamx - 0x124;

			float coverX = PS2_MEM_ReadFloat(coverbase + 0x4);
			coverX -= (float)xmouse * looksensitivity;
			PS2_MEM_WriteFloat(coverbase + 0x4, coverX);

			float coverY = PS2_MEM_ReadFloat(coverbase + 0x24);
			coverY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;
			PS2_MEM_WriteFloat(coverbase + 0x24, coverY);
		}
		else  // regular aiming
		{
			// uint32_t camYBase = PS2_MEM_ReadPointer(PS2_MEM_ReadPointer(RDR_camy_base) + 0x30);
			// float camy = PS2_MEM_ReadFloat(camYBase + 0x1C);
			// camy += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;
			// PS2_MEM_WriteFloat(camYBase + 0x1C, camy);

			uint32_t cameraBase = PS2_MEM_ReadPointer(RDR_camy_base);
			uint32_t camYAddress = PS2_MEM_ReadPointer(cameraBase + 0x30) + 0x1C;
			float camy = PS2_MEM_ReadFloat(camYAddress);
			camy += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;
			PS2_MEM_WriteFloat(camYAddress, (float)camy);

			// float camy = PS2_MEM_ReadFloat(RDR_ch26rooftopcamy);
			// camy += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;
			// PS2_MEM_WriteFloat(RDR_ch26rooftopcamy, camy);

			// float camx = PS2_MEM_ReadFloat(RDR_ch26rooftopcamx);
			// camx -= (float)xmouse * looksensitivity;
			// PS2_MEM_WriteFloat(RDR_ch26rooftopcamx, camx);

			float camx = PS2_MEM_ReadFloat(camxbase + 0x1A4);
			camx -= (float)xmouse * looksensitivity;
			PS2_MEM_WriteFloat(camxbase + 0x1A4, (float)camx);
		}
	}
	else
	{

		uint32_t cameraBase = PS2_MEM_ReadPointer(RDR_camy_base);
		uint32_t camYAddress = PS2_MEM_ReadPointer(cameraBase + 0x30) + 0x1C;
		float camy = PS2_MEM_ReadFloat(camYAddress);
		camy += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;
		PS2_MEM_WriteFloat(camYAddress, (float)camy);

		float camx = PS2_MEM_ReadFloat(camxbase - 0x25C);
		camx -= (float)xmouse * looksensitivity;
		PS2_MEM_WriteFloat(camxbase - 0x25C, (float)camx);
	}

}