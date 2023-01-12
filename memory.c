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
#include <stdio.h> // for text file debug output
#include <stdint.h>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include "memory.h"

static uint64_t emuoffset = 0;
static uint32_t aramoffset = 0x02000000; // REQUIRES that MMU is off
static HANDLE emuhandle = NULL;
static int isPS1handle = 0;
static int isN64handle = 0;
static int isMupenhandle = 0;
static int isBizHawkhandle = 0;
static int isBSNEShandle = 0;
static int isPcsx2handle = 0;
char hookedEmulatorName[80];

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
uint32_t PS1_MEM_ReadUInt(const uint32_t addr);
uint16_t PS1_MEM_ReadHalfword(const uint32_t addr);
uint8_t PS1_MEM_ReadByte(const uint32_t addr);
void PS1_MEM_WriteWord(const uint32_t addr, uint32_t value);
void PS1_MEM_WriteHalfword(const uint32_t addr, uint16_t value);
void PS1_MEM_WriteByte(const uint32_t addr, uint8_t value);
// static void MEM_ByteSwap16(uint16_t *input);

uint32_t N64_MEM_ReadUInt(const uint32_t addr);
float N64_MEM_ReadFloat(const uint32_t addr);
void N64_MEM_WriteFloat(const uint32_t addr, float value);
void N64_MEM_WriteUInt(const uint32_t addr, uint32_t value);

uint8_t SNES_MEM_ReadByte(const uint32_t addr);
uint16_t SNES_MEM_ReadWord(const uint32_t addr);
void SNES_MEM_WriteByte(const uint32_t addr, uint8_t value);
void SNES_MEM_WriteWord(const uint32_t addr, uint16_t value);

uint32_t PS2_MEM_ReadPointer(const uint32_t addr);
uint32_t PS2_MEM_ReadUInt(const uint32_t addr);
float PS2_MEM_ReadFloat(const uint32_t addr);
void PS2_MEM_WriteFloat(const uint32_t addr, float value);

