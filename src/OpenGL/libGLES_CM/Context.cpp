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

// Context.cpp: Implements the es1::Context class, managing all GL state and performing
// rendering operations. It is the GLES2 specific implementation of EGLContext.

#include "Context.h"

#include "main.h"
#include "mathutil.h"
#include "utilities.h"
#include "ResourceManager.h"
#include "Buffer.h"
#include "Framebuffer.h"
#include "Renderbuffer.h"
#include "Texture.h"
#include "VertexDataManager.h"
#include "IndexDataManager.h"
#include "libEGL/Display.h"
#include "libEGL/Surface.h"
#include "Common/Half.hpp"

#include <EGL/eglext.h>

#include <algorithm>

#undef near
#undef far

namespace es1
{
Context::Context(const egl::Config *config, const Context *shareContext)
    : modelViewStack(MAX_MODELVIEW_STACK_DEPTH),
      projectionStack(MAX_PROJECTION_STACK_DEPTH),
	  textureStack0(MAX_TEXTURE_STACK_DEPTH),
	  textureStack1(MAX_TEXTURE_STACK_DEPTH)
{
	sw::Context *context = new sw::Context();
	device = new es1::Device(context);

	mVertexDataManager = new VertexDataManager(this);
    mIndexDataManager = new IndexDataManager();

    setClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    mState.depthClearValue = 1.0f;
    mState.stencilClearValue = 0;

    mState.cullFace = false;
    mState.cullMode = GL_BACK;
    mState.frontFace = GL_CCW;
    mState.depthTest = false;
    mState.depthFunc = GL_LESS;
    mState.blend = false;
    mState.sourceBlendRGB = GL_ONE;
    mState.sourceBlendAlpha = GL_ONE;
    mState.destBlendRGB = GL_ZERO;
    mState.destBlendAlpha = GL_ZERO;
    mState.blendEquationRGB = GL_FUNC_ADD_OES;
    mState.blendEquationAlpha = GL_FUNC_ADD_OES;
    mState.stencilTest = false;
    mState.stencilFunc = GL_ALWAYS;
    mState.stencilRef = 0;
    mState.stencilMask = -1;
    mState.stencilWritemask = -1;
    mState.stencilFail = GL_KEEP;
    mState.stencilPassDepthFail = GL_KEEP;
    mState.stencilPassDepthPass = GL_KEEP;
    mState.polygonOffsetFill = false;
    mState.polygonOffsetFactor = 0.0f;
    mState.polygonOffsetUnits = 0.0f;
    mState.sampleAlphaToCoverage = false;
    mState.sampleCoverage = false;
    mState.sampleCoverageValue = 1.0f;
    mState.sampleCoverageInvert = false;
    mState.scissorTest = false;
    mState.dither = true;
	mState.shadeModel = GL_SMOOTH;
    mState.generateMipmapHint = GL_DONT_CARE;
	mState.perspectiveCorrectionHint = GL_DONT_CARE;
	mState.colorLogicOp = false;
	mState.logicalOperation = GL_COPY;

    mState.lineWidth = 1.0f;

    mState.viewportX = 0;
    mState.viewportY = 0;
    mState.viewportWidth = config->mDisplayMode.width;
    mState.viewportHeight = config->mDisplayMode.height;
    mState.zNear = 0.0f;
    mState.zFar = 1.0f;

    mState.scissorX = 0;
    mState.scissorY = 0;
    mState.scissorWidth = config->mDisplayMode.width;
    mState.scissorHeight = config->mDisplayMode.height;

    mState.colorMaskRed = true;
    mState.colorMaskGreen = true;
    mState.colorMaskBlue = true;
    mState.colorMaskAlpha = true;
    mState.depthMask = true;

	for(int i = 0; i < MAX_TEXTURE_UNITS; i++)
	{
		mState.textureUnit[i].environmentMode = GL_MODULATE;
		mState.textureUnit[i].combineRGB = GL_MODULATE;
		mState.textureUnit[i].combineAlpha = GL_MODULATE;
		mState.textureUnit[i].src0RGB = GL_TEXTURE;
		mState.textureUnit[i].src1RGB = GL_PREVIOUS;
		mState.textureUnit[i].src2RGB = GL_CONSTANT;
		mState.textureUnit[i].src0Alpha = GL_TEXTURE;
		mState.textureUnit[i].src1Alpha = GL_PREVIOUS;
		mState.textureUnit[i].src2Alpha = GL_CONSTANT;
		mState.textureUnit[i].operand0RGB = GL_SRC_COLOR;
		mState.textureUnit[i].operand1RGB = GL_SRC_COLOR;
		mState.textureUnit[i].operand2RGB = GL_SRC_ALPHA;
		mState.textureUnit[i].operand0Alpha = GL_SRC_ALPHA;
		mState.textureUnit[i].operand1Alpha = GL_SRC_ALPHA;
		mState.textureUnit[i].operand2Alpha = GL_SRC_ALPHA;
	}

    if(shareContext != NULL)
    {
        mResourceManager = shareContext->mResourceManager;
        mResourceManager->addRef();
    }
    else
    {
        mResourceManager = new ResourceManager();
    }

    // [OpenGL ES 2.0.24] section 3.7 page 83:
    // In the initial state, TEXTURE_2D and TEXTURE_CUBE_MAP have twodimensional
    // and cube map texture state vectors respectively associated with them.
    // In order that access to these initial textures not be lost, they are treated as texture
    // objects all of whose names are 0.

    mTexture2DZero = new Texture2D(0);
    mTextureExternalZero = new TextureExternal(0);

    mState.activeSampler = 0;
    bindArrayBuffer(0);
    bindElementArrayBuffer(0);
    bindTexture2D(0);
    bindFramebuffer(0);
    bindRenderbuffer(0);

    mState.packAlignment = 4;
    mState.unpackAlignment = 4;

    mInvalidEnum = false;
    mInvalidValue = false;
    mInvalidOperation = false;
    mOutOfMemory = false;
    mInvalidFramebufferOperation = false;

	lighting = false;

	for(int i = 0; i < MAX_LIGHTS; i++)
	{
		light[i].enable = false;
		light[i].ambient = {0.0f, 0.0f, 0.0f, 1.0f};
		light[i].diffuse = {0.0f, 0.0f, 0.0f, 1.0f};
		light[i].specular = {0.0f, 0.0f, 0.0f, 1.0f};
		light[i].position = {0.0f, 0.0f, 1.0f, 0.0f};
		light[i].direction = {0.0f, 0.0f, -1.0f};
		light[i].attenuation = {1.0f, 0.0f, 0.0f};
	}

	light[0].diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
	light[0].specular = {1.0f, 1.0f, 1.0f, 1.0f};

	globalAmbient = {0.2f, 0.2f, 0.2f, 1.0f};
	materialAmbient = {0.2f, 0.2f, 0.2f, 1.0f};
	materialDiffuse = {0.8f, 0.8f, 0.8f, 1.0f};
	materialSpecular = {0.0f, 0.0f, 0.0f, 1.0f};
	materialEmission = {0.0f, 0.0f, 0.0f, 1.0f};
	materialShininess = 0.0f;

	matrixMode = GL_MODELVIEW;
    
	for(int i = 0; i < MAX_TEXTURE_UNITS; i++)
	{
		texture2Denabled[i] = false;
		textureExternalEnabled[i] = false;
	}

	clientTexture = GL_TEXTURE0;

	setVertexAttrib(sw::Color0, 1.0f, 1.0f, 1.0f, 1.0f);

	for(int i = 0; i < MAX_TEXTURE_UNITS; i++)
	{
		setVertexAttrib(sw::TexCoord0 + i, 0.0f, 0.0f, 0.0f, 1.0f);
	}

	setVertexAttrib(sw::Normal, 0.0f, 0.0f, 1.0f, 1.0f);
	setVertexAttrib(sw::PointSize, 1.0f, 1.0f, 1.0f, 1.0f);

	clipFlags = 0;

	alphaTest = false;
	alphaTestFunc = GL_ALWAYS;
	alphaTestRef = 0;

    mHasBeenCurrent = false;

    markAllStateDirty();
}

Context::~Context()
{
    while(!mFramebufferMap.empty())
    {
        deleteFramebuffer(mFramebufferMap.begin()->first);
    }

    for(int type = 0; type < TEXTURE_TYPE_COUNT; type++)
    {
        for(int sampler = 0; sampler < MAX_TEXTURE_UNITS; sampler++)
        {
            mState.samplerTexture[type][sampler] = NULL;
        }
    }

    for(int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        mState.vertexAttribute[i].mBoundBuffer = NULL;
    }

    mState.arrayBuffer = NULL;
    mState.elementArrayBuffer = NULL;
    mState.renderbuffer = NULL;

    mTexture2DZero = NULL;
    mTextureExternalZero = NULL;

    delete mVertexDataManager;
    delete mIndexDataManager;

    mResourceManager->release();
	delete device;
}

void Context::makeCurrent(egl::Surface *surface)
{
    if(!mHasBeenCurrent)
    {
        mState.viewportX = 0;
        mState.viewportY = 0;
        mState.viewportWidth = surface->getWidth();
        mState.viewportHeight = surface->getHeight();

        mState.scissorX = 0;
        mState.scissorY = 0;
        mState.scissorWidth = surface->getWidth();
        mState.scissorHeight = surface->getHeight();

        mHasBeenCurrent = true;
    }

    // Wrap the existing resources into GL objects and assign them to the '0' names
    egl::Image *defaultRenderTarget = surface->getRenderTarget();
    egl::Image *depthStencil = surface->getDepthStencil();

    Colorbuffer *colorbufferZero = new Colorbuffer(defaultRenderTarget);
    DepthStencilbuffer *depthStencilbufferZero = new DepthStencilbuffer(depthStencil);
    Framebuffer *framebufferZero = new DefaultFramebuffer(colorbufferZero, depthStencilbufferZero);

    setFramebufferZero(framebufferZero);

    if(defaultRenderTarget)
    {
        defaultRenderTarget->release();
    }

    if(depthStencil)
    {
        depthStencil->release();
    }

    markAllStateDirty();
}

int Context::getClientVersion() const
{
	return 1;
}

// This function will set all of the state-related dirty flags, so that all state is set during next pre-draw.
void Context::markAllStateDirty()
{
    mDepthStateDirty = true;
    mMaskStateDirty = true;
    mBlendStateDirty = true;
    mStencilStateDirty = true;
    mPolygonOffsetStateDirty = true;
    mSampleStateDirty = true;
    mDitherStateDirty = true;
    mFrontFaceDirty = true;
}

void Context::setClearColor(float red, float green, float blue, float alpha)
{
    mState.colorClearValue.red = red;
    mState.colorClearValue.green = green;
    mState.colorClearValue.blue = blue;
    mState.colorClearValue.alpha = alpha;
}

void Context::setClearDepth(float depth)
{
    mState.depthClearValue = depth;
}

void Context::setClearStencil(int stencil)
{
    mState.stencilClearValue = stencil;
}

void Context::setCullFace(bool enabled)
{
    mState.cullFace = enabled;
}

bool Context::isCullFaceEnabled() const
{
    return mState.cullFace;
}

void Context::setCullMode(GLenum mode)
{
   mState.cullMode = mode;
}

void Context::setFrontFace(GLenum front)
{
    if(mState.frontFace != front)
    {
        mState.frontFace = front;
        mFrontFaceDirty = true;
    }
}

void Context::setDepthTest(bool enabled)
{
    if(mState.depthTest != enabled)
    {
        mState.depthTest = enabled;
        mDepthStateDirty = true;
    }
}

bool Context::isDepthTestEnabled() const
{
    return mState.depthTest;
}

void Context::setDepthFunc(GLenum depthFunc)
{
    if(mState.depthFunc != depthFunc)
    {
        mState.depthFunc = depthFunc;
        mDepthStateDirty = true;
    }
}

void Context::setDepthRange(float zNear, float zFar)
{
    mState.zNear = zNear;
    mState.zFar = zFar;
}

void Context::setAlphaTest(bool enabled)
{
	alphaTest = enabled;
}

bool Context::isAlphaTestEnabled() const
{
	return alphaTest;
}

void Context::setAlphaFunc(GLenum alphaFunc, GLclampf reference)
{
	alphaTestFunc = alphaFunc;
	alphaTestRef = reference;
}

void Context::setBlend(bool enabled)
{
    if(mState.blend != enabled)
    {
        mState.blend = enabled;
        mBlendStateDirty = true;
    }
}

bool Context::isBlendEnabled() const
{
    return mState.blend;
}

void Context::setBlendFactors(GLenum sourceRGB, GLenum destRGB, GLenum sourceAlpha, GLenum destAlpha)
{
    if(mState.sourceBlendRGB != sourceRGB ||
       mState.sourceBlendAlpha != sourceAlpha ||
       mState.destBlendRGB != destRGB ||
       mState.destBlendAlpha != destAlpha)
    {
        mState.sourceBlendRGB = sourceRGB;
        mState.destBlendRGB = destRGB;
        mState.sourceBlendAlpha = sourceAlpha;
        mState.destBlendAlpha = destAlpha;
        mBlendStateDirty = true;
    }
}

void Context::setBlendEquation(GLenum rgbEquation, GLenum alphaEquation)
{
    if(mState.blendEquationRGB != rgbEquation ||
       mState.blendEquationAlpha != alphaEquation)
    {
        mState.blendEquationRGB = rgbEquation;
        mState.blendEquationAlpha = alphaEquation;
        mBlendStateDirty = true;
    }
}

void Context::setStencilTest(bool enabled)
{
    if(mState.stencilTest != enabled)
    {
        mState.stencilTest = enabled;
        mStencilStateDirty = true;
    }
}

bool Context::isStencilTestEnabled() const
{
    return mState.stencilTest;
}

void Context::setStencilParams(GLenum stencilFunc, GLint stencilRef, GLuint stencilMask)
{
    if(mState.stencilFunc != stencilFunc ||
        mState.stencilRef != stencilRef ||
        mState.stencilMask != stencilMask)
    {
        mState.stencilFunc = stencilFunc;
        mState.stencilRef = (stencilRef > 0) ? stencilRef : 0;
        mState.stencilMask = stencilMask;
        mStencilStateDirty = true;
    }
}

void Context::setStencilWritemask(GLuint stencilWritemask)
{
    if(mState.stencilWritemask != stencilWritemask)
    {
        mState.stencilWritemask = stencilWritemask;
        mStencilStateDirty = true;
    }
}

void Context::setStencilOperations(GLenum stencilFail, GLenum stencilPassDepthFail, GLenum stencilPassDepthPass)
{
    if(mState.stencilFail != stencilFail ||
        mState.stencilPassDepthFail != stencilPassDepthFail ||
        mState.stencilPassDepthPass != stencilPassDepthPass)
    {
        mState.stencilFail = stencilFail;
        mState.stencilPassDepthFail = stencilPassDepthFail;
        mState.stencilPassDepthPass = stencilPassDepthPass;
        mStencilStateDirty = true;
    }
}

void Context::setPolygonOffsetFill(bool enabled)
{
    if(mState.polygonOffsetFill != enabled)
    {
        mState.polygonOffsetFill = enabled;
        mPolygonOffsetStateDirty = true;
    }
}

bool Context::isPolygonOffsetFillEnabled() const
{
    return mState.polygonOffsetFill;
}

void Context::setPolygonOffsetParams(GLfloat factor, GLfloat units)
{
    if(mState.polygonOffsetFactor != factor ||
        mState.polygonOffsetUnits != units)
    {
        mState.polygonOffsetFactor = factor;
        mState.polygonOffsetUnits = units;
        mPolygonOffsetStateDirty = true;
    }
}

void Context::setSampleAlphaToCoverage(bool enabled)
{
    if(mState.sampleAlphaToCoverage != enabled)
    {
        mState.sampleAlphaToCoverage = enabled;
        mSampleStateDirty = true;
    }
}

bool Context::isSampleAlphaToCoverageEnabled() const
{
    return mState.sampleAlphaToCoverage;
}

void Context::setSampleCoverage(bool enabled)
{
    if(mState.sampleCoverage != enabled)
    {
        mState.sampleCoverage = enabled;
        mSampleStateDirty = true;
    }
}

bool Context::isSampleCoverageEnabled() const
{
    return mState.sampleCoverage;
}

void Context::setSampleCoverageParams(GLclampf value, bool invert)
{
    if(mState.sampleCoverageValue != value ||
       mState.sampleCoverageInvert != invert)
    {
        mState.sampleCoverageValue = value;
        mState.sampleCoverageInvert = invert;
        mSampleStateDirty = true;
    }
}

void Context::setScissorTest(bool enabled)
{
    mState.scissorTest = enabled;
}

bool Context::isScissorTestEnabled() const
{
    return mState.scissorTest;
}

void Context::setShadeModel(GLenum mode)
{
    mState.shadeModel = mode;
}

void Context::setDither(bool enabled)
{
    if(mState.dither != enabled)
    {
        mState.dither = enabled;
        mDitherStateDirty = true;
    }
}

bool Context::isDitherEnabled() const
{
    return mState.dither;
}

void Context::setLighting(bool enable)
{
    lighting = enable;
}

void Context::setLight(int index, bool enable)
{
    light[index].enable = enable;
}

void Context::setLightAmbient(int index, float r, float g, float b, float a)
{
	light[index].ambient = {r, g, b, a};
}

void Context::setLightDiffuse(int index, float r, float g, float b, float a)
{
	light[index].diffuse = {r, g, b, a};
}

void Context::setLightSpecular(int index, float r, float g, float b, float a)
{
	light[index].specular = {r, g, b, a};
}

void Context::setLightPosition(int index, float x, float y, float z, float w)
{
	sw::float4 v = {x, y, z, w};

	// Transform from object coordinates to eye coordinates
	v = modelViewStack.current() * v;

	light[index].position = {v.x, v.y, v.z, v.w};
}

void Context::setLightDirection(int index, float x, float y, float z)
{
	// FIXME: Transform by inverse of 3x3 model-view matrix
	light[index].direction = {x, y, z};
}

void Context::setLightAttenuationConstant(int index, float constant)
{
	light[index].attenuation.constant = constant;
}

void Context::setLightAttenuationLinear(int index, float linear)
{
	light[index].attenuation.linear = linear;
}

void Context::setLightAttenuationQuadratic(int index, float quadratic)
{
	light[index].attenuation.quadratic = quadratic;
}

void Context::setGlobalAmbient(float red, float green, float blue, float alpha)
{
	globalAmbient.red = red;
	globalAmbient.green = green;
	globalAmbient.blue = blue;
	globalAmbient.alpha = alpha;
}

void Context::setMaterialAmbient(float red, float green, float blue, float alpha)
{
	materialAmbient.red = red;
	materialAmbient.green = green;
	materialAmbient.blue = blue;
	materialAmbient.alpha = alpha;
}

void Context::setMaterialDiffuse(float red, float green, float blue, float alpha)
{
	materialDiffuse.red = red;
	materialDiffuse.green = green;
	materialDiffuse.blue = blue;
	materialDiffuse.alpha = alpha;
}

void Context::setMaterialSpecular(float red, float green, float blue, float alpha)
{
	materialSpecular.red = red;
	materialSpecular.green = green;
	materialSpecular.blue = blue;
	materialSpecular.alpha = alpha;
}

void Context::setMaterialEmission(float red, float green, float blue, float alpha)
{
	materialEmission.red = red;
	materialEmission.green = green;
	materialEmission.blue = blue;
	materialEmission.alpha = alpha;
}

void Context::setMaterialShininess(float shininess)
{
	materialShininess = shininess;
}

void Context::setFog(bool enable)
{
	device->setFogEnable(enable);
}

void Context::setFogMode(GLenum mode)
{
	switch(mode)
	{
	case GL_LINEAR:
		device->setPixelFogMode(sw::FOG_LINEAR);
		break;
	case GL_EXP:
		device->setPixelFogMode(sw::FOG_EXP);
		break;
	case GL_EXP2:
		device->setPixelFogMode(sw::FOG_EXP2);
		break;
	default:
		UNREACHABLE(mode);
	}
}

void Context::setFogDensity(float fogDensity)
{
	device->setFogDensity(fogDensity);
}

void Context::setFogStart(float fogStart)
{
	device->setFogStart(fogStart);
}

void Context::setFogEnd(float fogEnd)
{
	device->setFogEnd(fogEnd);
}

void Context::setFogColor(float r, float g, float b, float a)
{
	device->setFogColor(sw::Color<float>(r, g, b, a));
}

void Context::setTexture2Denabled(bool enable)
{
    texture2Denabled[mState.activeSampler] = enable;
}

void Context::setTextureExternalEnabled(bool enable)
{
    textureExternalEnabled[mState.activeSampler] = enable;
}

void Context::setLineWidth(GLfloat width)
{
    mState.lineWidth = width;
	device->setLineWidth(clamp(width, ALIASED_LINE_WIDTH_RANGE_MIN, ALIASED_LINE_WIDTH_RANGE_MAX));
}

void Context::setGenerateMipmapHint(GLenum hint)
{
    mState.generateMipmapHint = hint;
}

void Context::setPerspectiveCorrectionHint(GLenum hint)
{
    mState.perspectiveCorrectionHint = hint;
}

void Context::setViewportParams(GLint x, GLint y, GLsizei width, GLsizei height)
{
    mState.viewportX = x;
    mState.viewportY = y;
    mState.viewportWidth = width;
    mState.viewportHeight = height;
}

void Context::setScissorParams(GLint x, GLint y, GLsizei width, GLsizei height)
{
    mState.scissorX = x;
    mState.scissorY = y;
    mState.scissorWidth = width;
    mState.scissorHeight = height;
}

void Context::setColorMask(bool red, bool green, bool blue, bool alpha)
{
    if(mState.colorMaskRed != red || mState.colorMaskGreen != green ||
       mState.colorMaskBlue != blue || mState.colorMaskAlpha != alpha)
    {
        mState.colorMaskRed = red;
        mState.colorMaskGreen = green;
        mState.colorMaskBlue = blue;
        mState.colorMaskAlpha = alpha;
        mMaskStateDirty = true;
    }
}

void Context::setDepthMask(bool mask)
{
    if(mState.depthMask != mask)
    {
        mState.depthMask = mask;
        mMaskStateDirty = true;
    }
}

void Context::setActiveSampler(unsigned int active)
{
    mState.activeSampler = active;
}

GLuint Context::getFramebufferName() const
{
    return mState.framebuffer;
}

GLuint Context::getRenderbufferName() const
{
    return mState.renderbuffer.name();
}

GLuint Context::getArrayBufferName() const
{
    return mState.arrayBuffer.name();
}

void Context::setEnableVertexAttribArray(unsigned int attribNum, bool enabled)
{
    mState.vertexAttribute[attribNum].mArrayEnabled = enabled;
}

const VertexAttribute &Context::getVertexAttribState(unsigned int attribNum)
{
    return mState.vertexAttribute[attribNum];
}

void Context::setVertexAttribState(unsigned int attribNum, Buffer *boundBuffer, GLint size, GLenum type, bool normalized,
                                   GLsizei stride, const void *pointer)
{
    mState.vertexAttribute[attribNum].mBoundBuffer = boundBuffer;
    mState.vertexAttribute[attribNum].mSize = size;
    mState.vertexAttribute[attribNum].mType = type;
    mState.vertexAttribute[attribNum].mNormalized = normalized;
    mState.vertexAttribute[attribNum].mStride = stride;
    mState.vertexAttribute[attribNum].mPointer = pointer;
}

const void *Context::getVertexAttribPointer(unsigned int attribNum) const
{
    return mState.vertexAttribute[attribNum].mPointer;
}

const VertexAttributeArray &Context::getVertexAttributes()
{
    return mState.vertexAttribute;
}

void Context::setPackAlignment(GLint alignment)
{
    mState.packAlignment = alignment;
}

GLint Context::getPackAlignment() const
{
    return mState.packAlignment;
}

void Context::setUnpackAlignment(GLint alignment)
{
    mState.unpackAlignment = alignment;
}

GLint Context::getUnpackAlignment() const
{
    return mState.unpackAlignment;
}

GLuint Context::createBuffer()
{
    return mResourceManager->createBuffer();
}

GLuint Context::createTexture()
{
    return mResourceManager->createTexture();
}

GLuint Context::createRenderbuffer()
{
    return mResourceManager->createRenderbuffer();
}

// Returns an unused framebuffer name
GLuint Context::createFramebuffer()
{
    GLuint handle = mFramebufferNameSpace.allocate();

    mFramebufferMap[handle] = NULL;

    return handle;
}

void Context::deleteBuffer(GLuint buffer)
{
    if(mResourceManager->getBuffer(buffer))
    {
        detachBuffer(buffer);
    }

    mResourceManager->deleteBuffer(buffer);
}

void Context::deleteTexture(GLuint texture)
{
    if(mResourceManager->getTexture(texture))
    {
        detachTexture(texture);
    }

    mResourceManager->deleteTexture(texture);
}

void Context::deleteRenderbuffer(GLuint renderbuffer)
{
    if(mResourceManager->getRenderbuffer(renderbuffer))
    {
        detachRenderbuffer(renderbuffer);
    }

    mResourceManager->deleteRenderbuffer(renderbuffer);
}

void Context::deleteFramebuffer(GLuint framebuffer)
{
    FramebufferMap::iterator framebufferObject = mFramebufferMap.find(framebuffer);

    if(framebufferObject != mFramebufferMap.end())
    {
        detachFramebuffer(framebuffer);

        mFramebufferNameSpace.release(framebufferObject->first);
        delete framebufferObject->second;
        mFramebufferMap.erase(framebufferObject);
    }
}

Buffer *Context::getBuffer(GLuint handle)
{
    return mResourceManager->getBuffer(handle);
}

Texture *Context::getTexture(GLuint handle)
{
    return mResourceManager->getTexture(handle);
}

Renderbuffer *Context::getRenderbuffer(GLuint handle)
{
    return mResourceManager->getRenderbuffer(handle);
}

Framebuffer *Context::getFramebuffer()
{
    return getFramebuffer(mState.framebuffer);
}

void Context::bindArrayBuffer(unsigned int buffer)
{
    mResourceManager->checkBufferAllocation(buffer);

    mState.arrayBuffer = getBuffer(buffer);
}

void Context::bindElementArrayBuffer(unsigned int buffer)
{
    mResourceManager->checkBufferAllocation(buffer);

    mState.elementArrayBuffer = getBuffer(buffer);
}

void Context::bindTexture2D(GLuint texture)
{
    mResourceManager->checkTextureAllocation(texture, TEXTURE_2D);

    mState.samplerTexture[TEXTURE_2D][mState.activeSampler] = getTexture(texture);
}

void Context::bindTextureExternal(GLuint texture)
{
    mResourceManager->checkTextureAllocation(texture, TEXTURE_EXTERNAL);

    mState.samplerTexture[TEXTURE_EXTERNAL][mState.activeSampler] = getTexture(texture);
}

void Context::bindFramebuffer(GLuint framebuffer)
{
    if(!getFramebuffer(framebuffer))
    {
        mFramebufferMap[framebuffer] = new Framebuffer();
    }

    mState.framebuffer = framebuffer;
}

void Context::bindRenderbuffer(GLuint renderbuffer)
{
    mState.renderbuffer = getRenderbuffer(renderbuffer);
}

void Context::setFramebufferZero(Framebuffer *buffer)
{
    delete mFramebufferMap[0];
    mFramebufferMap[0] = buffer;
}

void Context::setRenderbufferStorage(RenderbufferStorage *renderbuffer)
{
    Renderbuffer *renderbufferObject = mState.renderbuffer;
    renderbufferObject->setStorage(renderbuffer);
}

Framebuffer *Context::getFramebuffer(unsigned int handle)
{
    FramebufferMap::iterator framebuffer = mFramebufferMap.find(handle);

    if(framebuffer == mFramebufferMap.end())
    {
        return NULL;
    }
    else
    {
        return framebuffer->second;
    }
}

Buffer *Context::getArrayBuffer()
{
    return mState.arrayBuffer;
}

Buffer *Context::getElementArrayBuffer()
{
    return mState.elementArrayBuffer;
}

Texture2D *Context::getTexture2D()
{
    return static_cast<Texture2D*>(getSamplerTexture(mState.activeSampler, TEXTURE_2D));
}

TextureExternal *Context::getTextureExternal()
{
    return static_cast<TextureExternal*>(getSamplerTexture(mState.activeSampler, TEXTURE_EXTERNAL));
}

Texture *Context::getSamplerTexture(unsigned int sampler, TextureType type)
{
    GLuint texid = mState.samplerTexture[type][sampler].name();

    if(texid == 0)   // Special case: 0 refers to different initial textures based on the target
    {
        switch (type)
        {
        case TEXTURE_2D: return mTexture2DZero;
        case TEXTURE_EXTERNAL: return mTextureExternalZero;
        default: UNREACHABLE(type);
        }
    }

    return mState.samplerTexture[type][sampler];
}

bool Context::getBooleanv(GLenum pname, GLboolean *params)
{
    switch (pname)
    {
      case GL_SAMPLE_COVERAGE_INVERT:   *params = mState.sampleCoverageInvert;      break;
      case GL_DEPTH_WRITEMASK:          *params = mState.depthMask;                 break;
      case GL_COLOR_WRITEMASK:
        params[0] = mState.colorMaskRed;
        params[1] = mState.colorMaskGreen;
        params[2] = mState.colorMaskBlue;
        params[3] = mState.colorMaskAlpha;
        break;
      case GL_CULL_FACE:                *params = mState.cullFace;                  break;
      case GL_POLYGON_OFFSET_FILL:      *params = mState.polygonOffsetFill;         break;
      case GL_SAMPLE_ALPHA_TO_COVERAGE: *params = mState.sampleAlphaToCoverage;     break;
      case GL_SAMPLE_COVERAGE:          *params = mState.sampleCoverage;            break;
      case GL_SCISSOR_TEST:             *params = mState.scissorTest;               break;
      case GL_STENCIL_TEST:             *params = mState.stencilTest;               break;
      case GL_DEPTH_TEST:               *params = mState.depthTest;                 break;
      case GL_BLEND:                    *params = mState.blend;                     break;
      case GL_DITHER:                   *params = mState.dither;                    break;
      default:
        return false;
    }

    return true;
}

bool Context::getFloatv(GLenum pname, GLfloat *params)
{
    // Please note: DEPTH_CLEAR_VALUE is included in our internal getFloatv implementation
    // because it is stored as a float, despite the fact that the GL ES 2.0 spec names
    // GetIntegerv as its native query function. As it would require conversion in any
    // case, this should make no difference to the calling application.
    switch(pname)
    {
    case GL_LINE_WIDTH:               *params = mState.lineWidth;            break;
    case GL_SAMPLE_COVERAGE_VALUE:    *params = mState.sampleCoverageValue;  break;
    case GL_DEPTH_CLEAR_VALUE:        *params = mState.depthClearValue;      break;
    case GL_POLYGON_OFFSET_FACTOR:    *params = mState.polygonOffsetFactor;  break;
    case GL_POLYGON_OFFSET_UNITS:     *params = mState.polygonOffsetUnits;   break;
    case GL_ALIASED_LINE_WIDTH_RANGE:
        params[0] = ALIASED_LINE_WIDTH_RANGE_MIN;
        params[1] = ALIASED_LINE_WIDTH_RANGE_MAX;
        break;
    case GL_ALIASED_POINT_SIZE_RANGE:
        params[0] = ALIASED_POINT_SIZE_RANGE_MIN;
        params[1] = ALIASED_POINT_SIZE_RANGE_MAX;
        break;
    case GL_DEPTH_RANGE:
        params[0] = mState.zNear;
        params[1] = mState.zFar;
        break;
    case GL_COLOR_CLEAR_VALUE:
        params[0] = mState.colorClearValue.red;
        params[1] = mState.colorClearValue.green;
        params[2] = mState.colorClearValue.blue;
        params[3] = mState.colorClearValue.alpha;
        break;
	case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
        *params = MAX_TEXTURE_MAX_ANISOTROPY;
		break;
	case GL_MODELVIEW_MATRIX:
		for(int i = 0; i < 16; i++)
		{
			params[i] = modelViewStack.current()[i % 4][i / 4];
		}
		break;
	case GL_PROJECTION_MATRIX:
		for(int i = 0; i < 16; i++)
		{
			params[i] = projectionStack.current()[i % 4][i / 4];
		}
		break;
    default:
        return false;
    }

    return true;
}

bool Context::getIntegerv(GLenum pname, GLint *params)
{
    // Please note: DEPTH_CLEAR_VALUE is not included in our internal getIntegerv implementation
    // because it is stored as a float, despite the fact that the GL ES 2.0 spec names
    // GetIntegerv as its native query function. As it would require conversion in any
    // case, this should make no difference to the calling application. You may find it in
    // Context::getFloatv.
    switch (pname)
    {
    case GL_ARRAY_BUFFER_BINDING:             *params = mState.arrayBuffer.name();            break;
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:     *params = mState.elementArrayBuffer.name();     break;
	case GL_FRAMEBUFFER_BINDING_OES:          *params = mState.framebuffer;                   break;
    case GL_RENDERBUFFER_BINDING_OES:         *params = mState.renderbuffer.name();           break;
    case GL_PACK_ALIGNMENT:                   *params = mState.packAlignment;                 break;
    case GL_UNPACK_ALIGNMENT:                 *params = mState.unpackAlignment;               break;
    case GL_GENERATE_MIPMAP_HINT:             *params = mState.generateMipmapHint;            break;
	case GL_PERSPECTIVE_CORRECTION_HINT:      *params = mState.perspectiveCorrectionHint;     break;
    case GL_ACTIVE_TEXTURE:                   *params = (mState.activeSampler + GL_TEXTURE0); break;
    case GL_STENCIL_FUNC:                     *params = mState.stencilFunc;                   break;
    case GL_STENCIL_REF:                      *params = mState.stencilRef;                    break;
    case GL_STENCIL_VALUE_MASK:               *params = mState.stencilMask;                   break;
    case GL_STENCIL_FAIL:                     *params = mState.stencilFail;                   break;
    case GL_STENCIL_PASS_DEPTH_FAIL:          *params = mState.stencilPassDepthFail;          break;
    case GL_STENCIL_PASS_DEPTH_PASS:          *params = mState.stencilPassDepthPass;          break;
    case GL_DEPTH_FUNC:                       *params = mState.depthFunc;                     break;
    case GL_BLEND_SRC_RGB_OES:                *params = mState.sourceBlendRGB;                break;
    case GL_BLEND_SRC_ALPHA_OES:              *params = mState.sourceBlendAlpha;              break;
    case GL_BLEND_DST_RGB_OES:                *params = mState.destBlendRGB;                  break;
    case GL_BLEND_DST_ALPHA_OES:              *params = mState.destBlendAlpha;                break;
    case GL_BLEND_EQUATION_RGB_OES:           *params = mState.blendEquationRGB;              break;
    case GL_BLEND_EQUATION_ALPHA_OES:         *params = mState.blendEquationAlpha;            break;
    case GL_STENCIL_WRITEMASK:                *params = mState.stencilWritemask;              break;
    case GL_STENCIL_CLEAR_VALUE:              *params = mState.stencilClearValue;             break;
    case GL_SUBPIXEL_BITS:                    *params = 4;                                    break;
	case GL_MAX_TEXTURE_SIZE:                 *params = IMPLEMENTATION_MAX_TEXTURE_SIZE;      break;
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS:   *params = NUM_COMPRESSED_TEXTURE_FORMATS;       break;
	case GL_SAMPLE_BUFFERS:
    case GL_SAMPLES:
        {
            Framebuffer *framebuffer = getFramebuffer();
			int width, height, samples;

            if(framebuffer->completeness(width, height, samples) == GL_FRAMEBUFFER_COMPLETE_OES)
            {
                switch(pname)
                {
                case GL_SAMPLE_BUFFERS:
                    if(samples > 1)
                    {
                        *params = 1;
                    }
                    else
                    {
                        *params = 0;
                    }
                    break;
                case GL_SAMPLES:
                    *params = samples & ~1;
                    break;
                }
            }
            else
            {
                *params = 0;
            }
        }
        break;
    case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
		{
			Framebuffer *framebuffer = getFramebuffer();
			*params = framebuffer->getImplementationColorReadType();
		}
		break;
    case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
		{
			Framebuffer *framebuffer = getFramebuffer();
			*params = framebuffer->getImplementationColorReadFormat();
		}
		break;
    case GL_MAX_VIEWPORT_DIMS:
        {
			int maxDimension = IMPLEMENTATION_MAX_RENDERBUFFER_SIZE;
            params[0] = maxDimension;
            params[1] = maxDimension;
        }
        break;
    case GL_COMPRESSED_TEXTURE_FORMATS:
        {
			for(int i = 0; i < NUM_COMPRESSED_TEXTURE_FORMATS; i++)
			{
				params[i] = compressedTextureFormats[i];
			}
        }
        break;
    case GL_VIEWPORT:
        params[0] = mState.viewportX;
        params[1] = mState.viewportY;
        params[2] = mState.viewportWidth;
        params[3] = mState.viewportHeight;
        break;
    case GL_SCISSOR_BOX:
        params[0] = mState.scissorX;
        params[1] = mState.scissorY;
        params[2] = mState.scissorWidth;
        params[3] = mState.scissorHeight;
        break;
    case GL_CULL_FACE_MODE:                   *params = mState.cullMode;                 break;
    case GL_FRONT_FACE:                       *params = mState.frontFace;                break;
    case GL_RED_BITS:
    case GL_GREEN_BITS:
    case GL_BLUE_BITS:
    case GL_ALPHA_BITS:
        {
            Framebuffer *framebuffer = getFramebuffer();
            Renderbuffer *colorbuffer = framebuffer->getColorbuffer();

            if(colorbuffer)
            {
                switch (pname)
                {
                  case GL_RED_BITS:   *params = colorbuffer->getRedSize();   break;
                  case GL_GREEN_BITS: *params = colorbuffer->getGreenSize(); break;
                  case GL_BLUE_BITS:  *params = colorbuffer->getBlueSize();  break;
                  case GL_ALPHA_BITS: *params = colorbuffer->getAlphaSize(); break;
                }
            }
            else
            {
                *params = 0;
            }
        }
        break;
    case GL_DEPTH_BITS:
        {
            Framebuffer *framebuffer = getFramebuffer();
            Renderbuffer *depthbuffer = framebuffer->getDepthbuffer();

            if(depthbuffer)
            {
                *params = depthbuffer->getDepthSize();
            }
            else
            {
                *params = 0;
            }
        }
        break;
    case GL_STENCIL_BITS:
        {
            Framebuffer *framebuffer = getFramebuffer();
            Renderbuffer *stencilbuffer = framebuffer->getStencilbuffer();

            if(stencilbuffer)
            {
                *params = stencilbuffer->getStencilSize();
            }
            else
            {
                *params = 0;
            }
        }
        break;
    case GL_TEXTURE_BINDING_2D:
        {
            if(mState.activeSampler < 0 || mState.activeSampler > MAX_TEXTURE_UNITS - 1)
            {
                error(GL_INVALID_OPERATION);
                return false;
            }

            *params = mState.samplerTexture[TEXTURE_2D][mState.activeSampler].name();
        }
        break;
    case GL_TEXTURE_BINDING_CUBE_MAP_OES:
        {
            if(mState.activeSampler < 0 || mState.activeSampler > MAX_TEXTURE_UNITS - 1)
            {
                error(GL_INVALID_OPERATION);
                return false;
            }

            *params = mState.samplerTexture[TEXTURE_CUBE][mState.activeSampler].name();
        }
        break;
    case GL_TEXTURE_BINDING_EXTERNAL_OES:
        {
            if(mState.activeSampler < 0 || mState.activeSampler > MAX_TEXTURE_UNITS - 1)
            {
                error(GL_INVALID_OPERATION);
                return false;
            }

            *params = mState.samplerTexture[TEXTURE_EXTERNAL][mState.activeSampler].name();
        }
        break;
	case GL_MAX_LIGHTS:                 *params = MAX_LIGHTS;                 break;
    case GL_MAX_MODELVIEW_STACK_DEPTH:  *params = MAX_MODELVIEW_STACK_DEPTH;  break;
	case GL_MAX_PROJECTION_STACK_DEPTH: *params = MAX_PROJECTION_STACK_DEPTH; break;
	case GL_MAX_TEXTURE_STACK_DEPTH:    *params = MAX_TEXTURE_STACK_DEPTH;    break;
	case GL_MAX_TEXTURE_UNITS:          *params = MAX_TEXTURE_UNITS;          break;
	case GL_MAX_CLIP_PLANES:            *params = MAX_CLIP_PLANES;            break;
    default:
        return false;
    }

    return true;
}

int Context::getQueryParameterNum(GLenum pname)
{
    // Please note: the query type returned for DEPTH_CLEAR_VALUE in this implementation
    // is FLOAT rather than INT, as would be suggested by the GL ES 2.0 spec. This is due
    // to the fact that it is stored internally as a float, and so would require conversion
    // if returned from Context::getIntegerv. Since this conversion is already implemented
    // in the case that one calls glGetIntegerv to retrieve a float-typed state variable, we
    // place DEPTH_CLEAR_VALUE with the floats. This should make no difference to the calling
    // application.
    switch (pname)
    {
    case GL_COMPRESSED_TEXTURE_FORMATS:
		return NUM_COMPRESSED_TEXTURE_FORMATS;
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
    case GL_ARRAY_BUFFER_BINDING:
    case GL_FRAMEBUFFER_BINDING_OES:
    case GL_RENDERBUFFER_BINDING_OES:
    case GL_PACK_ALIGNMENT:
    case GL_UNPACK_ALIGNMENT:
    case GL_GENERATE_MIPMAP_HINT:
    case GL_RED_BITS:
    case GL_GREEN_BITS:
    case GL_BLUE_BITS:
    case GL_ALPHA_BITS:
    case GL_DEPTH_BITS:
    case GL_STENCIL_BITS:
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
    case GL_CULL_FACE_MODE:
    case GL_FRONT_FACE:
    case GL_ACTIVE_TEXTURE:
    case GL_STENCIL_FUNC:
    case GL_STENCIL_VALUE_MASK:
    case GL_STENCIL_REF:
    case GL_STENCIL_FAIL:
    case GL_STENCIL_PASS_DEPTH_FAIL:
    case GL_STENCIL_PASS_DEPTH_PASS:
    case GL_DEPTH_FUNC:
    case GL_BLEND_SRC_RGB_OES:
    case GL_BLEND_SRC_ALPHA_OES:
    case GL_BLEND_DST_RGB_OES:
    case GL_BLEND_DST_ALPHA_OES:
    case GL_BLEND_EQUATION_RGB_OES:
    case GL_BLEND_EQUATION_ALPHA_OES:
    case GL_STENCIL_WRITEMASK:
    case GL_STENCIL_CLEAR_VALUE:
    case GL_SUBPIXEL_BITS:
    case GL_MAX_TEXTURE_SIZE:
    case GL_MAX_CUBE_MAP_TEXTURE_SIZE_OES:
    case GL_SAMPLE_BUFFERS:
    case GL_SAMPLES:
    case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
    case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
    case GL_TEXTURE_BINDING_2D:
    case GL_TEXTURE_BINDING_CUBE_MAP_OES:
    case GL_TEXTURE_BINDING_EXTERNAL_OES:
        return 1;
    case GL_MAX_VIEWPORT_DIMS:
        return 2;
    case GL_VIEWPORT:
    case GL_SCISSOR_BOX:
        return 4;
    case GL_SAMPLE_COVERAGE_INVERT:
    case GL_DEPTH_WRITEMASK:
    case GL_CULL_FACE:                // CULL_FACE through DITHER are natural to IsEnabled,
    case GL_POLYGON_OFFSET_FILL:      // but can be retrieved through the Get{Type}v queries.
    case GL_SAMPLE_ALPHA_TO_COVERAGE: // For this purpose, they are treated here as bool-natural
    case GL_SAMPLE_COVERAGE:
    case GL_SCISSOR_TEST:
    case GL_STENCIL_TEST:
    case GL_DEPTH_TEST:
    case GL_BLEND:
    case GL_DITHER:
        return 1;
    case GL_COLOR_WRITEMASK:
        return 4;
    case GL_POLYGON_OFFSET_FACTOR:
    case GL_POLYGON_OFFSET_UNITS:
    case GL_SAMPLE_COVERAGE_VALUE:
    case GL_DEPTH_CLEAR_VALUE:
    case GL_LINE_WIDTH:
        return 1;
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_DEPTH_RANGE:
        return 2;
    case GL_COLOR_CLEAR_VALUE:
        return 4;
	case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
	case GL_MAX_LIGHTS:
	case GL_MAX_MODELVIEW_STACK_DEPTH:
	case GL_MAX_PROJECTION_STACK_DEPTH:
	case GL_MAX_TEXTURE_STACK_DEPTH:
	case GL_MAX_TEXTURE_UNITS:
	case GL_MAX_CLIP_PLANES:
        return 1;
	default:
		UNREACHABLE(pname);
    }

    return -1;
}

bool Context::isQueryParameterInt(GLenum pname)
{
    // Please note: the query type returned for DEPTH_CLEAR_VALUE in this implementation
    // is FLOAT rather than INT, as would be suggested by the GL ES 2.0 spec. This is due
    // to the fact that it is stored internally as a float, and so would require conversion
    // if returned from Context::getIntegerv. Since this conversion is already implemented
    // in the case that one calls glGetIntegerv to retrieve a float-typed state variable, we
    // place DEPTH_CLEAR_VALUE with the floats. This should make no difference to the calling
    // application.
    switch(pname)
    {
    case GL_COMPRESSED_TEXTURE_FORMATS:
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
    case GL_ARRAY_BUFFER_BINDING:
    case GL_FRAMEBUFFER_BINDING_OES:
    case GL_RENDERBUFFER_BINDING_OES:
    case GL_PACK_ALIGNMENT:
    case GL_UNPACK_ALIGNMENT:
    case GL_GENERATE_MIPMAP_HINT:
    case GL_RED_BITS:
    case GL_GREEN_BITS:
    case GL_BLUE_BITS:
    case GL_ALPHA_BITS:
    case GL_DEPTH_BITS:
    case GL_STENCIL_BITS:
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
    case GL_CULL_FACE_MODE:
    case GL_FRONT_FACE:
    case GL_ACTIVE_TEXTURE:
    case GL_STENCIL_FUNC:
    case GL_STENCIL_VALUE_MASK:
    case GL_STENCIL_REF:
    case GL_STENCIL_FAIL:
    case GL_STENCIL_PASS_DEPTH_FAIL:
    case GL_STENCIL_PASS_DEPTH_PASS:
    case GL_DEPTH_FUNC:
    case GL_BLEND_SRC_RGB_OES:
    case GL_BLEND_SRC_ALPHA_OES:
    case GL_BLEND_DST_RGB_OES:
    case GL_BLEND_DST_ALPHA_OES:
    case GL_BLEND_EQUATION_RGB_OES:
    case GL_BLEND_EQUATION_ALPHA_OES:
    case GL_STENCIL_WRITEMASK:
    case GL_STENCIL_CLEAR_VALUE:
    case GL_SUBPIXEL_BITS:
    case GL_MAX_TEXTURE_SIZE:
    case GL_MAX_CUBE_MAP_TEXTURE_SIZE_OES:
    case GL_SAMPLE_BUFFERS:
    case GL_SAMPLES:
    case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
    case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
    case GL_TEXTURE_BINDING_2D:
    case GL_TEXTURE_BINDING_CUBE_MAP_OES:
    case GL_TEXTURE_BINDING_EXTERNAL_OES:
    case GL_MAX_VIEWPORT_DIMS:
    case GL_VIEWPORT:
    case GL_SCISSOR_BOX:
	case GL_MAX_LIGHTS:
	case GL_MAX_MODELVIEW_STACK_DEPTH:
	case GL_MAX_PROJECTION_STACK_DEPTH:
	case GL_MAX_TEXTURE_STACK_DEPTH:
	case GL_MAX_TEXTURE_UNITS:
        return true;
	default:
		ASSERT(isQueryParameterFloat(pname) || isQueryParameterBool(pname));
    }

    return false;
}

bool Context::isQueryParameterFloat(GLenum pname)
{
    // Please note: the query type returned for DEPTH_CLEAR_VALUE in this implementation
    // is FLOAT rather than INT, as would be suggested by the GL ES 2.0 spec. This is due
    // to the fact that it is stored internally as a float, and so would require conversion
    // if returned from Context::getIntegerv. Since this conversion is already implemented
    // in the case that one calls glGetIntegerv to retrieve a float-typed state variable, we
    // place DEPTH_CLEAR_VALUE with the floats. This should make no difference to the calling
    // application.
    switch(pname)
    {
    case GL_POLYGON_OFFSET_FACTOR:
    case GL_POLYGON_OFFSET_UNITS:
    case GL_SAMPLE_COVERAGE_VALUE:
    case GL_DEPTH_CLEAR_VALUE:
    case GL_LINE_WIDTH:
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_DEPTH_RANGE:
    case GL_COLOR_CLEAR_VALUE:
	case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
        return true;
    default:
        ASSERT(isQueryParameterInt(pname) || isQueryParameterBool(pname));
    }

    return false;
}

bool Context::isQueryParameterBool(GLenum pname)
{
    switch(pname)
    {
    case GL_SAMPLE_COVERAGE_INVERT:
    case GL_DEPTH_WRITEMASK:
    case GL_CULL_FACE:                // CULL_FACE through DITHER are natural to IsEnabled,
    case GL_POLYGON_OFFSET_FILL:      // but can be retrieved through the Get{Type}v queries.
    case GL_SAMPLE_ALPHA_TO_COVERAGE: // For this purpose, they are treated here as bool-natural
    case GL_SAMPLE_COVERAGE:
    case GL_SCISSOR_TEST:
    case GL_STENCIL_TEST:
    case GL_DEPTH_TEST:
    case GL_BLEND:
    case GL_DITHER:
    case GL_COLOR_WRITEMASK:
        return true;
    default:
        ASSERT(isQueryParameterInt(pname) || isQueryParameterFloat(pname));
    }

    return false;
}

// Applies the render target surface, depth stencil surface, viewport rectangle and scissor rectangle
bool Context::applyRenderTarget()
{
    Framebuffer *framebuffer = getFramebuffer();
	int width, height, samples;

    if(!framebuffer || framebuffer->completeness(width, height, samples) != GL_FRAMEBUFFER_COMPLETE_OES)
    {
        return error(GL_INVALID_FRAMEBUFFER_OPERATION_OES, false);
    }

    egl::Image *renderTarget = framebuffer->getRenderTarget();
	device->setRenderTarget(renderTarget);
	if(renderTarget) renderTarget->release();

    egl::Image *depthStencil = framebuffer->getDepthStencil();
    device->setDepthStencilSurface(depthStencil);
	if(depthStencil) depthStencil->release();

    Viewport viewport;
    float zNear = clamp01(mState.zNear);
    float zFar = clamp01(mState.zFar);

    viewport.x0 = mState.viewportX;
    viewport.y0 = mState.viewportY;
    viewport.width = mState.viewportWidth;
    viewport.height = mState.viewportHeight;
    viewport.minZ = zNear;
    viewport.maxZ = zFar;

    device->setViewport(viewport);

    if(mState.scissorTest)
    {
		sw::Rect scissor = {mState.scissorX, mState.scissorY, mState.scissorX + mState.scissorWidth, mState.scissorY + mState.scissorHeight};
		scissor.clip(0, 0, width, height);

		device->setScissorRect(scissor);
        device->setScissorEnable(true);
    }
    else
    {
        device->setScissorEnable(false);
    }

    return true;
}

// Applies the fixed-function state (culling, depth test, alpha blending, stenciling, etc)
void Context::applyState(GLenum drawMode)
{
    Framebuffer *framebuffer = getFramebuffer();

    if(mState.cullFace)
    {
        device->setCullMode(es2sw::ConvertCullMode(mState.cullMode, mState.frontFace));
    }
    else
    {
		device->setCullMode(sw::CULL_NONE);
    }

    if(mDepthStateDirty)
    {
        if(mState.depthTest)
        {
			device->setDepthBufferEnable(true);
			device->setDepthCompare(es2sw::ConvertDepthComparison(mState.depthFunc));
        }
        else
        {
            device->setDepthBufferEnable(false);
        }

        mDepthStateDirty = false;
    }

    if(mBlendStateDirty)
    {
        if(mState.blend)
        {
			device->setAlphaBlendEnable(true);
			device->setSeparateAlphaBlendEnable(true);

			device->setSourceBlendFactor(es2sw::ConvertBlendFunc(mState.sourceBlendRGB));
			device->setDestBlendFactor(es2sw::ConvertBlendFunc(mState.destBlendRGB));
			device->setBlendOperation(es2sw::ConvertBlendOp(mState.blendEquationRGB));

            device->setSourceBlendFactorAlpha(es2sw::ConvertBlendFunc(mState.sourceBlendAlpha));
			device->setDestBlendFactorAlpha(es2sw::ConvertBlendFunc(mState.destBlendAlpha));
			device->setBlendOperationAlpha(es2sw::ConvertBlendOp(mState.blendEquationAlpha));
        }
        else
        {
			device->setAlphaBlendEnable(false);
        }

        mBlendStateDirty = false;
    }

    if(mStencilStateDirty || mFrontFaceDirty)
    {
        if(mState.stencilTest && framebuffer->hasStencil())
        {
			device->setStencilEnable(true);
			device->setTwoSidedStencil(true);

            // get the maximum size of the stencil ref
            Renderbuffer *stencilbuffer = framebuffer->getStencilbuffer();
            GLuint maxStencil = (1 << stencilbuffer->getStencilSize()) - 1;

			device->setStencilWriteMask(mState.stencilWritemask);
			device->setStencilCompare(es2sw::ConvertStencilComparison(mState.stencilFunc));

			device->setStencilReference((mState.stencilRef < (GLint)maxStencil) ? mState.stencilRef : maxStencil);
			device->setStencilMask(mState.stencilMask);

			device->setStencilFailOperation(es2sw::ConvertStencilOp(mState.stencilFail));
			device->setStencilZFailOperation(es2sw::ConvertStencilOp(mState.stencilPassDepthFail));
			device->setStencilPassOperation(es2sw::ConvertStencilOp(mState.stencilPassDepthPass));

			device->setStencilWriteMaskCCW(mState.stencilWritemask);
			device->setStencilCompareCCW(es2sw::ConvertStencilComparison(mState.stencilFunc));

			device->setStencilReferenceCCW((mState.stencilRef < (GLint)maxStencil) ? mState.stencilRef : maxStencil);
			device->setStencilMaskCCW(mState.stencilMask);

			device->setStencilFailOperationCCW(es2sw::ConvertStencilOp(mState.stencilFail));
			device->setStencilZFailOperationCCW(es2sw::ConvertStencilOp(mState.stencilPassDepthFail));
			device->setStencilPassOperationCCW(es2sw::ConvertStencilOp(mState.stencilPassDepthPass));
        }
        else
        {
			device->setStencilEnable(false);
        }

        mStencilStateDirty = false;
        mFrontFaceDirty = false;
    }

    if(mMaskStateDirty)
    {
		device->setColorWriteMask(0, es2sw::ConvertColorMask(mState.colorMaskRed, mState.colorMaskGreen, mState.colorMaskBlue, mState.colorMaskAlpha));
		device->setDepthWriteEnable(mState.depthMask);

        mMaskStateDirty = false;
    }

    if(mPolygonOffsetStateDirty)
    {
        if(mState.polygonOffsetFill)
        {
            Renderbuffer *depthbuffer = framebuffer->getDepthbuffer();
            if(depthbuffer)
            {
				device->setSlopeDepthBias(mState.polygonOffsetFactor);
                float depthBias = ldexp(mState.polygonOffsetUnits, -(int)(depthbuffer->getDepthSize()));
				device->setDepthBias(depthBias);
            }
        }
        else
        {
            device->setSlopeDepthBias(0);
            device->setDepthBias(0);
        }

        mPolygonOffsetStateDirty = false;
    }

    if(mSampleStateDirty)
    {
        if(mState.sampleAlphaToCoverage)
        {
            device->setTransparencyAntialiasing(sw::TRANSPARENCY_ALPHA_TO_COVERAGE);
        }
		else
		{
			device->setTransparencyAntialiasing(sw::TRANSPARENCY_NONE);
		}

        if(mState.sampleCoverage)
        {
            unsigned int mask = 0;
            if(mState.sampleCoverageValue != 0)
            {
				int width, height, samples;
				framebuffer->completeness(width, height, samples);

                float threshold = 0.5f;

                for(int i = 0; i < samples; i++)
                {
                    mask <<= 1;

                    if((i + 1) * mState.sampleCoverageValue >= threshold)
                    {
                        threshold += 1.0f;
                        mask |= 1;
                    }
                }
            }

            if(mState.sampleCoverageInvert)
            {
                mask = ~mask;
            }

			device->setMultiSampleMask(mask);
        }
        else
        {
			device->setMultiSampleMask(0xFFFFFFFF);
        }

        mSampleStateDirty = false;
    }

    if(mDitherStateDirty)
    {
    //	UNIMPLEMENTED();   // FIXME

        mDitherStateDirty = false;
    }

	switch(mState.shadeModel)
	{
	default: UNREACHABLE(mState.shadeModel);
	case GL_SMOOTH: device->setShadingMode(sw::SHADING_GOURAUD); break;
	case GL_FLAT:   device->setShadingMode(sw::SHADING_FLAT);    break;
	}

	device->setLightingEnable(lighting);
	device->setGlobalAmbient(sw::Color<float>(globalAmbient.red, globalAmbient.green, globalAmbient.blue, globalAmbient.alpha));

	for(int i = 0; i < MAX_LIGHTS; i++)
	{
		device->setLightEnable(i, light[i].enable);
		device->setLightAmbient(i, sw::Color<float>(light[i].ambient.red, light[i].ambient.green, light[i].ambient.blue, light[i].ambient.alpha));
		device->setLightDiffuse(i, sw::Color<float>(light[i].diffuse.red, light[i].diffuse.green, light[i].diffuse.blue, light[i].diffuse.alpha));
		device->setLightSpecular(i, sw::Color<float>(light[i].specular.red, light[i].specular.green, light[i].specular.blue, light[i].specular.alpha));
		device->setLightAttenuation(i, light[i].attenuation.constant, light[i].attenuation.linear, light[i].attenuation.quadratic);

		if(light[i].position.w != 0.0f)
		{
			device->setLightPosition(i, sw::Point(light[i].position.x / light[i].position.w, light[i].position.y / light[i].position.w, light[i].position.z / light[i].position.w));
		}
		else   // Directional light
		{
			// Hack: set the position far way
			float max = std::max(std::max(abs(light[i].position.x), abs(light[i].position.y)), abs(light[i].position.z));
			device->setLightPosition(i, sw::Point(1e10f * (light[i].position.x / max), 1e10f * (light[i].position.y / max), 1e10f * (light[i].position.z / max)));
		}
	}

	device->setMaterialAmbient(sw::Color<float>(materialAmbient.red, materialAmbient.green, materialAmbient.blue, materialAmbient.alpha));
	device->setMaterialDiffuse(sw::Color<float>(materialDiffuse.red, materialDiffuse.green, materialDiffuse.blue, materialDiffuse.alpha));
	device->setMaterialSpecular(sw::Color<float>(materialSpecular.red, materialSpecular.green, materialSpecular.blue, materialSpecular.alpha));
	device->setMaterialEmission(sw::Color<float>(materialEmission.red, materialEmission.green, materialEmission.blue, materialEmission.alpha));
	device->setMaterialShininess(materialShininess);

    device->setDiffuseMaterialSource(sw::MATERIAL_MATERIAL);
	device->setSpecularMaterialSource(sw::MATERIAL_MATERIAL);
	device->setAmbientMaterialSource(sw::MATERIAL_MATERIAL);
	device->setEmissiveMaterialSource(sw::MATERIAL_MATERIAL);

	const sw::Matrix Z(1, 0, 0, 0,
	                   0, 1, 0, 0,
	                   0, 0, 0.5, 0.5,
	                   0, 0, 0, 1);   // Map depth range from [-1, 1] to [0, 1]

    device->setProjectionMatrix(Z * projectionStack.current());
    device->setModelMatrix(modelViewStack.current());
    device->setTextureMatrix(0, textureStack0.current());
	device->setTextureMatrix(1, textureStack1.current());
	device->setTextureTransform(0, textureStack0.isIdentity() ? 0 : 4, false);
	device->setTextureTransform(1, textureStack1.isIdentity() ? 0 : 4, false);
	device->setTexGen(0, sw::TEXGEN_NONE);
	device->setTexGen(1, sw::TEXGEN_NONE);

	device->setAlphaTestEnable(alphaTest);
	device->setAlphaCompare(es2sw::ConvertAlphaComparison(alphaTestFunc));
	device->setAlphaReference(alphaTestRef * 0xFF);
}

GLenum Context::applyVertexBuffer(GLint base, GLint first, GLsizei count)
{
    TranslatedAttribute attributes[MAX_VERTEX_ATTRIBS];

    GLenum err = mVertexDataManager->prepareVertexData(first, count, attributes);
    if(err != GL_NO_ERROR)
    {
        return err;
    }

	device->resetInputStreams(false);

    for(int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
	{
		sw::Resource *resource = attributes[i].vertexBuffer;
		const void *buffer = (char*)resource->data() + attributes[i].offset;

		int stride = attributes[i].stride;

		buffer = (char*)buffer + stride * base;

		sw::Stream attribute(resource, buffer, stride);

		attribute.type = attributes[i].type;
		attribute.count = attributes[i].count;
		attribute.normalized = attributes[i].normalized;

		device->setInputStream(i, attribute);
	}

	return GL_NO_ERROR;
}

// Applies the indices and element array bindings
GLenum Context::applyIndexBuffer(const void *indices, GLsizei count, GLenum mode, GLenum type, TranslatedIndexData *indexInfo)
{
    GLenum err = mIndexDataManager->prepareIndexData(type, count, mState.elementArrayBuffer, indices, indexInfo);

    if(err == GL_NO_ERROR)
    {
        device->setIndexBuffer(indexInfo->indexBuffer);
    }

    return err;
}

void Context::applyTextures()
{
	for(int unit = 0; unit < MAX_TEXTURE_UNITS; unit++)
    {
        Texture *texture = nullptr;
		
		if(textureExternalEnabled[unit])
		{
			texture = getSamplerTexture(unit, TEXTURE_EXTERNAL);
		}
		else if(texture2Denabled[unit])
		{
			texture = getSamplerTexture(unit, TEXTURE_2D);
		}

		if(texture && texture->isSamplerComplete())
        {
			texture->autoGenerateMipmaps();

            GLenum wrapS = texture->getWrapS();
            GLenum wrapT = texture->getWrapT();
            GLenum texFilter = texture->getMinFilter();
            GLenum magFilter = texture->getMagFilter();
			GLfloat maxAnisotropy = texture->getMaxAnisotropy();

			device->setAddressingModeU(sw::SAMPLER_PIXEL, unit, es2sw::ConvertTextureWrap(wrapS));
            device->setAddressingModeV(sw::SAMPLER_PIXEL, unit, es2sw::ConvertTextureWrap(wrapT));

			sw::FilterType minFilter;
			sw::MipmapType mipFilter;
            es2sw::ConvertMinFilter(texFilter, &minFilter, &mipFilter, maxAnisotropy);
		//	ASSERT(minFilter == es2sw::ConvertMagFilter(magFilter));

			device->setTextureFilter(sw::SAMPLER_PIXEL, unit, minFilter);
		//	device->setTextureFilter(sw::SAMPLER_PIXEL, unit, es2sw::ConvertMagFilter(magFilter));
			device->setMipmapFilter(sw::SAMPLER_PIXEL, unit, mipFilter);
			device->setMaxAnisotropy(sw::SAMPLER_PIXEL, unit, maxAnisotropy);

			applyTexture(unit, texture);

			if(mState.textureUnit[unit].environmentMode != GL_COMBINE)
			{
				device->setFirstArgument(unit, sw::TextureStage::SOURCE_TEXTURE);    // Cs
				device->setFirstModifier(unit, sw::TextureStage::MODIFIER_COLOR);
				device->setSecondArgument(unit, sw::TextureStage::SOURCE_CURRENT);   // Cp
				device->setSecondModifier(unit, sw::TextureStage::MODIFIER_COLOR);
				device->setThirdArgument(unit, sw::TextureStage::SOURCE_CONSTANT);   // Cc
				device->setThirdModifier(unit, sw::TextureStage::MODIFIER_COLOR);

				device->setFirstArgumentAlpha(unit, sw::TextureStage::SOURCE_TEXTURE);    // As
				device->setFirstModifierAlpha(unit, sw::TextureStage::MODIFIER_ALPHA);
				device->setSecondArgumentAlpha(unit, sw::TextureStage::SOURCE_CURRENT);   // Ap
				device->setSecondModifierAlpha(unit, sw::TextureStage::MODIFIER_ALPHA);
				device->setThirdArgumentAlpha(unit, sw::TextureStage::SOURCE_CONSTANT);   // Ac
				device->setThirdModifierAlpha(unit, sw::TextureStage::MODIFIER_ALPHA);

				GLenum texFormat = texture->getFormat(GL_TEXTURE_2D, 0);

				switch(mState.textureUnit[unit].environmentMode)
				{
				case GL_REPLACE:
					if(IsAlpha(texFormat))   // GL_ALPHA
					{
						// Cv = Cp, Av = As
						device->setStageOperation(unit, sw::TextureStage::STAGE_SELECTARG2);
						device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_SELECTARG1);
					}
					else if(IsRGB(texFormat))   // GL_LUMINANCE (or 1) / GL_RGB (or 3)
					{
						// Cv = Cs, Av = Ap
						device->setStageOperation(unit, sw::TextureStage::STAGE_SELECTARG1);
						device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_SELECTARG2);
					}
					else if(IsRGBA(texFormat))   // GL_LUMINANCE_ALPHA (or 2) / GL_RGBA (or 4)
					{
						// Cv = Cs, Av = As
						device->setStageOperation(unit, sw::TextureStage::STAGE_SELECTARG1);
						device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_SELECTARG1);
					}
					else UNREACHABLE(texFormat);
					break;
				case GL_MODULATE:
					if(IsAlpha(texFormat))   // GL_ALPHA
					{
						// Cv = Cp, Av = ApAs
						device->setStageOperation(unit, sw::TextureStage::STAGE_SELECTARG2);
						device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_MODULATE);
					}
					else if(IsRGB(texFormat))   // GL_LUMINANCE (or 1) / GL_RGB (or 3)
					{
						// Cv = CpCs, Av = Ap
						device->setStageOperation(unit, sw::TextureStage::STAGE_MODULATE);
						device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_SELECTARG2);
					}
					else if(IsRGBA(texFormat))   // GL_LUMINANCE_ALPHA (or 2) / GL_RGBA (or 4)
					{
						// Cv = CpCs, Av = ApAs
						device->setStageOperation(unit, sw::TextureStage::STAGE_MODULATE);
						device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_MODULATE);
					}
					else UNREACHABLE(texFormat);
					break;
				case GL_DECAL:
					if(texFormat == GL_ALPHA ||
					   texFormat == GL_LUMINANCE ||
					   texFormat == GL_LUMINANCE_ALPHA)
					{
						// undefined   // FIXME: Log
						device->setStageOperation(unit, sw::TextureStage::STAGE_SELECTARG2);
						device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_SELECTARG2);
					}
					else if(IsRGB(texFormat))   // GL_LUMINANCE (or 1) / GL_RGB (or 3)
					{
						// Cv = Cs, Av = Ap
						device->setStageOperation(unit, sw::TextureStage::STAGE_SELECTARG1);
						device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_SELECTARG2);
					}
					else if(IsRGBA(texFormat))   // GL_LUMINANCE_ALPHA (or 2) / GL_RGBA (or 4)
					{
						// Cv = Cp(1 - As) + CsAs, Av = Ap
						device->setStageOperation(unit, sw::TextureStage::STAGE_BLENDTEXTUREALPHA);   // Alpha * (Arg1 - Arg2) + Arg2
						device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_SELECTARG2);
					}
					else UNREACHABLE(texFormat);
					break;
				case GL_BLEND:
					if(IsAlpha(texFormat))   // GL_ALPHA
					{
						// Cv = Cp, Av = ApAs
						device->setStageOperation(unit, sw::TextureStage::STAGE_SELECTARG2);
						device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_MODULATE);
					}
					else if(IsRGB(texFormat))   // GL_LUMINANCE (or 1) / GL_RGB (or 3)
					{
						// Cv = Cp(1 - Cs) + CcCs, Av = Ap
						device->setStageOperation(unit, sw::TextureStage::STAGE_LERP);   // Arg3 * (Arg1 - Arg2) + Arg2
						device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_SELECTARG2);
					}
					else if(IsRGBA(texFormat))   // GL_LUMINANCE_ALPHA (or 2) / GL_RGBA (or 4)
					{
						// Cv = Cp(1 - Cs) + CcCs, Av = ApAs
						device->setStageOperation(unit, sw::TextureStage::STAGE_LERP);   // Arg3 * (Arg1 - Arg2) + Arg2
						device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_MODULATE);
					}
					else UNREACHABLE(texFormat);
					break;
				case GL_ADD:
					if(IsAlpha(texFormat))   // GL_ALPHA
					{
						// Cv = Cp, Av = ApAs
						device->setStageOperation(unit, sw::TextureStage::STAGE_SELECTARG2);
						device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_MODULATE);
					}
					else if(IsRGB(texFormat))   // GL_LUMINANCE (or 1) / GL_RGB (or 3)
					{
						// Cv = Cp + Cs, Av = Ap
						device->setStageOperation(unit, sw::TextureStage::STAGE_ADD);
						device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_SELECTARG2);
					}
					else if(IsRGBA(texFormat))   // GL_LUMINANCE_ALPHA (or 2) / GL_RGBA (or 4)
					{
						// Cv = Cp + Cs, Av = ApAs
						device->setStageOperation(unit, sw::TextureStage::STAGE_ADD);
						device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_MODULATE);
					}
					else UNREACHABLE(texFormat);
					break;
				default:
					UNREACHABLE(mState.textureUnit[unit].environmentMode);
				}
			}
			else   // GL_COMBINE
			{
				device->setFirstArgument(unit, es2sw::ConvertSourceArgument(mState.textureUnit[unit].src0RGB));
				device->setFirstModifier(unit, es2sw::ConvertSourceOperand(mState.textureUnit[unit].operand0RGB));
				device->setSecondArgument(unit, es2sw::ConvertSourceArgument(mState.textureUnit[unit].src1RGB));
				device->setSecondModifier(unit, es2sw::ConvertSourceOperand(mState.textureUnit[unit].operand1RGB));
				device->setThirdArgument(unit, es2sw::ConvertSourceArgument(mState.textureUnit[unit].src2RGB));
				device->setThirdModifier(unit, es2sw::ConvertSourceOperand(mState.textureUnit[unit].operand2RGB));

				device->setStageOperation(unit, es2sw::ConvertCombineOperation(mState.textureUnit[unit].combineRGB));

				device->setFirstArgumentAlpha(unit, es2sw::ConvertSourceArgument(mState.textureUnit[unit].src0Alpha));
				device->setFirstModifierAlpha(unit, es2sw::ConvertSourceOperand(mState.textureUnit[unit].operand0Alpha));
				device->setSecondArgumentAlpha(unit, es2sw::ConvertSourceArgument(mState.textureUnit[unit].src1Alpha));
				device->setSecondModifierAlpha(unit, es2sw::ConvertSourceOperand(mState.textureUnit[unit].operand1Alpha));
				device->setThirdArgumentAlpha(unit, es2sw::ConvertSourceArgument(mState.textureUnit[unit].src2Alpha));
				device->setThirdModifierAlpha(unit, es2sw::ConvertSourceOperand(mState.textureUnit[unit].operand2Alpha));

				device->setStageOperationAlpha(unit, es2sw::ConvertCombineOperation(mState.textureUnit[unit].combineAlpha));
			}
        }
        else
        {
            applyTexture(unit, 0);

			device->setFirstArgument(unit, sw::TextureStage::SOURCE_CURRENT);
			device->setFirstModifier(unit, sw::TextureStage::MODIFIER_COLOR);
			device->setStageOperation(unit, sw::TextureStage::STAGE_SELECTARG1);

			device->setFirstArgumentAlpha(unit, sw::TextureStage::SOURCE_CURRENT);
			device->setFirstModifierAlpha(unit, sw::TextureStage::MODIFIER_ALPHA);
			device->setStageOperationAlpha(unit, sw::TextureStage::STAGE_SELECTARG1);
        }
    }
}

