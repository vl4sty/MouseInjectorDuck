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
#include <math.h>
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
#define BOT_STRUCT_UNIT_FRONT_Z_OFFSET 0x278
#define BOT_STRUCT_VEHICLE_OFFSET 0x3f0
#define BOT_STRUCT_MECH_OFFSET 0x56c

#define BOT_AAGUN_PITCH_OFFSET 0xa78
#define BOT_AAGUN_YAW_OFFSET 0xa74

// The vehicle offset points to an entity class so CBOTDEF should be same for all, since the structure doesnt change til after vehicle
#define ENTITY_CBOTDEF_OFFSET 0x1a8 //Also is likely the length of entity since cbotdef is the first value in bot
#define VEHICLE_SENTINEL_TURRET_YAW_OFFSET 0x2e30
#define VEHICLE_SENTINEL_TURRET_PITCH_OFFSET 0x2e38
#define VEHICLE_DRIVER_OFFSET 0xd14

#define VEHICLE_RAT_DRIVER_CAMERA_PITCH_OFFSET 0xcd8 // seems to be camera height idfk, cc0 seems to be the x of the camera look back vector???
#define VEHICLE_RAT_DRIVER_CAMERA_RAW_STEERING_POSITION_OFFSET 0x2e60
#define VEHICLE_RAT_GUNNER_OFFSET 0x2d70

#define VEHICLE_RAT_SITE_WEAPON_DATA_OFFSET 0x3ad0

#define SITE_WEAPON_STRUCT_WEAPON_DATA_OFFSET 0xc58

#define SITE_WEAPON_DATA_HALF_FOV 0x120
#define SITE_WEAPON_AIMX_RIGHT_X 0x40
#define SITE_WEAPON_AIMX_RIGHT_Y 0x44
#define SITE_WEAPON_AIMX_RIGHT_Z 0x48
#define SITE_WEAPON_AIMX_UP_X 0x50
#define SITE_WEAPON_AIMX_UP_Y 0x54
#define SITE_WEAPON_AIMX_UP_Z 0x58
#define SITE_WEAPON_AIMX_FRONT_X 0x60
#define SITE_WEAPON_AIMX_FRONT_Y 0x64
#define SITE_WEAPON_AIMX_FRONT_Z 0x68
#define SITE_WEAPON_DATA_YAW_OFFSET 0x14c
#define SITE_WEAPON_DATA_PITCH_OFFSET 0x150

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

void update_pitch(float* pitch, float mouse_modifier, float look_sensitivity) {
  *pitch += (float)(!invertpitch ? ymouse : -ymouse) * mouse_modifier * look_sensitivity;
  // Clamp pitch
  if (*pitch > PITCH_UPPER_BOUND) *pitch = PITCH_UPPER_BOUND;
  if (*pitch < PITCH_LOWER_BOUND) *pitch = PITCH_LOWER_BOUND;

}

