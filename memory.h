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
#define NOTWITHINARAMRANGE(X) (X < 0x7E000000 || X > 0x7EFFFFFF)
#define WITHINARAMRANGE(X) (!NOTWITHINARAMRANGE(X))
#define PS1WITHINMEMRANGE(X) (!PS1NOTWITHINMEMRANGE(X))
#define PS1NOTWITHINMEMRANGE(X) (X > 0x1FFFFF)
#define N64NOTWITHINMEMRANGE(X) (X < 0x80000000 || X > 0x807FFFFF)
#define N64WITHINMEMRANGE(X) (!N64NOTWITHINMEMRANGE(X))
#define SNESNOTWITHINMEMRANGE(X) (X > 0x1FFFF)
#define SNESWITHINMEMRANGE(X) (!SNESNOTWITHINMEMRANGE(X))

extern uint8_t MEM_Init(void);
extern void MEM_Quit(void);
extern uint8_t MEM_FindRamOffset(void);
extern int32_t MEM_ReadInt(const uint32_t addr);
extern uint32_t MEM_ReadUInt(const uint32_t addr);
extern float MEM_ReadFloat(const uint32_t addr);
extern void MEM_WriteInt(const uint32_t addr, int32_t value);
extern void MEM_WriteUInt(const uint32_t addr, uint32_t value);
extern void MEM_WriteFloat(const uint32_t addr, float value);

extern int32_t ARAM_ReadInt(const uint32_t addr);
extern uint32_t ARAM_ReadUInt(const uint32_t addr);
extern float ARAM_ReadFloat(const uint32_t addr);
extern void ARAM_WriteUInt(const uint32_t addr, uint32_t value);
extern void ARAM_WriteFloat(const uint32_t addr, float value);

extern uint32_t PS1_MEM_ReadPointer(const uint32_t addr);
extern uint32_t PS1_MEM_ReadWord(const uint32_t addr);
extern uint16_t PS1_MEM_ReadHalfword(const uint32_t addr);
extern uint8_t PS1_MEM_ReadByte(const uint32_t addr);
extern void PS1_MEM_WriteHalfword(const uint32_t addr, uint16_t value);
extern void PS1_MEM_WriteWord(const uint32_t addr, uint32_t value);
extern void PS1_MEM_WriteByte(const uint32_t addr, uint8_t value);

extern uint32_t N64_MEM_ReadUInt(const uint32_t addr);
extern float N64_MEM_ReadFloat(const uint32_t addr);
extern void N64_MEM_WriteUInt(const uint32_t addr, uint32_t value);
extern void N64_MEM_WriteFloat(const uint32_t addr, float value);

extern uint16_t SNES_MEM_ReadWord(const uint32_t addr);
extern void SNES_MEM_WriteWord(const uint32_t addr, uint16_t value);

extern char hookedEmulatorName[80];