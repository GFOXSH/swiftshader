// SwiftShader Software Renderer
//
// Copyright(c) 2005-2012 TransGaming Inc.
//
// All rights reserved. No part of this software may be copied, distributed, transmitted,
// transcribed, stored in a retrieval system, translated into any human or computer
// language by any means, or disclosed to third parties without the explicit written
// agreement of TransGaming Inc. Without such an agreement, no rights or licenses, express
// or implied, including but not limited to any patent rights, are granted to you.
//

#include "Sampler.hpp"

#include "MetaMacro.hpp"
#include "Context.hpp"
#include "Surface.hpp"
#include "CPUID.hpp"
#include "PixelRoutine.hpp"
#include "Debug.hpp"

#include <memory.h>
#include <string.h>

namespace sw
{
	FilterType Sampler::maximumTextureFilterQuality = FILTER_LINEAR;
	MipmapType Sampler::maximumMipmapFilterQuality = MIPMAP_POINT;

	Sampler::State::State()
	{
		memset(this, 0, sizeof(State));
	}

	Sampler::Sampler()
	{
		// FIXME: Mipmap::init
		static unsigned int zero = 0x00FF00FF;

		for(int level = 0; level < MIPMAP_LEVELS; level++)
		{
			Mipmap &mipmap = texture.mipmap[level];

			memset(&mipmap, 0, sizeof(Mipmap));

			for(int face = 0; face < 6; face++)   
			{
				mipmap.buffer[face] = &zero;
			}

			mipmap.uFrac = 16;
			mipmap.vFrac = 16;
			mipmap.wFrac = 16;
		}

		externalTextureFormat = FORMAT_NULL;
		internalTextureFormat = FORMAT_NULL;
		textureType = TEXTURE_NULL;

		textureFilter = FILTER_LINEAR;
		addressingModeU = ADDRESSING_WRAP;
		addressingModeV = ADDRESSING_WRAP;
		addressingModeW = ADDRESSING_WRAP;
		mipmapFilterState = MIPMAP_NONE;
		sRGB = false;
		gather = false;

		texture.LOD = 0.0f;
		exp2LOD = 1.0f;
	}

	Sampler::~Sampler()
	{
	}

	Sampler::State Sampler::samplerState() const
	{
		State state;

		if(textureType != TEXTURE_NULL)
		{
			state.textureType = textureType;
			state.textureFormat = internalTextureFormat;
			state.textureFilter = getTextureFilter();
			state.addressingModeU = getAddressingModeU();
			state.addressingModeV = getAddressingModeV();
			state.addressingModeW = getAddressingModeW();
			state.mipmapFilter = mipmapFilter();
			state.hasNPOTTexture = hasNPOTTexture();
			state.sRGB = sRGB && Surface::isSRGBreadable(externalTextureFormat);

			#if PERF_PROFILE
				state.compressedFormat = Surface::isCompressed(externalTextureFormat);
			#endif
		}

		return state;
	}

