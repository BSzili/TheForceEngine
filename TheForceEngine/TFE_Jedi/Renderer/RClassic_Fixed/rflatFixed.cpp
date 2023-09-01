#include <TFE_Jedi/Math/fixedPoint.h>
#include <TFE_Jedi/Level/rtexture.h>
#include "rflatFixed.h"
#include "rlightingFixed.h"
#include "redgePairFixed.h"
#include "rclassicFixed.h"
#include "rclassicFixedSharedState.h"
#ifdef __AMIGA__
#include "../rscanline.cpp"
#else
#include "../rscanline.h"
#endif
#include "../rsectorRender.h"
#include "../redgePair.h"
#include "../rcommon.h"
#ifdef __AMIGA__
#define FIXED_6_26 // 6.26 fixed point
#define s_width (320)
#define s_height (200)
#endif
#include <assert.h>

namespace TFE_Jedi
{

namespace RClassic_Fixed
{
	static s32 s_scanlineX0;

	static fixed16_16 s_scanlineU0;
	static fixed16_16 s_scanlineV0;
	static fixed16_16 s_scanline_dUdX;
	static fixed16_16 s_scanline_dVdX;

	static s32 s_scanlineWidth;
	static const u8* s_scanlineLight;
	static u8* s_scanlineOut;

	static u8* s_ftexImage;
	static s32 s_ftexDataEnd;
	static s32 s_ftexHeight;
	static s32 s_ftexWidthMask;
	static s32 s_ftexHeightMask;
	static s32 s_ftexHeightLog2;
#ifdef __AMIGA__1
	#define s_ftexDataEnd (4095)
#endif
		
	void flat_addEdges(s32 length, s32 x0, fixed16_16 dyFloor_dx, fixed16_16 yFloor, fixed16_16 dyCeil_dx, fixed16_16 yCeil)
	{//t2++;
		if (s_flatCount < MAX_SEG && length > 0)
		{
#ifdef __AMIGA__
			const s32 lengthMinusOne = length - 1;

			fixed16_16 yCeil1 = yCeil;
			if (dyCeil_dx != 0)
			{
				yCeil1 += dyCeil_dx * lengthMinusOne;
			}
			fixed16_16 yFloor1 = yFloor;
			if (dyFloor_dx != 0)
			{
				yFloor1 += dyFloor_dx * lengthMinusOne;
			}
#else
			const fixed16_16 lengthFixed = intToFixed16(length - 1);

			fixed16_16 yCeil1 = yCeil;
			if (dyCeil_dx != 0)
			{
				yCeil1 += mul16(dyCeil_dx, lengthFixed);
			}
			fixed16_16 yFloor1 = yFloor;
			if (dyFloor_dx != 0)
			{
				yFloor1 += mul16(dyFloor_dx, lengthFixed);
			}
#endif

			edgePair_setup(length, x0, dyFloor_dx, yFloor1, yFloor, dyCeil_dx, yCeil, yCeil1, s_rcfState.flatEdge);

			if (s_rcfState.flatEdge->yPixel_C1 - 1 > s_wallMaxCeilY)
			{
				s_wallMaxCeilY = s_rcfState.flatEdge->yPixel_C1 - 1;
			}
			if (s_rcfState.flatEdge->yPixel_F1 + 1 < s_wallMinFloorY)
			{
				s_wallMinFloorY = s_rcfState.flatEdge->yPixel_F1 + 1;
			}
			if (s_wallMaxCeilY < s_windowMinY_Pixels)
			{
				s_wallMaxCeilY = s_windowMinY_Pixels;
			}
			if (s_wallMinFloorY > s_windowMaxY_Pixels)
			{
				s_wallMinFloorY = s_windowMaxY_Pixels;
			}

			s_rcfState.flatEdge++;
			s_flatCount++;
		}
	}
				
