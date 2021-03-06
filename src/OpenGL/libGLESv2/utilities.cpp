// SwiftShader Software Renderer
//
// Copyright(c) 2005-2013 TransGaming Inc.
//
// All rights reserved. No part of this software may be copied, distributed, transmitted,
// transcribed, stored in a retrieval system, translated into any human or computer
// language by any means, or disclosed to third parties without the explicit written
// agreement of TransGaming Inc. Without such an agreement, no rights or licenses, express
// or implied, including but not limited to any patent rights, are granted to you.
//

// utilities.cpp: Conversion functions and other utility routines.

#include "utilities.h"

#include "mathutil.h"
#include "Context.h"
#include "common/debug.h"

#include <limits>
#include <stdio.h>

namespace es2
{
	unsigned int UniformComponentCount(GLenum type)
	{
		switch(type)
		{
		case GL_BOOL:
		case GL_FLOAT:
		case GL_INT:
		case GL_UNSIGNED_INT:
		case GL_SAMPLER_2D:
		case GL_SAMPLER_CUBE:
        case GL_SAMPLER_EXTERNAL_OES:
		case GL_SAMPLER_3D_OES:
		case GL_SAMPLER_2D_ARRAY:
		case GL_SAMPLER_2D_SHADOW:
		case GL_SAMPLER_CUBE_SHADOW:
		case GL_SAMPLER_2D_ARRAY_SHADOW:
		case GL_INT_SAMPLER_2D:
		case GL_UNSIGNED_INT_SAMPLER_2D:
		case GL_INT_SAMPLER_CUBE:
		case GL_UNSIGNED_INT_SAMPLER_CUBE:
		case GL_INT_SAMPLER_3D:
		case GL_UNSIGNED_INT_SAMPLER_3D:
		case GL_INT_SAMPLER_2D_ARRAY:
		case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
			return 1;
		case GL_BOOL_VEC2:
		case GL_FLOAT_VEC2:
		case GL_INT_VEC2:
		case GL_UNSIGNED_INT_VEC2:
			return 2;
		case GL_INT_VEC3:
		case GL_UNSIGNED_INT_VEC3:
		case GL_FLOAT_VEC3:
		case GL_BOOL_VEC3:
			return 3;
		case GL_BOOL_VEC4:
		case GL_FLOAT_VEC4:
		case GL_INT_VEC4:
		case GL_UNSIGNED_INT_VEC4:
		case GL_FLOAT_MAT2:
			return 4;
		case GL_FLOAT_MAT2x3:
		case GL_FLOAT_MAT3x2:
			return 6;
		case GL_FLOAT_MAT2x4:
		case GL_FLOAT_MAT4x2:
			return 8;
		case GL_FLOAT_MAT3:
			return 9;
		case GL_FLOAT_MAT3x4:
		case GL_FLOAT_MAT4x3:
			return 12;
		case GL_FLOAT_MAT4:
			return 16;
		default:
			UNREACHABLE(type);
		}

		return 0;
	}

	GLenum UniformComponentType(GLenum type)
	{
		switch(type)
		{
		case GL_BOOL:
		case GL_BOOL_VEC2:
		case GL_BOOL_VEC3:
		case GL_BOOL_VEC4:
			return GL_BOOL;
		case GL_FLOAT:
		case GL_FLOAT_VEC2:
		case GL_FLOAT_VEC3:
		case GL_FLOAT_VEC4:
		case GL_FLOAT_MAT2:
		case GL_FLOAT_MAT2x3:
		case GL_FLOAT_MAT2x4:
		case GL_FLOAT_MAT3:
		case GL_FLOAT_MAT3x2:
		case GL_FLOAT_MAT3x4:
		case GL_FLOAT_MAT4:
		case GL_FLOAT_MAT4x2:
		case GL_FLOAT_MAT4x3:
			return GL_FLOAT;
		case GL_INT:
		case GL_SAMPLER_2D:
		case GL_SAMPLER_CUBE:
		case GL_SAMPLER_EXTERNAL_OES:
		case GL_SAMPLER_3D_OES:
		case GL_SAMPLER_2D_ARRAY:
		case GL_SAMPLER_2D_SHADOW:
		case GL_SAMPLER_CUBE_SHADOW:
		case GL_SAMPLER_2D_ARRAY_SHADOW:
		case GL_INT_SAMPLER_2D:
		case GL_UNSIGNED_INT_SAMPLER_2D:
		case GL_INT_SAMPLER_CUBE:
		case GL_UNSIGNED_INT_SAMPLER_CUBE:
		case GL_INT_SAMPLER_3D:
		case GL_UNSIGNED_INT_SAMPLER_3D:
		case GL_INT_SAMPLER_2D_ARRAY:
		case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
		case GL_INT_VEC2:
		case GL_INT_VEC3:
		case GL_INT_VEC4:
			return GL_INT;
		case GL_UNSIGNED_INT:
		case GL_UNSIGNED_INT_VEC2:
		case GL_UNSIGNED_INT_VEC3:
		case GL_UNSIGNED_INT_VEC4:
			return GL_UNSIGNED_INT;
		default:
			UNREACHABLE(type);
		}

		return GL_NONE;
	}