void Context::setTextureEnvMode(GLenum texEnvMode)
{
	mState.textureUnit[mState.activeSampler].environmentMode = texEnvMode;
}

void Context::setCombineRGB(GLenum combineRGB)
{
	mState.textureUnit[mState.activeSampler].combineRGB = combineRGB;
}

void Context::setCombineAlpha(GLenum combineAlpha)
{
	mState.textureUnit[mState.activeSampler].combineAlpha = combineAlpha;
}

void Context::applyTexture(int index, Texture *baseTexture)
{
	sw::Resource *resource = 0;

	if(baseTexture)
	{
		resource = baseTexture->getResource();
	}

	device->setTextureResource(index, resource);

	if(baseTexture)
	{
		int levelCount = baseTexture->getLevelCount();

		if(baseTexture->getTarget() == GL_TEXTURE_2D || baseTexture->getTarget() == GL_TEXTURE_EXTERNAL_OES)
		{
			Texture2D *texture = static_cast<Texture2D*>(baseTexture);

			for(int mipmapLevel = 0; mipmapLevel < MIPMAP_LEVELS; mipmapLevel++)
			{
				int surfaceLevel = mipmapLevel;

				if(surfaceLevel < 0)
				{
					surfaceLevel = 0;
				}
				else if(surfaceLevel >= levelCount)
				{
					surfaceLevel = levelCount - 1;
				}

				egl::Image *surface = texture->getImage(surfaceLevel);
				device->setTextureLevel(index, 0, mipmapLevel, surface, sw::TEXTURE_2D);
			}
		}
		else UNIMPLEMENTED();
	}
	else
	{
		device->setTextureLevel(index, 0, 0, 0, sw::TEXTURE_NULL);
	}
}

