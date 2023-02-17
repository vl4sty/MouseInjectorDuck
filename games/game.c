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
#include <stdlib.h>
#include <stdint.h>
#include "../main.h"
#include "game.h"

extern const GAMEDRIVER *GAME_TS2;
extern const GAMEDRIVER *GAME_TS3;
extern const GAMEDRIVER *GAME_NF;
extern const GAMEDRIVER *GAME_MOHF;
extern const GAMEDRIVER *GAME_MOHEA;
extern const GAMEDRIVER *GAME_MOHRS;
extern const GAMEDRIVER *GAME_DHV;
extern const GAMEDRIVER *GAME_COD2BRO;
extern const GAMEDRIVER *GAME_SERIOUS;
extern const GAMEDRIVER *GAME_METALARMS;
extern const GAMEDRIVER *GAME_TRIGGERMAN;
extern const GAMEDRIVER *GAME_GEIST;
extern const GAMEDRIVER *GAME_PS1_MENINBLACKCRASHDOWN;
extern const GAMEDRIVER *GAME_PS1_CODENAMETENKA;
extern const GAMEDRIVER *GAME_PS1_MOHUNDERGROUND;
extern const GAMEDRIVER *GAME_PS1_REVOLUTIONX;
extern const GAMEDRIVER *GAME_PS1_ARMORINES;
extern const GAMEDRIVER *GAME_N64_GOLDENEYE;
extern const GAMEDRIVER *GAME_N64_SINPUNISHMENT;
extern const GAMEDRIVER *GAME_SNES_PACMAN2;
extern const GAMEDRIVER *GAME_SNES_TIMONANDPUMBAA;
extern const GAMEDRIVER *GAME_SNES_SHADOWRUN;
extern const GAMEDRIVER *GAME_SNES_STARFOX;
extern const GAMEDRIVER *GAME_SNES_UNTOUCHABLES;
extern const GAMEDRIVER *GAME_SNES_RTYPE3;
extern const GAMEDRIVER *GAME_PS2_007AGENTUNDERFIRE;
extern const GAMEDRIVER *GAME_PS2_REDDEADREVOLVER;
extern const GAMEDRIVER *GAME_PS2_TIMECRISIS2;
extern const GAMEDRIVER *GAME_PS2_REDEADAIM;
extern const GAMEDRIVER *GAME_PS2_GUNSLINGERGIRL1;
extern const GAMEDRIVER *GAME_PS2_VAMPIRENIGHT;
extern const GAMEDRIVER *GAME_PS2_NINJAASSAULT;
extern const GAMEDRIVER *GAME_PS2_DARKWATCH;
extern const GAMEDRIVER *GAME_PS2_BLACK;
extern const GAMEDRIVER *GAME_PS2_URBANCHAOS;
extern const GAMEDRIVER *GAME_PS2_QUAKE3;
extern const GAMEDRIVER *GAME_PS2_CODFINESTHOUR;
extern const GAMEDRIVER *GAME_PS2_50CENTBULLETPROOF;
extern const GAMEDRIVER *GAME_PS2_COLDWINTER;
extern const GAMEDRIVER *GAME_PS1_RESIDENTEVILSURVIVOR;
extern const GAMEDRIVER *GAME_PS1_DNLANDOFTHEBABES;
extern const GAMEDRIVER *GAME_PS2_MERCENARIES;
extern const GAMEDRIVER *GAME_PS2_MOHVANGUARD;

static const GAMEDRIVER **GAMELIST[] =
{
	&GAME_TS2,
	&GAME_TS3,
	&GAME_NF,
	&GAME_MOHF,
	&GAME_MOHEA,
	&GAME_MOHRS,
	&GAME_DHV,
	&GAME_COD2BRO,
	&GAME_SERIOUS,
  	&GAME_METALARMS,
  	&GAME_TRIGGERMAN,
	&GAME_GEIST,
	&GAME_PS1_MENINBLACKCRASHDOWN,
	&GAME_PS1_CODENAMETENKA,
	&GAME_PS1_MOHUNDERGROUND,
	&GAME_PS1_REVOLUTIONX,
	&GAME_PS1_ARMORINES,
	&GAME_N64_GOLDENEYE,
	&GAME_N64_SINPUNISHMENT,
	&GAME_SNES_PACMAN2,
	&GAME_SNES_TIMONANDPUMBAA,
	&GAME_SNES_SHADOWRUN,
	&GAME_SNES_STARFOX,
	&GAME_SNES_UNTOUCHABLES,
	&GAME_SNES_RTYPE3,
	&GAME_PS2_007AGENTUNDERFIRE,
	&GAME_PS2_REDDEADREVOLVER,
	&GAME_PS2_TIMECRISIS2,
	&GAME_PS2_REDEADAIM,
	&GAME_PS2_GUNSLINGERGIRL1,
	&GAME_PS2_VAMPIRENIGHT,
	&GAME_PS2_NINJAASSAULT,
	&GAME_PS2_DARKWATCH,
	&GAME_PS2_BLACK,
	&GAME_PS2_URBANCHAOS,
	&GAME_PS2_QUAKE3,
	&GAME_PS2_CODFINESTHOUR,
	&GAME_PS2_50CENTBULLETPROOF,
	&GAME_PS2_COLDWINTER,
	&GAME_PS1_RESIDENTEVILSURVIVOR,
	&GAME_PS1_DNLANDOFTHEBABES,
	&GAME_PS2_MERCENARIES,
	&GAME_PS2_MOHVANGUARD
};

static const GAMEDRIVER *CURRENT_GAME = NULL;
static const uint8_t upper = (sizeof(GAMELIST) / sizeof(GAMELIST[0]));

uint8_t GAME_Status(void);
void GAME_Inject(void);
const char *GAME_Name(void);

//==========================================================================
// Purpose: check all game interfaces for game
//==========================================================================
uint8_t GAME_Status(void)
{
	if(CURRENT_GAME != NULL) // if any game has been detected previously
	{
		if(CURRENT_GAME->Status()) // check if game is still active, else check every supported driver
			return 1;
		CURRENT_GAME = NULL;
	}
	const GAMEDRIVER *THIS_GAME;
	for(uint8_t i = 0; (i < upper) && (CURRENT_GAME == NULL); i++)
	{
		THIS_GAME = *(GAMELIST[i]);
		if(THIS_GAME != NULL && THIS_GAME->Status())
			CURRENT_GAME = THIS_GAME;
	}
	return (CURRENT_GAME != NULL);
}
//==========================================================================
// Purpose: inject via game driver
//==========================================================================
void GAME_Inject(void)
{
	if(CURRENT_GAME != NULL)
		CURRENT_GAME->Inject();
}
//==========================================================================
// Purpose: return game driver name
//==========================================================================
const char *GAME_Name(void)
{
	if(CURRENT_GAME != NULL)
		return CURRENT_GAME->Name;
	// return DOLPHINVERSION; // if no driver active, return dolphin name
	return "No game loaded"; // if no driver active, return dolphin name
}
//==========================================================================
// Purpose: return game driver's required tickrate
//==========================================================================
uint16_t GAME_Tickrate(void)
{
	if(CURRENT_GAME != NULL)
		return CURRENT_GAME->Tickrate;
	return 1; // if no driver active, use 1000 Hz tickrate
}
//==========================================================================
// Purpose: return game driver's crosshair sway support
//==========================================================================
uint8_t GAME_CrosshairSwaySupported(void)
{
	if(CURRENT_GAME != NULL)
		return CURRENT_GAME->Crosshair;
	return 1; // return 1 if no drivers are available, so user can edit crosshair sway while dolphin isn't playing a game
}
