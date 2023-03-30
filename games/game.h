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
typedef struct
{
	const char *Name;
	uint8_t (*Status)(void);
	void (*Inject)(void);
	uint16_t Tickrate;
	uint8_t Crosshair;
	char *Option;
	char *Option2;
} GAMEDRIVER;

extern uint8_t GAME_Status(void);
extern void GAME_Inject(void);
extern const char *GAME_Name(void);
extern uint16_t GAME_Tickrate(void);
extern uint8_t GAME_CrosshairSwaySupported(void);
extern uint8_t GAME_OptionSupported(void);
extern const char *GAME_OptionMessage(void);