	size_t UniformTypeSize(GLenum type)
	{
		switch(type)
		{
		case GL_BOOL:  return sizeof(GLboolean);
		case GL_FLOAT: return sizeof(GLfloat);
		case GL_INT:   return sizeof(GLint);
		case GL_UNSIGNED_INT: return sizeof(GLuint);
		}

		return UniformTypeSize(UniformComponentType(type)) * UniformComponentCount(type);
	}

	bool IsSamplerUniform(GLenum type)
	{
		switch(type)
		{
		case GL_SAMPLER_2D:
		case GL_SAMPLER_CUBE:
		case GL_SAMPLER_EXTERNAL_OES:
		case GL_SAMPLER_3D_OES:
		case GL_SAMPLER_2D_ARRAY:
		case GL_SAMPLER_2D_SHADOW:
		case GL_SAMPLER_CUBE_SHADOW:
		case GL_SAMPLER_2D_ARRAY_SHADOW:
		case GL_INT_SAMPLER_2D:
		case GL_UNSIGNED_INT_SAMPLER_2D:
		case GL_INT_SAMPLER_CUBE:
		case GL_UNSIGNED_INT_SAMPLER_CUBE:
		case GL_INT_SAMPLER_3D:
		case GL_UNSIGNED_INT_SAMPLER_3D:
		case GL_INT_SAMPLER_2D_ARRAY:
		case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
			return true;
		default:
			return false;
		}
	}

	int VariableRowCount(GLenum type)
	{
		switch(type)
		{
		case GL_NONE:
			return 0;
		case GL_BOOL:
		case GL_FLOAT:
		case GL_INT:
		case GL_UNSIGNED_INT:
		case GL_BOOL_VEC2:
		case GL_FLOAT_VEC2:
		case GL_INT_VEC2:
		case GL_UNSIGNED_INT_VEC2:
		case GL_INT_VEC3:
		case GL_UNSIGNED_INT_VEC3:
		case GL_FLOAT_VEC3:
		case GL_BOOL_VEC3:
		case GL_BOOL_VEC4:
		case GL_FLOAT_VEC4:
		case GL_INT_VEC4:
		case GL_UNSIGNED_INT_VEC4:
		case GL_SAMPLER_2D:
		case GL_SAMPLER_CUBE:
        case GL_SAMPLER_EXTERNAL_OES:
		case GL_SAMPLER_3D_OES:
		case GL_SAMPLER_2D_ARRAY:
		case GL_SAMPLER_2D_SHADOW:
		case GL_SAMPLER_CUBE_SHADOW:
		case GL_SAMPLER_2D_ARRAY_SHADOW:
		case GL_INT_SAMPLER_2D:
		case GL_UNSIGNED_INT_SAMPLER_2D:
		case GL_INT_SAMPLER_CUBE:
		case GL_UNSIGNED_INT_SAMPLER_CUBE:
		case GL_INT_SAMPLER_3D:
		case GL_UNSIGNED_INT_SAMPLER_3D:
		case GL_INT_SAMPLER_2D_ARRAY:
		case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
			return 1;
		case GL_FLOAT_MAT2:
		case GL_FLOAT_MAT3x2:
		case GL_FLOAT_MAT4x2:
			return 2;
		case GL_FLOAT_MAT3:
		case GL_FLOAT_MAT2x3:
		case GL_FLOAT_MAT4x3:
			return 3;
		case GL_FLOAT_MAT4:
		case GL_FLOAT_MAT2x4:
		case GL_FLOAT_MAT3x4:
			return 4;
		default:
			UNREACHABLE(type);
		}

		return 0;
	}

