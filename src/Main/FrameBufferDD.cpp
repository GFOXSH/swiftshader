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

#include "FrameBufferDD.hpp"

#include "Debug.hpp"

namespace sw
{
	extern bool forceWindowed;

	GUID secondaryDisplay = {0};

	int __stdcall enumDisplayCallback(GUID* guid, char *driverDescription, char *driverName, void *context, HMONITOR monitor)
	{
		if(strcmp(driverName, "\\\\.\\DISPLAY2") == 0)
		{
			secondaryDisplay = *guid;
		}

		return 1;
	}

	FrameBufferDD::FrameBufferDD(HWND windowHandle, int width, int height, bool fullscreen, bool topLeftOrigin) : FrameBufferWin(windowHandle, width, height, fullscreen, topLeftOrigin)
	{
		directDraw = 0;
		frontBuffer = 0;
		backBuffer = 0;

		locked = 0;

		ddraw = LoadLibrary("ddraw.dll");
		DirectDrawCreate = (DIRECTDRAWCREATE)GetProcAddress(ddraw, "DirectDrawCreate");
		DirectDrawEnumerateExA = (DIRECTDRAWENUMERATEEXA)GetProcAddress(ddraw, "DirectDrawEnumerateExA");

		if(!windowed)
		{
			initFullscreen();
		}
		else
		{
			initWindowed();
		}
	}

	FrameBufferDD::~FrameBufferDD()
	{
		releaseAll();

		FreeLibrary(ddraw);
	}

	void FrameBufferDD::createSurfaces()
	{
		if(backBuffer)
		{
			backBuffer->Release();
			backBuffer = 0;
		}

		if(frontBuffer)
		{
			frontBuffer->Release();
			frontBuffer = 0;
		}

		if(!windowed)
		{
			DDSURFACEDESC surfaceDescription = {0};
			surfaceDescription.dwSize = sizeof(surfaceDescription);
			surfaceDescription.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
			surfaceDescription.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
			surfaceDescription.dwBackBufferCount = 1;
			long result = directDraw->CreateSurface(&surfaceDescription, &frontBuffer, 0);

			if(frontBuffer)
			{
				DDSCAPS surfaceCapabilties = {0};
				surfaceCapabilties.dwCaps = DDSCAPS_BACKBUFFER;
				frontBuffer->GetAttachedSurface(&surfaceCapabilties, &backBuffer);
				backBuffer->AddRef();
			}
		}
		else
		{
			IDirectDrawClipper *clipper;
			
			DDSURFACEDESC ddsd = {0};
			ddsd.dwSize = sizeof(ddsd);
			ddsd.dwFlags = DDSD_CAPS;
			ddsd.ddsCaps.dwCaps	= DDSCAPS_PRIMARYSURFACE;

			long result = directDraw->CreateSurface(&ddsd, &frontBuffer, 0);
			directDraw->GetDisplayMode(&ddsd);
			
			switch(ddsd.ddpfPixelFormat.dwRGBBitCount)
			{
			case 32: destFormat = FORMAT_X8R8G8B8; break;
			case 24: destFormat = FORMAT_R8G8B8;   break;
			case 16: destFormat = FORMAT_R5G6B5;   break;
			default: destFormat = FORMAT_NULL;     break;
			}

			if((result != DD_OK && result != DDERR_PRIMARYSURFACEALREADYEXISTS) || (destFormat == FORMAT_NULL))
			{
				assert(!"Failed to initialize graphics: Incompatible display mode.");
			}
			else
			{
				ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
				ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
				ddsd.dwWidth = width;
				ddsd.dwHeight = height;

				directDraw->CreateSurface(&ddsd, &backBuffer, 0);

				directDraw->CreateClipper(0, &clipper, 0);
				clipper->SetHWnd(0, windowHandle);
				frontBuffer->SetClipper(clipper);
				clipper->Release();
			}
		}
	}

	bool FrameBufferDD::readySurfaces()
	{
		if(!frontBuffer || !backBuffer)
		{
			createSurfaces();
		}

		if(frontBuffer && backBuffer)
		{
			if(frontBuffer->IsLost() || backBuffer->IsLost())
			{
				restoreSurfaces();
			}

			if(frontBuffer && backBuffer)
			{
				if(!frontBuffer->IsLost() && !backBuffer->IsLost())
				{
					return true;
				}
			}
		}

		return false;
	}

