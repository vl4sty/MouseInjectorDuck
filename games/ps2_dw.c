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

#define DW_cambase 0x6C0230 // requires use of cambase cheat
#define DW_cambase_sanity 0x80
#define DW_cambase_sanity_value 0xCCCCCC3F
// #define DW_cambase_sanity2 0x8
// #define DW_cambase_sanity2_value 0x285E7800U
#define DW_cambase_sanity2 0x13C
#define DW_cambase_sanity2_value 0x6666E63F
#define DW_cambase_sanity3 0x1EC
#define DW_cambase_sanity3_value 0x00001643U
#define DW_cambase_sanity4 0x0
#define DW_cambase_sanity4_value 0x50A57000U
#define DW_camx 0x144
#define DW_camy 0x140
// #define DW_cambase 0x1FFF2A0 // blips between different values but consistently gives a cambase value
// #define DW_cambase_sanity 0xC0 // negative offset
// #define DW_cambase_sanity_value 0x6666E63FU
// #define DW_cambase_sanity2 0x10 // negative offset
// #define DW_cambase_sanity2_value 0x00001643U
// #define DW_camx 0xB8 // negative offset
// #define DW_camy 0xBC // negative offset

#define DW_coyoteCamPtr 0x953D44
// offsets from coyote base
#define DW_coyote_camY 0x6C
#define DW_coyote_camX_sin 0x70
#define DW_coyote_camX_cos 0x68
#define DW_coyote_sanity 0x8C
#define DW_player_in_coyote 0x7C
#define DW_coyote_sanity_value 0x696E2FA6

#define DW_turretBase 0x720590 // Requires use of turretBase cheat
#define DW_turret_camY 0x108
#define DW_turret_camX 0x10C
#define DW_turret_sanity 0x104
#define DW_turret_sanity_value 0xCF09DE58
#define DW_player_in_turret_offset 0x1E8
#define DW_player_in_turret 0x120
#define DW_player_in_turret2 0x178

#define DW_horsebase_ptr_1 0xEC8F18 // chain of pointers
#define DW_horsebase_ptr_2_offset 0x8
#define DW_horsebase_ptr_3_offset 0x4
// offsets from horsebase
#define DW_horse_camY 0xE0
#define DW_horse_camX_sin 0xE4
#define DW_horse_camX_cos 0xDC

#define DW_fov_base 0x6C0090 // requires use of FOV cheat
#define DW_fov 0x170
// #define DW_fov_default 0x16C

#define DW_COYOTE_TOTAL_ANGLE_UNSET -99
#define DW_HORSE_TOTAL_ANGLE_UNSET -99

#define DW_auto_center 0x20517C

static uint8_t PS2_DW_Status(void);
static uint8_t PS2_DW_DetectCamera(void);
static void PS2_DW_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"PS2 Darkwatch",
	PS2_DW_Status,
	PS2_DW_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_DARKWATCH = &GAMEDRIVER_INTERFACE;

static uint32_t cambase = 0;
static uint32_t coyotebase = 0;
static float coyoteTotalAngle = DW_COYOTE_TOTAL_ANGLE_UNSET;
static uint32_t turretBase = 0;
static uint32_t horsebase = 0;
static float horseTotalAngle = DW_HORSE_TOTAL_ANGLE_UNSET;

static uint8_t PS2_DW_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && PS2_MEM_ReadWord(0x00093394) == 0x5F323130U) &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E34323BU;
}

static uint8_t PS2_DW_DetectCamera(void)
{
	uint32_t tempcambase = PS2_MEM_ReadPointer(DW_cambase);
	if (PS2_MEM_ReadWord(tempcambase + DW_cambase_sanity) == DW_cambase_sanity_value && PS2_MEM_ReadWord(tempcambase + DW_cambase_sanity2) == DW_cambase_sanity2_value
		&& PS2_MEM_ReadWord(tempcambase + DW_cambase_sanity4) == DW_cambase_sanity4_value)
	{
		cambase = tempcambase;
		return 1;
	}

	return 0;
}

