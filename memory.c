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
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include "memory.h"

static uint64_t emuoffset = 0;
static uint32_t aramoffset = 0x02000000; // REQUIRES that MMU is off
static HANDLE emuhandle = NULL;
static int isPS1handle = 0;

uint8_t MEM_Init(void);
void MEM_Quit(void);
void MEM_UpdateEmuoffset(void);
int32_t MEM_ReadInt(const uint32_t addr);
uint32_t MEM_ReadUInt(const uint32_t addr);
float MEM_ReadFloat(const uint32_t addr);
void MEM_WriteInt(const uint32_t addr, int32_t value);
void MEM_WriteUInt(const uint32_t addr, uint32_t value);
void MEM_WriteFloat(const uint32_t addr, float value);
static void MEM_ByteSwap32(uint32_t *input);

int32_t ARAM_ReadInt(const uint32_t addr);
uint32_t ARAM_ReadUInt(const uint32_t addr);
float ARAM_ReadFloat(const uint32_t addr);
void ARAM_WriteUInt(const uint32_t addr, uint32_t value);
void ARAM_WriteFloat(const uint32_t addr, float value);

uint32_t PS1_MEM_ReadPointer(const uint32_t addr);
uint32_t PS1_MEM_ReadWord(const uint32_t addr);
uint16_t PS1_MEM_ReadHalfword(const uint32_t addr);
uint8_t PS1_MEM_ReadByte(const uint32_t addr);
void PS1_MEM_WriteWord(const uint32_t addr, uint32_t value);
void PS1_MEM_WriteHalfword(const uint32_t addr, uint16_t value);
void PS1_MEM_WriteByte(const uint32_t addr, uint8_t value);
// static void MEM_ByteSwap16(uint16_t *input);

//==========================================================================
// Purpose: initialize dolphin handle and setup for memory injection
// Changed Globals: emuhandle
//==========================================================================
uint8_t MEM_Init(void)
{
	emuhandle = NULL;
	HANDLE processes; // will store a snapshot of all processes
	PROCESSENTRY32 pe32; // stores basic info of a process, using this one to read the ProcessID from
	processes = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // make process snapshot
	pe32.dwSize = sizeof(PROCESSENTRY32); // correct size
	Process32First(processes, &pe32); // read info about the first process into pe32
	do // loop to find emulator
	{
		if(strcmp(pe32.szExeFile, "Dolphin.exe") == 0) // if dolphin was found
		{
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "duckstation-qt-x64-ReleaseLTCG.exe") == 0) // if DuckStation was found
		{
			isPS1handle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
	}
	while(Process32Next(processes, &pe32)); // loop continued until Process32Next deliver NULL or its interrupted with the "break" above
	CloseHandle(processes);
	return (emuhandle != NULL);
}
//==========================================================================
// Purpose: close emuhandle safely
// Changed Globals: emuhandle
//==========================================================================
void MEM_Quit(void)
{
	if(emuhandle != NULL)
		CloseHandle(emuhandle);
}
//==========================================================================
// Purpose: update emuoffset pointer to location of gamecube memory
// Changed Globals: emuoffset
//==========================================================================
uint8_t MEM_FindRamOffset(void)
{
	emuoffset = 0;

	MEMORY_BASIC_INFORMATION info; // store a snapshot of memory information

	PVOID gamecube_ptr = NULL;

	while (VirtualQueryEx(emuhandle, gamecube_ptr, &info, sizeof(info))) // loop continues until we reach the last possible memory region
	{
		gamecube_ptr = info.BaseAddress + info.RegionSize; // update address to next region of memory for loop

		uint32_t emuRegionSize = 0x2000000;
		if (isPS1handle == 1){
			emuRegionSize = 0x200000;
		}

		// if (info.RegionSize == 0x2000000 && info.Type == MEM_MAPPED) { // check if the mapped memory region is the size of the maximum gamecube ram address
		if (info.RegionSize == emuRegionSize && info.Type == MEM_MAPPED) { // check if the mapped memory region is the size of the maximum gamecube ram address
			PSAPI_WORKING_SET_EX_INFORMATION wsinfo;
			wsinfo.VirtualAddress = info.BaseAddress;

			if (QueryWorkingSetEx(emuhandle, &wsinfo, sizeof(wsinfo))) { // query extended info about the memory page at the current virtual address space
				if (wsinfo.VirtualAttributes.Valid) { // check if the address space is valid
					memcpy(&emuoffset, &(info.BaseAddress), sizeof(info.BaseAddress)); // copy the base address location to our emuoffset pointer

					// if (MEM_ReadUInt(0x80000000) == 0x30000000U) // quick dirty check
					return (emuoffset != 0x0);
				}
			}
		}
	}
}
//==========================================================================
// Purpose: read int from memory
// Parameter: address location
//==========================================================================
int32_t MEM_ReadInt(const uint32_t addr)
{
	if(!emuoffset || NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
		return 0;
	int32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &output, sizeof(output), NULL);
	MEM_ByteSwap32((uint32_t *)&output); // byteswap
	return output;
}
//==========================================================================
// Purpose: read unsigned int from memory
// Parameter: address location
//==========================================================================
uint32_t MEM_ReadUInt(const uint32_t addr)
{
	if(!emuoffset || NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &output, sizeof(output), NULL);
	MEM_ByteSwap32(&output); // byteswap
	return output;
}
//==========================================================================
// Purpose: read float from memory
// Parameter: address location
//==========================================================================
float MEM_ReadFloat(const uint32_t addr)
{
	if(!emuoffset || NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
		return 0;
	float output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &output, sizeof(output), NULL);
	MEM_ByteSwap32((uint32_t *)&output); // byteswap
	return output;
}
//==========================================================================
// Purpose: write int to memory
// Parameter: address location and value
//==========================================================================
void MEM_WriteInt(const uint32_t addr, int32_t value)
{
	if(!emuoffset || NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or writing to outside of memory range
		return;
	MEM_ByteSwap32((uint32_t *)&value); // byteswap
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &value, sizeof(value), NULL);
}
//==========================================================================
// Purpose: write unsigned int to memory
// Parameter: address location and value
//==========================================================================
void MEM_WriteUInt(const uint32_t addr, uint32_t value)
{
	if(!emuoffset || NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or writing to outside of memory range
		return;
	MEM_ByteSwap32(&value); // byteswap
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &value, sizeof(value), NULL);
}
//==========================================================================
// Purpose: write float to memory
// Parameter: address location and value
//==========================================================================
void MEM_WriteFloat(const uint32_t addr, float value)
{
	if(!emuoffset || NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or writing to outside of memory range
		return;
	MEM_ByteSwap32((uint32_t *)&value); // byteswap
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &value, sizeof(value), NULL);
}
//==========================================================================
// Purpose: byteswap input value
// Parameter: pointer of value (must be 4 byte long)
//==========================================================================
static void MEM_ByteSwap32(uint32_t *input)
{
	const uint8_t *inputarray = ((uint8_t *)input); // set byte array to input
	*input = (uint32_t)((inputarray[0] << 24) | (inputarray[1] << 16) | (inputarray[2] << 8) | (inputarray[3])); // reassign input to swapped value
}