void update_yaw(float* yaw, float mouse_modifier, float look_sensitivity) {
    *yaw += (float)xmouse * mouse_modifier * look_sensitivity;
    
    while(*yaw <= -PI)
    	*yaw += TAU;
    while(*yaw >= PI)
    	*yaw -= TAU;

}

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
  
  const float look_sensitivity = (float)sensitivity / 325.f; //Using equivalent 360 distance at my dpi for tf2
  const float mouse_modifier = 1./40.;

  float pitch;
  float yaw;

  // Get Bot Type
  uint32_t cbotdef_offset = MEM_ReadUInt(current_bot_offset + BOT_STRUCT_CBOTDEF_OFFSET);
  BotClass_e bot_class = MEM_ReadUInt(cbotdef_offset + CBOTDEF_STRUCT_BOT_CLASS_OFFSET);

  // Check if in vehicle
  uint32_t vehicle_offset = MEM_ReadUInt(current_bot_offset + BOT_STRUCT_VEHICLE_OFFSET);
  // Check if in a turret
  uint32_t mech_offset = MEM_ReadUInt(current_bot_offset + BOT_STRUCT_MECH_OFFSET);
  // If not null must be in a vehicle
  if (vehicle_offset) {
    // Get Vehicle type
    uint32_t vehicle_cbotdef_offset = MEM_ReadUInt(vehicle_offset + ENTITY_CBOTDEF_OFFSET);
    BotClass_e vehicle_class = MEM_ReadUInt(vehicle_cbotdef_offset + CBOTDEF_STRUCT_BOT_CLASS_OFFSET);
    if (vehicle_class == BOTCLASS_SENTINEL) {
      pitch = MEM_ReadFloat(vehicle_offset + VEHICLE_SENTINEL_TURRET_PITCH_OFFSET);
      yaw = MEM_ReadFloat(vehicle_offset + VEHICLE_SENTINEL_TURRET_YAW_OFFSET);
    
      update_pitch(&pitch, mouse_modifier, look_sensitivity);
      update_yaw(&yaw, mouse_modifier, look_sensitivity);
      
      MEM_WriteFloat(vehicle_offset + VEHICLE_SENTINEL_TURRET_PITCH_OFFSET, pitch);
      MEM_WriteFloat(vehicle_offset + VEHICLE_SENTINEL_TURRET_YAW_OFFSET, yaw);
    } else if (vehicle_class == BOTCLASS_LOADER) {
      // seems to just use the normal bot stuff
      pitch = MEM_ReadFloat(vehicle_offset + BOT_STRUCT_PITCH_OFFSET);
      yaw = MEM_ReadFloat(vehicle_offset + BOT_STRUCT_YAW_OFFSET);

      update_pitch(&pitch, mouse_modifier, look_sensitivity);
      update_yaw(&yaw, mouse_modifier, look_sensitivity);

      float yaw_sin = sin(yaw);
      float yaw_cos = cos(yaw);

      // Added cbotdef offset since vehicle points to entity base not bot base and cbotdef is the start of the bot offset
      MEM_WriteFloat(vehicle_offset + BOT_STRUCT_PITCH_OFFSET, pitch);
      MEM_WriteFloat(vehicle_offset + BOT_STRUCT_YAW_OFFSET, yaw);
      MEM_WriteFloat(vehicle_offset + BOT_STRUCT_YAW_SIN_OFFSET, yaw_sin);
      MEM_WriteFloat(vehicle_offset + BOT_STRUCT_YAW_COS_OFFSET, yaw_cos);
      MEM_WriteFloat(vehicle_offset + BOT_STRUCT_UNIT_FRONT_X_OFFSET, yaw_sin);
      MEM_WriteFloat(vehicle_offset + BOT_STRUCT_UNIT_FRONT_Z_OFFSET, yaw_cos);

    } else if (vehicle_class == BOTCLASS_RAT) {
      //If RAT then have to check if gunner or driver
      const uint32_t driver_offset = MEM_ReadUInt(vehicle_offset + VEHICLE_DRIVER_OFFSET);
      const uint32_t gunner_offset = MEM_ReadUInt(vehicle_offset + VEHICLE_RAT_GUNNER_OFFSET);
      if (driver_offset == current_bot_offset) {
        //We the driver yoooo
        // pitch doesnt work, not sure what to do
        // pitch = MEM_ReadFloat(vehicle_offset + VEHICLE_RAT_DRIVER_CAMERA_PITCH_OFFSET);
        yaw = MEM_ReadFloat(vehicle_offset + VEHICLE_RAT_DRIVER_CAMERA_RAW_STEERING_POSITION_OFFSET);

        //update_pitch(&pitch, mouse_modifier, look_sensitivity);
        // Turning wheels seems to be bound from -1 to 1 and triple sense for feel
        update_yaw(&yaw, mouse_modifier, look_sensitivity * 3.);
        if (yaw > 1.) yaw = 1.;
        if (yaw < -1.) yaw = -1.;

        // Added cbotdef offset since vehicle points to entity base not bot base and cbotdef is the start of the bot offset
        //MEM_WriteFloat(vehicle_offset + VEHICLE_RAT_DRIVER_CAMERA_PITCH_OFFSET, pitch);
        MEM_WriteFloat(vehicle_offset + VEHICLE_RAT_DRIVER_CAMERA_RAW_STEERING_POSITION_OFFSET, yaw);

      } else if (gunner_offset == current_bot_offset) {
        const uint32_t site_weapon_data_offset = MEM_ReadUInt(vehicle_offset + VEHICLE_RAT_SITE_WEAPON_DATA_OFFSET);
        pitch = MEM_ReadFloat(site_weapon_data_offset + SITE_WEAPON_DATA_PITCH_OFFSET);
        yaw = MEM_ReadFloat(site_weapon_data_offset + SITE_WEAPON_DATA_YAW_OFFSET);
        
        update_pitch(&pitch, mouse_modifier, look_sensitivity);
        update_yaw(&yaw, mouse_modifier, look_sensitivity);

        // Gotta update camera myself i guess
        float pitch_sin = sin(pitch);
        float pitch_cos = cos(pitch);

        MEM_WriteFloat(site_weapon_data_offset + SITE_WEAPON_DATA_PITCH_OFFSET, pitch);
        MEM_WriteFloat(site_weapon_data_offset + SITE_WEAPON_DATA_YAW_OFFSET, yaw);
        MEM_WriteFloat(site_weapon_data_offset + SITE_WEAPON_AIMX_UP_Z, pitch_sin);
        MEM_WriteFloat(site_weapon_data_offset + SITE_WEAPON_AIMX_UP_Y, pitch_cos);
        MEM_WriteFloat(site_weapon_data_offset + SITE_WEAPON_AIMX_FRONT_Y, -pitch_sin);
        MEM_WriteFloat(site_weapon_data_offset + SITE_WEAPON_AIMX_FRONT_Z, pitch_cos);

        // Gunner gang
      } else {
        // Something broke lol
        // Do nothing, maybe a race condition
      }
    }
  } else if (mech_offset) {
    //Is a turret i guess
    uint32_t mech_cbotdef_offset = MEM_ReadUInt(mech_offset + ENTITY_CBOTDEF_OFFSET);
    BotClass_e mech_class = MEM_ReadUInt(mech_cbotdef_offset + CBOTDEF_STRUCT_BOT_CLASS_OFFSET);
    if (mech_class == BOTCLASS_AAGUN) {
      pitch = MEM_ReadFloat(mech_offset + BOT_AAGUN_PITCH_OFFSET);
      yaw = MEM_ReadFloat(mech_offset + BOT_AAGUN_YAW_OFFSET);
      
      update_pitch(&pitch, mouse_modifier, look_sensitivity);
      update_yaw(&yaw, mouse_modifier, look_sensitivity);


      MEM_WriteFloat(mech_offset + BOT_AAGUN_PITCH_OFFSET, pitch);
      MEM_WriteFloat(mech_offset + BOT_AAGUN_YAW_OFFSET, yaw);
    } else {
      // All other turrets seem to work the same
      const uint32_t site_weapon_data_offset = MEM_ReadUInt(mech_offset + SITE_WEAPON_STRUCT_WEAPON_DATA_OFFSET);
      pitch = MEM_ReadFloat(site_weapon_data_offset + SITE_WEAPON_DATA_PITCH_OFFSET);
      yaw = MEM_ReadFloat(site_weapon_data_offset + SITE_WEAPON_DATA_YAW_OFFSET);
      
      update_pitch(&pitch, mouse_modifier, look_sensitivity);
      update_yaw(&yaw, mouse_modifier, look_sensitivity);

      // Gotta update camera myself i guess
      float pitch_sin = sin(pitch);
      float pitch_cos = cos(pitch);

      MEM_WriteFloat(site_weapon_data_offset + SITE_WEAPON_DATA_PITCH_OFFSET, pitch);
      MEM_WriteFloat(site_weapon_data_offset + SITE_WEAPON_DATA_YAW_OFFSET, yaw);
      MEM_WriteFloat(site_weapon_data_offset + SITE_WEAPON_AIMX_UP_Z, pitch_sin);
      MEM_WriteFloat(site_weapon_data_offset + SITE_WEAPON_AIMX_UP_Y, pitch_cos);
      MEM_WriteFloat(site_weapon_data_offset + SITE_WEAPON_AIMX_FRONT_Y, -pitch_sin);
      MEM_WriteFloat(site_weapon_data_offset + SITE_WEAPON_AIMX_FRONT_Z, pitch_cos);
    }

  } else {
    
    pitch = MEM_ReadFloat(current_bot_offset + BOT_STRUCT_PITCH_OFFSET);
    yaw = MEM_ReadFloat(current_bot_offset + BOT_STRUCT_YAW_OFFSET);

    update_pitch(&pitch, mouse_modifier, look_sensitivity);
    update_yaw(&yaw, mouse_modifier, look_sensitivity);

    float yaw_sin = sin(yaw);
    float yaw_cos = cos(yaw);

    MEM_WriteFloat(current_bot_offset + BOT_STRUCT_PITCH_OFFSET, pitch);
    MEM_WriteFloat(current_bot_offset + BOT_STRUCT_YAW_OFFSET, yaw);
    MEM_WriteFloat(current_bot_offset + BOT_STRUCT_YAW_SIN_OFFSET, yaw_sin);
    MEM_WriteFloat(current_bot_offset + BOT_STRUCT_YAW_COS_OFFSET, yaw_cos);
    MEM_WriteFloat(current_bot_offset + BOT_STRUCT_UNIT_FRONT_X_OFFSET, yaw_sin);
    MEM_WriteFloat(current_bot_offset + BOT_STRUCT_UNIT_FRONT_Z_OFFSET, yaw_cos);
  }

}