	int VariableColumnCount(GLenum type)
	{
		switch(type)
		{
		case GL_NONE:
			return 0;
		case GL_BOOL:
		case GL_FLOAT:
		case GL_INT:
		case GL_UNSIGNED_INT:
			return 1;
		case GL_BOOL_VEC2:
		case GL_FLOAT_VEC2:
		case GL_INT_VEC2:
		case GL_UNSIGNED_INT_VEC2:
		case GL_FLOAT_MAT2:
		case GL_FLOAT_MAT2x3:
		case GL_FLOAT_MAT2x4:
			return 2;
		case GL_INT_VEC3:
		case GL_UNSIGNED_INT_VEC3:
		case GL_FLOAT_VEC3:
		case GL_BOOL_VEC3:
		case GL_FLOAT_MAT3:
		case GL_FLOAT_MAT3x2:
		case GL_FLOAT_MAT3x4:
			return 3;
		case GL_BOOL_VEC4:
		case GL_FLOAT_VEC4:
		case GL_INT_VEC4:
		case GL_UNSIGNED_INT_VEC4:
		case GL_FLOAT_MAT4:
		case GL_FLOAT_MAT4x2:
		case GL_FLOAT_MAT4x3:
			return 4;
		default:
			UNREACHABLE(type);
		}

		return 0;
	}

	int VariableRegisterCount(GLenum type)
	{
		// Number of registers used is the number of columns for matrices or 1 for scalars and vectors
		return (VariableRowCount(type) > 1) ? VariableColumnCount(type) : 1;
	}

	int VariableRegisterSize(GLenum type)
	{
		// Number of components per register is the number of rows for matrices or columns for scalars and vectors
		int nbRows = VariableRowCount(type);
		return (nbRows > 1) ? nbRows : VariableColumnCount(type);
	}

	int AllocateFirstFreeBits(unsigned int *bits, unsigned int allocationSize, unsigned int bitsSize)
	{
		ASSERT(allocationSize <= bitsSize);

		unsigned int mask = std::numeric_limits<unsigned int>::max() >> (std::numeric_limits<unsigned int>::digits - allocationSize);

		for(unsigned int i = 0; i < bitsSize - allocationSize + 1; i++)
		{
			if((*bits & mask) == 0)
			{
				*bits |= mask;
				return i;
			}

			mask <<= 1;
		}

		return -1;
	}

	GLint floatToInt(GLfloat value)
	{
		return static_cast<GLint>((static_cast<GLfloat>(0xFFFFFFFF) * value - 1.0f) * 0.5f);
	}

	bool IsCompressed(GLenum format)
	{
		return format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
		       format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
               format == GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE ||
               format == GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE ||
               format == GL_ETC1_RGB8_OES ||
               format == GL_COMPRESSED_R11_EAC ||
               format == GL_COMPRESSED_SIGNED_R11_EAC ||
               format == GL_COMPRESSED_RG11_EAC ||
               format == GL_COMPRESSED_SIGNED_RG11_EAC ||
               format == GL_COMPRESSED_RGB8_ETC2 ||
               format == GL_COMPRESSED_SRGB8_ETC2 ||
               format == GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 ||
               format == GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 ||
               format == GL_COMPRESSED_RGBA8_ETC2_EAC ||
               format == GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC;
	}

	bool IsDepthTexture(GLenum format)
	{
		return format == GL_DEPTH_COMPONENT ||
		       format == GL_DEPTH_STENCIL_OES;
	}

	bool IsStencilTexture(GLenum format)
	{
		return format == GL_STENCIL_INDEX_OES ||
		       format == GL_DEPTH_STENCIL_OES;
	}