	// This produces functionally identical results to the original but splits apart the U/V and dUdx/dVdx into seperate variables
	// to account for C vs ASM differences.
	void drawScanline()
	{
#ifdef FIXED_6_26
		u32 U = s_scanlineU0 << 10;
		u32 V = s_scanlineV0 << 10;
		const u32 dUdX = s_scanline_dUdX << 10;
		const u32 dVdX = s_scanline_dVdX << 10;

		// Note this produces a distorted mapping if the texture is not 64x64.
		// This behavior matches the original.
		u32 texel = ((U >> 26)<<6) + (V >> 26);
		texel &= s_ftexDataEnd;
		U += dUdX;
		V += dVdX;

		for (s32 i = s_scanlineWidth - 1; i >= 0; i--)
		{
			u8 c = s_scanlineLight[s_ftexImage[texel]];
			texel = ((U >> 26)<<6) + (V >> 26);
			texel &= s_ftexDataEnd;
			U += dUdX;
			V += dVdX;
			s_scanlineOut[i] = c;
			//s_scanlineOut[i] = ((u32)s_scanlineOut) & 0xFF;
		}
#else
		fixed16_16 U = s_scanlineU0;
		fixed16_16 V = s_scanlineV0;
		const fixed16_16 dUdX = s_scanline_dUdX;
		const fixed16_16 dVdX = s_scanline_dVdX;

		// Note this produces a distorted mapping if the texture is not 64x64.
		// This behavior matches the original.
		u32 texel = ((floor16(U) & 63)<<6) + (floor16(V) & 63);
		texel &= s_ftexDataEnd;
		U += dUdX;
		V += dVdX;

		for (s32 i = s_scanlineWidth - 1; i >= 0; i--)
		{
			u8 c = s_scanlineLight[s_ftexImage[texel]];
			texel = ((floor16(U) & 63)<<6) + (floor16(V) & 63);
			texel &= s_ftexDataEnd;
			U += dUdX;
			V += dVdX;
			s_scanlineOut[i] = c;
		}
#endif
	}

	void drawScanline_Fullbright()
	{
#ifdef FIXED_6_26
		u32 V = s_scanlineV0 << 10;
		u32 U = s_scanlineU0 << 10;
		u32 dVdX = s_scanline_dVdX << 10;
		u32 dUdX = s_scanline_dUdX << 10;

		// Note this produces a distorted mapping if the texture is not 64x64.
		// This behavior matches the original.
		u32 texel = ((U >> 26)<<6) + (V >> 26);
		texel &= s_ftexDataEnd;
		U += dUdX;
		V += dVdX;

		for (s32 i = s_scanlineWidth - 1; i >= 0; i--)
		{
			u8 c = s_ftexImage[texel];
			texel = ((U >> 26)<<6) + (V >> 26);
			texel &= s_ftexDataEnd;
			U += dUdX;
			V += dVdX;
			s_scanlineOut[i] = c;
		}
#else
		fixed16_16 V = s_scanlineV0;
		fixed16_16 U = s_scanlineU0;
		fixed16_16 dVdX = s_scanline_dVdX;
		fixed16_16 dUdX = s_scanline_dUdX;

		// Note this produces a distorted mapping if the texture is not 64x64.
		// This behavior matches the original.
		u32 texel = (floor16(U) & 63) * 64 + (floor16(V) & 63);
		texel &= s_ftexDataEnd;
		U += dUdX;
		V += dVdX;

		for (s32 i = s_scanlineWidth - 1; i >= 0; i--)
		{
			u8 c = s_ftexImage[texel];
			texel = (floor16(U) & 63) * 64 + (floor16(V) & 63);
			texel &= s_ftexDataEnd;
			U += dUdX;
			V += dVdX;
			s_scanlineOut[i] = c;
		}
#endif
	}