	void Sampler::setTextureLevel(int face, int level, Surface *surface, TextureType type)
	{
		if(surface)
		{
			Mipmap &mipmap = texture.mipmap[level];

			mipmap.buffer[face] = surface->lockInternal(0, 0, 0, LOCK_UNLOCKED, PRIVATE);

			if(face == 0)
			{
				externalTextureFormat = surface->getExternalFormat();
				internalTextureFormat = surface->getInternalFormat();

				int width = surface->getWidth();
				int height = surface->getHeight();
				int depth = surface->getDepth();
				int pitchP = surface->getInternalPitchP();
				int sliceP = surface->getInternalSliceP();

				int logWidth = log2(width);
				int logHeight = log2(height);
				int logDepth = log2(depth);

				if(level == 0)
				{
					texture.widthHeightLOD[0] = width * exp2LOD;
					texture.widthHeightLOD[1] = width * exp2LOD;
					texture.widthHeightLOD[2] = height * exp2LOD;
					texture.widthHeightLOD[3] = height * exp2LOD;

					texture.widthLOD[0] = width * exp2LOD;
					texture.widthLOD[1] = width * exp2LOD;
					texture.widthLOD[2] = width * exp2LOD;
					texture.widthLOD[3] = width * exp2LOD;

					texture.heightLOD[0] = height * exp2LOD;
					texture.heightLOD[1] = height * exp2LOD;
					texture.heightLOD[2] = height * exp2LOD;
					texture.heightLOD[3] = height * exp2LOD;

					texture.depthLOD[0] = depth * exp2LOD;
					texture.depthLOD[1] = depth * exp2LOD;
					texture.depthLOD[2] = depth * exp2LOD;
					texture.depthLOD[3] = depth * exp2LOD;
				}

				if(!Surface::isFloatFormat(internalTextureFormat))
				{
					mipmap.uInt = logWidth;
					mipmap.vInt = logHeight;
					mipmap.wInt = logDepth;
					mipmap.uFrac = 16 - logWidth;
					mipmap.vFrac = 16 - logHeight;
					mipmap.wFrac = 16 - logDepth;
				}
				else
				{
					mipmap.fWidth[0] = (float)width / 65536.0f;
					mipmap.fWidth[1] = (float)width / 65536.0f;
					mipmap.fWidth[2] = (float)width / 65536.0f;
					mipmap.fWidth[3] = (float)width / 65536.0f;

					mipmap.fHeight[0] = (float)height / 65536.0f;
					mipmap.fHeight[1] = (float)height / 65536.0f;
					mipmap.fHeight[2] = (float)height / 65536.0f;
					mipmap.fHeight[3] = (float)height / 65536.0f;

					mipmap.fDepth[0] = (float)depth / 65536.0f;
					mipmap.fDepth[1] = (float)depth / 65536.0f;
					mipmap.fDepth[2] = (float)depth / 65536.0f;
					mipmap.fDepth[3] = (float)depth / 65536.0f;
				}

				short halfTexelU = isPow2(width)  ? 0x8000 >> logWidth  : 0x8000 / width;
				short halfTexelV = isPow2(height) ? 0x8000 >> logHeight : 0x8000 / height;
				short halfTexelW = isPow2(depth)  ? 0x8000 >> logDepth  : 0x8000 / depth;

				mipmap.uHalf[0] = halfTexelU;
				mipmap.uHalf[1] = halfTexelU;
				mipmap.uHalf[2] = halfTexelU;
				mipmap.uHalf[3] = halfTexelU;

				mipmap.vHalf[0] = halfTexelV;
				mipmap.vHalf[1] = halfTexelV;
				mipmap.vHalf[2] = halfTexelV;
				mipmap.vHalf[3] = halfTexelV;

				mipmap.wHalf[0] = halfTexelW;
				mipmap.wHalf[1] = halfTexelW;
				mipmap.wHalf[2] = halfTexelW;
				mipmap.wHalf[3] = halfTexelW;

				mipmap.width[0] = width;
				mipmap.width[1] = width;
				mipmap.width[2] = width;
				mipmap.width[3] = width;

				mipmap.height[0] = height;
				mipmap.height[1] = height;
				mipmap.height[2] = height;
				mipmap.height[3] = height;

				mipmap.depth[0] = depth;
				mipmap.depth[1] = depth;
				mipmap.depth[2] = depth;
				mipmap.depth[3] = depth;

				mipmap.onePitchP[0] = 1;
				mipmap.onePitchP[1] = pitchP;
				mipmap.onePitchP[2] = 1;
				mipmap.onePitchP[3] = pitchP;

				mipmap.sliceP[0] = sliceP;
				mipmap.sliceP[1] = sliceP;
			}
		}

		textureType = type;
	}

	void Sampler::setTextureFilter(FilterType textureFilter)
	{
		this->textureFilter = (FilterType)min(textureFilter, maximumTextureFilterQuality);
	}

	void Sampler::setMipmapFilter(MipmapType mipmapFilter)
	{
		mipmapFilterState = (MipmapType)min(mipmapFilter, maximumMipmapFilterQuality);
	}

	void Sampler::setGatherEnable(bool enable)
	{
		gather = enable;
	}

	void Sampler::setAddressingModeU(AddressingMode addressingMode)
	{
		addressingModeU = addressingMode;
	}

	void Sampler::setAddressingModeV(AddressingMode addressingMode)
	{
		addressingModeV = addressingMode;
	}

	void Sampler::setAddressingModeW(AddressingMode addressingMode)
	{
		addressingModeW = addressingMode;
	}

	void Sampler::setReadSRGB(bool sRGB)
	{
		this->sRGB = sRGB;
	}