	bool IsCubemapTextureTarget(GLenum target)
	{
		return (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
	}

	int CubeFaceIndex(GLenum cubeFace)
	{
		switch(cubeFace)
		{
		case GL_TEXTURE_CUBE_MAP:
		case GL_TEXTURE_CUBE_MAP_POSITIVE_X: return 0;
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_X: return 1;
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Y: return 2;
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: return 3;
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Z: return 4;
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: return 5;
		default: UNREACHABLE(cubeFace); return 0;
		}
	}

	bool IsTextureTarget(GLenum target)
	{
		return target == GL_TEXTURE_2D || IsCubemapTextureTarget(target) || target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY;
	}

	// Verify that format/type are one of the combinations from table 3.4.
	bool CheckTextureFormatType(GLenum format, GLenum type, egl::GLint clientVersion)
	{
		switch(type)
		{
		case GL_UNSIGNED_BYTE:
			switch(format)
			{
			case GL_RGBA:
			case GL_BGRA_EXT:
			case GL_RGB:
			case GL_ALPHA:
			case GL_LUMINANCE:
			case GL_LUMINANCE_ALPHA:
				return true;
			default:
				return false;
			}
		case GL_HALF_FLOAT:
			if(clientVersion < 3)
			{
				return false;
			}
		case GL_FLOAT:
		case GL_HALF_FLOAT_OES:
			switch(format)
			{
			case GL_RGBA:
			case GL_RGB:
			case GL_ALPHA:
			case GL_LUMINANCE:
			case GL_LUMINANCE_ALPHA:
				return true;
			default:
				return false;
			}
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_5_5_5_1:
			return (format == GL_RGBA);
		case GL_UNSIGNED_SHORT_5_6_5:
			return (format == GL_RGB);
		case GL_UNSIGNED_INT:
			return (format == GL_DEPTH_COMPONENT);
		case GL_UNSIGNED_INT_24_8_OES:
			return (format == GL_DEPTH_STENCIL_OES);
		default:
			return false;
		}
	}

	bool IsColorRenderable(GLenum internalformat)
	{
		switch(internalformat)
		{
		case GL_R8:
		case GL_R8UI:
		case GL_R8I:
		case GL_R16UI:
		case GL_R16I:
		case GL_R32UI:
		case GL_R32I:
		case GL_RG8:
		case GL_RG8UI:
		case GL_RG8I:
		case GL_RG16UI:
		case GL_RG16I:
		case GL_RG32UI:
		case GL_RG32I:
		case GL_SRGB8_ALPHA8:
		case GL_RGB10_A2:
		case GL_RGBA8UI:
		case GL_RGBA8I:
		case GL_RGB10_A2UI:
		case GL_RGBA16UI:
		case GL_RGBA16I:
		case GL_RGBA32I:
		case GL_RGBA32UI:
		case GL_RGBA4:
		case GL_RGB5_A1:
		case GL_RGB565:
		case GL_RGB8_OES:
		case GL_RGBA8_OES:
			return true;
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32F:
		case GL_DEPTH32F_STENCIL8:
		case GL_DEPTH_COMPONENT16:
		case GL_STENCIL_INDEX8:
		case GL_DEPTH24_STENCIL8_OES:
			return false;
		default:
			UNIMPLEMENTED();
		}

		return false;
	}

	bool IsDepthRenderable(GLenum internalformat)
	{
		switch(internalformat)
		{
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32F:
		case GL_DEPTH32F_STENCIL8:
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH24_STENCIL8_OES:
			return true;
		case GL_STENCIL_INDEX8:
		case GL_R8:
		case GL_R8UI:
		case GL_R8I:
		case GL_R16UI:
		case GL_R16I:
		case GL_R32UI:
		case GL_R32I:
		case GL_RG8:
		case GL_RG8UI:
		case GL_RG8I:
		case GL_RG16UI:
		case GL_RG16I:
		case GL_RG32UI:
		case GL_RG32I:
		case GL_SRGB8_ALPHA8:
		case GL_RGB10_A2:
		case GL_RGBA8UI:
		case GL_RGBA8I:
		case GL_RGB10_A2UI:
		case GL_RGBA16UI:
		case GL_RGBA16I:
		case GL_RGBA32I:
		case GL_RGBA32UI:
		case GL_RGBA4:
		case GL_RGB5_A1:
		case GL_RGB565:
		case GL_RGB8_OES:
		case GL_RGBA8_OES:
			return false;
		default:
			UNIMPLEMENTED();
		}

		return false;
	}

	bool IsStencilRenderable(GLenum internalformat)
	{
		switch(internalformat)
		{
		case GL_DEPTH32F_STENCIL8:
		case GL_STENCIL_INDEX8:
		case GL_DEPTH24_STENCIL8_OES:
			return true;
		case GL_R8:
		case GL_R8UI:
		case GL_R8I:
		case GL_R16UI:
		case GL_R16I:
		case GL_R32UI:
		case GL_R32I:
		case GL_RG8:
		case GL_RG8UI:
		case GL_RG8I:
		case GL_RG16UI:
		case GL_RG16I:
		case GL_RG32UI:
		case GL_RG32I:
		case GL_SRGB8_ALPHA8:
		case GL_RGB10_A2:
		case GL_RGBA8UI:
		case GL_RGBA8I:
		case GL_RGB10_A2UI:
		case GL_RGBA16UI:
		case GL_RGBA16I:
		case GL_RGBA32I:
		case GL_RGBA32UI:
		case GL_RGBA4:
		case GL_RGB5_A1:
		case GL_RGB565:
		case GL_RGB8_OES:
		case GL_RGBA8_OES:
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32F:
			return false;
		default:
			UNIMPLEMENTED();
		}

		return false;
	}

	std::string ParseUniformName(const std::string &name, size_t *outSubscript)
	{
		// Strip any trailing array operator and retrieve the subscript
		size_t open = name.find_last_of('[');
		size_t close = name.find_last_of(']');
		bool hasIndex = (open != std::string::npos) && (close == name.length() - 1);
		if(!hasIndex)
		{
			if(outSubscript)
			{
				*outSubscript = GL_INVALID_INDEX;
			}
			return name;
		}

		if(outSubscript)
		{
			int index = atoi(name.substr(open + 1).c_str());
			if(index >= 0)
			{
				*outSubscript = index;
			}
			else
			{
				*outSubscript = GL_INVALID_INDEX;
			}
		}

		return name.substr(0, open);
	}
}

namespace es2sw
{
	sw::DepthCompareMode ConvertDepthComparison(GLenum comparison)
	{
		switch(comparison)
		{
		case GL_NEVER:    return sw::DEPTH_NEVER;
		case GL_ALWAYS:   return sw::DEPTH_ALWAYS;
		case GL_LESS:     return sw::DEPTH_LESS;
		case GL_LEQUAL:   return sw::DEPTH_LESSEQUAL;
		case GL_EQUAL:    return sw::DEPTH_EQUAL;
		case GL_GREATER:  return sw::DEPTH_GREATER;
		case GL_GEQUAL:   return sw::DEPTH_GREATEREQUAL;
		case GL_NOTEQUAL: return sw::DEPTH_NOTEQUAL;
		default: UNREACHABLE(comparison);
		}

		return sw::DEPTH_ALWAYS;
	}