void Context::readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                         GLenum format, GLenum type, GLsizei *bufSize, void* pixels)
{
    Framebuffer *framebuffer = getFramebuffer();
	int framebufferWidth, framebufferHeight, framebufferSamples;

    if(framebuffer->completeness(framebufferWidth, framebufferHeight, framebufferSamples) != GL_FRAMEBUFFER_COMPLETE_OES)
    {
        return error(GL_INVALID_FRAMEBUFFER_OPERATION_OES);
    }

    if(getFramebufferName() != 0 && framebufferSamples != 0)
    {
        return error(GL_INVALID_OPERATION);
    }

	if(format != GL_RGBA || type != GL_UNSIGNED_BYTE)
	{
		if(format != framebuffer->getImplementationColorReadFormat() || type != framebuffer->getImplementationColorReadType())
		{
			return error(GL_INVALID_OPERATION);
		}
	}

	GLsizei outputPitch = egl::ComputePitch(width, format, type, mState.packAlignment);

	// Sized query sanity check
    if(bufSize)
    {
        int requiredSize = outputPitch * height;
        if(requiredSize > *bufSize)
        {
            return error(GL_INVALID_OPERATION);
        }
    }

    egl::Image *renderTarget = framebuffer->getRenderTarget();

    if(!renderTarget)
    {
        return error(GL_OUT_OF_MEMORY);
    }

	sw::Rect rect = {x, y, x + width, y + height};
	rect.clip(0, 0, renderTarget->getWidth(), renderTarget->getHeight());

    unsigned char *source = (unsigned char*)renderTarget->lock(rect.x0, rect.y0, sw::LOCK_READONLY);
    unsigned char *dest = (unsigned char*)pixels;
    int inputPitch = (int)renderTarget->getPitch();

    for(int j = 0; j < rect.y1 - rect.y0; j++)
    {
		unsigned short *dest16 = (unsigned short*)dest;
		unsigned int *dest32 = (unsigned int*)dest;

		if(renderTarget->getInternalFormat() == sw::FORMAT_A8B8G8R8 &&
           format == GL_RGBA && type == GL_UNSIGNED_BYTE)
        {
            memcpy(dest, source, (rect.x1 - rect.x0) * 4);
        }
		else if(renderTarget->getInternalFormat() == sw::FORMAT_A8R8G8B8 &&
                format == GL_RGBA && type == GL_UNSIGNED_BYTE)
        {
            for(int i = 0; i < rect.x1 - rect.x0; i++)
			{
				unsigned int argb = *(unsigned int*)(source + 4 * i);

				dest32[i] = (argb & 0xFF00FF00) | ((argb & 0x000000FF) << 16) | ((argb & 0x00FF0000) >> 16);
			}
        }
		else if(renderTarget->getInternalFormat() == sw::FORMAT_X8R8G8B8 &&
                format == GL_RGBA && type == GL_UNSIGNED_BYTE)
        {
            for(int i = 0; i < rect.x1 - rect.x0; i++)
			{
				unsigned int xrgb = *(unsigned int*)(source + 4 * i);

				dest32[i] = (xrgb & 0xFF00FF00) | ((xrgb & 0x000000FF) << 16) | ((xrgb & 0x00FF0000) >> 16) | 0xFF000000;
			}
        }
		else if(renderTarget->getInternalFormat() == sw::FORMAT_X8R8G8B8 &&
                format == GL_BGRA_EXT && type == GL_UNSIGNED_BYTE)
        {
            for(int i = 0; i < rect.x1 - rect.x0; i++)
			{
				unsigned int xrgb = *(unsigned int*)(source + 4 * i);

				dest32[i] = xrgb | 0xFF000000;
			}
        }
        else if(renderTarget->getInternalFormat() == sw::FORMAT_A8R8G8B8 &&
                format == GL_BGRA_EXT && type == GL_UNSIGNED_BYTE)
        {
            memcpy(dest, source, (rect.x1 - rect.x0) * 4);
        }
		else if(renderTarget->getInternalFormat() == sw::FORMAT_A1R5G5B5 &&
                format == GL_BGRA_EXT && type == GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT)
        {
            memcpy(dest, source, (rect.x1 - rect.x0) * 2);
        }
		else if(renderTarget->getInternalFormat() == sw::FORMAT_R5G6B5 &&
                format == 0x80E0 && type == GL_UNSIGNED_SHORT_5_6_5)   // GL_BGR_EXT
        {
            memcpy(dest, source, (rect.x1 - rect.x0) * 2);
        }
		else
		{
			for(int i = 0; i < rect.x1 - rect.x0; i++)
			{
				float r;
				float g;
				float b;
				float a;

				switch(renderTarget->getInternalFormat())
				{
				case sw::FORMAT_R5G6B5:
					{
						unsigned short rgb = *(unsigned short*)(source + 2 * i);

						a = 1.0f;
						b = (rgb & 0x001F) * (1.0f / 0x001F);
						g = (rgb & 0x07E0) * (1.0f / 0x07E0);
						r = (rgb & 0xF800) * (1.0f / 0xF800);
					}
					break;
				case sw::FORMAT_A1R5G5B5:
					{
						unsigned short argb = *(unsigned short*)(source + 2 * i);

						a = (argb & 0x8000) ? 1.0f : 0.0f;
						b = (argb & 0x001F) * (1.0f / 0x001F);
						g = (argb & 0x03E0) * (1.0f / 0x03E0);
						r = (argb & 0x7C00) * (1.0f / 0x7C00);
					}
					break;
				case sw::FORMAT_A8R8G8B8:
					{
						unsigned int argb = *(unsigned int*)(source + 4 * i);

						a = (argb & 0xFF000000) * (1.0f / 0xFF000000);
						b = (argb & 0x000000FF) * (1.0f / 0x000000FF);
						g = (argb & 0x0000FF00) * (1.0f / 0x0000FF00);
						r = (argb & 0x00FF0000) * (1.0f / 0x00FF0000);
					}
					break;
				case sw::FORMAT_A8B8G8R8:
					{
						unsigned int abgr = *(unsigned int*)(source + 4 * i);

						a = (abgr & 0xFF000000) * (1.0f / 0xFF000000);
						b = (abgr & 0x00FF0000) * (1.0f / 0x00FF0000);
						g = (abgr & 0x0000FF00) * (1.0f / 0x0000FF00);
						r = (abgr & 0x000000FF) * (1.0f / 0x000000FF);
					}
					break;
				case sw::FORMAT_X8R8G8B8:
					{
						unsigned int xrgb = *(unsigned int*)(source + 4 * i);

						a = 1.0f;
						b = (xrgb & 0x000000FF) * (1.0f / 0x000000FF);
						g = (xrgb & 0x0000FF00) * (1.0f / 0x0000FF00);
						r = (xrgb & 0x00FF0000) * (1.0f / 0x00FF0000);
					}
					break;
				case sw::FORMAT_X8B8G8R8:
					{
						unsigned int xbgr = *(unsigned int*)(source + 4 * i);

						a = 1.0f;
						b = (xbgr & 0x00FF0000) * (1.0f / 0x00FF0000);
						g = (xbgr & 0x0000FF00) * (1.0f / 0x0000FF00);
						r = (xbgr & 0x000000FF) * (1.0f / 0x000000FF);
					}
					break;
				case sw::FORMAT_A2R10G10B10:
					{
						unsigned int argb = *(unsigned int*)(source + 4 * i);

						a = (argb & 0xC0000000) * (1.0f / 0xC0000000);
						b = (argb & 0x000003FF) * (1.0f / 0x000003FF);
						g = (argb & 0x000FFC00) * (1.0f / 0x000FFC00);
						r = (argb & 0x3FF00000) * (1.0f / 0x3FF00000);
					}
					break;
				default:
					UNIMPLEMENTED();   // FIXME
					UNREACHABLE(renderTarget->getInternalFormat());
				}

				switch(format)
				{
				case GL_RGBA:
					switch(type)
					{
					case GL_UNSIGNED_BYTE:
						dest[4 * i + 0] = (unsigned char)(255 * r + 0.5f);
						dest[4 * i + 1] = (unsigned char)(255 * g + 0.5f);
						dest[4 * i + 2] = (unsigned char)(255 * b + 0.5f);
						dest[4 * i + 3] = (unsigned char)(255 * a + 0.5f);
						break;
					default: UNREACHABLE(type);
					}
					break;
				case GL_BGRA_EXT:
					switch(type)
					{
					case GL_UNSIGNED_BYTE:
						dest[4 * i + 0] = (unsigned char)(255 * b + 0.5f);
						dest[4 * i + 1] = (unsigned char)(255 * g + 0.5f);
						dest[4 * i + 2] = (unsigned char)(255 * r + 0.5f);
						dest[4 * i + 3] = (unsigned char)(255 * a + 0.5f);
						break;
					case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT:
						// According to the desktop GL spec in the "Transfer of Pixel Rectangles" section
						// this type is packed as follows:
						//   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0
						//  --------------------------------------------------------------------------------
						// |       4th         |        3rd         |        2nd        |   1st component   |
						//  --------------------------------------------------------------------------------
						// in the case of BGRA_EXT, B is the first component, G the second, and so forth.
						dest16[i] =
							((unsigned short)(15 * a + 0.5f) << 12)|
							((unsigned short)(15 * r + 0.5f) << 8) |
							((unsigned short)(15 * g + 0.5f) << 4) |
							((unsigned short)(15 * b + 0.5f) << 0);
						break;
					case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:
						// According to the desktop GL spec in the "Transfer of Pixel Rectangles" section
						// this type is packed as follows:
						//   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0
						//  --------------------------------------------------------------------------------
						// | 4th |          3rd           |           2nd          |      1st component     |
						//  --------------------------------------------------------------------------------
						// in the case of BGRA_EXT, B is the first component, G the second, and so forth.
						dest16[i] =
							((unsigned short)(     a + 0.5f) << 15) |
							((unsigned short)(31 * r + 0.5f) << 10) |
							((unsigned short)(31 * g + 0.5f) << 5) |
							((unsigned short)(31 * b + 0.5f) << 0);
						break;
					default: UNREACHABLE(type);
					}
					break;
				case GL_RGB:
					switch(type)
					{
					case GL_UNSIGNED_SHORT_5_6_5:
						dest16[i] =
							((unsigned short)(31 * b + 0.5f) << 0) |
							((unsigned short)(63 * g + 0.5f) << 5) |
							((unsigned short)(31 * r + 0.5f) << 11);
						break;
					default: UNREACHABLE(type);
					}
					break;
				default: UNREACHABLE(format);
				}
			}
        }

		source += inputPitch;
		dest += outputPitch;
    }

	renderTarget->unlock();
	renderTarget->release();
}

