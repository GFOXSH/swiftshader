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

// ResourceManager.cpp: Implements the ResourceManager class, which tracks and 
// retrieves objects which may be shared by multiple Contexts.

#include "ResourceManager.h"

#include "Buffer.h"
#include "Program.h"
#include "Renderbuffer.h"
#include "Shader.h"
#include "Texture.h"

namespace gl
{
ResourceManager::ResourceManager()
{
    mRefCount = 1;
}

ResourceManager::~ResourceManager()
{
    while(!mBufferMap.empty())
    {
        deleteBuffer(mBufferMap.begin()->first);
    }

    while(!mProgramMap.empty())
    {
        deleteProgram(mProgramMap.begin()->first);
    }

    while(!mShaderMap.empty())
    {
        deleteShader(mShaderMap.begin()->first);
    }

    while(!mRenderbufferMap.empty())
    {
        deleteRenderbuffer(mRenderbufferMap.begin()->first);
    }

    while(!mTextureMap.empty())
    {
        deleteTexture(mTextureMap.begin()->first);
    }
}

void ResourceManager::addRef()
{
    mRefCount++;
}

void ResourceManager::release()
{
    if(--mRefCount == 0)
    {
        delete this;
    }
}

// Returns an unused buffer name
GLuint ResourceManager::createBuffer()
{
    //GLuint handle = mBufferNameSpace.allocate();
    unsigned int handle = 1;

    while (mBufferMap.find(handle) != mBufferMap.end())
    {
        handle++;
    }

    mBufferMap[handle] = NULL;

    return handle;
}

// Returns an unused shader/program name
GLuint ResourceManager::createShader(GLenum type)
{
    //GLuint handle = mProgramShaderNameSpace.allocate();
    unsigned int handle = 1;

    while (mShaderMap.find(handle) != mShaderMap.end())
    {
        handle++;
    }

    if(type == GL_VERTEX_SHADER)
    {
        mShaderMap[handle] = new VertexShader(this, handle);
    }
    else if(type == GL_FRAGMENT_SHADER)
    {
        mShaderMap[handle] = new FragmentShader(this, handle);
    }
    else UNREACHABLE(type);

    return handle;
}

// Returns an unused program/shader name
GLuint ResourceManager::createProgram()
{
    //GLuint handle = mProgramShaderNameSpace.allocate();
    unsigned int handle = 1;

    while (mProgramMap.find(handle) != mProgramMap.end())
    {
        handle++;
    }

    mProgramMap[handle] = new Program(this, handle);

    return handle;
}

// Returns an unused texture name
GLuint ResourceManager::createTexture()
{
    //GLuint handle = mTextureNameSpace.allocate();
    unsigned int handle = 1;

    while (mTextureMap.find(handle) != mTextureMap.end())
    {
        handle++;
    }

    mTextureMap[handle] = NULL;

    return handle;
}

// Returns an unused renderbuffer name
GLuint ResourceManager::createRenderbuffer()
{
    //GLuint handle = mRenderbufferNameSpace.allocate();
    unsigned int handle = 1;

    while (mRenderbufferMap.find(handle) != mRenderbufferMap.end())
    {
        handle++;
    }

    mRenderbufferMap[handle] = NULL;

    return handle;
}

void ResourceManager::deleteBuffer(GLuint buffer)
{
    BufferMap::iterator bufferObject = mBufferMap.find(buffer);

    if(bufferObject != mBufferMap.end())
    {
        //mBufferNameSpace.release(bufferObject->first);
        if(bufferObject->second) bufferObject->second->release();
        mBufferMap.erase(bufferObject);
    }
}

void ResourceManager::deleteShader(GLuint shader)
{
    ShaderMap::iterator shaderObject = mShaderMap.find(shader);

    if(shaderObject != mShaderMap.end())
    {
        if(shaderObject->second->getRefCount() == 0)
        {
            //mProgramShaderNameSpace.release(shaderObject->first);
            delete shaderObject->second;
            mShaderMap.erase(shaderObject);
        }
        else
        {
            shaderObject->second->flagForDeletion();
        }
    }
}

void ResourceManager::deleteProgram(GLuint program)
{
    ProgramMap::iterator programObject = mProgramMap.find(program);

    if(programObject != mProgramMap.end())
    {
        if(programObject->second->getRefCount() == 0)
        {
            //mProgramShaderNameSpace.release(programObject->first);
            delete programObject->second;
            mProgramMap.erase(programObject);
        }
        else
        { 
            programObject->second->flagForDeletion();
        }
    }
}

void ResourceManager::deleteTexture(GLuint texture)
{
    TextureMap::iterator textureObject = mTextureMap.find(texture);

    if(textureObject != mTextureMap.end())
    {
        //mTextureNameSpace.release(textureObject->first);
        if(textureObject->second) textureObject->second->release();
        mTextureMap.erase(textureObject);
    }
}

void ResourceManager::deleteRenderbuffer(GLuint renderbuffer)
{
    RenderbufferMap::iterator renderbufferObject = mRenderbufferMap.find(renderbuffer);

    if(renderbufferObject != mRenderbufferMap.end())
    {
        //mRenderbufferNameSpace.release(renderbufferObject->first);
        if(renderbufferObject->second) renderbufferObject->second->release();
        mRenderbufferMap.erase(renderbufferObject);
    }
}

Buffer *ResourceManager::getBuffer(unsigned int handle)
{
    BufferMap::iterator buffer = mBufferMap.find(handle);

    if(buffer == mBufferMap.end())
    {
        return NULL;
    }
    else
    {
        return buffer->second;
    }
}

Shader *ResourceManager::getShader(unsigned int handle)
{
    ShaderMap::iterator shader = mShaderMap.find(handle);

    if(shader == mShaderMap.end())
    {
        return NULL;
    }
    else
    {
        return shader->second;
    }
}

Texture *ResourceManager::getTexture(unsigned int handle)
{
    if(handle == 0) return NULL;

    TextureMap::iterator texture = mTextureMap.find(handle);

    if(texture == mTextureMap.end())
    {
        return NULL;
    }
    else
    {
        return texture->second;
    }
}

Program *ResourceManager::getProgram(unsigned int handle)
{
    ProgramMap::iterator program = mProgramMap.find(handle);

    if(program == mProgramMap.end())
    {
        return NULL;
    }
    else
    {
        return program->second;
    }
}

Renderbuffer *ResourceManager::getRenderbuffer(unsigned int handle)
{
    RenderbufferMap::iterator renderbuffer = mRenderbufferMap.find(handle);

    if(renderbuffer == mRenderbufferMap.end())
    {
        return NULL;
    }
    else
    {
        return renderbuffer->second;
    }
}

void ResourceManager::setRenderbuffer(GLuint handle, Renderbuffer *buffer)
{
    mRenderbufferMap[handle] = buffer;
}

void ResourceManager::checkBufferAllocation(unsigned int buffer)
{
    if(buffer != 0 && !getBuffer(buffer))
    {
        Buffer *bufferObject = new Buffer(buffer);
        mBufferMap[buffer] = bufferObject;
        bufferObject->addRef();
    }
}

void ResourceManager::checkTextureAllocation(GLuint texture, TextureType type)
{
    if(!getTexture(texture) && texture != 0)
    {
        Texture *textureObject;

        if(type == TEXTURE_2D)
        {
            textureObject = new Texture2D(texture);
        }
        else if(type == TEXTURE_CUBE)
        {
            textureObject = new TextureCubeMap(texture);
        }
        else
        {
            UNREACHABLE(type);
            return;
        }

        mTextureMap[texture] = textureObject;
        textureObject->addRef();
    }
}

void ResourceManager::checkRenderbufferAllocation(GLuint renderbuffer)
{
    if(renderbuffer != 0 && !getRenderbuffer(renderbuffer))
    {
        Renderbuffer *renderbufferObject = new Renderbuffer(renderbuffer, new Colorbuffer(0, 0, GL_RGBA4, 0));
        mRenderbufferMap[renderbuffer] = renderbufferObject;
        renderbufferObject->addRef();
    }
}

}