	sw::StencilCompareMode ConvertStencilComparison(GLenum comparison)
	{
		switch(comparison)
		{
		case GL_NEVER:    return sw::STENCIL_NEVER;
		case GL_ALWAYS:   return sw::STENCIL_ALWAYS;
		case GL_LESS:     return sw::STENCIL_LESS;
		case GL_LEQUAL:   return sw::STENCIL_LESSEQUAL;
		case GL_EQUAL:    return sw::STENCIL_EQUAL;
		case GL_GREATER:  return sw::STENCIL_GREATER;
		case GL_GEQUAL:   return sw::STENCIL_GREATEREQUAL;
		case GL_NOTEQUAL: return sw::STENCIL_NOTEQUAL;
		default: UNREACHABLE(comparison);
		}

		return sw::STENCIL_ALWAYS;
	}

	sw::Color<float> ConvertColor(es2::Color color)
	{
		return sw::Color<float>(color.red, color.green, color.blue, color.alpha);
	}

	sw::BlendFactor ConvertBlendFunc(GLenum blend)
	{
		switch(blend)
		{
		case GL_ZERO:                     return sw::BLEND_ZERO;
		case GL_ONE:                      return sw::BLEND_ONE;
		case GL_SRC_COLOR:                return sw::BLEND_SOURCE;
		case GL_ONE_MINUS_SRC_COLOR:      return sw::BLEND_INVSOURCE;
		case GL_DST_COLOR:                return sw::BLEND_DEST;
		case GL_ONE_MINUS_DST_COLOR:      return sw::BLEND_INVDEST;
		case GL_SRC_ALPHA:                return sw::BLEND_SOURCEALPHA;
		case GL_ONE_MINUS_SRC_ALPHA:      return sw::BLEND_INVSOURCEALPHA;
		case GL_DST_ALPHA:                return sw::BLEND_DESTALPHA;
		case GL_ONE_MINUS_DST_ALPHA:      return sw::BLEND_INVDESTALPHA;
		case GL_CONSTANT_COLOR:           return sw::BLEND_CONSTANT;
		case GL_ONE_MINUS_CONSTANT_COLOR: return sw::BLEND_INVCONSTANT;
		case GL_CONSTANT_ALPHA:           return sw::BLEND_CONSTANTALPHA;
		case GL_ONE_MINUS_CONSTANT_ALPHA: return sw::BLEND_INVCONSTANTALPHA;
		case GL_SRC_ALPHA_SATURATE:       return sw::BLEND_SRCALPHASAT;
		default: UNREACHABLE(blend);
		}

		return sw::BLEND_ZERO;
	}

	sw::BlendOperation ConvertBlendOp(GLenum blendOp)
	{
		switch(blendOp)
		{
		case GL_FUNC_ADD:              return sw::BLENDOP_ADD;
		case GL_FUNC_SUBTRACT:         return sw::BLENDOP_SUB;
		case GL_FUNC_REVERSE_SUBTRACT: return sw::BLENDOP_INVSUB;
		case GL_MIN_EXT:               return sw::BLENDOP_MIN;
		case GL_MAX_EXT:               return sw::BLENDOP_MAX;
		default: UNREACHABLE(blendOp);
		}

		return sw::BLENDOP_ADD;
	}

