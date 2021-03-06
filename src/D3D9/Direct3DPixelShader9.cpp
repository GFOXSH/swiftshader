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

#include "Direct3DPixelShader9.hpp"

#include "Direct3DDevice9.hpp"
#include "Debug.hpp"

namespace D3D9
{
	Direct3DPixelShader9::Direct3DPixelShader9(Direct3DDevice9 *device, const unsigned long *shaderToken) : device(device), pixelShader(shaderToken)
	{
		tokenCount = 0;

		while(shaderToken[tokenCount] != 0x0000FFFF)
		{
			tokenCount += sw::Shader::size(shaderToken[tokenCount], (unsigned short)(shaderToken[0] & 0xFFFF)) + 1;
		}

		tokenCount += 1;

		this->shaderToken = new unsigned long[tokenCount];
		memcpy(this->shaderToken, shaderToken, tokenCount * sizeof(unsigned long));
	}

	Direct3DPixelShader9::~Direct3DPixelShader9()
	{
		delete[] shaderToken;
		shaderToken = 0;
	}

	long Direct3DPixelShader9::QueryInterface(const IID &iid, void **object)
	{
		CriticalSection cs(device);

		TRACE("");

		if(iid == IID_IDirect3DPixelShader9 ||
		   iid == IID_IUnknown)
		{
			AddRef();
			*object = this;

			return S_OK;
		}

		*object = 0;

		return NOINTERFACE(iid);
	}

	unsigned long Direct3DPixelShader9::AddRef()
	{
		TRACE("");

		return Unknown::AddRef();
	}
	
	unsigned long Direct3DPixelShader9::Release()
	{
		TRACE("");

		return Unknown::Release();
	}

	long Direct3DPixelShader9::GetDevice(IDirect3DDevice9 **device)
	{
		CriticalSection cs(this->device);

		TRACE("");

		if(!device)
		{
			return INVALIDCALL();
		}

		this->device->AddRef();
		*device = this->device;

		return D3D_OK;
	}

	long Direct3DPixelShader9::GetFunction(void *data, unsigned int *size)
	{
		CriticalSection cs(device);

		TRACE("");

		if(!size)
		{
			return INVALIDCALL();
		}

		if(data)
		{
			memcpy(data, shaderToken, tokenCount * 4);
		}

		*size = tokenCount * 4;

		return D3D_OK;
	}

	const sw::PixelShader *Direct3DPixelShader9::getPixelShader() const
	{
		return &pixelShader;
	}
}
