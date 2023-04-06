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
#include <math.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define PI 3.1415926f // 0x40490FDB
// #define PI 3.14159265359f // 0x40490FDB
// #define TAU 6.2831853f // 0x40C90FDB
#define TAU 6.28319f // 0x40C90FDB

#define POED_CAMY 0x89F94
#define TAU 6.2831853f // 0x40C90FDB

#define POED_CAMY 0x89F94
#define POED_CAMY_SIGN 0x89F96
#define POED_CAMY2 0x89FB0
#define POED_CAMY2_SIGN 0x89FB2
#define POED_CAMX 0x9A6C0

#define POED_CAMX_SIN 0x9A6C0
#define POED_CAMX_SIN_SIGN 0x9A6C2
#define POED_CAMX_COS 0x9A6C8
#define POED_CAMX_COS_SIGN 0x9A6CA
// #define POED_CAMX_COS 0x9A6C0
// #define POED_CAMX_COS_SIGN 0x9A6C2
// #define POED_CAMX_SIN 0x9A6C8
// #define POED_CAMX_SIN_SIGN 0x9A6CA

#define POED_ALLOW_MOUSE_MOVEMENT 0x89E62
#define POED_TOTAL_ANGLE_UNSET -99.f

static uint8_t PS1_POED_Status(void);
static void PS1_POED_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"PO'ed",
	PS1_POED_Status,
	PS1_POED_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_POED = &GAMEDRIVER_INTERFACE;