	sw::StencilOperation ConvertStencilOp(GLenum stencilOp)
	{
		switch(stencilOp)
		{
		case GL_ZERO:      return sw::OPERATION_ZERO;
		case GL_KEEP:      return sw::OPERATION_KEEP;
		case GL_REPLACE:   return sw::OPERATION_REPLACE;
		case GL_INCR:      return sw::OPERATION_INCRSAT;
		case GL_DECR:      return sw::OPERATION_DECRSAT;
		case GL_INVERT:    return sw::OPERATION_INVERT;
		case GL_INCR_WRAP: return sw::OPERATION_INCR;
		case GL_DECR_WRAP: return sw::OPERATION_DECR;
		default: UNREACHABLE(stencilOp);
		}

		return sw::OPERATION_KEEP;
	}

	sw::AddressingMode ConvertTextureWrap(GLenum wrap)
	{
		switch(wrap)
		{
		case GL_REPEAT:            return sw::ADDRESSING_WRAP;
		case GL_CLAMP_TO_EDGE:     return sw::ADDRESSING_CLAMP;
		case GL_MIRRORED_REPEAT:   return sw::ADDRESSING_MIRROR;
		default: UNREACHABLE(wrap);
		}

		return sw::ADDRESSING_WRAP;
	}

	sw::CullMode ConvertCullMode(GLenum cullFace, GLenum frontFace)
	{
		switch(cullFace)
		{
		case GL_FRONT:
			return (frontFace == GL_CCW ? sw::CULL_CLOCKWISE : sw::CULL_COUNTERCLOCKWISE);
		case GL_BACK:
			return (frontFace == GL_CCW ? sw::CULL_COUNTERCLOCKWISE : sw::CULL_CLOCKWISE);
		case GL_FRONT_AND_BACK:
			return sw::CULL_NONE;   // culling will be handled during draw
		default: UNREACHABLE(cullFace);
		}

		return sw::CULL_COUNTERCLOCKWISE;
	}

	unsigned int ConvertColorMask(bool red, bool green, bool blue, bool alpha)
	{
		return (red   ? 0x00000001 : 0) |
			   (green ? 0x00000002 : 0) |
			   (blue  ? 0x00000004 : 0) |
			   (alpha ? 0x00000008 : 0);
	}

	sw::FilterType ConvertMagFilter(GLenum magFilter)
	{
		switch(magFilter)
		{
		case GL_NEAREST: return sw::FILTER_POINT;
		case GL_LINEAR:  return sw::FILTER_LINEAR;
		default: UNREACHABLE(magFilter);
		}

		return sw::FILTER_POINT;
	}

	void ConvertMinFilter(GLenum texFilter, sw::FilterType *minFilter, sw::MipmapType *mipFilter, float maxAnisotropy)
	{
		switch(texFilter)
		{
		case GL_NEAREST:
			*minFilter = sw::FILTER_POINT;
			*mipFilter = sw::MIPMAP_NONE;
			break;
		case GL_LINEAR:
			*minFilter = sw::FILTER_LINEAR;
			*mipFilter = sw::MIPMAP_NONE;
			break;
		case GL_NEAREST_MIPMAP_NEAREST:
			*minFilter = sw::FILTER_POINT;
			*mipFilter = sw::MIPMAP_POINT;
			break;
		case GL_LINEAR_MIPMAP_NEAREST:
			*minFilter = sw::FILTER_LINEAR;
			*mipFilter = sw::MIPMAP_POINT;
			break;
		case GL_NEAREST_MIPMAP_LINEAR:
			*minFilter = sw::FILTER_POINT;
			*mipFilter = sw::MIPMAP_LINEAR;
			break;
		case GL_LINEAR_MIPMAP_LINEAR:
			*minFilter = sw::FILTER_LINEAR;
			*mipFilter = sw::MIPMAP_LINEAR;
			break;
		default:
			*minFilter = sw::FILTER_POINT;
			*mipFilter = sw::MIPMAP_NONE;
			UNREACHABLE(texFilter);
		}

		if(maxAnisotropy > 1.0f)
		{
			*minFilter = sw::FILTER_ANISOTROPIC;
		}
	}

	bool ConvertPrimitiveType(GLenum primitiveType, GLsizei elementCount,  es2::PrimitiveType &swPrimitiveType, int &primitiveCount)
	{
		switch(primitiveType)
		{
		case GL_POINTS:
			swPrimitiveType = es2::DRAW_POINTLIST;
			primitiveCount = elementCount;
			break;
		case GL_LINES:
			swPrimitiveType = es2::DRAW_LINELIST;
			primitiveCount = elementCount / 2;
			break;
		case GL_LINE_LOOP:
			swPrimitiveType = es2::DRAW_LINELOOP;
			primitiveCount = elementCount;
			break;
		case GL_LINE_STRIP:
			swPrimitiveType = es2::DRAW_LINESTRIP;
			primitiveCount = elementCount - 1;
			break;
		case GL_TRIANGLES:
			swPrimitiveType = es2::DRAW_TRIANGLELIST;
			primitiveCount = elementCount / 3;
			break;
		case GL_TRIANGLE_STRIP:
			swPrimitiveType = es2::DRAW_TRIANGLESTRIP;
			primitiveCount = elementCount - 2;
			break;
		case GL_TRIANGLE_FAN:
			swPrimitiveType = es2::DRAW_TRIANGLEFAN;
			primitiveCount = elementCount - 2;
			break;
		default:
			return false;
		}

		return true;
	}