void Context::clear(GLbitfield mask)
{
    Framebuffer *framebuffer = getFramebuffer();

    if(!framebuffer || framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE_OES)
    {
        return error(GL_INVALID_FRAMEBUFFER_OPERATION_OES);
    }

    if(!applyRenderTarget())
    {
        return;
    }

	unsigned int color = (unorm<8>(mState.colorClearValue.alpha) << 24) |
                         (unorm<8>(mState.colorClearValue.red) << 16) |
                         (unorm<8>(mState.colorClearValue.green) << 8) |
                         (unorm<8>(mState.colorClearValue.blue) << 0);
    float depth = clamp01(mState.depthClearValue);
    int stencil = mState.stencilClearValue & 0x000000FF;

	if(mask & GL_COLOR_BUFFER_BIT)
	{
		unsigned int rgbaMask = (mState.colorMaskRed ? 0x1 : 0) |
		                        (mState.colorMaskGreen ? 0x2 : 0) |
		                        (mState.colorMaskBlue ? 0x4 : 0) |
		                        (mState.colorMaskAlpha ? 0x8 : 0);

		if(rgbaMask != 0)
		{
			device->clearColor(color, rgbaMask);
		}
	}

	if(mask & GL_DEPTH_BUFFER_BIT)
	{
		if(mState.depthMask != 0)
		{
			device->clearDepth(depth);
		}
	}

	if(mask & GL_STENCIL_BUFFER_BIT)
	{
		if(mState.stencilWritemask != 0)
		{
			device->clearStencil(stencil, mState.stencilWritemask);
		}
	}
}

