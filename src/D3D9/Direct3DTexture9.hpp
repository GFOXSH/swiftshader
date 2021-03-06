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

#ifndef D3D9_Direct3DTexture9_hpp
#define D3D9_Direct3DTexture9_hpp

#include "Direct3DBaseTexture9.hpp"

#include "Config.hpp"

#include <d3d9.h>

namespace D3D9
{
	class Direct3DSurface9;

	class Direct3DTexture9 : public IDirect3DTexture9, public Direct3DBaseTexture9
	{
	public:	
		Direct3DTexture9(Direct3DDevice9 *device, unsigned int width, unsigned int height, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool);

		virtual ~Direct3DTexture9();

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object);
		unsigned long __stdcall AddRef();
		unsigned long __stdcall Release();

		// IDirect3DResource9 methods
		long __stdcall GetDevice(IDirect3DDevice9 **device);
		long __stdcall SetPrivateData(const GUID &guid, const void *data, unsigned long size, unsigned long flags);
		long __stdcall GetPrivateData(const GUID &guid, void *data, unsigned long *size);
		long __stdcall FreePrivateData(const GUID &guid);
		unsigned long __stdcall SetPriority(unsigned long newPriority);
		unsigned long __stdcall GetPriority();
		void __stdcall PreLoad();
		D3DRESOURCETYPE __stdcall GetType();

		// IDirect3DBaseTexture9 methods
		unsigned long __stdcall SetLOD(unsigned long newLOD);
		unsigned long __stdcall GetLOD();
		unsigned long __stdcall GetLevelCount();
		long __stdcall SetAutoGenFilterType(D3DTEXTUREFILTERTYPE filterType);
		D3DTEXTUREFILTERTYPE __stdcall GetAutoGenFilterType();
		void __stdcall GenerateMipSubLevels();

		// IDirect3DTexture9 methods
		long __stdcall GetLevelDesc(unsigned int level, D3DSURFACE_DESC *description);
		long __stdcall GetSurfaceLevel(unsigned int level, IDirect3DSurface9 **surface);
		long __stdcall LockRect(unsigned int level, D3DLOCKED_RECT *lockedRect, const RECT *rect, unsigned long flags);
		long __stdcall UnlockRect(unsigned int level);
		long __stdcall AddDirtyRect(const RECT *dirtyRect);

		// Internal methods
		Direct3DSurface9 *getInternalSurfaceLevel(unsigned int level);

	private:
		// Creation parameters
		const unsigned int width;
		const unsigned int height;

		Direct3DSurface9 *surfaceLevel[MIPMAP_LEVELS];
	};
}

#endif // D3D9_Direct3DTexture9_hpp