//==========================================================================
// Purpose: read int from ARAM ***REQUIRES MMU TO BE DISABLED IN DOLPHIN***
// Parameter: address location
//==========================================================================
int32_t ARAM_ReadInt(const uint32_t addr)
{
	if(!emuoffset || NOTWITHINARAMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
		return 0;
	int32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + aramoffset + (addr - 0x7E000000)), &output, sizeof(output), NULL);
	MEM_ByteSwap32((uint32_t *)&output); // byteswap
	return output;
}
//==========================================================================
// Purpose: read unsigned int from ARAM ***REQUIRES MMU TO BE DISABLED IN DOLPHIN***
// Parameter: address location
//==========================================================================
uint32_t ARAM_ReadUInt(const uint32_t addr)
{
	if(!emuoffset || NOTWITHINARAMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + aramoffset + (addr - 0x7E000000)), &output, sizeof(output), NULL);
	MEM_ByteSwap32(&output); // byteswap
	return output;
}
//==========================================================================
// Purpose: read float from ARAM ***REQUIRES MMU TO BE DISABLED IN DOLPHIN***
// Parameter: address location
//==========================================================================
float ARAM_ReadFloat(const uint32_t addr)
{
	if(!emuoffset || NOTWITHINARAMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
		return 0;
	float output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + aramoffset + (addr - 0x7E000000)), &output, sizeof(output), NULL);
	MEM_ByteSwap32((uint32_t *)&output); // byteswap
	return output;
}
//==========================================================================
// Purpose: write unsigned int to ARAM ***REQUIRES MMU TO BE DISABLED IN DOLPHIN***
// Parameter: address location and value
//==========================================================================
void ARAM_WriteUInt(const uint32_t addr, uint32_t value)
{
	if(!emuoffset || NOTWITHINARAMRANGE(addr)) // if gamecube memory has not been init by dolphin or writing to outside of memory range
		return;
	MEM_ByteSwap32(&value); // byteswap
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + aramoffset + (addr - 0x7E000000)), &value, sizeof(value), NULL);
}
//==========================================================================
// Purpose: write float to ARAM ***REQUIRES MMU TO BE DISABLED IN DOLPHIN***
// Parameter: address location and value
//==========================================================================
void ARAM_WriteFloat(const uint32_t addr, float value)
{
	if(!emuoffset || NOTWITHINARAMRANGE(addr)) // if gamecube memory has not been init by dolphin or writing to outside of memory range
		return;
	MEM_ByteSwap32((uint32_t *)&value); // byteswap
	// ARAM offset = 0x02000000
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + aramoffset + (addr - 0x7E000000)), &value, sizeof(value), NULL);
}

uint32_t PS1_MEM_ReadPointer(const uint32_t addr)
{
	// assumes the address of a ps1 pointer in the form 0x80BbA1A2 - Bb = Bank, A1A2 = Address in bank
	// PS1 pointer stored in little endian (A2A1Bb80), ReadProcessMemory reads it in reverse resulting in 80BbA1A2
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return 0;
	uint32_t output;
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return (output - 0x80000000); // return address minus the 0x8 on the front
}

uint32_t PS1_MEM_ReadWord(const uint32_t addr)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return 0;
	uint32_t output;
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	MEM_ByteSwap32(&output); // byteswap
	return output;
}

uint16_t PS1_MEM_ReadHalfword(const uint32_t addr)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return 0;
	// read only 2 bytes
	uint16_t output;
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return output;
}

uint8_t PS1_MEM_ReadByte(const uint32_t addr)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return 0;
	uint8_t output;
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return output;
}

void PS1_MEM_WriteWord(const uint32_t addr, uint32_t value)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

void PS1_MEM_WriteHalfword(const uint32_t addr, uint16_t value)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

void PS1_MEM_WriteByte(const uint32_t addr, uint8_t value)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}