void Context::drawArrays(GLenum mode, GLint first, GLsizei count)
{
    PrimitiveType primitiveType;
    int primitiveCount;

    if(!es2sw::ConvertPrimitiveType(mode, count, primitiveType, primitiveCount))
        return error(GL_INVALID_ENUM);

    if(primitiveCount <= 0)
    {
        return;
    }

    if(!applyRenderTarget())
    {
        return;
    }

    applyState(mode);

    GLenum err = applyVertexBuffer(0, first, count);
    if(err != GL_NO_ERROR)
    {
        return error(err);
    }

    applyTextures();

    if(!cullSkipsDraw(mode))
    {
        device->drawPrimitive(primitiveType, primitiveCount);
    }
}

void Context::drawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
    if(!indices && !mState.elementArrayBuffer)
    {
        return error(GL_INVALID_OPERATION);
    }

    PrimitiveType primitiveType;
    int primitiveCount;

    if(!es2sw::ConvertPrimitiveType(mode, count, primitiveType, primitiveCount))
        return error(GL_INVALID_ENUM);

    if(primitiveCount <= 0)
    {
        return;
    }

    if(!applyRenderTarget())
    {
        return;
    }

    applyState(mode);

    TranslatedIndexData indexInfo;
    GLenum err = applyIndexBuffer(indices, count, mode, type, &indexInfo);
    if(err != GL_NO_ERROR)
    {
        return error(err);
    }

    GLsizei vertexCount = indexInfo.maxIndex - indexInfo.minIndex + 1;
    err = applyVertexBuffer(-(int)indexInfo.minIndex, indexInfo.minIndex, vertexCount);
    if(err != GL_NO_ERROR)
    {
        return error(err);
    }

    applyTextures();

    if(!cullSkipsDraw(mode))
    {
		device->drawIndexedPrimitive(primitiveType, indexInfo.indexOffset, primitiveCount, IndexDataManager::typeSize(type));
    }
}

