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

#ifndef sw_SamplerCore_hpp
#define sw_SamplerCore_hpp

#include "PixelRoutine.hpp"
#include "Reactor/Nucleus.hpp"

namespace sw
{
	class SamplerCore
	{
	public:
		SamplerCore(Pointer<Byte> &r, const Sampler::State &state);

		void sampleTexture(Pointer<Byte> &texture, Vector4s &c, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Vector4f &dsx, Vector4f &dsy, bool bias = false, bool gradients = false, bool lodProvided = false, bool fixed12 = true);
		void sampleTexture(Pointer<Byte> &texture, Vector4f &c, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Vector4f &dsx, Vector4f &dsy, bool bias = false, bool gradients = false, bool lodProvided = false);

	private:
		void border(Short4 &mask, Float4 &coordinates);
		void border(Int4 &mask, Float4 &coordinates);
		Short4 offsetSample(Short4 &uvw, Pointer<Byte> &mipmap, int halfOffset, bool wrap, int count);
		void sampleFilter(Pointer<Byte> &texture, Vector4s &c, Float4 &u, Float4 &v, Float4 &w, Float &lod, Float &anisotropy, Float4 &uDelta, Float4 &vDelta, Int face[4], bool lodProvided);
		void sampleAniso(Pointer<Byte> &texture, Vector4s &c, Float4 &u, Float4 &v, Float4 &w, Float &lod, Float &anisotropy, Float4 &uDelta, Float4 &vDelta, Int face[4], bool secondLOD, bool lodProvided);
		void sampleQuad(Pointer<Byte> &texture, Vector4s &c, Float4 &u, Float4 &v, Float4 &w, Float &lod, Int face[4], bool secondLOD);
		void sampleQuad2D(Pointer<Byte> &texture, Vector4s &c, Float4 &u, Float4 &v, Float &lod, Int face[4], bool secondLOD);
		void sample3D(Pointer<Byte> &texture, Vector4s &c, Float4 &u, Float4 &v, Float4 &w, Float &lod, bool secondLOD);
		void sampleFloatFilter(Pointer<Byte> &texture, Vector4f &c, Float4 &u, Float4 &v, Float4 &w, Float &lod, Float &anisotropy, Float4 &uDelta, Float4 &vDelta, Int face[4], bool lodProvided);
		void sampleFloatAniso(Pointer<Byte> &texture, Vector4f &c, Float4 &u, Float4 &v, Float4 &w, Float &lod, Float &anisotropy, Float4 &uDelta, Float4 &vDelta, Int face[4], bool secondLOD, bool lodProvided);
		void sampleFloat(Pointer<Byte> &texture, Vector4f &c, Float4 &u, Float4 &v, Float4 &w, Float &lod, Int face[4], bool secondLOD);
		void sampleFloat2D(Pointer<Byte> &texture, Vector4f &c, Float4 &u, Float4 &v, Float4 &z, Float &lod, Int face[4], bool secondLOD);
		void sampleFloat3D(Pointer<Byte> &texture, Vector4f &c, Float4 &u, Float4 &v, Float4 &w, Float &lod, bool secondLOD);
		void computeLod(Pointer<Byte> &texture, Float &lod, Float &anisotropy, Float4 &uDelta, Float4 &vDelta, Float4 &u, Float4 &v, const Float &lodBias, Vector4f &dsx, Vector4f &dsy, bool bias, bool gradients, bool lodProvided);
		void computeLod3D(Pointer<Byte> &texture, Float &lod, Float4 &u, Float4 &v, Float4 &w, const Float &lodBias, Vector4f &dsx, Vector4f &dsy, bool bias, bool gradients, bool lodProvided);
		void cubeFace(Int face[4], Float4 &U, Float4 &V, Float4 &lodU, Float4 &lodV, Float4 &x, Float4 &y, Float4 &z);
		void computeIndices(Int index[4], Short4 uuuu, Short4 vvvv, Short4 wwww, const Pointer<Byte> &mipmap);
		void sampleTexel(Vector4s &c, Short4 &u, Short4 &v, Short4 &s, Pointer<Byte> &mipmap, Pointer<Byte> buffer[4]);
		void sampleTexel(Vector4f &c, Short4 &u, Short4 &v, Short4 &s, Float4 &z, Pointer<Byte> &mipmap, Pointer<Byte> buffer[4]);
		void selectMipmap(Pointer<Byte> &texture, Pointer<Byte> buffer[4], Pointer<Byte> &mipmap, Float &lod, Int face[4], bool secondLOD);
		void address(Short4 &uuuu, Float4 &uw, AddressingMode addressingMode);

		void convertFixed12(Short4 &ci, Float4 &cf);
		void convertFixed12(Vector4s &cs, Vector4f &cf);
		void convertSigned12(Float4 &cf, Short4 &ci);
		void convertSigned15(Float4 &cf, Short4 &ci);
		void convertUnsigned16(Float4 &cf, Short4 &ci);
		void sRGBtoLinear16_8_12(Short4 &c);
		void sRGBtoLinear16_6_12(Short4 &c);
		void sRGBtoLinear16_5_12(Short4 &c);

		bool hasFloatTexture() const;
		bool hasUnsignedTextureComponent(int component) const;
		int textureComponentCount() const;
		bool has16bitTextureFormat() const;
		bool has16bitTextureComponents() const;
		bool isRGBComponent(int component) const;

		Pointer<Byte> &constants;
		const Sampler::State &state;
	};
}

#endif   // sw_SamplerCore_hpp