void printdebug(uint32_t val);

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
			strcpy(hookedEmulatorName, "Dolphin");
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "duckstation-qt-x64-ReleaseLTCG.exe") == 0) // if DuckStation was found
		{
			strcpy(hookedEmulatorName, "DuckStation");
			isPS1handle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "EmuHawk.exe") == 0) // if EmuHawk was found, 2.8 oldest tested working - 2.9 not supported
		{
			strcpy(hookedEmulatorName, "BizHawk N64");
			isN64handle = 1;
			isBizHawkhandle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "RMG.exe") == 0) // if simple64 was found
		{
			strcpy(hookedEmulatorName, "Rosalie's Mupen GUI");
			isN64handle = 1;
			isMupenhandle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "simple64-gui.exe") == 0) // if simple64 was found
		{
			strcpy(hookedEmulatorName, "simple64");
			isN64handle = 1;
			isMupenhandle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "retroarch.exe") == 0) // if retroarch was found, for N64 games using Mupen64Plus-Next
		{
			strcpy(hookedEmulatorName, "RetroArch Mupen64Plus-Next");
			isN64handle = 1;
			isMupenhandle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "bsnes.exe") == 0) // if BSNES was found
		{
			strcpy(hookedEmulatorName, "BSNES");
			isBSNEShandle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "pcsx2-qtx64-avx2.exe") == 0) // if pcsx2 was found
		{
			strcpy(hookedEmulatorName, "pcsx2-qtx64-avx2");
			isPcsx2handle = 1;
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

	uint32_t lastRegionSize = 0;
	uint32_t lastlastRegionSize = 0;
	uint32_t lastlastlastRegionSize = 0;
	uint32_t lastlastlastlastRegionSize = 0;

	while (VirtualQueryEx(emuhandle, gamecube_ptr, &info, sizeof(info))) // loop continues until we reach the last possible memory region
	{
		gamecube_ptr = info.BaseAddress + info.RegionSize; // update address to next region of memory for loop

		// set region size to look for based on the emulator detected
		uint32_t emuRegionSize = 0x2000000; // Dolphin
		if (isPS1handle == 1){
			emuRegionSize = 0x200000; 		// DuckStation
		} else if (isN64handle == 1) {
			if (isMupenhandle)
				emuRegionSize = 0x20011000; // RetroArch(Mupen64Plus core)/simple64/RMG
			else
				emuRegionSize = 0x22D0000; 	// BizHawk 2.8 (Mupen64Plus)
		} else if (isBSNEShandle == 1) {
			emuRegionSize = 0x34000;
		} else if (isPcsx2handle == 1) {
			// emuRegionSize = 0x80000;
			emuRegionSize = 0x1000;
			// emuRegionSize = 0xE000;
			// emuRegionSize = 0x97000;
			// emuRegionSize = 0x3AA000;
			// emuRegionSize = 0xF004000;
		}

		DWORD regionType = MEM_MAPPED; // Dolphin and DuckStation regions are type MEM_MAPPED
		if (isN64handle == 1)
			regionType = MEM_PRIVATE;  // All N64 emulator regions are type MEM_PRIVATE
		if (isBSNEShandle == 1)
			regionType = MEM_IMAGE;
		if (isPcsx2handle == 1)
		{
			// regionType = MEM_IMAGE;
			// regionType = MEM_PRIVATE;
			regionType = MEM_MAPPED;
		}

		// check if region is the size of region where console memory is located
		uint8_t regionFound = 0;
		if (info.RegionSize == emuRegionSize && ((info.Type == regionType) || isN64handle))
			regionFound = 1;

		if (regionFound == 1) { // why '|| isN64handle'? one N64 emulator is not MEM_PRIVATE?
		// if (info.RegionSize == emuRegionSize) {
			// printdebug(info.Type); // debug
			PSAPI_WORKING_SET_EX_INFORMATION wsinfo;
			wsinfo.VirtualAddress = info.BaseAddress;

			if (QueryWorkingSetEx(emuhandle, &wsinfo, sizeof(wsinfo))) { // query extended info about the memory page at the current virtual address space
				if (wsinfo.VirtualAttributes.Valid) { // check if the address space is valid
					memcpy(&emuoffset, &(info.BaseAddress), sizeof(info.BaseAddress)); // copy the base address location to our emuoffset pointer

					if (isN64handle == 1)
					{
						if (isMupenhandle) { // RetroArch/simple64/RMG (mupen64plus GUIs)
							// simple64/RMG/retroarch: determine buffer size before RDRAM as it changes every startup
							emuoffset += 0x1000;
							// while (PS1_MEM_ReadWord(0x0) == 0) // look for non-zero bytes at first address to signify the start of RDRAM
							while (MEM_ReadUInt(0x80000000) == 0) // look for non-zero bytes at first address to signify the start of RDRAM
								emuoffset += 0x1000; // RDRAM always begins at a multiple 0x1000 away from the initial emuhandle offset
						}
						else {
							// BizHawk has small offset before N64 RDRAM
							emuoffset += 0x8E0;
						}
					} else if (isBSNEShandle == 1) {
						emuoffset += 0x2D7C; // WRAM always here? 0xB14000 + 0x2D7C, but region size is not fixed
					} else if (isPcsx2handle == 1) {
						// emuoffset += 0x881000 + 0x1000 + 0xE000;
						// emuoffset += 0x881000 + 0x1000 + 0xE000;
						// emuoffset += 0x46F000;
						// emuoffset -= 0x2000000U;
						// emuoffset += 0x885000;
						// emuoffset += 0x884000;
						// emuoffset += 0x98000;
						// if (lastRegionSize != 0x1000)
						// 	continue;
						// TODO: check if region before 0x80000 has a size in a range, 0x1000 <= regionsize <= 0xF000
						if (lastRegionSize != 0x80000 || lastlastRegionSize != 0x1000)
						// if (lastlastlastlastRegionSize != 0x7000 || lastlastlastRegionSize != 0x80000 || lastlastRegionSize != 0x1000 || lastRegionSize != 0x1000)
							continue;
						
						// emuoffset -= 0x2000U;
					}

					// printdebug(lastRegionSize); // debug

					return (emuoffset != 0x0);
				}
			}
		}

		// // check if region is the size of region where console memory is located
		// if (info.RegionSize == emuRegionSize && (info.Type == regionType || isN64handle)) { // why '|| isN64handle'? one N64 emulator is not MEM_PRIVATE?
		// // if (info.RegionSize == emuRegionSize) {
		// 	// printdebug(info.Type); // debug
		// 	PSAPI_WORKING_SET_EX_INFORMATION wsinfo;
		// 	wsinfo.VirtualAddress = info.BaseAddress;

		// 	if (QueryWorkingSetEx(emuhandle, &wsinfo, sizeof(wsinfo))) { // query extended info about the memory page at the current virtual address space
		// 		if (wsinfo.VirtualAttributes.Valid) { // check if the address space is valid
		// 			memcpy(&emuoffset, &(info.BaseAddress), sizeof(info.BaseAddress)); // copy the base address location to our emuoffset pointer

		// 			if (isN64handle == 1)
		// 			{
		// 				if (isMupenhandle) { // RetroArch/simple64/RMG (mupen64plus GUIs)
		// 					// simple64/RMG/retroarch: determine buffer size before RDRAM as it changes every startup
		// 					emuoffset += 0x1000;
		// 					// while (PS1_MEM_ReadWord(0x0) == 0) // look for non-zero bytes at first address to signify the start of RDRAM
		// 					while (MEM_ReadUInt(0x80000000) == 0) // look for non-zero bytes at first address to signify the start of RDRAM
		// 						emuoffset += 0x1000; // RDRAM always begins at a multiple 0x1000 away from the initial emuhandle offset
		// 				}
		// 				else {
		// 					// BizHawk has small offset before N64 RDRAM
		// 					emuoffset += 0x8E0;
		// 				}
		// 			} else if (isBSNEShandle == 1) {
		// 				emuoffset += 0x2D7C; // WRAM always here? 0xB14000 + 0x2D7C, but region size is not fixed
		// 			} else if (isPcsx2handle == 1) {
		// 				// emuoffset += 0x881000 + 0x1000 + 0xE000;
		// 				// emuoffset += 0x881000 + 0x1000 + 0xE000;
		// 				// emuoffset += 0x46F000;
		// 				// emuoffset -= 0x2000000U;
		// 				// emuoffset += 0x885000;
		// 				// emuoffset += 0x884000;
		// 				emuoffset += 0x98000;
		// 			}

		// 			// printdebug(emuRegionSize); // debug

		// 			return (emuoffset != 0x0);
		// 		}
		// 	}
		// }

		lastlastlastlastRegionSize = lastlastlastRegionSize;
		lastlastlastRegionSize = lastlastRegionSize;
		lastlastRegionSize = lastRegionSize;
		lastRegionSize = info.RegionSize;
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

uint32_t PS1_MEM_ReadUInt(const uint32_t addr)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return 0;
	uint32_t output;
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
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

//==========================================================================
// N64 addresses should not be byteswapped since they are stored little endian in emulator memory
//==========================================================================
uint32_t N64_MEM_ReadUInt(const uint32_t addr)
{
	if(!emuoffset || N64NOTWITHINMEMRANGE(addr)) // if n64 memory has not been init by emulator or reading from outside of memory range
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &output, sizeof(output), NULL);
	return output;
}

float N64_MEM_ReadFloat(const uint32_t addr)
{
	if(!emuoffset || N64NOTWITHINMEMRANGE(addr)) // if n64 memory has not been init by emulator or reading from outside of memory range
		return 0;
	float output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &output, sizeof(output), NULL);
	return output;
}