void Context::drawTexture(GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height)
{
	es1::Framebuffer *framebuffer = getFramebuffer();
	es1::Renderbuffer *renderbuffer = framebuffer->getColorbuffer();
	float targetWidth = renderbuffer->getWidth();
	float targetHeight = renderbuffer->getHeight();
	float x0 = 2.0f * x / targetWidth - 1.0f;
	float y0 = 2.0f * y / targetHeight - 1.0f;
	float x1 = 2.0f * (x + width) / targetWidth - 1.0f;
	float y1 = 2.0f * (y + height) / targetHeight - 1.0f;
	float Zw = sw::clamp(mState.zNear + z * (mState.zFar - mState.zNear), mState.zNear, mState.zFar);

	float vertices[][3] = {{x0, y0, Zw},
						   {x0, y1, Zw},
						   {x1, y0, Zw},
						   {x1, y1, Zw}};

	ASSERT(mState.samplerTexture[TEXTURE_2D][1].name() == 0);   // Multi-texturing unimplemented
	es1::Texture *texture = getSamplerTexture(0, TEXTURE_2D);
	float textureWidth = texture->getWidth(GL_TEXTURE_2D, 0);
	float textureHeight = texture->getHeight(GL_TEXTURE_2D, 0);
	int Ucr = texture->getCropRectU();
	int Vcr = texture->getCropRectV();
	int Wcr = texture->getCropRectW();
	int Hcr = texture->getCropRectH();

	float texCoords[][2] = {{Ucr / textureWidth, Vcr / textureHeight},
							{Ucr / textureWidth, (Vcr + Hcr) / textureHeight},
							{(Ucr + Wcr) / textureWidth, Vcr / textureHeight},
							{(Ucr + Wcr) / textureWidth, (Vcr + Hcr) / textureHeight}};

	VertexAttribute oldPositionAttribute = mState.vertexAttribute[sw::Position];
	VertexAttribute oldTexCoord0Attribute = mState.vertexAttribute[sw::TexCoord0];

	glVertexPointer(3, GL_FLOAT, 3 * sizeof(float), vertices);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 2 * sizeof(float), texCoords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	textureStack0.push();
	textureStack0.identity();   // Disable texture coordinate transformation

	drawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Restore state
	mState.vertexAttribute[sw::Position] = oldPositionAttribute;
	mState.vertexAttribute[sw::TexCoord0] = oldTexCoord0Attribute;
	textureStack0.pop();
}

