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

// Framebuffer.cpp: Implements the Framebuffer class. Implements GL framebuffer
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4 page 105.

#include "Framebuffer.h"

#include "main.h"
#include "Renderbuffer.h"
#include "Texture.h"
#include "utilities.h"

namespace es2
{

Framebuffer::Framebuffer()
{
	for(int i = 0; i < IMPLEMENTATION_MAX_COLOR_ATTACHMENTS; ++i)
	{
		mColorbufferType[i] = GL_NONE;
	}
	mDepthbufferType = GL_NONE;
	mStencilbufferType = GL_NONE;
}

Framebuffer::~Framebuffer()
{
	for(int i = 0; i < IMPLEMENTATION_MAX_COLOR_ATTACHMENTS; ++i)
	{
		mColorbufferPointer[i] = NULL;
	}
	mDepthbufferPointer = NULL;
	mStencilbufferPointer = NULL;
}

Renderbuffer *Framebuffer::lookupRenderbuffer(GLenum type, GLuint handle, GLint level, GLint layer) const
{
	Context *context = getContext();
	Renderbuffer *buffer = NULL;

	if(type == GL_NONE)
	{
		buffer = NULL;
	}
	else if(type == GL_RENDERBUFFER)
	{
		buffer = context->getRenderbuffer(handle);
	}
	else if(IsTextureTarget(type))
	{
		buffer = context->getTexture(handle)->getRenderbuffer(type, level, layer);
	}
	else UNREACHABLE(type);

	return buffer;
}

void Framebuffer::setColorbuffer(GLenum type, GLuint colorbuffer, GLuint index, GLint level, GLint layer)
{
	mColorbufferType[index] = (colorbuffer != 0) ? type : GL_NONE;
	mColorbufferPointer[index] = lookupRenderbuffer(type, colorbuffer, level, layer);
}

void Framebuffer::setDepthbuffer(GLenum type, GLuint depthbuffer, GLint level, GLint layer)
{
	mDepthbufferType = (depthbuffer != 0) ? type : GL_NONE;
	mDepthbufferPointer = lookupRenderbuffer(type, depthbuffer, level, layer);
}

void Framebuffer::setStencilbuffer(GLenum type, GLuint stencilbuffer, GLint level, GLint layer)
{
	mStencilbufferType = (stencilbuffer != 0) ? type : GL_NONE;
	mStencilbufferPointer = lookupRenderbuffer(type, stencilbuffer, level, layer);
}

void Framebuffer::detachTexture(GLuint texture)
{
	for(int i = 0; i < IMPLEMENTATION_MAX_COLOR_ATTACHMENTS; ++i)
	{
		if(mColorbufferPointer[i].name() == texture && IsTextureTarget(mColorbufferType[i]))
		{
			mColorbufferType[i] = GL_NONE;
			mColorbufferPointer[i] = NULL;
		}
	}

	if(mDepthbufferPointer.name() == texture && IsTextureTarget(mDepthbufferType))
	{
		mDepthbufferType = GL_NONE;
		mDepthbufferPointer = NULL;
	}

	if(mStencilbufferPointer.name() == texture && IsTextureTarget(mStencilbufferType))
	{
		mStencilbufferType = GL_NONE;
		mStencilbufferPointer = NULL;
	}
}

void Framebuffer::detachRenderbuffer(GLuint renderbuffer)
{
	for(int i = 0; i < IMPLEMENTATION_MAX_COLOR_ATTACHMENTS; ++i)
	{
		if(mColorbufferPointer[i].name() == renderbuffer && mColorbufferType[i] == GL_RENDERBUFFER)
		{
			mColorbufferType[i] = GL_NONE;
			mColorbufferPointer[i] = NULL;
		}
	}

	if(mDepthbufferPointer.name() == renderbuffer && mDepthbufferType == GL_RENDERBUFFER)
	{
		mDepthbufferType = GL_NONE;
		mDepthbufferPointer = NULL;
	}

	if(mStencilbufferPointer.name() == renderbuffer && mStencilbufferType == GL_RENDERBUFFER)
	{
		mStencilbufferType = GL_NONE;
		mStencilbufferPointer = NULL;
	}
}

// Increments refcount on surface.
// caller must Release() the returned surface
egl::Image *Framebuffer::getRenderTarget(GLuint index)
{
	Renderbuffer *colorbuffer = mColorbufferPointer[index];

	if(colorbuffer)
	{
		return colorbuffer->getRenderTarget();
	}

	return NULL;
}

egl::Image *Framebuffer::getReadRenderTarget()
{
	Context *context = getContext();
	return getRenderTarget(context->getReadFramebufferColorIndex());
}

// Increments refcount on surface.
// caller must Release() the returned surface
egl::Image *Framebuffer::getDepthStencil()
{
	Renderbuffer *depthstencilbuffer = mDepthbufferPointer;
	
	if(!depthstencilbuffer)
	{
		depthstencilbuffer = mStencilbufferPointer;
	}

	if(depthstencilbuffer)
	{
		return depthstencilbuffer->getRenderTarget();
	}

	return NULL;
}

Renderbuffer *Framebuffer::getColorbuffer(GLuint index)
{
	return (index < MAX_COLOR_ATTACHMENTS) ? mColorbufferPointer[index] : (Renderbuffer*)NULL;
}

Renderbuffer *Framebuffer::getReadColorbuffer()
{
	Context *context = getContext();
	return getColorbuffer(context->getReadFramebufferColorIndex());
}

Renderbuffer *Framebuffer::getDepthbuffer()
{
	return mDepthbufferPointer;
}

Renderbuffer *Framebuffer::getStencilbuffer()
{
	return mStencilbufferPointer;
}

GLenum Framebuffer::getColorbufferType(GLuint index)
{
	return mColorbufferType[index];
}

GLenum Framebuffer::getDepthbufferType()
{
	return mDepthbufferType;
}

GLenum Framebuffer::getStencilbufferType()
{
	return mStencilbufferType;
}

GLuint Framebuffer::getColorbufferName(GLuint index)
{
	return mColorbufferPointer[index].name();
}

GLuint Framebuffer::getDepthbufferName()
{
	return mDepthbufferPointer.name();
}

GLuint Framebuffer::getStencilbufferName()
{
	return mStencilbufferPointer.name();
}

GLint Framebuffer::getColorbufferLayer(GLuint index)
{
	Renderbuffer *colorbuffer = mColorbufferPointer[index];
	return (colorbuffer != nullptr) ? colorbuffer->getLayer() : 0;
}

bool Framebuffer::hasStencil()
{
	if(mStencilbufferType != GL_NONE)
	{
		Renderbuffer *stencilbufferObject = getStencilbuffer();

		if(stencilbufferObject)
		{
			return stencilbufferObject->getStencilSize() > 0;
		}
	}

	return false;
}

GLenum Framebuffer::completeness()
{
	int width;
	int height;
	int samples;

	return completeness(width, height, samples);
}

GLenum Framebuffer::completeness(int &width, int &height, int &samples)
{
	width = -1;
	height = -1;
	samples = -1;

	for(int i = 0; i < IMPLEMENTATION_MAX_COLOR_ATTACHMENTS; ++i)
	{
		if(mColorbufferType[i] != GL_NONE)
		{
			Renderbuffer *colorbuffer = getColorbuffer(i);

			if(!colorbuffer)
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}

			if(colorbuffer->getWidth() == 0 || colorbuffer->getHeight() == 0)
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}

			if(mColorbufferType[i] == GL_RENDERBUFFER)
			{
				if(!es2::IsColorRenderable(colorbuffer->getFormat()))
				{
					return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
				}
			}
			else if(IsTextureTarget(mColorbufferType[i]))
			{
				GLenum format = colorbuffer->getFormat();

				if(IsCompressed(format) ||
					format == GL_ALPHA ||
					format == GL_LUMINANCE ||
					format == GL_LUMINANCE_ALPHA)
				{
					return GL_FRAMEBUFFER_UNSUPPORTED;
				}

				if(es2::IsDepthTexture(format) || es2::IsStencilTexture(format))
				{
					return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
				}
			}
			else
			{
				UNREACHABLE(mColorbufferType[i]);
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}

			width = colorbuffer->getWidth();
			height = colorbuffer->getHeight();
			samples = colorbuffer->getSamples();
		}
	}

	Renderbuffer *depthbuffer = NULL;
	Renderbuffer *stencilbuffer = NULL;

	if(mDepthbufferType != GL_NONE)
	{
		depthbuffer = getDepthbuffer();

		if(!depthbuffer)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(depthbuffer->getWidth() == 0 || depthbuffer->getHeight() == 0)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(mDepthbufferType == GL_RENDERBUFFER)
		{
			if(!es2::IsDepthRenderable(depthbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else if(IsTextureTarget(mDepthbufferType))
		{
			if(!es2::IsDepthTexture(depthbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else
		{
			UNREACHABLE(mDepthbufferType);
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(width == -1 || height == -1)
		{
			width = depthbuffer->getWidth();
			height = depthbuffer->getHeight();
			samples = depthbuffer->getSamples();
		}
		else if(width != depthbuffer->getWidth() || height != depthbuffer->getHeight())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
		}
		else if(samples != depthbuffer->getSamples())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_ANGLE;
		}
	}

	if(mStencilbufferType != GL_NONE)
	{
		stencilbuffer = getStencilbuffer();

		if(!stencilbuffer)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(stencilbuffer->getWidth() == 0 || stencilbuffer->getHeight() == 0)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(mStencilbufferType == GL_RENDERBUFFER)
		{
			if(!es2::IsStencilRenderable(stencilbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else if(IsTextureTarget(mStencilbufferType))
		{
			GLenum internalformat = stencilbuffer->getFormat();

			if(!es2::IsStencilTexture(internalformat))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else
		{
			UNREACHABLE(mStencilbufferType);
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(width == -1 || height == -1)
		{
			width = stencilbuffer->getWidth();
			height = stencilbuffer->getHeight();
			samples = stencilbuffer->getSamples();
		}
		else if(width != stencilbuffer->getWidth() || height != stencilbuffer->getHeight())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
		}
		else if(samples != stencilbuffer->getSamples())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_ANGLE;
		}
	}

	// If we have both a depth and stencil buffer, they must refer to the same object
	// since we only support packed_depth_stencil and not separate depth and stencil
	if(depthbuffer && stencilbuffer && (depthbuffer != stencilbuffer))
	{
		return GL_FRAMEBUFFER_UNSUPPORTED;
	}

	// We need to have at least one attachment to be complete
	if(width == -1 || height == -1)
	{
		return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
	}

	return GL_FRAMEBUFFER_COMPLETE;
}

GLenum Framebuffer::getImplementationColorReadFormat()
{
	Renderbuffer *colorbuffer = getReadColorbuffer();
	
	if(colorbuffer)
	{
		// Don't return GL_RGBA since that's always supported. Provide a second option here.
		switch(colorbuffer->getInternalFormat())
		{
		case sw::FORMAT_A16B16G16R16F: return GL_BGRA_EXT;
		case sw::FORMAT_A32B32G32R32F: return GL_BGRA_EXT;
		case sw::FORMAT_A8R8G8B8:      return GL_BGRA_EXT;
		case sw::FORMAT_A8B8G8R8:      return GL_BGRA_EXT;
		case sw::FORMAT_X8R8G8B8:      return 0x80E0;   // GL_BGR_EXT
		case sw::FORMAT_X8B8G8R8:      return 0x80E0;   // GL_BGR_EXT
		case sw::FORMAT_A1R5G5B5:      return GL_BGRA_EXT;
		case sw::FORMAT_R5G6B5:        return 0x80E0;   // GL_BGR_EXT
		default:
			UNREACHABLE(colorbuffer->getInternalFormat());
		}
	}

	return GL_RGBA;
}

GLenum Framebuffer::getImplementationColorReadType()
{
	Renderbuffer *colorbuffer = getReadColorbuffer();
	
	if(colorbuffer)
	{
		switch(colorbuffer->getInternalFormat())
		{
		case sw::FORMAT_A16B16G16R16F: return (egl::getClientVersion() < 3) ? GL_HALF_FLOAT_OES : GL_HALF_FLOAT;
		case sw::FORMAT_A32B32G32R32F: return GL_FLOAT;
		case sw::FORMAT_A8R8G8B8:      return GL_UNSIGNED_BYTE;
		case sw::FORMAT_A8B8G8R8:      return GL_UNSIGNED_BYTE;
		case sw::FORMAT_X8R8G8B8:      return GL_UNSIGNED_BYTE;
		case sw::FORMAT_X8B8G8R8:      return GL_UNSIGNED_BYTE;
		case sw::FORMAT_A1R5G5B5:      return GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT;
		case sw::FORMAT_R5G6B5:        return GL_UNSIGNED_SHORT_5_6_5;
		default:
			UNREACHABLE(colorbuffer->getInternalFormat());
		}
	}

	return GL_UNSIGNED_BYTE;
}

DefaultFramebuffer::DefaultFramebuffer(Colorbuffer *colorbuffer, DepthStencilbuffer *depthStencil)
{
	mColorbufferPointer[0] = new Renderbuffer(0, colorbuffer);
	mColorbufferType[0] = GL_RENDERBUFFER;

	for(int i = 1; i < IMPLEMENTATION_MAX_COLOR_ATTACHMENTS; ++i)
	{
		mColorbufferPointer[i] = nullptr;
		mColorbufferType[i] = GL_NONE;
	}

	Renderbuffer *depthStencilRenderbuffer = new Renderbuffer(0, depthStencil);
	mDepthbufferPointer = depthStencilRenderbuffer;
	mStencilbufferPointer = depthStencilRenderbuffer;

	mDepthbufferType = (depthStencilRenderbuffer->getDepthSize() != 0) ? GL_RENDERBUFFER : GL_NONE;
	mStencilbufferType = (depthStencilRenderbuffer->getStencilSize() != 0) ? GL_RENDERBUFFER : GL_NONE;
}

GLenum DefaultFramebuffer::completeness()
{
	// The default framebuffer should always be complete
	ASSERT(Framebuffer::completeness() == GL_FRAMEBUFFER_COMPLETE);

	return GL_FRAMEBUFFER_COMPLETE;
}

}