void N64_MEM_WriteUInt(const uint32_t addr, uint32_t value)
{
	if(!emuoffset || N64NOTWITHINMEMRANGE(addr)) // if n64 memory has not been init by emulator or writing to outside of memory range
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &value, sizeof(value), NULL);
}

void N64_MEM_WriteFloat(const uint32_t addr, float value)
{
	if(!emuoffset || N64NOTWITHINMEMRANGE(addr)) // if n64 memory has not been init by emulator or writing to outside of memory range
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &value, sizeof(value), NULL);
}

uint8_t SNES_MEM_ReadByte(const uint32_t addr)
{
	if(!emuoffset || SNESNOTWITHINMEMRANGE(addr)) // if snes memory has not been init by emulator or reading from outside of memory range
		return 0;
	uint8_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return output;
}

uint16_t SNES_MEM_ReadWord(const uint32_t addr) // 16bit word
{
	if(!emuoffset || SNESNOTWITHINMEMRANGE(addr)) // if snes memory has not been init by emulator or reading from outside of memory range
		return 0;
	uint16_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return output;
}

void SNES_MEM_WriteByte(const uint32_t addr, uint8_t value)
{
	if(!emuoffset || SNESNOTWITHINMEMRANGE(addr)) // if snes memory has not been init by emulator or writing to outside of memory range
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

void SNES_MEM_WriteWord(const uint32_t addr, uint16_t value) // 16bit word
{
	if(!emuoffset || SNESNOTWITHINMEMRANGE(addr)) // if snes memory has not been init by emulator or writing to outside of memory range
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

uint32_t PS2_MEM_ReadPointer(const uint32_t addr)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr)) // if ps2 memory has not been init by emulator or reading from outside of memory range
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000)), &output, sizeof(output), NULL);
	// printdebug(1); // debug
	// MEM_ByteSwap32(&output); // byteswap
	return output;
}

uint32_t PS2_MEM_ReadUInt(const uint32_t addr)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr)) // if ps2 memory has not been init by emulator or reading from outside of memory range
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000)), &output, sizeof(output), NULL);
	// printdebug(1); // debug
	MEM_ByteSwap32(&output); // byteswap
	return output;
}

float PS2_MEM_ReadFloat(const uint32_t addr)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
		return 0;
	float output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000)), &output, sizeof(output), NULL);
	// MEM_ByteSwap32((uint32_t *)&output); // byteswap
	return output;
}

void PS2_MEM_WriteFloat(const uint32_t addr, float value)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or writing to outside of memory range
		return;
	// MEM_ByteSwap32((uint32_t *)&value); // byteswap
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000)), &value, sizeof(value), NULL);
}

void printdebug(uint32_t val)
{
	FILE *fp;

	fp = fopen("test.txt", "w");
	char output[255];
	sprintf(output, "%u", val);
	fprintf(fp, output);
	
	fclose(fp);
}