	void drawScanline_Trans()
	{
#ifdef FIXED_6_26
		u32 V = s_scanlineV0 << 10;
		u32 U = s_scanlineU0 << 10;
		u32 dVdX = s_scanline_dVdX << 10;
		u32 dUdX = s_scanline_dUdX << 10;

		// Note this produces a distorted mapping if the texture is not 64x64.
		// This behavior matches the original.
		u32 texel = ((U >> 26)<<6) + (V >> 26);
		texel &= s_ftexDataEnd;
		U += dUdX;
		V += dVdX;

		for (s32 i = s_scanlineWidth - 1; i >= 0; i--)
		{
			u8 baseColor = s_ftexImage[texel];
			u8 c = s_scanlineLight[baseColor];
			texel = ((U >> 26)<<6) + (V >> 26);
			texel &= s_ftexDataEnd;
			U += dUdX;
			V += dVdX;

			if (baseColor) { s_scanlineOut[i] = c; }
		}
#else
		fixed16_16 V = s_scanlineV0;
		fixed16_16 U = s_scanlineU0;
		fixed16_16 dVdX = s_scanline_dVdX;
		fixed16_16 dUdX = s_scanline_dUdX;

		// Note this produces a distorted mapping if the texture is not 64x64.
		// This behavior matches the original.
		u32 texel = (floor16(U) & 63) * 64 + (floor16(V) & 63);
		texel &= s_ftexDataEnd;
		U += dUdX;
		V += dVdX;

		for (s32 i = s_scanlineWidth - 1; i >= 0; i--)
		{
			u8 baseColor = s_ftexImage[texel];
			u8 c = s_scanlineLight[baseColor];
			texel = (floor16(U) & 63) * 64 + (floor16(V) & 63);
			texel &= s_ftexDataEnd;
			U += dUdX;
			V += dVdX;

			if (baseColor) { s_scanlineOut[i] = c; }
		}
#endif
	}

	void drawScanline_Fullbright_Trans()
	{
#ifdef FIXED_6_26
		u32 V = s_scanlineV0 << 10;
		u32 U = s_scanlineU0 << 10;
		u32 dVdX = s_scanline_dVdX << 10;
		u32 dUdX = s_scanline_dUdX << 10;

		// Note this produces a distorted mapping if the texture is not 64x64.
		// This behavior matches the original.
		u32 texel = ((U >> 26)<<6) + (V >> 26);
		texel &= s_ftexDataEnd;
		U += dUdX;
		V += dVdX;

		for (s32 i = s_scanlineWidth - 1; i >= 0; i--)
		{
			u8 c = s_ftexImage[texel];
			texel = ((U >> 26)<<6) + (V >> 26);
			texel &= s_ftexDataEnd;
			U += dUdX;
			V += dVdX;
			if (c) { s_scanlineOut[i] = c; }
		}
#else
		fixed16_16 V = s_scanlineV0;
		fixed16_16 U = s_scanlineU0;
		fixed16_16 dVdX = s_scanline_dVdX;
		fixed16_16 dUdX = s_scanline_dUdX;

		// Note this produces a distorted mapping if the texture is not 64x64.
		// This behavior matches the original.
		u32 texel = (floor16(U) & 63) * 64 + (floor16(V) & 63);
		texel &= s_ftexDataEnd;
		U += dUdX;
		V += dVdX;

		for (s32 i = s_scanlineWidth - 1; i >= 0; i--)
		{
			u8 c = s_ftexImage[texel];
			texel = (floor16(U) & 63) * 64 + (floor16(V) & 63);
			texel &= s_ftexDataEnd;
			U += dUdX;
			V += dVdX;
			if (c) { s_scanlineOut[i] = c; }
		}
#endif
	}
			   
	bool flat_setTexture(TextureData* tex)
	{
		if (!tex) { return false; }

		s_ftexHeight = tex->height;
		s_ftexWidthMask = tex->width - 1;
		s_ftexHeightMask = tex->height - 1;
		s_ftexHeightLog2 = tex->logSizeY;
		s_ftexImage = tex->image;
#ifndef __AMIGA__1
		s_ftexDataEnd = tex->width * tex->height - 1;
#endif

		return true;
	}

