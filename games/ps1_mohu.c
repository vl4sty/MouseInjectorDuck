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
#include <stdio.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"


//TODO: find fov to make zoomed sniper sens lower

// OFFSET addresses, requires cam_base to use
#define MOHU_camx 0x000F116E - 0x000F0BE0
#define MOHU_camy 0x000F1162 - 0x000F0BE0
#define MOHU_on_sentry_flag 0x000E6D2E - 0x000E6D20 // 2 = on sentry, 0 = not on sentry?
#define MOHU_machinegun_camx 0x242
#define MOHU_machinegun_sanity_address 0xC // -C from machinegun base
#define MOHU_machinegun_sanity_value 0xE003F900
// STATIC addresses
#define MOHU_playerbase 0x000AA1A8
#define MOHU_playerbase_sanity 0x14
#define MOHU_rightstick_x 0x000CB840 // 1 byte, 00 left - 80 middle - FF right
#define MOHU_rightstick_y 0x000CB841 // 1 byte, 00 top - 80 middle - FF bottom
#define MOHU_machinegunbase 0x001FFDB4

static uint8_t PS1_MOHU_Status(void);
static uint8_t PS1_MOHU_DetectPlayer(void);
static void PS1_MOHU_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"PS1 Medal of Honor: Underground",
	PS1_MOHU_Status,
	PS1_MOHU_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_MOHUNDERGROUND = &GAMEDRIVER_INTERFACE;

static uint32_t playerbase = 0;
static uint32_t machinegunbase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_MOHU_Status(void)
{
	return (PS1_MEM_ReadWord(0xB8B4) == 0x6D3A5C53U && PS1_MEM_ReadWord(0xB8B8) == 0x4C55535FU &&
			PS1_MEM_ReadWord(0xB8BC) == 0x3031322EU && PS1_MEM_ReadWord(0xB8C0) == 0x37303B31U); // m:\SLUS_012.70;1
}
//==========================================================================
// Purpose: determines if there is a player
//==========================================================================
static uint8_t PS1_MOHU_DetectPlayer(void)
{
	const uint32_t tempplayerbase = PS1_MEM_ReadPointer(MOHU_playerbase);
	if (PS1WITHINMEMRANGE(tempplayerbase))
	{
		const uint32_t tempsanity_1 = PS1_MEM_ReadWord(tempplayerbase);
		const uint32_t tempsanity_2 = PS1_MEM_ReadWord(tempplayerbase + MOHU_playerbase_sanity);
		if (tempsanity_1 == 0x02000300 && tempsanity_2 == 0xD04664C8)
		{
			playerbase = tempplayerbase;
			return 1;
		}
	}

	return 0;
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_MOHU_Inject(void)
{
	if(!PS1_MOHU_DetectPlayer())
		return;
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity / 20.f;

	if (PS1_MEM_ReadByte(playerbase + MOHU_on_sentry_flag)) // on mounted machinegun
	{
		if (machinegunbase == 0)
		{
			// return if not a pointer
			if (PS1_MEM_ReadByte(MOHU_machinegunbase + 0x3) != 0x80) 
				return;

			machinegunbase = PS1_MEM_ReadPointer(MOHU_machinegunbase);

			uint32_t sanity_address = machinegunbase - MOHU_machinegun_sanity_address;
			// check that pointer points to machinegun
			if (PS1_MEM_ReadWord(sanity_address) != MOHU_machinegun_sanity_value)
			{
				machinegunbase = 0;
				return;
			}
		}

		uint16_t mg_camx = PS1_MEM_ReadHalfword(machinegunbase - MOHU_machinegun_camx);
		uint8_t stick_y = PS1_MEM_ReadByte(MOHU_rightstick_y);

		mg_camx += (float)xmouse * looksensitivity;

		// simulate right stick movement due to not being able to find a writeable camy value
		if (ymouse < 0)
			stick_y = 0x0;
		else
			stick_y = 0xFF;

		PS1_MEM_WriteHalfword(machinegunbase - MOHU_machinegun_camx, (uint16_t)mg_camx);
		PS1_MEM_WriteByte(MOHU_rightstick_y, stick_y);
	}
	else { // on foot
		machinegunbase = 0;

		uint16_t camx = PS1_MEM_ReadHalfword(playerbase + MOHU_camx);
		uint16_t camy = PS1_MEM_ReadHalfword(playerbase + MOHU_camy);

		camx += (float)xmouse * looksensitivity;

		camy -= (float)ymouse * looksensitivity;
		// clamp camy
		if (camy > 60000 && camy < 64754)
			camy = 64754U;
		if (camy > 682 && camy < 4000)
			camy = 682U;

		PS1_MEM_WriteHalfword(playerbase + MOHU_camx, (uint16_t)camx);
		PS1_MEM_WriteHalfword(playerbase + MOHU_camy, (uint16_t)camy);
	}
}