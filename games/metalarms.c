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

#define SENTINEL_PITCH_UPPER_BOUND 1.
#define SENTINEL_PITCH_LOWER_BOUND -1.

#define BOT_STRUCT_CBOTDEF_OFFSET 0x0

#define BOT_STRUCT_PITCH_OFFSET 0x258
#define BOT_STRUCT_YAW_OFFSET 0x264
#define BOT_STRUCT_YAW_SIN_OFFSET 0x268
#define BOT_STRUCT_YAW_COS_OFFSET 0x26c
#define BOT_STRUCT_UNIT_FRONT_X_OFFSET 0x270
#define BOT_STRUCT_UNIT_FRONT_Y_OFFSET 0x274
#define BOT_STRUCT_VEHICLE_OFFSET 0x3F0

#define VEHICLE_SENTINEL_TURRET_YAW_OFFSET 0x2e30
#define VEHICLE_SENTINEL_TURRET_PITCH_OFFSET 0x2e38

#define CBOTDEF_STRUCT_BOT_RACE_OFFSET 0x0
#define CBOTDEF_STRUCT_BOT_CLASS_OFFSET 0x4
#define CBOTDEF_STRUCT_BOT_SUB_CLASS_OFFSET 0x8

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

typedef enum {
  BOTCLASS_MINER,
  BOTCLASS_COLOSSUS,
  BOTCLASS_SCIENTIST,
  BOTCLASS_CHEMBOT,
  BOTCLASS_ALLOY,
  BOTCLASS_ZOBBY,
  BOTCLASS_BARTER,
  BOTCLASS_VERMIN,
  BOTCLASS_AMBIENT,
  BOTCLASS_KRUNK,
  BOTCLASS_GRUNT,
  BOTCLASS_ELITE_GUARD,
  BOTCLASS_JUMP_TROOPER,
  BOTCLASS_TITAN,
  BOTCLASS_SNIPER,
  BOTCLASS_SWARMER,
  BOTCLASS_SWARMER_BOSS,
  BOTCLASS_PREDATOR,
  BOTCLASS_SITEWEAPON,
  BOTCLASS_MORTAR,
  BOTCLASS_CORROSIVE,
  BOTCLASS_LOADER,
  BOTCLASS_RAT,
  BOTCLASS_SENTINEL,
  BOTCLASS_PROBE,
  BOTCLASS_SCOUT,
  BOTCLASS_AAGUN,
  BOTCLASS_SNARQ,
  BOTCLASS_ZOMBIE,
  BOTCLASS_ZOMBIE_BOSS
} BotClass_e;

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
  
  const float looksensitivity = (float)sensitivity / 325.f; //Using equivalent 360 distance at my dpi for tf2
  const float mouse_modifier = 1./40.;

  float pitch;
  float yaw;

  // Get Bot Type
  uint32_t cbotdef_offset = MEM_ReadUInt(current_bot_offset + BOT_STRUCT_CBOTDEF_OFFSET);
  BotClass_e bot_class = MEM_ReadUInt(cbotdef_offset + CBOTDEF_STRUCT_BOT_CLASS_OFFSET);

  // Check if in vehicle
  uint32_t vehicle_offset = MEM_ReadUInt(current_bot_offset + BOT_STRUCT_VEHICLE_OFFSET);
  // If not null must be in a vehicle
  if (vehicle_offset) {
    // Get Vehicle type
    uint32_t vehicle_cbotdef_offset = MEM_ReadUInt(vehicle_offset + BOT_STRUCT_CBOTDEF_OFFSET);
    BotClass_e vehicle_class = MEM_ReadUInt(vehicle_cbotdef_offset + CBOTDEF_STRUCT_BOT_CLASS_OFFSET);
    //if (vehicle_class == BOTCLASS_SENTINEL) {
      pitch = MEM_ReadFloat(vehicle_offset + VEHICLE_SENTINEL_TURRET_PITCH_OFFSET);
      yaw = MEM_ReadFloat(vehicle_offset + VEHICLE_SENTINEL_TURRET_YAW_OFFSET);
    
      pitch += (float)(!invertpitch ? ymouse : -ymouse) * mouse_modifier * looksensitivity;
      yaw += (float)xmouse * mouse_modifier * looksensitivity;
    
      if (pitch > SENTINEL_PITCH_UPPER_BOUND) pitch = SENTINEL_PITCH_UPPER_BOUND;
      if (pitch < SENTINEL_PITCH_LOWER_BOUND) pitch = SENTINEL_PITCH_LOWER_BOUND;
      while(yaw <= -PI)
    	  yaw += TAU;
      while(yaw >= PI)
    	  yaw -= TAU;

      MEM_WriteFloat(vehicle_offset + VEHICLE_SENTINEL_TURRET_PITCH_OFFSET, pitch);
      MEM_WriteFloat(vehicle_offset + VEHICLE_SENTINEL_TURRET_YAW_OFFSET, yaw);
    //}

  } else {
    pitch = MEM_ReadFloat(current_bot_offset + BOT_STRUCT_PITCH_OFFSET);
    yaw = MEM_ReadFloat(current_bot_offset + BOT_STRUCT_YAW_OFFSET);


    pitch += (float)(!invertpitch ? ymouse : -ymouse) * mouse_modifier * looksensitivity;
    yaw += (float)xmouse * mouse_modifier * looksensitivity;
    
    if (pitch > PITCH_UPPER_BOUND) pitch = PITCH_UPPER_BOUND;
    if (pitch < PITCH_LOWER_BOUND) pitch = PITCH_LOWER_BOUND;

    while(yaw <= -PI)
    	yaw += TAU;
    while(yaw >= PI)
    	yaw -= TAU;
    float yaw_sin = sin(yaw);
    float yaw_cos = cos(yaw);

    MEM_WriteFloat(current_bot_offset + BOT_STRUCT_PITCH_OFFSET, pitch);
    MEM_WriteFloat(current_bot_offset + BOT_STRUCT_YAW_OFFSET, yaw);
    MEM_WriteFloat(current_bot_offset + BOT_STRUCT_YAW_SIN_OFFSET, yaw_sin);
    MEM_WriteFloat(current_bot_offset + BOT_STRUCT_YAW_COS_OFFSET, yaw_cos);
    MEM_WriteFloat(current_bot_offset + BOT_STRUCT_UNIT_FRONT_X_OFFSET, yaw_sin);
    MEM_WriteFloat(current_bot_offset + BOT_STRUCT_UNIT_FRONT_Y_OFFSET, yaw_cos);
  }

}