	sw::Format ConvertRenderbufferFormat(GLenum format)
	{
		switch(format)
		{
		case GL_RGBA4:
		case GL_RGB5_A1:
		case GL_RGBA8_OES:            return sw::FORMAT_A8B8G8R8;
		case GL_RGB565:               return sw::FORMAT_R5G6B5;
		case GL_RGB8_OES:             return sw::FORMAT_X8B8G8R8;
		case GL_DEPTH_COMPONENT16:
		case GL_STENCIL_INDEX8:       
		case GL_DEPTH24_STENCIL8_OES: return sw::FORMAT_D24S8;
		default: UNREACHABLE(format); return sw::FORMAT_A8B8G8R8;
		}
	}
}

namespace sw2es
{
	unsigned int GetStencilSize(sw::Format stencilFormat)
	{
		switch(stencilFormat)
		{
		case sw::FORMAT_D24FS8:
		case sw::FORMAT_D24S8:
		case sw::FORMAT_D32FS8_TEXTURE:
			return 8;
	//	case sw::FORMAT_D24X4S4:
	//		return 4;
	//	case sw::FORMAT_D15S1:
	//		return 1;
	//	case sw::FORMAT_D16_LOCKABLE:
		case sw::FORMAT_D32:
		case sw::FORMAT_D24X8:
		case sw::FORMAT_D32F_LOCKABLE:
		case sw::FORMAT_D16:
			return 0;
	//	case sw::FORMAT_D32_LOCKABLE:  return 0;
	//	case sw::FORMAT_S8_LOCKABLE:   return 8;
		default:
			return 0;
		}
	}

	unsigned int GetAlphaSize(sw::Format colorFormat)
	{
		switch(colorFormat)
		{
		case sw::FORMAT_A16B16G16R16F:
			return 16;
		case sw::FORMAT_A32B32G32R32F:
			return 32;
		case sw::FORMAT_A2R10G10B10:
			return 2;
		case sw::FORMAT_A8R8G8B8:
		case sw::FORMAT_A8B8G8R8:
			return 8;
		case sw::FORMAT_A1R5G5B5:
			return 1;
		case sw::FORMAT_X8R8G8B8:
		case sw::FORMAT_X8B8G8R8:
		case sw::FORMAT_R5G6B5:
			return 0;
		default:
			return 0;
		}
	}

	unsigned int GetRedSize(sw::Format colorFormat)
	{
		switch(colorFormat)
		{
		case sw::FORMAT_A16B16G16R16F:
			return 16;
		case sw::FORMAT_A32B32G32R32F:
			return 32;
		case sw::FORMAT_A2R10G10B10:
			return 10;
		case sw::FORMAT_A8R8G8B8:
		case sw::FORMAT_A8B8G8R8:
		case sw::FORMAT_X8R8G8B8:
		case sw::FORMAT_X8B8G8R8:
			return 8;
		case sw::FORMAT_A1R5G5B5:
		case sw::FORMAT_R5G6B5:
			return 5;
		default:
			return 0;
		}
	}

	unsigned int GetGreenSize(sw::Format colorFormat)
	{
		switch(colorFormat)
		{
		case sw::FORMAT_A16B16G16R16F:
			return 16;
		case sw::FORMAT_A32B32G32R32F:
			return 32;
		case sw::FORMAT_A2R10G10B10:
			return 10;
		case sw::FORMAT_A8R8G8B8:
		case sw::FORMAT_A8B8G8R8:
		case sw::FORMAT_X8R8G8B8:
		case sw::FORMAT_X8B8G8R8:
			return 8;
		case sw::FORMAT_A1R5G5B5:
			return 5;
		case sw::FORMAT_R5G6B5:
			return 6;
		default:
			return 0;
		}
	}

