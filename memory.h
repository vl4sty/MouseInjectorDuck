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
#define NOTWITHINMEMRANGE(X) (X < 0x80000000 || X > 0x81800000) // if X is not within GC memory range
#define WITHINMEMRANGE(X) (!NOTWITHINMEMRANGE(X)) // if X is within GC memory range

extern uint8_t MEM_Init(void);
extern void MEM_Quit(void);
extern uint8_t MEM_FindRamOffset(void);
extern int32_t MEM_ReadInt(const uint32_t addr);
extern uint32_t MEM_ReadUInt(const uint32_t addr);
extern float MEM_ReadFloat(const uint32_t addr);
extern void MEM_WriteInt(const uint32_t addr, int32_t value);
extern void MEM_WriteUInt(const uint32_t addr, uint32_t value);
extern void MEM_WriteFloat(const uint32_t addr, float value);