	void FrameBufferDD::updateClipper(HWND windowOverride)
	{
		if(windowed)
		{
			if(frontBuffer)
			{
				HWND window = windowOverride ? windowOverride : windowHandle;

				IDirectDrawClipper *clipper;
				frontBuffer->GetClipper(&clipper);
				clipper->SetHWnd(0, window);
				clipper->Release();
			}
		}
	}

	void FrameBufferDD::restoreSurfaces()
	{
		long result1 = frontBuffer->Restore();
		long result2 = backBuffer->Restore();

		if(result1 != DD_OK || result2 != DD_OK)   // Surfaces could not be restored; recreate them
		{
			createSurfaces();
		}
	}

	void FrameBufferDD::initFullscreen()
	{
		releaseAll();

		if(true)   // Render to primary display
		{
			DirectDrawCreate(0, &directDraw, 0);
		}
		else   // Render to secondary display
		{
			DirectDrawEnumerateEx(&enumDisplayCallback, 0, DDENUM_ATTACHEDSECONDARYDEVICES);
			DirectDrawCreate(&secondaryDisplay, &directDraw, 0);
		}

		directDraw->SetCooperativeLevel(windowHandle, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);

		long result;

		do
		{
			destFormat = FORMAT_X8R8G8B8;
			result = directDraw->SetDisplayMode(width, height, 32);

			if(result == DDERR_INVALIDMODE)
			{
				destFormat = FORMAT_R8G8B8;
				result = directDraw->SetDisplayMode(width, height, 24);

				if(result == DDERR_INVALIDMODE)
				{
					destFormat = FORMAT_R5G6B5;
					result = directDraw->SetDisplayMode(width, height, 16);

					if(result == DDERR_INVALIDMODE)
					{
						assert(!"Failed to initialize graphics: Display mode not supported.");
					}
				}
			}

			if(result != DD_OK)
			{
				Sleep(1);
			}
		}
		while(result != DD_OK);

		createSurfaces();

		updateBounds(windowHandle);
	}

	void FrameBufferDD::initWindowed()
	{
		releaseAll();

		DirectDrawCreate(0, &directDraw, 0);
		directDraw->SetCooperativeLevel(windowHandle, DDSCL_NORMAL);

		createSurfaces();

		updateBounds(windowHandle);
	}

	void FrameBufferDD::flip(void *source, Format format)
	{
		copy(source, format);

		if(!readySurfaces())
		{
			return;
		}

		while(true)
		{
			long result;

			if(windowed)
			{
				result = frontBuffer->Blt(&bounds, backBuffer, 0, DDBLT_WAIT, 0);
			}
			else
			{
				result = frontBuffer->Flip(0, DDFLIP_NOVSYNC);
			}

			if(result != DDERR_WASSTILLDRAWING)
			{
				break;
			}

			Sleep(0);
		}
	}

	void FrameBufferDD::blit(void *source, const Rect *sourceRect, const Rect *destRect, Format format)
	{
		copy(source, format);

		if(!readySurfaces())
		{
			return;
		}

		RECT dRect;

		if(destRect)
		{
			dRect.bottom = bounds.top + destRect->y1;
			dRect.left = bounds.left + destRect->x0;
			dRect.right = bounds.left + destRect->x1;
			dRect.top = bounds.top + destRect->y0;
		}
		else
		{
			dRect.bottom = bounds.top + height;
			dRect.left = bounds.left + 0;
			dRect.right = bounds.left + width;
			dRect.top = bounds.top + 0;
		}

		while(true)
		{
			long result = frontBuffer->Blt(&dRect, backBuffer, (LPRECT)sourceRect, DDBLT_WAIT, 0);

			if(result != DDERR_WASSTILLDRAWING)
			{
				break;
			}

			Sleep(0);
		}
	}

	void FrameBufferDD::flip(HWND windowOverride, void *source, Format format)
	{
		updateClipper(windowOverride);
		updateBounds(windowOverride);

		flip(source, format);
	}

	void FrameBufferDD::blit(HWND windowOverride, void *source, const Rect *sourceRect, const Rect *destRect, Format format)
	{
		updateClipper(windowOverride);
		updateBounds(windowOverride);

		blit(source, sourceRect, destRect, format);
	}

