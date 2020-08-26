//==========================================================================
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

#define PI 3.14159265f // 0x40490FDB
#define TAU 6.2831853f // 0x40C90FDB


//TODO
//1. Yaw does not work on elevators???
//2. Does not work at all when inside a vehicle
//3. When stationary, glitch moves to the center of the screen, only when moving again does it snap to being correct - fix math

// MA CONSTANTS
#define PLAYER_NUMBER 0
// Metal Arms Addresses
// MOHEA ADDRESSES - OFFSET ADDRESSES BELOW (REQUIRES PLAYERBASE TO USE)
#define PLAYER_STRUCT_OFFSET 0x80481af8
#define PLAYER_STRUCT_SIZE 0x24c8
#define PLAYER_STRUCT_ACTUAL (PLAYER_STRUCT_OFFSET + PLAYER_NUMBER * PLAYER_STRUCT_SIZE)
#define PLAYER_STRUCT_ORIG_BOT_POINTER 0x18d8
#define PLAYER_STRUCT_CURRENT_BOT_POINTER 0x18dc

#define PITCH_UPPER_BOUND 1.
#define PITCH_LOWER_BOUND -1.

#define YAW_UPPER_BOUND 1.
#define YAW_LOWER_BOUND -1.

#define BOT_STRUCT_PITCH_OFFSET 0x258
#define BOT_STRUCT_YAW_OFFSET 0x264

static uint8_t METALARMS_Status(void);
static void METALARMS_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Metal Arms: Glitch in the System",
	METALARMS_Status,
	METALARMS_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_METALARMS = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t METALARMS_Status(void)
{
	return (MEM_ReadUInt(0x80000000) == 0x474d3545U && MEM_ReadUInt(0x80000004) == 0x37440000U); // check game header to see if it matches Metal Arms: GM5E7D
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void METALARMS_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	const uint32_t current_bot_offset = MEM_ReadUInt(PLAYER_STRUCT_ACTUAL + PLAYER_STRUCT_CURRENT_BOT_POINTER);
	if(NOTWITHINMEMRANGE(current_bot_offset))
		return;
  float pitch = MEM_ReadFloat(current_bot_offset + BOT_STRUCT_PITCH_OFFSET);
  float yaw = MEM_ReadFloat(current_bot_offset + BOT_STRUCT_YAW_OFFSET);

  const float looksensitivity = (float)sensitivity / 1500.f; //Using equivalent 360 distance at my dpi for tf2

  pitch += (float)(!invertpitch ? ymouse : -ymouse)/10. * looksensitivity;
  yaw += (float)xmouse/10 * looksensitivity;
  
  if (pitch > PITCH_UPPER_BOUND) pitch = PITCH_UPPER_BOUND;
  if (pitch < PITCH_LOWER_BOUND) pitch = PITCH_LOWER_BOUND;

	while(yaw <= -TAU)
		yaw += TAU;
	while(yaw >= TAU)
		yaw -= TAU;

  MEM_WriteFloat(current_bot_offset + BOT_STRUCT_PITCH_OFFSET, pitch);
  MEM_WriteFloat(current_bot_offset + BOT_STRUCT_YAW_OFFSET, yaw);
}