	void flat_drawCeiling(RSector* sector, EdgePairFixed* edges, s32 count)
	{
		fixed16_16 textureOffsetU = s_rcfState.cameraPos.x - sector->ceilOffset.x;
		fixed16_16 textureOffsetV = sector->ceilOffset.z - s_rcfState.cameraPos.z;

		fixed16_16 relCeil          =  sector->ceilingHeight - s_rcfState.eyeHeight;
#ifdef __AMIGA__
		fixed16_16 scaledRelCeil    =  (relCeil * s_rcfState.focalLenAspect);
#else
		fixed16_16 scaledRelCeil    =  mul16(relCeil, s_rcfState.focalLenAspect);
#endif
		fixed16_16 cosScaledRelCeil =  mul16(scaledRelCeil, s_rcfState.cosYaw);
		fixed16_16 negSinRelCeil    = -mul16(relCeil, s_rcfState.sinYaw);
		fixed16_16 sinScaledRelCeil =  mul16(scaledRelCeil, s_rcfState.sinYaw);
		fixed16_16 negCosRelCeil    = -mul16(relCeil, s_rcfState.cosYaw);

		if (!flat_setTexture(*sector->ceilTex)) { return; }

		for (s32 y = s_windowMinY_Pixels; y <= s_wallMaxCeilY && y < s_windowMaxY_Pixels; y++)
		{
			s32 x = s_windowMinX_Pixels;
			s32 yOffset = y * s_width;
			s32 yShear = s_screenYMidBase - s_screenYMidFix;
			assert(yShear + y + s_height * 2 >= 0 && yShear + y + s_height * 2 <= s_height * 4);
			fixed16_16 yRcp = s_rcfState.rcpY[yShear + y + s_height*2];
			fixed16_16 z = mul16(scaledRelCeil, yRcp);

			s32 left = 0;
			s32 right = 0;
			for (s32 i = 0; i < count;)
			{
				if (!flat_buildScanlineCeiling(i, count, x, y, left, right, s_scanlineWidth, edges))
				{
					break;
				}

				if (s_scanlineWidth > 0)
				{
					assert(left >= 0 && left + s_scanlineWidth <= s_width);
					assert(y >= 0 && y < s_height);
					s_scanlineX0  = left;
					s_scanlineOut = &s_display[left + yOffset];

					const fixed16_16 worldToTexelScale = fixed16_16(8);
#ifdef __AMIGA__
					s32 rightClip = right - s_screenXMid;
#else
					fixed16_16 rightClip = intToFixed16(right - s_screenXMid);
#endif

#ifdef __AMIGA__
					fixed16_16 v0 = mul16(cosScaledRelCeil - (negSinRelCeil * rightClip), yRcp);
					fixed16_16 u0 = mul16(sinScaledRelCeil + (negCosRelCeil * rightClip), yRcp);
#else
					fixed16_16 v0 = mul16(cosScaledRelCeil - mul16(negSinRelCeil, rightClip), yRcp);
					fixed16_16 u0 = mul16(sinScaledRelCeil + mul16(negCosRelCeil, rightClip), yRcp);
#endif

					s_scanlineV0 = (v0 - textureOffsetV) * worldToTexelScale;
					s_scanlineU0 = (u0 - textureOffsetU) * worldToTexelScale;

					s_scanline_dVdX =  mul16(negSinRelCeil, yRcp) * worldToTexelScale;
					s_scanline_dUdX = -mul16(negCosRelCeil, yRcp) * worldToTexelScale;

					s_scanlineLight =  computeLighting(z, 0);
					
					if (s_scanlineLight)
					{
						drawScanline();
					}
					else
					{
						drawScanline_Fullbright();
					}
				}
			} // while (i < count)
		}
	}
		
