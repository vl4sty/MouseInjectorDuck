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
#include <math.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define TAU 6.2831853f // 0x40C90FDB

#define RT3_shipy 0x00001644
#define RT3_shipx 0x00001844
#define RT3_animframe 0x00009C
#define RT3_isPaused 0x0000DA
#define RT3_inMenu 0x000080
#define RT3_isDead 0x00007E

static uint8_t SNES_RT3_Status(void);
static void SNES_RT3_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"R-Type III",
	SNES_RT3_Status,
	SNES_RT3_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

static uint8_t lastX = 0;
static uint8_t lastY = 0;
static uint8_t upFrames = 0;
static uint8_t downFrames = 0;
static uint8_t animFrame = 0;
static float MAXDIFF = 10.f;

const GAMEDRIVER *GAME_SNES_RTYPE3 = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0;
static float yAccumulator = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SNES_RT3_Status(void)
{
	return (SNES_MEM_ReadWord(0x130) == 0xF3A6); // 1 random static address
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SNES_RT3_Inject(void)
{
	if (SNES_MEM_ReadByte(RT3_isPaused) != 0) // return if paused
		return;

	// TODO: fix jitter on boss 1, find another value to determine menu
	// if (SNES_MEM_ReadByte(RT3_inMenu) == 0) // causes jittering on boss 1 due to flipping between 0 and other values
	// 	return;

	// TODO: find different value as returning on this value interferes Hyper of weapon 3
	// if (SNES_MEM_ReadByte(RT3_isDead) == 1) // return if ship destroyed
	// 	return;

	// pause scrolling
	// SNES_MEM_WriteWord(0x1C, 0);
	// SNES_MEM_WriteWord(0x20, 0);
	// SNES_MEM_WriteWord(0x24, 0);
	// SNES_MEM_WriteWord(0x5E, 0);
	// SNES_MEM_WriteWord(0x62, 0);
	// SNES_MEM_WriteWord(0x66, 0);
	// SNES_MEM_WriteWord(0x200, 0);
	// SNES_MEM_WriteWord(0x2E4, 0);
	// SNES_MEM_WriteWord(0x39E, 0);

	SNES_MEM_WriteByte(RT3_animframe, animFrame);

	if (ymouse == 0)
	{
		if (upFrames > 0)
		{
			if (upFrames <= 3)
				animFrame = 129;
			upFrames -= 1;
		}
		else
		{
			animFrame = 128;
			upFrames = 0;
		}

		if (downFrames > 0)
		{
			if (downFrames <= 3)
				animFrame = 131;
			downFrames -= 1;
		}
		else
		{
			animFrame = 128;
			downFrames = 0;
		}
	}

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	

	// const float looksensitivity = (float)sensitivity / 30.f;
	// uint8_t shipx = SNES_MEM_ReadByte(RT3_shipx);
	// uint8_t shipy = SNES_MEM_ReadByte(RT3_shipy);
	// shipx = (float)shipx;
	// shipy = (float)shipy;

	const float looksensitivity = (float)sensitivity / 40.f;

	uint8_t shipXInt = SNES_MEM_ReadByte(RT3_shipx);
	uint8_t shipYInt = SNES_MEM_ReadByte(RT3_shipy);
	float shipX = (float)shipXInt;
	float shipY = (float)shipYInt;

	// // count how many frames you've been doing in one direction
	if (ymouse < 0 && lastY > shipY)
	{
		downFrames = 0;
		if (ymouse > -12.f && ymouse < -6.f)
		{
			animFrame = 129;
			upFrames = 3;
		}
		else if (ymouse <= -12.f)
		{
			animFrame = 130;
			upFrames = 14;
		}
	}
	else if (ymouse > 0 && lastY < shipY)
	{
		upFrames = 0;
		if (ymouse > 6.f && ymouse < 12.f)
		{
			animFrame = 131;
			downFrames = 3;
		}
		else if (ymouse >= 12.f)
		{
			animFrame = 132;
			downFrames = 14;
		}
	}

	lastX = shipX;
	lastY = shipY;

	// int8_t diff = 0;
	// diff = xmouse * looksensitivity;
	// diff = ClampInt(diff, -MAXDIFF, MAXDIFF);
	// shipx += diff;
	// diff = ymouse * looksensitivity;
	// diff = ClampInt(diff, -MAXDIFF, MAXDIFF);
	// shipy += diff;

	// // prevent wrapping
	// if (lastX > 0 && lastX < 100 && shipx > 200)
	// 	shipx = 0.f;
	// if (lastY > 0 && lastY < 100 && shipy > 150)
	// 	shipy = 0.f;

	// shipx = ClampFloat(shipx, 1.f, 224.f);
	// shipy = ClampFloat(shipy, 10.f, 192.f);

	// SNES_MEM_WriteByte(RT3_shipx, (uint8_t)shipx);
	// SNES_MEM_WriteByte(RT3_shipy, (uint8_t)shipy);

	if (xmouse != 0)
	{
		float dx = (float)xmouse * looksensitivity;
		if (xmouse < 0)
			shipX += ceil(dx);
		else
			shipX += (uint16_t)dx;

		float r = fmod(dx, 1.f);

		if (abs(r + xAccumulator) >= 1)
		{
			if (xmouse > 0)
				shipX += 1.f;
			else
				shipX -= 1.f;
		}
	
		xAccumulator = fmod(r + xAccumulator, 1.f);
	}

	if (ymouse != 0)
	{
		int ym = (invertpitch ? -ymouse : ymouse);
		float dy = (float)ym * looksensitivity;
		// if (ymouse < 0)
		if (ym < 0)
			shipY += ceil(dy);
		else
			shipY += (uint16_t)dy;

		float r = fmod(dy, 1.f);

		if (abs(r + yAccumulator) >= 1)
		{
			if (ym > 0)
				shipY += 1.f;
			else
				shipY -= 1.f;
		}
		
		yAccumulator = fmod(r + yAccumulator, 1.f);
	}

	shipX = ClampFloat(shipX, 1.f, 224.f);
	shipY = ClampFloat(shipY, 10.f, 192.f);

	SNES_MEM_WriteByte(RT3_shipx, (uint8_t)shipX);
	SNES_MEM_WriteByte(RT3_shipy, (uint8_t)shipY);
}