	void Sampler::setBorderColor(const Color<float> &borderColor)
	{
		// FIXME: Compact into generic function   // FIXME: Clamp
		short r = iround(0xFFFF * borderColor.r);
		short g = iround(0xFFFF * borderColor.g);
		short b = iround(0xFFFF * borderColor.b);
		short a = iround(0xFFFF * borderColor.a);

		texture.borderColor4[0][0] = texture.borderColor4[0][1] = texture.borderColor4[0][2] = texture.borderColor4[0][3] = r;
		texture.borderColor4[1][0] = texture.borderColor4[1][1] = texture.borderColor4[1][2] = texture.borderColor4[1][3] = g;
		texture.borderColor4[2][0] = texture.borderColor4[2][1] = texture.borderColor4[2][2] = texture.borderColor4[2][3] = b;
		texture.borderColor4[3][0] = texture.borderColor4[3][1] = texture.borderColor4[3][2] = texture.borderColor4[3][3] = a;

		texture.borderColorF[0][0] = texture.borderColorF[0][1] = texture.borderColorF[0][2] = texture.borderColorF[0][3] = borderColor.r;
		texture.borderColorF[1][0] = texture.borderColorF[1][1] = texture.borderColorF[1][2] = texture.borderColorF[1][3] = borderColor.g;
		texture.borderColorF[2][0] = texture.borderColorF[2][1] = texture.borderColorF[2][2] = texture.borderColorF[2][3] = borderColor.b;
		texture.borderColorF[3][0] = texture.borderColorF[3][1] = texture.borderColorF[3][2] = texture.borderColorF[3][3] = borderColor.a;
	}

	void Sampler::setMaxAnisotropy(float maxAnisotropy)
	{
		texture.maxAnisotropy = maxAnisotropy;
	}

	void Sampler::setFilterQuality(FilterType maximumFilterQuality)
	{
		Sampler::maximumTextureFilterQuality = maximumFilterQuality;
	}

	void Sampler::setMipmapQuality(MipmapType maximumFilterQuality)
	{
		Sampler::maximumMipmapFilterQuality = maximumFilterQuality;
	}

	void Sampler::setMipmapLOD(float LOD)
	{
		texture.LOD = LOD;
		exp2LOD = exp2(LOD);
	}

	bool Sampler::hasTexture() const
	{
		return textureType != TEXTURE_NULL;
	}

	bool Sampler::hasUnsignedTexture() const
	{
		return Surface::isUnsignedComponent(internalTextureFormat, 0) &&
		       Surface::isUnsignedComponent(internalTextureFormat, 1) &&
			   Surface::isUnsignedComponent(internalTextureFormat, 2) &&
			   Surface::isUnsignedComponent(internalTextureFormat, 3);
	}

	bool Sampler::hasCubeTexture() const
	{
		return textureType == TEXTURE_CUBE;
	}

	bool Sampler::hasVolumeTexture() const
	{
		return textureType == TEXTURE_3D;
	}

	const Texture &Sampler::getTextureData()
	{
		return texture;
	}

	MipmapType Sampler::mipmapFilter() const
	{
		if(mipmapFilterState != MIPMAP_NONE)
		{
			for(int i = 1; i < MIPMAP_LEVELS; i++)
			{
				if(texture.mipmap[0].buffer[0] != texture.mipmap[i].buffer[0])
				{
					return mipmapFilterState;
				}
			}
		}

		// Only one mipmap level
		return MIPMAP_NONE;
	}

	bool Sampler::hasNPOTTexture() const
	{
		if(textureType == TEXTURE_NULL)
		{
			return false;
		}

		if(texture.mipmap[0].width[0] != texture.mipmap[0].onePitchP[1])
		{
			return true;   // Shifting of the texture coordinates doesn't yield the correct address, so using multiply by pitch
		}

		return !isPow2(texture.mipmap[0].width[0]) || !isPow2(texture.mipmap[0].height[0]) || !isPow2(texture.mipmap[0].depth[0]);
	}

	TextureType Sampler::getTextureType() const
	{
		return textureType;
	}

	FilterType Sampler::getTextureFilter() const
	{
		FilterType filter = textureFilter;

		if(gather && Surface::componentCount(internalTextureFormat) == 1)
		{
			filter = FILTER_GATHER;
		}

		if(textureType != TEXTURE_2D || texture.maxAnisotropy == 1.0f)
		{
			return (FilterType)min(filter, FILTER_LINEAR);
		}

		return filter;
	}

	AddressingMode Sampler::getAddressingModeU() const
	{
		if(hasCubeTexture())
		{
			return ADDRESSING_CLAMP;
		}

		return addressingModeU;
	}

	AddressingMode Sampler::getAddressingModeV() const
	{
		if(hasCubeTexture())
		{
			return ADDRESSING_CLAMP;
		}

		return addressingModeV;
	}

	AddressingMode Sampler::getAddressingModeW() const
	{
		if(hasCubeTexture())
		{
			return ADDRESSING_CLAMP;
		}

		return addressingModeW;
	}
}