	void flat_drawFloor(RSector* sector, EdgePairFixed* edges, s32 count)
	{
		fixed16_16 textureOffsetU = s_rcfState.cameraPos.x - sector->floorOffset.x;
		fixed16_16 textureOffsetV = sector->floorOffset.z - s_rcfState.cameraPos.z;

		fixed16_16 relFloor       = sector->floorHeight - s_rcfState.eyeHeight;
#ifdef __AMIGA__
		fixed16_16 scaledRelFloor = (relFloor * s_rcfState.focalLenAspect);
#else
		fixed16_16 scaledRelFloor = mul16(relFloor, s_rcfState.focalLenAspect);
#endif

		fixed16_16 cosScaledRelFloor = mul16(scaledRelFloor, s_rcfState.cosYaw);
		fixed16_16 negSinRelFloor    =-mul16(relFloor, s_rcfState.sinYaw);
		fixed16_16 sinScaledRelFloor = mul16(scaledRelFloor, s_rcfState.sinYaw);
		fixed16_16 negCosRelFloor    =-mul16(relFloor, s_rcfState.cosYaw);

		if (!flat_setTexture(*sector->floorTex)) { return; }

		for (s32 y = max(s_wallMinFloorY, s_windowMinY_Pixels); y <= s_windowMaxY_Pixels; y++)
		{
			s32 x = s_windowMinX_Pixels;
			s32 yOffset = y * s_width;
			s32 yShear = s_screenYMidBase - s_screenYMidFix;
			assert(yShear + y + s_height * 2 >= 0 && yShear + y + s_height * 2 <= s_height * 4);
			fixed16_16 yRcp = s_rcfState.rcpY[yShear + y + s_height*2];
			fixed16_16 z = mul16(scaledRelFloor, yRcp);

			s32 left = 0;
			s32 right = 0;
			for (s32 i = 0; i < count;)
			{
				s32 winMaxX = s_windowMaxX_Pixels;

				// Search for the left edge of the scanline.
				if (!flat_buildScanlineFloor(i, count, x, y, left, right, s_scanlineWidth, edges))
				{
					break;
				}

				if (s_scanlineWidth > 0)
				{
					assert(left >= 0 && left + s_scanlineWidth <= s_width);
					assert(y >= 0 && y < s_height);
					s_scanlineX0 = left;
					s_scanlineOut = &s_display[left + yOffset];

#ifdef __AMIGA__
					s32 rightClip = right - s_screenXMid;
#else
					fixed16_16 rightClip = intToFixed16(right - s_screenXMid);
#endif
					fixed16_16 worldToTexelScale = fixed16_16(8);

#ifdef __AMIGA__
					fixed16_16 v0 = mul16(cosScaledRelFloor - (negSinRelFloor * rightClip), yRcp);
					fixed16_16 u0 = mul16(sinScaledRelFloor + (negCosRelFloor * rightClip), yRcp);
#else
					fixed16_16 v0 = mul16(cosScaledRelFloor - mul16(negSinRelFloor, rightClip), yRcp);
					fixed16_16 u0 = mul16(sinScaledRelFloor + mul16(negCosRelFloor, rightClip), yRcp);
#endif
					s_scanlineV0 = (v0 - textureOffsetV) * worldToTexelScale;
					s_scanlineU0 = (u0 - textureOffsetU) * worldToTexelScale;

					s_scanline_dVdX =  mul16(negSinRelFloor, yRcp) * worldToTexelScale;
					s_scanline_dUdX = -mul16(negCosRelFloor, yRcp) * worldToTexelScale;
					s_scanlineLight = computeLighting(z, 0);

					if (s_scanlineLight)
					{
						drawScanline();
					}
					else
					{
						drawScanline_Fullbright();
					}
				}
			} // while (i < count)
		}
	}

	//////////////////////////////////////////////////////////////////////
	// Polygon Scanline rendering using the same algorithms as flats.
	//////////////////////////////////////////////////////////////////////
	typedef void(*ScanlineFunction)();
	static const ScanlineFunction c_scanlineDrawFunc[] =
	{
		drawScanline,
		drawScanline_Fullbright,
		drawScanline_Trans,
		drawScanline_Fullbright_Trans
	};

	static fixed16_16 s_poly_offsetX;
	static fixed16_16 s_poly_offsetZ;