	void FrameBufferDD::screenshot(void *destBuffer)
	{
		if(!readySurfaces())
		{
			return;
		}

		DDSURFACEDESC DDSD;
		DDSD.dwSize = sizeof(DDSD);

		long result = frontBuffer->Lock(0, &DDSD, DDLOCK_WAIT, 0);

		if(result == DD_OK)
		{
			int width = DDSD.dwWidth;
			int height = DDSD.dwHeight;
			int bitDepth = DDSD.ddpfPixelFormat.dwRGBBitCount;
			int stride = DDSD.lPitch;

			void *sourceBuffer = DDSD.lpSurface;

			for(int y = 0; y < height; y++)
			{
				memcpy(destBuffer, sourceBuffer, width * 4);   // FIXME: Assumes 32-bit buffer

				(char*&)sourceBuffer += stride;
				(char*&)destBuffer += 4 * width;
			}

			frontBuffer->Unlock(0);
		}
	}

	void FrameBufferDD::setGammaRamp(GammaRamp *gammaRamp, bool calibrate)
	{
		IDirectDrawGammaControl *gammaControl = 0;

		if(frontBuffer)
		{
			frontBuffer->QueryInterface(IID_IDirectDrawGammaControl, (void**)&gammaControl); 

			if(gammaControl)
			{
				gammaControl->SetGammaRamp(calibrate ? DDSGR_CALIBRATE : 0, (DDGAMMARAMP*)gammaRamp);

				gammaControl->Release();
			}
		}
	}

	void FrameBufferDD::getGammaRamp(GammaRamp *gammaRamp)
	{
		IDirectDrawGammaControl *gammaControl = 0;

		if(frontBuffer)
		{
			frontBuffer->QueryInterface(IID_IDirectDrawGammaControl, (void**)&gammaControl); 

			if(gammaControl)
			{
				gammaControl->GetGammaRamp(0, (DDGAMMARAMP*)gammaRamp);

				gammaControl->Release();
			}
		}
	}

	void *FrameBufferDD::lock()
	{
		if(locked)
		{
			return locked;
		}
		
		if(!readySurfaces())
		{
			return 0;
		}

		DDSURFACEDESC DDSD;
		DDSD.dwSize = sizeof(DDSD);

		long result = backBuffer->Lock(0, &DDSD, DDLOCK_WAIT, 0);

		if(result == DD_OK)
		{
			width = DDSD.dwWidth;
			height = DDSD.dwHeight;
			int bitDepth = DDSD.ddpfPixelFormat.dwRGBBitCount;
			stride = DDSD.lPitch;

			locked = DDSD.lpSurface;

			return locked;
		}

		return 0;
	}

	void FrameBufferDD::unlock()
	{
		if(!locked || !backBuffer) return;

		backBuffer->Unlock(0);

		locked = 0;
	}

	void FrameBufferDD::drawText(int x, int y, const char *string, ...)
	{
		char buffer[256];
		va_list arglist;

		va_start(arglist, string);
		vsprintf(buffer, string, arglist);
		va_end(arglist);

		HDC hdc;

		backBuffer->GetDC(&hdc);

		SetBkColor(hdc, RGB(0, 0, 255));
		SetTextColor(hdc, RGB(255, 255, 255));

		TextOut(hdc, x, y, buffer, lstrlen(buffer));

		backBuffer->ReleaseDC(hdc);
	}

	bool FrameBufferDD::getScanline(bool &inVerticalBlank, unsigned int &scanline)
	{
		HRESULT result = directDraw->GetScanLine((unsigned long*)&scanline);

		if(result == DD_OK)
		{
			inVerticalBlank = false;
		}
		else if(result == DDERR_VERTICALBLANKINPROGRESS)
		{
			inVerticalBlank = true;
		}
		else if(result == DDERR_UNSUPPORTED)
		{
			return false;
		}
		else ASSERT(false);

		return true;
	}

	void FrameBufferDD::releaseAll()
	{
		unlock();

		if(backBuffer)
		{
			backBuffer->Release();
			backBuffer = 0;
		}

		if(frontBuffer)
		{
			frontBuffer->Release();
			frontBuffer = 0;
		}

		if(directDraw)
		{
			directDraw->SetCooperativeLevel(0, DDSCL_NORMAL);
			directDraw->Release();
			directDraw = 0;
		}
	}
}