void Context::finish()
{
	device->finish();
}

void Context::flush()
{
    // We don't queue anything without processing it as fast as possible
}

void Context::recordInvalidEnum()
{
    mInvalidEnum = true;
}

void Context::recordInvalidValue()
{
    mInvalidValue = true;
}

void Context::recordInvalidOperation()
{
    mInvalidOperation = true;
}

void Context::recordOutOfMemory()
{
    mOutOfMemory = true;
}

void Context::recordInvalidFramebufferOperation()
{
    mInvalidFramebufferOperation = true;
}

// Get one of the recorded errors and clear its flag, if any.
// [OpenGL ES 2.0.24] section 2.5 page 13.
GLenum Context::getError()
{
    if(mInvalidEnum)
    {
        mInvalidEnum = false;

        return GL_INVALID_ENUM;
    }

    if(mInvalidValue)
    {
        mInvalidValue = false;

        return GL_INVALID_VALUE;
    }

    if(mInvalidOperation)
    {
        mInvalidOperation = false;

        return GL_INVALID_OPERATION;
    }

    if(mOutOfMemory)
    {
        mOutOfMemory = false;

        return GL_OUT_OF_MEMORY;
    }

    if(mInvalidFramebufferOperation)
    {
        mInvalidFramebufferOperation = false;

        return GL_INVALID_FRAMEBUFFER_OPERATION_OES;
    }

    return GL_NO_ERROR;
}