	static fixed16_16 s_poly_scaledHOffset;
	static fixed16_16 s_poly_sinYawHOffset;
	static fixed16_16 s_poly_cosYawHOffset;

	static fixed16_16 s_poly_cosYawScaledHOffset;
	static fixed16_16 s_poly_sinYawScaledHOffset;
		
	void flat_preparePolygon(fixed16_16 heightOffset, fixed16_16 offsetX, fixed16_16 offsetZ, TextureData* texture)
	{
		s_poly_offsetX = s_rcfState.cameraPos.x - offsetX;
		s_poly_offsetZ = offsetZ - s_rcfState.cameraPos.z;

#ifdef __AMIGA__
		s_poly_scaledHOffset = (heightOffset * s_rcfState.focalLenAspect);
#else
		s_poly_scaledHOffset = mul16(heightOffset, s_rcfState.focalLenAspect);
#endif
		s_poly_sinYawHOffset = mul16(s_rcfState.sinYaw, heightOffset);
		s_poly_cosYawHOffset = mul16(s_rcfState.cosYaw, heightOffset);

		s_poly_cosYawScaledHOffset = mul16(s_rcfState.cosYaw, s_poly_scaledHOffset);
		s_poly_sinYawScaledHOffset = mul16(s_rcfState.sinYaw, s_poly_scaledHOffset);

		s_ftexWidthMask  = texture->width - 1;
		s_ftexHeightMask = texture->height - 1;
		s_ftexHeightLog2 = texture->logSizeY;
		s_ftexImage      = texture->image;
#ifndef __AMIGA__1
		s_ftexDataEnd    = texture->width * texture->height - 1;
#endif
	}

	void flat_drawPolygonScanline(s32 x0, s32 x1, s32 y, bool trans)
	{
		x0 = max(x0, s_windowMinX_Pixels);
		x1 = min(x1, s_windowMaxX_Pixels);
		clipScanline(&x0, &x1, y);

		s_scanlineWidth = x1 - x0 + 1;
		if (s_scanlineWidth <= 0) { return; }

		s_scanlineX0  = x0;
		s_scanlineOut = &s_display[y * s_width + x0];

		const s32 yShear = s_screenYMidBase - s_screenYMidFix;
		assert(yShear + y + s_height * 2 >= 0 && yShear + y + s_height * 2 <= s_height * 4);
		const fixed16_16 yRcp = s_rcfState.rcpY[yShear + y + s_height*2];
		const fixed16_16 z = mul16(s_poly_scaledHOffset, yRcp);
#ifdef __AMIGA__
		const s32 right = x1 - 1 - s_screenXMid;
#else		
		const fixed16_16 right = intToFixed16(x1 - 1 - s_screenXMid);
#endif

#ifdef __AMIGA__
		const fixed16_16 u0 = s_poly_sinYawScaledHOffset - (s_poly_cosYawHOffset * right);
		const fixed16_16 v0 = s_poly_cosYawScaledHOffset + (s_poly_sinYawHOffset * right);
#else
		const fixed16_16 u0 = s_poly_sinYawScaledHOffset - mul16(s_poly_cosYawHOffset, right);
		const fixed16_16 v0 = s_poly_cosYawScaledHOffset + mul16(s_poly_sinYawHOffset, right);
#endif
		s_scanlineU0 = (mul16(u0, yRcp) - s_poly_offsetX) * 8;
		s_scanlineV0 = (mul16(v0, yRcp) - s_poly_offsetZ) * 8;
		s_scanline_dVdX = -mul16(s_poly_sinYawHOffset, yRcp) * 8;
		s_scanline_dUdX =  mul16(s_poly_cosYawHOffset, yRcp) * 8;

		s_scanlineLight = computeLighting(z, 0);
		const s32 index = (!s_scanlineLight) + trans*2;
		c_scanlineDrawFunc[index]();
	}

}  // RFlatFixed

}  // TFE_Jedi