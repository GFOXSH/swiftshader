// SwiftShader Software Renderer
//
// Copyright(c) 2005-2011 TransGaming Inc.
//
// All rights reserved. No part of this software may be copied, distributed, transmitted,
// transcribed, stored in a retrieval system, translated into any human or computer
// language by any means, or disclosed to third parties without the explicit written
// agreement of TransGaming Inc. Without such an agreement, no rights or licenses, express
// or implied, including but not limited to any patent rights, are granted to you.
//

#ifndef sw_Sampler_hpp
#define sw_Sampler_hpp

#include "Main/Config.hpp"
#include "Renderer/Surface.hpp"

namespace sw
{
	struct Mipmap
	{
		void *buffer[6];

		union
		{
			struct
			{
				int64_t uInt;
				int64_t vInt;
				int64_t wInt;
				int64_t uFrac;
				int64_t vFrac;
				int64_t wFrac;
			};

			struct
			{
				float4 fWidth;
				float4 fHeight;
				float4 fDepth;
			};
		};

		short uHalf[4];
		short vHalf[4];
		short wHalf[4];
		short width[4];
		short height[4];
		short depth[4];
		short onePitchP[4];
		int sliceP[2];
	};

	struct Texture
	{
		Mipmap mipmap[MIPMAP_LEVELS];

		float LOD;
		float4 widthHeightLOD;
		float4 widthLOD;
		float4 heightLOD;
		float4 depthLOD;

		word4 borderColor4[4];
		float4 borderColorF[4];
		float maxAnisotropy;
	};

	enum SamplerType
	{
		SAMPLER_PIXEL,
		SAMPLER_VERTEX
	};

	enum TextureType : unsigned int
	{
		TEXTURE_NULL,
		TEXTURE_2D,
		TEXTURE_CUBE,
		TEXTURE_3D,
		TEXTURE_2D_ARRAY,

		TEXTURE_LAST = TEXTURE_2D_ARRAY
	};

	enum FilterType : unsigned int
	{
		FILTER_POINT,
		FILTER_GATHER,
		FILTER_LINEAR,
		FILTER_ANISOTROPIC,

		FILTER_LAST = FILTER_ANISOTROPIC
	};

	enum MipmapType : unsigned int
	{
		MIPMAP_NONE,
		MIPMAP_POINT,
		MIPMAP_LINEAR,
		
		MIPMAP_LAST = MIPMAP_LINEAR
	};

	enum AddressingMode : unsigned int
	{
		ADDRESSING_WRAP,
		ADDRESSING_CLAMP,
		ADDRESSING_MIRROR,
		ADDRESSING_MIRRORONCE,
		ADDRESSING_BORDER,

		ADDRESSING_LAST = ADDRESSING_BORDER
	};

	class Sampler
	{
	public:
		struct State
		{
			State();

			TextureType textureType        : BITS(TEXTURE_LAST);
			Format textureFormat           : BITS(FORMAT_LAST);
			FilterType textureFilter       : BITS(FILTER_LAST);
			AddressingMode addressingModeU : BITS(ADDRESSING_LAST);
			AddressingMode addressingModeV : BITS(ADDRESSING_LAST);
			AddressingMode addressingModeW : BITS(ADDRESSING_LAST);
			MipmapType mipmapFilter        : BITS(FILTER_LAST);
			bool hasNPOTTexture	           : 1;
			bool sRGB                      : 1;

			#if PERF_PROFILE
			bool compressedFormat          : 1;
			#endif
		};

		Sampler();

		~Sampler();

		State samplerState() const;

		void setTextureLevel(int face, int level, Surface *surface, TextureType type);

		void setTextureFilter(FilterType textureFilter);
		void setMipmapFilter(MipmapType mipmapFilter);
		void setGatherEnable(bool enable);
		void setAddressingModeU(AddressingMode addressingMode);
		void setAddressingModeV(AddressingMode addressingMode);
		void setAddressingModeW(AddressingMode addressingMode);
		void setReadSRGB(bool sRGB);
		void setBorderColor(const Color<float> &borderColor);
		void setMaxAnisotropy(float maxAnisotropy);

		static void setFilterQuality(FilterType maximumFilterQuality);
		static void setMipmapQuality(MipmapType maximumFilterQuality);
		void setMipmapLOD(float lod);

		bool hasTexture() const;
		bool hasUnsignedTexture() const;
		bool hasCubeTexture() const;
		bool hasVolumeTexture() const;

		const Texture &getTextureData();

	private:
		MipmapType mipmapFilter() const;
		bool hasNPOTTexture() const;
		TextureType getTextureType() const;
		FilterType getTextureFilter() const;
		AddressingMode getAddressingModeU() const;
		AddressingMode getAddressingModeV() const;
		AddressingMode getAddressingModeW() const;

		Format externalTextureFormat;
		Format internalTextureFormat;
		TextureType textureType;

		FilterType textureFilter;
		AddressingMode addressingModeU;
		AddressingMode addressingModeV;
		AddressingMode addressingModeW;
		MipmapType mipmapFilterState;
		bool sRGB;
		bool gather;

		Texture texture;
		float exp2LOD;

		static FilterType maximumTextureFilterQuality;
		static MipmapType maximumMipmapFilterQuality;
	};
}

#endif   // sw_Sampler_hpp