static uint16_t lastCamY = 0;
static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static float totalAngle = POED_TOTAL_ANGLE_UNSET;
static float normalizeValue = 65536.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_POED_Status(void)
{
	return (PS1_MEM_ReadWord(0x946C) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x9470) == 0x5F303030U && 
			PS1_MEM_ReadWord(0x9474) == 0x2E39373BU); // SLUS_000.97;

	// for no$psx debugger
	// return (PS1_MEM_ReadWord(0x0) == 0x03000000 && 
	// 		PS1_MEM_ReadWord(0x4) == 0x800C5A27 && 
	// 		PS1_MEM_ReadWord(0x8) == 0x08004003);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_POED_Inject(void)
{
	// TODO: make cheat file
	// TODO: disable during karate flip and reset direction after
	// FIXME: camX flicker at cardinal directions
	// 		TODO: use signed values?
	//		TODO: try 0 to TAU scale
	//		TODO: try fixed-point sine/cosine approximation instead of using float sin and casting

	// TODO: prevent writing to camx sin/cos

	const float looksensitivity = (float)sensitivity / 200.f;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	PS1_MEM_WriteHalfword(POED_ALLOW_MOUSE_MOVEMENT, 0xFFFF);

	if (xmouse)
	{
		int32_t camXSin = PS1_MEM_ReadInt(POED_CAMX_SIN);
		int32_t camXCos = PS1_MEM_ReadInt(POED_CAMX_COS);
		// if (camXSin == 0) camXSin = 1;
		// if (camXCos == 0) camXCos = 1;
		float camXSinF = (float)camXSin / 65536.f;
		float camXCosF = (float)camXCos / 65536.f;

		// if (camXCosF == 0)
		// 	camXCosF = 1.f / 65536.f;
		// if (camXSinF == 0)
		// 	camXSinF = 1.f / 65536.f;

		// if (totalAngle == POED_TOTAL_ANGLE_UNSET)
		// totalAngle = (float)atan((float)camXSin / (float)camXCos);
		totalAngle = (float)atan((float)camXSinF / (float)camXCosF);

		if (camXCos < 0)
			totalAngle += PI;

		totalAngle += (float)xmouse * looksensitivity / 20.f;


		while (totalAngle > TAU)
			totalAngle -= TAU;
		while (totalAngle < 0)
			totalAngle += TAU;
		
		totalAngleOut = totalAngle;

		// camXSinF = (float)sin(totalAngle) * 65535.f;
		// camXCosF = (float)cos(totalAngle) * 65535.f;

		// PS1_MEM_WriteInt(POED_CAMX_SIN, (int32_t)camXSinF);
		// PS1_MEM_WriteInt(POED_CAMX_COS, (int32_t)camXCosF);
		PS1_MEM_WriteInt(POED_CAMX_SIN, (int32_t)((float)sin(totalAngle) * 65536.f));
		PS1_MEM_WriteInt(POED_CAMX_COS, (int32_t)((float)cos(totalAngle) * 65536.f));
	}

	if (ymouse)
	// if (0)
	{
		uint16_t camY = PS1_MEM_ReadHalfword(POED_CAMY2);
		uint16_t camYSign = PS1_MEM_ReadHalfword(POED_CAMY2_SIGN);
		float camYF = (float)camY;
		if (camYSign)
		{
			// normalize negatives to a -65536 to +65536 scale
			camYF = 65536 - camYF;
			camYF *= -1;
		}

		float ym = (float)(invertpitch ? -ymouse : ymouse);
		float dy = -ym * looksensitivity * 5000;
		AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);

		if (camYF > 60000)
			camYF = 60000;
		if (camYF < -60000)
			camYF = -60000;

		if (camYF < 0)
		{
			camYSign = 0xFFFF;
			camYF += 65536;
		}
		else
			camYSign = 0x0;

		PS1_MEM_WriteHalfword(POED_CAMY_SIGN, (uint16_t)camYSign);
		PS1_MEM_WriteHalfword(POED_CAMY2_SIGN, (uint16_t)camYSign);

		PS1_MEM_WriteHalfword(POED_CAMY, (uint16_t)camYF);
		PS1_MEM_WriteHalfword(POED_CAMY2, (uint16_t)camYF);
	}
}


		// uint16_t camXSin = PS1_MEM_ReadHalfword(POED_CAMX_SIN);
		// uint16_t camXCos = PS1_MEM_ReadHalfword(POED_CAMX_COS);
		// float camXSinF = (float)camXSin / normalizeValue;
		// float camXCosF = (float)camXCos / normalizeValue;
		// uint16_t camXSinSign = PS1_MEM_ReadHalfword(POED_CAMX_SIN_SIGN);
		// uint16_t camXCosSign = PS1_MEM_ReadHalfword(POED_CAMX_COS_SIGN);
		// float sinSignF = 1;
		// float cosSignF = 1;
		// if (camXSinSign == 0xFFFF) sinSignF = -1;
		// if (camXCosSign == 0xFFFF) cosSignF = -1;

		// if (totalAngle == POED_TOTAL_ANGLE_UNSET)
		// { 
		// 	totalAngle = atan((camXSinF * sinSignF) / (camXCosF * cosSignF));
		// 	if (cosSignF == -1)
		// 	{
		// 		totalAngle += TAU / 2;
		// 	}
		// }
		// // TODO: add PI if cos/sin are negative

		// totalAngle += (float)xmouse * looksensitivity / 20.f;

		// while (totalAngle < 0)
		// 	totalAngle += (2 * PI);
		// while (totalAngle > (2 * PI))
		// 	totalAngle -= (2 * PI);
		// // while (totalAngle > PI)
		// // 	totalAngle -= (2 * PI);
		// // while (totalAngle < -PI)
		// // 	totalAngle += (2 * PI);
		
		// totalAngleOut = totalAngle;

		// camXSinF = (float)sin(totalAngle) * normalizeValue;
		// camXCosF = (float)cos(totalAngle) * normalizeValue;

		// // while (camXSinF == 0 && camXCosF == 0)
		// // {
		// // 	totalAngle += (float)xmouse * looksensitivity / 20.f;

		// // 	while (totalAngle >= (2 * PI))
		// // 		totalAngle -= (2 * PI);
		// // 	while (totalAngle < 0)
		// // 		totalAngle += (2 * PI);

		// // 	camXSinF = sin(totalAngle) * 65536.f;
		// // 	camXCosF = cos(totalAngle) * 65536.f;
		// // }


		// // if (totalAngle >= 0 && totalAngle < PI / 2){
		// if (totalAngle < PI / 2){
		// 	PS1_MEM_WriteHalfword(POED_CAMX_SIN_SIGN, 0x0);
		// 	PS1_MEM_WriteHalfword(POED_CAMX_COS_SIGN, 0x0);
		// }
		// else if (totalAngle > PI / 2 && totalAngle < PI){
		// 	PS1_MEM_WriteHalfword(POED_CAMX_SIN_SIGN, 0x0);
		// 	PS1_MEM_WriteHalfword(POED_CAMX_COS_SIGN, 0xFFFF);
		// }
		// else if (totalAngle > PI && totalAngle < (PI * 3) / 2){
		// 	PS1_MEM_WriteHalfword(POED_CAMX_SIN_SIGN, 0xFFFF);
		// 	PS1_MEM_WriteHalfword(POED_CAMX_COS_SIGN, 0xFFFF);
		// }
		// // else if (totalAngle > (PI * 3) / 2 && totalAngle < PI * 2){
		// else if (totalAngle > (PI * 3) / 2){
		// 	PS1_MEM_WriteHalfword(POED_CAMX_SIN_SIGN, 0xFFFF);
		// 	PS1_MEM_WriteHalfword(POED_CAMX_COS_SIGN, 0x0);
		// }


		// // if (totalAngle > PI / 2 && totalAngle < (3 * PI) / 2)
		// // 	PS1_MEM_WriteHalfword(POED_CAMX_SIN_SIGN, 0xFFFF);
		// // else
		// // 	PS1_MEM_WriteHalfword(POED_CAMX_SIN_SIGN, 0x0);

		// // if (totalAngle > PI && totalAngle < (2 * PI))
		// // 	PS1_MEM_WriteHalfword(POED_CAMX_COS_SIGN, 0xFFFF);
		// // else
		// // 	PS1_MEM_WriteHalfword(POED_CAMX_COS_SIGN, 0x0);
		

		// // if (angle > PI / 2)
		// // {
		// // 	PS1_MEM_WriteHalfword(POED_CAMX_SIN_SIGN, 0xFFFF);
		// // 	PS1_MEM_WriteHalfword(POED_CAMX_COS_SIGN, 0xFFFF);
		// // }

		// PS1_MEM_WriteHalfword(POED_CAMX_SIN, (uint16_t)camXSinF);
		// PS1_MEM_WriteHalfword(POED_CAMX_COS, (uint16_t)camXCosF);