int Context::getSupportedMultiSampleDepth(sw::Format format, int requested)
{
    if(requested <= 1)
    {
        return 1;
    }

	if(requested == 2)
	{
		return 2;
	}

	return 4;
}

void Context::detachBuffer(GLuint buffer)
{
    // [OpenGL ES 2.0.24] section 2.9 page 22:
    // If a buffer object is deleted while it is bound, all bindings to that object in the current context
    // (i.e. in the thread that called Delete-Buffers) are reset to zero.

    if(mState.arrayBuffer.name() == buffer)
    {
        mState.arrayBuffer = NULL;
    }

    if(mState.elementArrayBuffer.name() == buffer)
    {
        mState.elementArrayBuffer = NULL;
    }

    for(int attribute = 0; attribute < MAX_VERTEX_ATTRIBS; attribute++)
    {
        if(mState.vertexAttribute[attribute].mBoundBuffer.name() == buffer)
        {
            mState.vertexAttribute[attribute].mBoundBuffer = NULL;
        }
    }
}

void Context::detachTexture(GLuint texture)
{
    // [OpenGL ES 2.0.24] section 3.8 page 84:
    // If a texture object is deleted, it is as if all texture units which are bound to that texture object are
    // rebound to texture object zero

    for(int type = 0; type < TEXTURE_TYPE_COUNT; type++)
    {
        for(int sampler = 0; sampler < MAX_TEXTURE_UNITS; sampler++)
        {
            if(mState.samplerTexture[type][sampler].name() == texture)
            {
                mState.samplerTexture[type][sampler] = NULL;
            }
        }
    }

    // [OpenGL ES 2.0.24] section 4.4 page 112:
    // If a texture object is deleted while its image is attached to the currently bound framebuffer, then it is
    // as if FramebufferTexture2D had been called, with a texture of 0, for each attachment point to which this
    // image was attached in the currently bound framebuffer.

    Framebuffer *framebuffer = getFramebuffer();

    if(framebuffer)
    {
        framebuffer->detachTexture(texture);
    }
}

void Context::detachFramebuffer(GLuint framebuffer)
{
    // [OpenGL ES 2.0.24] section 4.4 page 107:
    // If a framebuffer that is currently bound to the target FRAMEBUFFER is deleted, it is as though
    // BindFramebuffer had been executed with the target of FRAMEBUFFER and framebuffer of zero.

    if(mState.framebuffer == framebuffer)
    {
        bindFramebuffer(0);
    }
}

void Context::detachRenderbuffer(GLuint renderbuffer)
{
    // [OpenGL ES 2.0.24] section 4.4 page 109:
    // If a renderbuffer that is currently bound to RENDERBUFFER is deleted, it is as though BindRenderbuffer
    // had been executed with the target RENDERBUFFER and name of zero.

    if(mState.renderbuffer.name() == renderbuffer)
    {
        bindRenderbuffer(0);
    }

    // [OpenGL ES 2.0.24] section 4.4 page 111:
    // If a renderbuffer object is deleted while its image is attached to the currently bound framebuffer,
    // then it is as if FramebufferRenderbuffer had been called, with a renderbuffer of 0, for each attachment
    // point to which this image was attached in the currently bound framebuffer.

    Framebuffer *framebuffer = getFramebuffer();

    if(framebuffer)
    {
        framebuffer->detachRenderbuffer(renderbuffer);
    }
}

bool Context::cullSkipsDraw(GLenum drawMode)
{
    return mState.cullFace && mState.cullMode == GL_FRONT_AND_BACK && isTriangleMode(drawMode);
}

bool Context::isTriangleMode(GLenum drawMode)
{
    switch (drawMode)
    {
      case GL_TRIANGLES:
      case GL_TRIANGLE_FAN:
      case GL_TRIANGLE_STRIP:
        return true;
      case GL_POINTS:
      case GL_LINES:
      case GL_LINE_LOOP:
      case GL_LINE_STRIP:
        return false;
      default: UNREACHABLE(drawMode);
    }

    return false;
}

void Context::setVertexAttrib(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    ASSERT(index < MAX_VERTEX_ATTRIBS);

    mState.vertexAttribute[index].mCurrentValue[0] = x;
    mState.vertexAttribute[index].mCurrentValue[1] = y;
    mState.vertexAttribute[index].mCurrentValue[2] = z;
    mState.vertexAttribute[index].mCurrentValue[3] = w;

    mVertexDataManager->dirtyCurrentValue(index);
}

void Context::bindTexImage(egl::Surface *surface)
{
	es1::Texture2D *textureObject = getTexture2D();

    if(textureObject)
    {
		textureObject->bindTexImage(surface);
	}
}

EGLenum Context::validateSharedImage(EGLenum target, GLuint name, GLuint textureLevel)
{
    switch(target)
    {
    case EGL_GL_TEXTURE_2D_KHR:
        break;
    case EGL_GL_RENDERBUFFER_KHR:
        break;
    default:
        return EGL_BAD_PARAMETER;
    }

    if(textureLevel >= IMPLEMENTATION_MAX_TEXTURE_LEVELS)
    {
        return EGL_BAD_MATCH;
    }

	if(target == EGL_GL_TEXTURE_2D_KHR)
    {
        Texture *texture = getTexture(name);

        if(!texture || texture->getTarget() != GL_TEXTURE_2D)
        {
            return EGL_BAD_PARAMETER;
        }

        if(texture->isShared(GL_TEXTURE_2D, textureLevel))   // Bound to an EGLSurface or already an EGLImage sibling
        {
            return EGL_BAD_ACCESS;
        }

        if(textureLevel != 0 && !texture->isSamplerComplete())
        {
            return EGL_BAD_PARAMETER;
        }

        if(textureLevel == 0 && !(texture->isSamplerComplete() && texture->getLevelCount() == 1))
        {
            return EGL_BAD_PARAMETER;
        }
    }
    else if(target == EGL_GL_RENDERBUFFER_KHR)
    {
        Renderbuffer *renderbuffer = getRenderbuffer(name);

        if(!renderbuffer)
        {
            return EGL_BAD_PARAMETER;
        }

        if(renderbuffer->isShared())   // Already an EGLImage sibling
        {
            return EGL_BAD_ACCESS;
        }
    }
    else UNREACHABLE(target);

	return EGL_SUCCESS;
}

egl::Image *Context::createSharedImage(EGLenum target, GLuint name, GLuint textureLevel)
{
    if(target == EGL_GL_TEXTURE_2D_KHR)
    {
        es1::Texture *texture = getTexture(name);

        return texture->createSharedImage(GL_TEXTURE_2D, textureLevel);
    }
    else if(target == EGL_GL_RENDERBUFFER_KHR)
    {
        es1::Renderbuffer *renderbuffer = getRenderbuffer(name);

        return renderbuffer->createSharedImage();
    }
    else UNREACHABLE(target);

	return 0;
}

Device *Context::getDevice()
{
	return device;
}

void Context::setMatrixMode(GLenum mode)
{
    matrixMode = mode;
}

sw::MatrixStack &Context::currentMatrixStack()
{
	switch(matrixMode)
	{
	case GL_MODELVIEW:
		return modelViewStack;
	case GL_PROJECTION:
		return projectionStack;
	case GL_TEXTURE:
		switch(mState.activeSampler)
		{
		case 0: return textureStack0;
		case 1: return textureStack1;
		}
		break;
	}

	UNREACHABLE(matrixMode);
	return textureStack0;
}

void Context::loadIdentity()
{
	currentMatrixStack().identity();
}

void Context::load(const GLfloat *m)
{
    currentMatrixStack().load(m);
}

void Context::pushMatrix()
{
	if(!currentMatrixStack().push())
	{
		return error(GL_STACK_OVERFLOW);
	}
}

void Context::popMatrix()
{
    if(!currentMatrixStack().pop())
	{
		return error(GL_STACK_OVERFLOW);
	}
}

void Context::rotate(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    currentMatrixStack().rotate(angle, x, y, z);
}

void Context::translate(GLfloat x, GLfloat y, GLfloat z)
{
    currentMatrixStack().translate(x, y, z);
}

void Context::scale(GLfloat x, GLfloat y, GLfloat z)
{
    currentMatrixStack().scale(x, y, z);
}

void Context::multiply(const GLfloat *m)
{
    currentMatrixStack().multiply(m);
}

void Context::frustum(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
	currentMatrixStack().frustum(left, right, bottom, top, zNear, zFar);
}

void Context::ortho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
	currentMatrixStack().ortho(left, right, bottom, top, zNear, zFar);
}

void Context::setClipPlane(int index, const float plane[4])
{
	sw::Plane clipPlane = modelViewStack.current() * sw::Plane(plane);
	device->setClipPlane(index, &clipPlane.A);
}

void Context::setClipPlaneEnable(int index, bool enable)
{
	clipFlags = clipFlags & ~((int)!enable << index) | ((int)enable << index);
	device->setClipFlags(clipFlags);
}

bool Context::isClipPlaneEnabled(int index) const
{
	return (clipFlags & (1 << index)) != 0;
}

void Context::clientActiveTexture(GLenum texture)
{
	clientTexture = texture;
}

GLenum Context::getClientActiveTexture() const
{
	return clientTexture;
}

unsigned int Context::getActiveTexture() const
{
	return mState.activeSampler;
}

}

egl::Context *es1CreateContext(const egl::Config *config, const egl::Context *shareContext)
{
	ASSERT(!shareContext || shareContext->getClientVersion() == 1);   // Should be checked by eglCreateContext
	return new es1::Context(config, static_cast<const es1::Context*>(shareContext));
}
