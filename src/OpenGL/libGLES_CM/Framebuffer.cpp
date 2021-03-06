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

namespace es1
{

Framebuffer::Framebuffer()
{
	mColorbufferType = GL_NONE_OES;
	mDepthbufferType = GL_NONE_OES;
	mStencilbufferType = GL_NONE_OES;
}

Framebuffer::~Framebuffer()
{
	mColorbufferPointer = NULL;
	mDepthbufferPointer = NULL;
	mStencilbufferPointer = NULL;
}

Renderbuffer *Framebuffer::lookupRenderbuffer(GLenum type, GLuint handle) const
{
	Context *context = getContext();
	Renderbuffer *buffer = NULL;

	if(type == GL_NONE_OES)
	{
		buffer = NULL;
	}
	else if(type == GL_RENDERBUFFER_OES)
	{
		buffer = context->getRenderbuffer(handle);
	}
	else if(IsTextureTarget(type))
	{
		buffer = context->getTexture(handle)->getRenderbuffer(type);
	}
	else UNREACHABLE(type);

	return buffer;
}

void Framebuffer::setColorbuffer(GLenum type, GLuint colorbuffer)
{
	mColorbufferType = (colorbuffer != 0) ? type : GL_NONE_OES;
	mColorbufferPointer = lookupRenderbuffer(type, colorbuffer);
}

void Framebuffer::setDepthbuffer(GLenum type, GLuint depthbuffer)
{
	mDepthbufferType = (depthbuffer != 0) ? type : GL_NONE_OES;
	mDepthbufferPointer = lookupRenderbuffer(type, depthbuffer);
}

void Framebuffer::setStencilbuffer(GLenum type, GLuint stencilbuffer)
{
	mStencilbufferType = (stencilbuffer != 0) ? type : GL_NONE_OES;
	mStencilbufferPointer = lookupRenderbuffer(type, stencilbuffer);
}

void Framebuffer::detachTexture(GLuint texture)
{
	if(mColorbufferPointer.name() == texture && IsTextureTarget(mColorbufferType))
	{
		mColorbufferType = GL_NONE_OES;
		mColorbufferPointer = NULL;
	}

	if(mDepthbufferPointer.name() == texture && IsTextureTarget(mDepthbufferType))
	{
		mDepthbufferType = GL_NONE_OES;
		mDepthbufferPointer = NULL;
	}

	if(mStencilbufferPointer.name() == texture && IsTextureTarget(mStencilbufferType))
	{
		mStencilbufferType = GL_NONE_OES;
		mStencilbufferPointer = NULL;
	}
}

void Framebuffer::detachRenderbuffer(GLuint renderbuffer)
{
	if(mColorbufferPointer.name() == renderbuffer && mColorbufferType == GL_RENDERBUFFER_OES)
	{
		mColorbufferType = GL_NONE_OES;
		mColorbufferPointer = NULL;
	}

	if(mDepthbufferPointer.name() == renderbuffer && mDepthbufferType == GL_RENDERBUFFER_OES)
	{
		mDepthbufferType = GL_NONE_OES;
		mDepthbufferPointer = NULL;
	}

	if(mStencilbufferPointer.name() == renderbuffer && mStencilbufferType == GL_RENDERBUFFER_OES)
	{
		mStencilbufferType = GL_NONE_OES;
		mStencilbufferPointer = NULL;
	}
}

// Increments refcount on surface.
// caller must Release() the returned surface
egl::Image *Framebuffer::getRenderTarget()
{
	Renderbuffer *colorbuffer = mColorbufferPointer;

	if(colorbuffer)
	{
		return colorbuffer->getRenderTarget();
	}

	return NULL;
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

Renderbuffer *Framebuffer::getColorbuffer()
{
	return mColorbufferPointer;
}

Renderbuffer *Framebuffer::getDepthbuffer()
{
	return mDepthbufferPointer;
}

Renderbuffer *Framebuffer::getStencilbuffer()
{
	return mStencilbufferPointer;
}

GLenum Framebuffer::getColorbufferType()
{
	return mColorbufferType;
}

GLenum Framebuffer::getDepthbufferType()
{
	return mDepthbufferType;
}

GLenum Framebuffer::getStencilbufferType()
{
	return mStencilbufferType;
}

GLuint Framebuffer::getColorbufferName()
{
	return mColorbufferPointer.name();
}

GLuint Framebuffer::getDepthbufferName()
{
	return mDepthbufferPointer.name();
}

GLuint Framebuffer::getStencilbufferName()
{
	return mStencilbufferPointer.name();
}

bool Framebuffer::hasStencil()
{
	if(mStencilbufferType != GL_NONE_OES)
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

	if(mColorbufferType != GL_NONE_OES)
	{
		Renderbuffer *colorbuffer = getColorbuffer();

		if(!colorbuffer)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(colorbuffer->getWidth() == 0 || colorbuffer->getHeight() == 0)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(mColorbufferType == GL_RENDERBUFFER_OES)
		{
			if(!es1::IsColorRenderable(colorbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			}
		}
		else if(IsTextureTarget(mColorbufferType))
		{
			GLenum format = colorbuffer->getFormat();

			if(IsCompressed(format) ||
			   format == GL_ALPHA ||
			   format == GL_LUMINANCE ||
			   format == GL_LUMINANCE_ALPHA)
			{
				return GL_FRAMEBUFFER_UNSUPPORTED_OES;
			}

			if(es1::IsDepthTexture(format) || es1::IsStencilTexture(format))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			}
		}
		else
		{
			UNREACHABLE(mColorbufferType);
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		width = colorbuffer->getWidth();
		height = colorbuffer->getHeight();
		samples = colorbuffer->getSamples();
	}

	Renderbuffer *depthbuffer = NULL;
	Renderbuffer *stencilbuffer = NULL;

	if(mDepthbufferType != GL_NONE_OES)
	{
		depthbuffer = getDepthbuffer();

		if(!depthbuffer)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(depthbuffer->getWidth() == 0 || depthbuffer->getHeight() == 0)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(mDepthbufferType == GL_RENDERBUFFER_OES)
		{
			if(!es1::IsDepthRenderable(depthbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			}
		}
		else if(IsTextureTarget(mDepthbufferType))
		{
			if(!es1::IsDepthTexture(depthbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			}
		}
		else
		{
			UNREACHABLE(mDepthbufferType);
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(width == -1 || height == -1)
		{
			width = depthbuffer->getWidth();
			height = depthbuffer->getHeight();
			samples = depthbuffer->getSamples();
		}
		else if(width != depthbuffer->getWidth() || height != depthbuffer->getHeight())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES;
		}
		else if(samples != depthbuffer->getSamples())
		{
			UNREACHABLE(0);
		}
	}

	if(mStencilbufferType != GL_NONE_OES)
	{
		stencilbuffer = getStencilbuffer();

		if(!stencilbuffer)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(stencilbuffer->getWidth() == 0 || stencilbuffer->getHeight() == 0)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(mStencilbufferType == GL_RENDERBUFFER_OES)
		{
			if(!es1::IsStencilRenderable(stencilbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			}
		}
		else if(IsTextureTarget(mStencilbufferType))
		{
			GLenum internalformat = stencilbuffer->getFormat();

			if(!es1::IsStencilTexture(internalformat))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			}
		}
		else
		{
			UNREACHABLE(mStencilbufferType);
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(width == -1 || height == -1)
		{
			width = stencilbuffer->getWidth();
			height = stencilbuffer->getHeight();
			samples = stencilbuffer->getSamples();
		}
		else if(width != stencilbuffer->getWidth() || height != stencilbuffer->getHeight())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES;
		}
		else if(samples != stencilbuffer->getSamples())
		{
			UNREACHABLE(0);
			return GL_FRAMEBUFFER_UNSUPPORTED_OES;   // GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_OES;
		}
	}

	// If we have both a depth and stencil buffer, they must refer to the same object
	// since we only support packed_depth_stencil and not separate depth and stencil
	if(depthbuffer && stencilbuffer && (depthbuffer != stencilbuffer))
	{
		return GL_FRAMEBUFFER_UNSUPPORTED_OES;
	}

	// We need to have at least one attachment to be complete
	if(width == -1 || height == -1)
	{
		return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES;
	}

	return GL_FRAMEBUFFER_COMPLETE_OES;
}

GLenum Framebuffer::getImplementationColorReadFormat()
{
	Renderbuffer *colorbuffer = mColorbufferPointer;
	
	if(colorbuffer)
	{
		// Don't return GL_RGBA since that's always supported. Provide a second option here.
		switch(colorbuffer->getInternalFormat())
		{
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
	Renderbuffer *colorbuffer = mColorbufferPointer;
	
	if(colorbuffer)
	{
		switch(colorbuffer->getInternalFormat())
		{
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
	mColorbufferPointer = new Renderbuffer(0, colorbuffer);

	Renderbuffer *depthStencilRenderbuffer = new Renderbuffer(0, depthStencil);
	mDepthbufferPointer = depthStencilRenderbuffer;
	mStencilbufferPointer = depthStencilRenderbuffer;

	mColorbufferType = GL_RENDERBUFFER_OES;
	mDepthbufferType = (depthStencilRenderbuffer->getDepthSize() != 0) ? GL_RENDERBUFFER_OES : GL_NONE_OES;
	mStencilbufferType = (depthStencilRenderbuffer->getStencilSize() != 0) ? GL_RENDERBUFFER_OES : GL_NONE_OES;
}

GLenum DefaultFramebuffer::completeness()
{
	// The default framebuffer should always be complete
	ASSERT(Framebuffer::completeness() == GL_FRAMEBUFFER_COMPLETE_OES);

	return GL_FRAMEBUFFER_COMPLETE_OES;
}

}