// signed integer solution

		// int32_t camXSin = PS1_MEM_ReadInt(POED_CAMX_SIN);
		// int32_t camXCos = PS1_MEM_ReadInt(POED_CAMX_COS);
		// // if (camXSin == 0) camXSin = 1;
		// // if (camXCos == 0) camXCos = 1;
		// float camXSinF = (float)camXSin / 65535.f;
		// float camXCosF = (float)camXCos / 65535.f;

		// // if (camXCosF == 0)
		// // 	camXCosF = 1.f / 65536.f;
		// // if (camXSinF == 0)
		// // 	camXSinF = 1.f / 65536.f;

		// // if (totalAngle == POED_TOTAL_ANGLE_UNSET)
		// totalAngle = (float)atan(camXSinF / camXCosF);

		// if (camXCosF < 0)
		// 	totalAngle += PI;

		// totalAngle += (float)xmouse * looksensitivity / 20.f;

		// while (totalAngle > TAU / 2)
		// 	totalAngle -= TAU;
		// while (totalAngle < -TAU / 2)
		// 	totalAngle += TAU;
		
		// totalAngleOut = totalAngle;

		// camXSinF = (float)sin(totalAngle) * 65535.f;
		// camXCosF = (float)cos(totalAngle) * 65535.f;

		// PS1_MEM_WriteInt(POED_CAMX_SIN, (int32_t)camXSinF);
		// PS1_MEM_WriteInt(POED_CAMX_COS, (int32_t)camXCosF);