	unsigned int GetBlueSize(sw::Format colorFormat)
	{
		switch(colorFormat)
		{
		case sw::FORMAT_A16B16G16R16F:
			return 16;
		case sw::FORMAT_A32B32G32R32F:
			return 32;
		case sw::FORMAT_A2R10G10B10:
			return 10;
		case sw::FORMAT_A8R8G8B8:
		case sw::FORMAT_A8B8G8R8:
		case sw::FORMAT_X8R8G8B8:
		case sw::FORMAT_X8B8G8R8:
			return 8;
		case sw::FORMAT_A1R5G5B5:
		case sw::FORMAT_R5G6B5:
			return 5;
		default:
			return 0;
		}
	}

	unsigned int GetDepthSize(sw::Format depthFormat)
	{
		switch(depthFormat)
		{
	//	case sw::FORMAT_D16_LOCKABLE:   return 16;
		case sw::FORMAT_D32:            return 32;
	//	case sw::FORMAT_D15S1:          return 15;
		case sw::FORMAT_D24S8:          return 24;
		case sw::FORMAT_D24X8:          return 24;
	//	case sw::FORMAT_D24X4S4:        return 24;
		case sw::FORMAT_D16:            return 16;
		case sw::FORMAT_D32F_LOCKABLE:  return 32;
		case sw::FORMAT_D24FS8:         return 24;
	//	case sw::FORMAT_D32_LOCKABLE:   return 32;
	//	case sw::FORMAT_S8_LOCKABLE:    return 0;
		case sw::FORMAT_D32FS8_TEXTURE: return 32;
		default:                        return 0;
		}
	}

	GLenum GetComponentType(sw::Format format, GLenum attachment)
	{
		// Can be one of GL_FLOAT, GL_INT, GL_UNSIGNED_INT, GL_SIGNED_NORMALIZED, or GL_UNSIGNED_NORMALIZED
		switch(attachment)
		{
		case GL_COLOR_ATTACHMENT0:
		case GL_COLOR_ATTACHMENT1:
		case GL_COLOR_ATTACHMENT2:
		case GL_COLOR_ATTACHMENT3:
		case GL_COLOR_ATTACHMENT4:
		case GL_COLOR_ATTACHMENT5:
		case GL_COLOR_ATTACHMENT6:
		case GL_COLOR_ATTACHMENT7:
		case GL_COLOR_ATTACHMENT8:
		case GL_COLOR_ATTACHMENT9:
		case GL_COLOR_ATTACHMENT10:
		case GL_COLOR_ATTACHMENT11:
		case GL_COLOR_ATTACHMENT12:
		case GL_COLOR_ATTACHMENT13:
		case GL_COLOR_ATTACHMENT14:
		case GL_COLOR_ATTACHMENT15:
			switch(format)
			{
			case sw::FORMAT_A16B16G16R16F:
			case sw::FORMAT_A32B32G32R32F:
				return GL_FLOAT;
			case sw::FORMAT_A2R10G10B10:
			case sw::FORMAT_A8R8G8B8:
			case sw::FORMAT_A8B8G8R8:
			case sw::FORMAT_X8R8G8B8:
			case sw::FORMAT_X8B8G8R8:
			case sw::FORMAT_A1R5G5B5:
			case sw::FORMAT_R5G6B5:
				return GL_UNSIGNED_NORMALIZED;
			default:
				UNREACHABLE(format);
				return 0;
			}
		case GL_DEPTH_ATTACHMENT:
		case GL_STENCIL_ATTACHMENT:
			// Only color buffers may have integer components.
			return GL_FLOAT;
		default:
			UNREACHABLE(attachment);
			return 0;
		}
	}

	GLenum ConvertBackBufferFormat(sw::Format format)
	{
		switch(format)
		{
		case sw::FORMAT_A4R4G4B4: return GL_RGBA4;
		case sw::FORMAT_A8R8G8B8: return GL_RGBA8_OES;
		case sw::FORMAT_A8B8G8R8: return GL_RGBA8_OES;
		case sw::FORMAT_A1R5G5B5: return GL_RGB5_A1;
		case sw::FORMAT_R5G6B5:   return GL_RGB565;
		case sw::FORMAT_X8R8G8B8: return GL_RGB8_OES;
		case sw::FORMAT_X8B8G8R8: return GL_RGB8_OES;
		default:
			UNREACHABLE(format);
		}

		return GL_RGBA4;
	}

	GLenum ConvertDepthStencilFormat(sw::Format format)
	{
		switch(format)
		{
		case sw::FORMAT_D16:
		case sw::FORMAT_D24X8:
		case sw::FORMAT_D32:
			return GL_DEPTH_COMPONENT16;
		case sw::FORMAT_D24S8:
			return GL_DEPTH24_STENCIL8_OES;
		default:
			UNREACHABLE(format);
		}

		return GL_DEPTH24_STENCIL8_OES;
	}
}