static void PS2_DW_Inject(void)
{
	// TODO: find more consistent coyoteBase pointer? does it break sometimes?
	// TODO: horse camY on chapter 14 doesn't go as high as it should, due to weird transform used
	//			the float used for camY also affects the camXSin when pitch is high
	//			may require keeping track of the pitch angle as well like the camX
	// TODO: prevent mouse while on pause/main menu
	// TODO: FOV cheat to increase max FOV
	// TODO: find pointer chains for cambase, turretbase, etc., to avoid having to use external cheats

	// did player get out of coyote?
	if (coyotebase && PS2_MEM_ReadUInt(coyotebase + DW_player_in_coyote) == 0) {
		coyotebase = 0;
	}
	// check for valid coyotebase if unset
	if (coyotebase == 0)
	{
		uint32_t coyoteBasePtr = PS2_MEM_ReadPointer(DW_coyoteCamPtr) + 0xC;
		uint32_t tempcoyotebase = PS2_MEM_ReadPointer(coyoteBasePtr);

		if (PS2_MEM_ReadWord(tempcoyotebase + DW_coyote_sanity) == DW_coyote_sanity_value)
		{
			coyotebase = tempcoyotebase;
			coyoteTotalAngle = DW_COYOTE_TOTAL_ANGLE_UNSET;
		}
	}

	uint32_t tempturretbase = PS2_MEM_ReadPointer(DW_turretBase);
	if (PS2_MEM_ReadWord(tempturretbase + DW_turret_sanity) == DW_turret_sanity_value)
	{
		// if (PS2_MEM_ReadUInt(turretBase + DW_player_in_turret) == 0x0) { // do not set if player is not in a turret
		// if (PS2_MEM_ReadUInt(tempturretbase + DW_player_in_turret_offset) != DW_player_in_turret) { // do not set if player is not in a turret
		// 	turretBase = 0;
		// }
		// else {
		// 	turretBase = tempturretbase;
		// }
		// if (PS2_MEM_ReadUInt(tempturretbase + DW_player_in_turret) != 0x0) {
		// if (PS2_MEM_ReadUInt(tempturretbase + DW_player_in_turret_offset) == 0x00020002) {
		// check that turret is occupied, and 2 pointers are set indicating the player is occupying it
		if (PS2_MEM_ReadUInt(tempturretbase + DW_player_in_turret_offset) == 0x00020002 && PS2_MEM_ReadUInt(tempturretbase + 0x20) != 0x0
			&& PS2_MEM_ReadUInt(tempturretbase + DW_player_in_turret2) != 0x0) {
			turretBase = tempturretbase;
		}
		else {
			turretBase = 0;
		}
	}

	// check for valid horsebase if unset
	if (horsebase == 0)
	{
		// go through a chain of pointers to get to horsebase since it is not statically stored anywhere
		uint32_t horse1 = PS2_MEM_ReadPointer(DW_horsebase_ptr_1); // address @ 0xEC8F18 | [0x161F9F0]
		uint32_t horse2 = PS2_MEM_ReadPointer(horse1 + DW_horsebase_ptr_2_offset); // address @ 0x161F9F0 + 0x8 | [0x1629080]
		uint32_t temphorsebase = PS2_MEM_ReadPointer(horse2 + DW_horsebase_ptr_3_offset); // address @ 0x1629080 + 0x4 | [0x16327F8]

		if (PS2_MEM_ReadWord(temphorsebase) == 0xE0A07000) // horsebase sanity check
		{
			horsebase = temphorsebase;
		}
	}

	if(xmouse == 0 && ymouse == 0) { // if mouse is idle
		return;
	}

	float looksensitivity = (float)sensitivity / 40.f;

	if (PS2_MEM_ReadWord(horsebase) == 0xE0A07000) // on horse
	{
		float horseCamY = PS2_MEM_ReadFloat(horsebase + DW_horse_camY);

		float camXSin = PS2_MEM_ReadFloat(horsebase + DW_horse_camX_sin);
		float camXCos = PS2_MEM_ReadFloat(horsebase + DW_horse_camX_cos);

		// keep track of total rotation angle since it is not kept in-game
		float angle = atan(camXSin / camXCos);
		if (horseTotalAngle == DW_HORSE_TOTAL_ANGLE_UNSET) {
			horseTotalAngle = angle;
		}

		float angleChange = (float)xmouse * looksensitivity / 200.f;

		angle += angleChange;
		horseTotalAngle += angleChange;
		// TODO: while totalAngle > or < TAU, -TAU

		camXSin = sin(horseTotalAngle);
		camXCos = cos(horseTotalAngle);

		PS2_MEM_WriteFloat(horsebase + DW_horse_camX_sin, (float)camXSin);
		PS2_MEM_WriteFloat(horsebase + DW_horse_camX_cos, (float)camXCos);

		horseCamY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / 200.f;

		PS2_MEM_WriteFloat(horsebase + DW_horse_camY, horseCamY);
	}
	else if (turretBase != 0 && PS2_MEM_ReadWord(turretBase + DW_turret_sanity) == DW_turret_sanity_value) // in turret | CURRENTLY NOT WORKING
	// else if (turretBase && PS2_MEM_ReadWord(turretBase + DW_player_in_turret_offset) == DW_player_in_turret) // in turret | CURRENTLY NOT WORKING
	{
		float camX = PS2_MEM_ReadFloat(turretBase + DW_turret_camX);
		float camY = PS2_MEM_ReadFloat(turretBase + DW_turret_camY);

		camX -= (float)xmouse * looksensitivity / 200.f;
		camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / 200.f;

		PS2_MEM_WriteFloat(turretBase + DW_turret_camX, camX);
		PS2_MEM_WriteFloat(turretBase + DW_turret_camY, camY);
	}
	else if (coyotebase && PS2_MEM_ReadWord(coyotebase + DW_coyote_sanity) == DW_coyote_sanity_value) // in coyote vehicle
	{
		// in coyote
		float camY = PS2_MEM_ReadFloat(coyotebase + DW_coyote_camY);

		float camXSin = PS2_MEM_ReadFloat(coyotebase + DW_coyote_camX_sin);
		float camXCos = PS2_MEM_ReadFloat(coyotebase + DW_coyote_camX_cos);

		float angle = atan(camXSin / camXCos);
		if (coyoteTotalAngle == DW_COYOTE_TOTAL_ANGLE_UNSET) {
			coyoteTotalAngle = angle;
		}

		float angleChange = (float)xmouse * looksensitivity / 200.f;

		angle += angleChange;
		coyoteTotalAngle += angleChange;
		// TODO: while totalAngle > or < TAU, -TAU

		camXSin = sin(coyoteTotalAngle);
		camXCos = cos(coyoteTotalAngle);

		PS2_MEM_WriteFloat(coyotebase + DW_coyote_camX_sin, (float)camXSin);
		PS2_MEM_WriteFloat(coyotebase + DW_coyote_camX_cos, (float)camXCos);

		camY += (float)(invertpitch ? ymouse : -ymouse) * looksensitivity / 200.f;
		PS2_MEM_WriteFloat(coyotebase + DW_coyote_camY, camY);
	}
	else
	{
		// on foot
		coyotebase = 0;
		horsebase = 0;
		horseTotalAngle = DW_HORSE_TOTAL_ANGLE_UNSET;
		coyoteTotalAngle = DW_COYOTE_TOTAL_ANGLE_UNSET;

		uint32_t fovBase = PS2_MEM_ReadPointer(DW_fov_base);
		float fov = 55.f;
		if (fovBase)
			fov = PS2_MEM_ReadFloat(fovBase + DW_fov);


		if (PS2_MEM_ReadWord(cambase + DW_cambase_sanity) != DW_cambase_sanity_value || PS2_MEM_ReadWord(cambase + DW_cambase_sanity2) != DW_cambase_sanity2_value
			|| PS2_MEM_ReadWord(cambase + DW_cambase_sanity4) != DW_cambase_sanity4_value)
		{
			if (!PS2_DW_DetectCamera()) {
				return;
			}
		}

		float camX = PS2_MEM_ReadFloat(cambase + DW_camx);
		float camY = PS2_MEM_ReadFloat(cambase + DW_camy);
		camX /= 180.f;
		camY /= 180.f;

		camX -= (float)xmouse * looksensitivity / 1200.f * (fov / 55.f);
		camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / 1200.f * 1.1f * (fov / 55.f);

		camX *= 180.f;
		camY *= 180.f;

		camY = ClampFloat(camY, -80.f, 80.f);

		PS2_MEM_WriteFloat(cambase + DW_camx, camX);
		PS2_MEM_WriteFloat(cambase + DW_camy, camY);
	}
}