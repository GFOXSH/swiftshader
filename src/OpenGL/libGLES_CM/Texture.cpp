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

// Texture.cpp: Implements the Texture class and its derived classes
// Texture2D and TextureCubeMap. Implements GL texture objects and related
// functionality. [OpenGL ES 2.0.24] section 3.7 page 63.

#include "Texture.h"

#include "main.h"
#include "mathutil.h"
#include "Framebuffer.h"
#include "Device.hpp"
#include "libEGL/Display.h"
#include "libEGL/Surface.h"
#include "common/debug.h"

#include <algorithm>

namespace es1
{

Texture::Texture(GLuint name) : egl::Texture(name)
{
    mMinFilter = GL_NEAREST_MIPMAP_LINEAR;
    mMagFilter = GL_LINEAR;
    mWrapS = GL_REPEAT;
    mWrapT = GL_REPEAT;
	mMaxAnisotropy = 1.0f;
	generateMipmap = GL_FALSE;
	cropRectU = 0;
	cropRectV = 0;
	cropRectW = 0;
	cropRectH = 0;

	resource = new sw::Resource(0);
}

Texture::~Texture()
{
	resource->destruct();
}

sw::Resource *Texture::getResource() const
{
	return resource;
}

// Returns true on successful filter state update (valid enum parameter)
bool Texture::setMinFilter(GLenum filter)
{
    switch(filter)
    {
    case GL_NEAREST_MIPMAP_NEAREST:
    case GL_LINEAR_MIPMAP_NEAREST:
    case GL_NEAREST_MIPMAP_LINEAR:
    case GL_LINEAR_MIPMAP_LINEAR:
        if(getTarget() == GL_TEXTURE_EXTERNAL_OES)
        {
            return false;
        }
        // Fall through
    case GL_NEAREST:
    case GL_LINEAR:
        mMinFilter = filter;
        return true;
    default:
        return false;
    }
}

// Returns true on successful filter state update (valid enum parameter)
bool Texture::setMagFilter(GLenum filter)
{
    switch(filter)
    {
    case GL_NEAREST:
    case GL_LINEAR:
        mMagFilter = filter;
        return true;
    default:
        return false;
    }
}

// Returns true on successful wrap state update (valid enum parameter)
bool Texture::setWrapS(GLenum wrap)
{
    switch(wrap)
    {
    case GL_REPEAT:
    case GL_MIRRORED_REPEAT_OES:
        if(getTarget() == GL_TEXTURE_EXTERNAL_OES)
        {
            return false;
        }
        // Fall through
    case GL_CLAMP_TO_EDGE:
        mWrapS = wrap;
        return true;
    default:
        return false;
    }
}

// Returns true on successful wrap state update (valid enum parameter)
bool Texture::setWrapT(GLenum wrap)
{
    switch(wrap)
    {
    case GL_REPEAT:
    case GL_MIRRORED_REPEAT_OES:
        if(getTarget() == GL_TEXTURE_EXTERNAL_OES)
        {
            return false;
        }
        // Fall through
    case GL_CLAMP_TO_EDGE:
         mWrapT = wrap;
         return true;
    default:
        return false;
    }
}

// Returns true on successful max anisotropy update (valid anisotropy value)
bool Texture::setMaxAnisotropy(float textureMaxAnisotropy)
{
    textureMaxAnisotropy = std::min(textureMaxAnisotropy, MAX_TEXTURE_MAX_ANISOTROPY);

    if(textureMaxAnisotropy < 1.0f)
    {
        return false;
    }

	if(mMaxAnisotropy != textureMaxAnisotropy)
    {
        mMaxAnisotropy = textureMaxAnisotropy;
    }

    return true;
}

void Texture::setGenerateMipmap(GLboolean enable)
{
	generateMipmap = enable;
}

void Texture::setCropRect(GLint u, GLint v, GLint w, GLint h)
{
	cropRectU = u;
	cropRectV = v;
	cropRectW = w;
	cropRectH = h;
}

GLenum Texture::getMinFilter() const
{
    return mMinFilter;
}

GLenum Texture::getMagFilter() const
{
    return mMagFilter;
}

GLenum Texture::getWrapS() const
{
    return mWrapS;
}

GLenum Texture::getWrapT() const
{
    return mWrapT;
}

GLfloat Texture::getMaxAnisotropy() const
{
    return mMaxAnisotropy;
}

GLboolean Texture::getGenerateMipmap() const
{
	return generateMipmap;
}

GLint Texture::getCropRectU() const
{
	return cropRectU;
}

GLint Texture::getCropRectV() const
{
	return cropRectV;
}

GLint Texture::getCropRectW() const
{
	return cropRectW;
}

GLint Texture::getCropRectH() const
{
	return cropRectH;
}

egl::Image *Texture::createSharedImage(GLenum target, unsigned int level)
{
    egl::Image *image = getRenderTarget(target, level);   // Increments reference count

    if(image)
    {
        image->markShared();
    }

    return image;
}

void Texture::setImage(GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, egl::Image *image)
{
    if(pixels && image)
    {
		egl::Image::UnpackInfo unpackInfo;
		unpackInfo.alignment = unpackAlignment;
		image->loadImageData(0, 0, 0, image->getWidth(), image->getHeight(), 1, format, type, unpackInfo, pixels);
    }
}

void Texture::setCompressedImage(GLsizei imageSize, const void *pixels, egl::Image *image)
{
    if(pixels && image)
    {
		image->loadCompressedData(0, 0, 0, image->getWidth(), image->getHeight(), 1, imageSize, pixels);
    }
}

void Texture::subImage(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, egl::Image *image)
{
	if(!image)
	{
		return error(GL_INVALID_OPERATION);
	}

    if(width + xoffset > image->getWidth() || height + yoffset > image->getHeight())
    {
        return error(GL_INVALID_VALUE);
    }

    if(IsCompressed(image->getFormat()))
    {
        return error(GL_INVALID_OPERATION);
    }

    if(format != image->getFormat())
    {
        return error(GL_INVALID_OPERATION);
    }

    if(pixels)
    {
		egl::Image::UnpackInfo unpackInfo;
		unpackInfo.alignment = unpackAlignment;
		image->loadImageData(xoffset, yoffset, 0, width, height, 1, format, type, unpackInfo, pixels);
    }
}

void Texture::subImageCompressed(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels, egl::Image *image)
{
	if(!image)
	{
		return error(GL_INVALID_OPERATION);
	}

    if(width + xoffset > image->getWidth() || height + yoffset > image->getHeight())
    {
        return error(GL_INVALID_VALUE);
    }

    if(format != image->getFormat())
    {
        return error(GL_INVALID_OPERATION);
    }

    if(pixels)
    {
		image->loadCompressedData(xoffset, yoffset, 0, width, height, 1, imageSize, pixels);
    }
}

bool Texture::copy(egl::Image *source, const sw::Rect &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, egl::Image *dest)
{
    Device *device = getDevice();

    sw::SliceRect destRect(xoffset, yoffset, xoffset + (sourceRect.x1 - sourceRect.x0), yoffset + (sourceRect.y1 - sourceRect.y0), 0);
    sw::SliceRect sourceSliceRect(sourceRect);
    bool success = device->stretchRect(source, &sourceSliceRect, dest, &destRect, false);

    if(!success)
    {
        return error(GL_OUT_OF_MEMORY, false);
    }

    return true;
}

bool Texture::isMipmapFiltered() const
{
	switch(mMinFilter)
    {
    case GL_NEAREST:
    case GL_LINEAR:
        return false;
    case GL_NEAREST_MIPMAP_NEAREST:
    case GL_LINEAR_MIPMAP_NEAREST:
    case GL_NEAREST_MIPMAP_LINEAR:
    case GL_LINEAR_MIPMAP_LINEAR:
        return true;
    default: UNREACHABLE(mMinFilter);
    }

	return false;
}

Texture2D::Texture2D(GLuint name) : Texture(name)
{
	for(int i = 0; i < MIPMAP_LEVELS; i++)
	{
		image[i] = 0;
	}

    mSurface = NULL;

	mColorbufferProxy = NULL;
	mProxyRefs = 0;
}

Texture2D::~Texture2D()
{
	resource->lock(sw::DESTRUCT);

	for(int i = 0; i < MIPMAP_LEVELS; i++)
	{
		if(image[i])
		{
			image[i]->unbind(this);
			image[i] = 0;
		}
	}

	resource->unlock();

    if(mSurface)
    {
        mSurface->setBoundTexture(NULL);
        mSurface = NULL;
    }

	mColorbufferProxy = NULL;
}

// We need to maintain a count of references to renderbuffers acting as
// proxies for this texture, so that we do not attempt to use a pointer
// to a renderbuffer proxy which has been deleted.
void Texture2D::addProxyRef(const Renderbuffer *proxy)
{
    mProxyRefs++;
}

void Texture2D::releaseProxy(const Renderbuffer *proxy)
{
    if(mProxyRefs > 0)
	{
        mProxyRefs--;
	}

    if(mProxyRefs == 0)
	{
		mColorbufferProxy = NULL;
	}
}

GLenum Texture2D::getTarget() const
{
    return GL_TEXTURE_2D;
}

GLsizei Texture2D::getWidth(GLenum target, GLint level) const
{
	ASSERT(target == GL_TEXTURE_2D);
    return image[level] ? image[level]->getWidth() : 0;
}

GLsizei Texture2D::getHeight(GLenum target, GLint level) const
{
	ASSERT(target == GL_TEXTURE_2D);
    return image[level] ? image[level]->getHeight() : 0;
}

GLenum Texture2D::getFormat(GLenum target, GLint level) const
{
	ASSERT(target == GL_TEXTURE_2D);
    return image[level] ? image[level]->getFormat() : 0;
}

GLenum Texture2D::getType(GLenum target, GLint level) const
{
	ASSERT(target == GL_TEXTURE_2D);
    return image[level] ? image[level]->getType() : 0;
}

sw::Format Texture2D::getInternalFormat(GLenum target, GLint level) const
{
	ASSERT(target == GL_TEXTURE_2D);
	return image[level] ? image[level]->getInternalFormat() : sw::FORMAT_NULL;
}

int Texture2D::getLevelCount() const
{
	ASSERT(isSamplerComplete());
	int levels = 0;

	while(levels < MIPMAP_LEVELS && image[levels])
	{
		levels++;
	}

	return levels;
}

void Texture2D::setImage(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
	if(image[level])
	{
		image[level]->unbind(this);
	}

	image[level] = new egl::Image(this, width, height, format, type);

	if(!image[level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

    Texture::setImage(format, type, unpackAlignment, pixels, image[level]);
}

void Texture2D::bindTexImage(egl::Surface *surface)
{
    GLenum format;

    switch(surface->getInternalFormat())
    {
    case sw::FORMAT_A8R8G8B8:
		format = GL_BGRA_EXT;
        break;
	case sw::FORMAT_A8B8G8R8:
        format = GL_RGBA;
        break;
    case sw::FORMAT_X8B8G8R8:
	case sw::FORMAT_X8R8G8B8:
        format = GL_RGB;
        break;
    default:
        UNIMPLEMENTED();
        return;
    }

	for(int level = 0; level < MIPMAP_LEVELS; level++)
	{
		if(image[level])
		{
			image[level]->unbind(this);
			image[level] = 0;
		}
	}

	image[0] = surface->getRenderTarget();

    mSurface = surface;
    mSurface->setBoundTexture(this);
}

void Texture2D::releaseTexImage()
{
    for(int level = 0; level < MIPMAP_LEVELS; level++)
	{
		if(image[level])
		{
			image[level]->unbind(this);
			image[level] = 0;
		}
	}
}

void Texture2D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
	if(image[level])
	{
		image[level]->unbind(this);
	}

	image[level] = new egl::Image(this, width, height, format, GL_UNSIGNED_BYTE);

	if(!image[level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

    Texture::setCompressedImage(imageSize, pixels, image[level]);
}

void Texture2D::subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
	Texture::subImage(xoffset, yoffset, width, height, format, type, unpackAlignment, pixels, image[level]);
}

void Texture2D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
    Texture::subImageCompressed(xoffset, yoffset, width, height, format, imageSize, pixels, image[level]);
}

void Texture2D::copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    egl::Image *renderTarget = source->getRenderTarget();

    if(!renderTarget)
    {
        ERR("Failed to retrieve the render target.");
        return error(GL_OUT_OF_MEMORY);
    }

	if(image[level])
	{
		image[level]->unbind(this);
	}

	image[level] = new egl::Image(this, width, height, format, GL_UNSIGNED_BYTE);

	if(!image[level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

    if(width != 0 && height != 0)
    {
		sw::Rect sourceRect = {x, y, x + width, y + height};
		sourceRect.clip(0, 0, source->getColorbuffer()->getWidth(), source->getColorbuffer()->getHeight());

        copy(renderTarget, sourceRect, format, 0, 0, image[level]);
    }

	renderTarget->release();
}

void Texture2D::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
	if(!image[level])
	{
		return error(GL_INVALID_OPERATION);
	}

    if(xoffset + width > image[level]->getWidth() || yoffset + height > image[level]->getHeight())
    {
        return error(GL_INVALID_VALUE);
    }

    egl::Image *renderTarget = source->getRenderTarget();

    if(!renderTarget)
    {
        ERR("Failed to retrieve the render target.");
        return error(GL_OUT_OF_MEMORY);
    }

	sw::Rect sourceRect = {x, y, x + width, y + height};
	sourceRect.clip(0, 0, source->getColorbuffer()->getWidth(), source->getColorbuffer()->getHeight());

	copy(renderTarget, sourceRect, image[level]->getFormat(), xoffset, yoffset, image[level]);

	renderTarget->release();
}

void Texture2D::setImage(egl::Image *sharedImage)
{
	sharedImage->addRef();

    if(image[0])
    {
        image[0]->unbind(this);
    }

    image[0] = sharedImage;
}

// Tests for 2D texture sampling completeness. [OpenGL ES 2.0.24] section 3.8.2 page 85.
bool Texture2D::isSamplerComplete() const
{
	if(!image[0])
	{
		return false;
	}

    GLsizei width = image[0]->getWidth();
    GLsizei height = image[0]->getHeight();

    if(width <= 0 || height <= 0)
    {
        return false;
    }

    if(isMipmapFiltered())
    {
        if(!generateMipmap && !isMipmapComplete())
        {
            return false;
        }
    }

    return true;
}

// Tests for 2D texture (mipmap) completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool Texture2D::isMipmapComplete() const
{
    GLsizei width = image[0]->getWidth();
    GLsizei height = image[0]->getHeight();

    int q = log2(std::max(width, height));

    for(int level = 1; level <= q; level++)
    {
		if(!image[level])
		{
			return false;
		}

        if(image[level]->getFormat() != image[0]->getFormat())
        {
            return false;
        }

        if(image[level]->getType() != image[0]->getType())
        {
            return false;
        }

        if(image[level]->getWidth() != std::max(1, width >> level))
        {
            return false;
        }

        if(image[level]->getHeight() != std::max(1, height >> level))
        {
            return false;
        }
    }

    return true;
}

bool Texture2D::isCompressed(GLenum target, GLint level) const
{
    return IsCompressed(getFormat(target, level));
}

bool Texture2D::isDepth(GLenum target, GLint level) const
{
    return IsDepthTexture(getFormat(target, level));
}

void Texture2D::generateMipmaps()
{
	if(!image[0])
	{
		return;   // FIXME: error?
	}

    unsigned int q = log2(std::max(image[0]->getWidth(), image[0]->getHeight()));

	for(unsigned int i = 1; i <= q; i++)
    {
		if(image[i])
		{
			image[i]->unbind(this);
		}

		image[i] = new egl::Image(this, std::max(image[0]->getWidth() >> i, 1), std::max(image[0]->getHeight() >> i, 1), image[0]->getFormat(), image[0]->getType());

		if(!image[i])
		{
			return error(GL_OUT_OF_MEMORY);
		}

		getDevice()->stretchRect(image[i - 1], 0, image[i], 0, true);
    }
}

void Texture2D::autoGenerateMipmaps()
{
	if(generateMipmap && image[0]->hasDirtyMipmaps())
	{
		generateMipmaps();
		image[0]->cleanMipmaps();
	}
}

egl::Image *Texture2D::getImage(unsigned int level)
{
	return image[level];
}

Renderbuffer *Texture2D::getRenderbuffer(GLenum target)
{
    if(target != GL_TEXTURE_2D)
    {
        return error(GL_INVALID_OPERATION, (Renderbuffer *)NULL);
    }

    if(mColorbufferProxy == NULL)
    {
        mColorbufferProxy = new Renderbuffer(name, new RenderbufferTexture2D(this));
    }

    return mColorbufferProxy;
}

egl::Image *Texture2D::getRenderTarget(GLenum target, unsigned int level)
{
    ASSERT(target == GL_TEXTURE_2D);
	ASSERT(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

	if(image[level])
	{
		image[level]->addRef();
	}

	return image[level];
}

bool Texture2D::isShared(GLenum target, unsigned int level) const
{
    ASSERT(target == GL_TEXTURE_2D);
    ASSERT(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

    if(mSurface)   // Bound to an EGLSurface
    {
        return true;
    }

    if(!image[level])
    {
        return false;
    }

    return image[level]->isShared();
}

TextureExternal::TextureExternal(GLuint name) : Texture2D(name)
{
    mMinFilter = GL_LINEAR;
    mMagFilter = GL_LINEAR;
    mWrapS = GL_CLAMP_TO_EDGE;
    mWrapT = GL_CLAMP_TO_EDGE;
}

TextureExternal::~TextureExternal()
{
}

GLenum TextureExternal::getTarget() const
{
    return GL_TEXTURE_EXTERNAL_OES;
}

}

egl::Image *createBackBuffer(int width, int height, const egl::Config *config)
{
	if(config)
	{
		return new egl::Image(width, height, config->mRenderTargetFormat, 1, false, true);
	}

	return 0;
}

egl::Image *createDepthStencil(unsigned int width, unsigned int height, sw::Format format, int multiSampleDepth, bool discard)
{
	if(width == 0 || height == 0 || height > OUTLINE_RESOLUTION)
	{
		ERR("Invalid parameters");
		return 0;
	}

	bool lockable = true;

	switch(format)
	{
//	case sw::FORMAT_D15S1:
	case sw::FORMAT_D24S8:
	case sw::FORMAT_D24X8:
//	case sw::FORMAT_D24X4S4:
	case sw::FORMAT_D24FS8:
	case sw::FORMAT_D32:
	case sw::FORMAT_D16:
		lockable = false;
		break;
//	case sw::FORMAT_S8_LOCKABLE:
//	case sw::FORMAT_D16_LOCKABLE:
	case sw::FORMAT_D32F_LOCKABLE:
//	case sw::FORMAT_D32_LOCKABLE:
	case sw::FORMAT_DF24S8:
	case sw::FORMAT_DF16S8:
		lockable = true;
		break;
	default:
		UNREACHABLE(format);
	}

	egl::Image *surface = new egl::Image(width, height, format, multiSampleDepth, lockable, true);

	if(!surface)
	{
		ERR("Out of memory");
		return 0;
	}

	return surface;
}
