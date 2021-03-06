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

#ifndef D3D9_Direct3DDevice9Ex_hpp
#define D3D9_Direct3DDevice9Ex_hpp

#include "Direct3DDevice9.hpp"

#include "Direct3D9Ex.hpp"
#include "Direct3DSwapChain9.hpp"

#include "Stream.hpp"

#include <d3d9.h>
#include <map>
#include <list>
#include <vector>

namespace sw
{
	class Renderer;
	class Context;
}

namespace D3D9
{
	class Direct3DVertexDeclaration9;
	class Direct3DStateBlock9;
	class Direct3DSurface9;
	class Direct3DPixelShader9;
	class Direct3DVertexShader9;
	class irect3DVertexDeclaration9;
	class Direct3DVertexBuffer9;
	class Direct3DIndexBuffer9;

	class Direct3DDevice9Ex : public IDirect3DDevice9Ex, public Direct3DDevice9
	{
	public:
		Direct3DDevice9Ex(const HINSTANCE instance, Direct3D9Ex *d3d9ex, unsigned int adapter, D3DDEVTYPE deviceType, HWND focusWindow, unsigned long behaviourFlags, D3DPRESENT_PARAMETERS *presentParameters);

		virtual ~Direct3DDevice9Ex();

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object);
		unsigned long __stdcall AddRef();
		unsigned long __stdcall Release();

		// IDirect3DDevice9 methods
		long __stdcall TestCooperativeLevel();
		unsigned int __stdcall GetAvailableTextureMem();
		long __stdcall EvictManagedResources();
		long __stdcall GetDirect3D(IDirect3D9 **D3D);
		long __stdcall GetDeviceCaps(D3DCAPS9 *caps);
		long __stdcall GetDisplayMode(unsigned int swapChain ,D3DDISPLAYMODE *mode);
		long __stdcall GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *parameters);
		long __stdcall SetCursorProperties(unsigned int x, unsigned int y, IDirect3DSurface9 *cursorBitmap);
		void __stdcall SetCursorPosition(int x, int y, unsigned long flags);
		int __stdcall ShowCursor(int show);
		long __stdcall CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *presentParameters, IDirect3DSwapChain9 **swapChain);
		long __stdcall GetSwapChain(unsigned int index, IDirect3DSwapChain9 **swapChain);
		unsigned int __stdcall GetNumberOfSwapChains();
		long __stdcall Reset(D3DPRESENT_PARAMETERS *presentParameters);
		long __stdcall Present(const RECT *sourceRect, const RECT *destRect, HWND destWindowOverride, const RGNDATA *dirtyRegion);
		long __stdcall GetBackBuffer(unsigned int swapChain, unsigned int index, D3DBACKBUFFER_TYPE type, IDirect3DSurface9 **backBuffer);
		long __stdcall GetRasterStatus(unsigned int swapChain, D3DRASTER_STATUS *rasterStatus);
		long __stdcall SetDialogBoxMode(int enableDialogs);
		void __stdcall SetGammaRamp(unsigned int swapChain, unsigned long flags, const D3DGAMMARAMP *ramp);
		void __stdcall GetGammaRamp(unsigned int swapChain, D3DGAMMARAMP *ramp);
		long __stdcall CreateTexture(unsigned int width, unsigned int height, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture9 **texture, void **sharedHandle);
		long __stdcall CreateVolumeTexture(unsigned int width, unsigned int height, unsigned int depth, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DVolumeTexture9 **volumeTexture, void **sharedHandle);
		long __stdcall CreateCubeTexture(unsigned int edgeLength, unsigned int levels, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DCubeTexture9 **cubeTexture, void **sharedHandle);
		long __stdcall CreateVertexBuffer(unsigned int length, unsigned long usage, unsigned long FVF, D3DPOOL, IDirect3DVertexBuffer9 **vertexBuffer, void **sharedHandle);
		long __stdcall CreateIndexBuffer(unsigned int length, unsigned long usage, D3DFORMAT format, D3DPOOL pool, IDirect3DIndexBuffer9 **indexBuffer, void **sharedHandle);
		long __stdcall CreateRenderTarget(unsigned int width, unsigned int height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multiSample, unsigned long multiSampleQuality, int lockable, IDirect3DSurface9 **surface, void **sharedHandle);
		long __stdcall CreateDepthStencilSurface(unsigned int width, unsigned int height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multiSample, unsigned long multiSampleQuality, int discard, IDirect3DSurface9 **surface, void **sharedHandle);
		long __stdcall UpdateSurface(IDirect3DSurface9 *sourceSurface, const RECT *sourceRect, IDirect3DSurface9 *destinationSurface, const POINT *destPoint);
		long __stdcall UpdateTexture(IDirect3DBaseTexture9 *sourceTexture, IDirect3DBaseTexture9 *destinationTexture);
		long __stdcall GetRenderTargetData(IDirect3DSurface9 *renderTarget, IDirect3DSurface9 *destSurface);
		long __stdcall GetFrontBufferData(unsigned int swapChain, IDirect3DSurface9 *destSurface);
		long __stdcall StretchRect(IDirect3DSurface9 *sourceSurface, const RECT *sourceRect, IDirect3DSurface9 *destSurface, const RECT *destRect, D3DTEXTUREFILTERTYPE filter);
		long __stdcall ColorFill(IDirect3DSurface9 *surface, const RECT *rect, D3DCOLOR color);
		long __stdcall CreateOffscreenPlainSurface(unsigned int width, unsigned int height, D3DFORMAT format, D3DPOOL pool, IDirect3DSurface9 **surface, void **sharedHandle);
		long __stdcall SetRenderTarget(unsigned long index, IDirect3DSurface9 *renderTarget);
		long __stdcall GetRenderTarget(unsigned long index, IDirect3DSurface9 **renderTarget);
		long __stdcall SetDepthStencilSurface(IDirect3DSurface9 *newDepthStencil);
		long __stdcall GetDepthStencilSurface(IDirect3DSurface9 **depthStencilSurface);
		long __stdcall BeginScene();
		long __stdcall EndScene();
		long __stdcall Clear(unsigned long Count, const D3DRECT *rects, unsigned long Flags, unsigned long Color, float Z, unsigned long Stencil);
		long __stdcall SetTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix);
		long __stdcall GetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix);
		long __stdcall MultiplyTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix);
		long __stdcall SetViewport(const D3DVIEWPORT9 *viewport);
		long __stdcall GetViewport(D3DVIEWPORT9 *viewport);
		long __stdcall SetMaterial(const D3DMATERIAL9 *material);
		long __stdcall GetMaterial(D3DMATERIAL9 *material);
		long __stdcall SetLight(unsigned long index, const D3DLIGHT9 *light);
		long __stdcall GetLight(unsigned long index, D3DLIGHT9 *light);
		long __stdcall LightEnable(unsigned long index, int enable);
		long __stdcall GetLightEnable(unsigned long index , int *enable);
		long __stdcall SetClipPlane(unsigned long index, const float *plane);
		long __stdcall GetClipPlane(unsigned long index, float *plane);
		long __stdcall SetRenderState(D3DRENDERSTATETYPE state, unsigned long value);
		long __stdcall GetRenderState(D3DRENDERSTATETYPE State, unsigned long *value);
		long __stdcall CreateStateBlock(D3DSTATEBLOCKTYPE type, IDirect3DStateBlock9 **stateBlock);
		long __stdcall BeginStateBlock();
		long __stdcall EndStateBlock(IDirect3DStateBlock9 **stateBlock);
		long __stdcall SetClipStatus(const D3DCLIPSTATUS9 *clipStatus);
		long __stdcall GetClipStatus(D3DCLIPSTATUS9 *clipStatus);
		long __stdcall GetTexture(unsigned long sampler, IDirect3DBaseTexture9 **texture);
		long __stdcall SetTexture(unsigned long sampler, IDirect3DBaseTexture9 *texture);
		long __stdcall GetTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type, unsigned long *value);
		long __stdcall SetTextureStageState(unsigned long stage, D3DTEXTURESTAGESTATETYPE type, unsigned long value);
		long __stdcall GetSamplerState(unsigned long sampler, D3DSAMPLERSTATETYPE state, unsigned long *value);
		long __stdcall SetSamplerState(unsigned long sampler, D3DSAMPLERSTATETYPE state, unsigned long value);
		long __stdcall ValidateDevice(unsigned long *numPasses);
		long __stdcall SetPaletteEntries(unsigned int paletteNumber, const PALETTEENTRY *entries);
		long __stdcall GetPaletteEntries(unsigned int paletteNumber, PALETTEENTRY *entries);
		long __stdcall SetCurrentTexturePalette(unsigned int paletteNumber);
		long __stdcall GetCurrentTexturePalette(unsigned int *paletteNumber);
		long __stdcall SetScissorRect(const RECT *rect);
		long __stdcall GetScissorRect(RECT *rect);
		long __stdcall SetSoftwareVertexProcessing(int software);
		int __stdcall GetSoftwareVertexProcessing();
		long __stdcall SetNPatchMode(float segments);
		float __stdcall GetNPatchMode();
		long __stdcall DrawPrimitive(D3DPRIMITIVETYPE primitiveType, unsigned int startVertex, unsigned int primiveCount);
		long __stdcall DrawIndexedPrimitive(D3DPRIMITIVETYPE type, int baseVertexIndex, unsigned int minIndex, unsigned int numVertices, unsigned int startIndex, unsigned int primitiveCount);
		long __stdcall DrawPrimitiveUP(D3DPRIMITIVETYPE primitiveType, unsigned int primitiveCount, const void *vertexStreamZeroData, unsigned int vertexStreamZeroStride);
		long __stdcall DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE type, unsigned int minVertexIndex, unsigned int numVertexIndices, unsigned int PrimitiveCount, const void *indexData, D3DFORMAT indexDataFormat, const void *vertexStreamZeroData, unsigned int VertexStreamZeroStride);
		long __stdcall ProcessVertices(unsigned int srcStartIndex, unsigned int destIndex, unsigned int vertexCount, IDirect3DVertexBuffer9 *destBuffer, IDirect3DVertexDeclaration9 *vertexDeclaration, unsigned long flags);
		long __stdcall CreateVertexDeclaration(const D3DVERTEXELEMENT9 *vertexElements, IDirect3DVertexDeclaration9 **declaration);
		long __stdcall SetVertexDeclaration(IDirect3DVertexDeclaration9 *declaration);
		long __stdcall GetVertexDeclaration(IDirect3DVertexDeclaration9 **declaration);
		long __stdcall SetFVF(unsigned long FVF);
		long __stdcall GetFVF(unsigned long *FVF);
		long __stdcall CreateVertexShader(const unsigned long *function, IDirect3DVertexShader9 **shader);
		long __stdcall SetVertexShader(IDirect3DVertexShader9 *shader);
		long __stdcall GetVertexShader(IDirect3DVertexShader9 **shader);
		long __stdcall SetVertexShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count);
		long __stdcall GetVertexShaderConstantF(unsigned int startRegister, float *constantData, unsigned int count);
		long __stdcall SetVertexShaderConstantI(unsigned int startRegister, const int *constantData, unsigned int count);
		long __stdcall GetVertexShaderConstantI(unsigned int startRegister, int *constantData, unsigned int count);
		long __stdcall SetVertexShaderConstantB(unsigned int startRegister, const int *constantData, unsigned int count);
		long __stdcall GetVertexShaderConstantB(unsigned int startRegister, int *constantData, unsigned int count);
		long __stdcall SetStreamSource(unsigned int stream, IDirect3DVertexBuffer9 *data, unsigned int offset, unsigned int stride);
		long __stdcall GetStreamSource(unsigned int streamNumber, IDirect3DVertexBuffer9 **streamData, unsigned int *offset, unsigned int *stride);
		long __stdcall SetStreamSourceFreq(unsigned int streamNumber, unsigned int divider);
		long __stdcall GetStreamSourceFreq(unsigned int streamNumber, unsigned int *divider);
		long __stdcall SetIndices(IDirect3DIndexBuffer9 *indexData);
		long __stdcall GetIndices(IDirect3DIndexBuffer9 **indexData);
		long __stdcall CreatePixelShader(const unsigned long *function, IDirect3DPixelShader9 **shader);
		long __stdcall SetPixelShader(IDirect3DPixelShader9 *shader);
		long __stdcall GetPixelShader(IDirect3DPixelShader9 **shader);
		long __stdcall SetPixelShaderConstantI(unsigned int startRegister, const int *constantData, unsigned int count);
		long __stdcall GetPixelShaderConstantI(unsigned int startRegister, int *constantData, unsigned int count);
		long __stdcall SetPixelShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count);
		long __stdcall GetPixelShaderConstantF(unsigned int startRegister, float *constantData, unsigned int count);
		long __stdcall SetPixelShaderConstantB(unsigned int startRegister, const int *constantData, unsigned int count);
		long __stdcall GetPixelShaderConstantB(unsigned int startRegister, int *constantData, unsigned int count);
		long __stdcall DrawRectPatch(unsigned int handle, const float *numSegs, const D3DRECTPATCH_INFO *rectPatchInfo);
		long __stdcall DrawTriPatch(unsigned int handle, const float *numSegs, const D3DTRIPATCH_INFO *triPatchInfo);
		long __stdcall DeletePatch(unsigned int handle);
		long __stdcall CreateQuery(D3DQUERYTYPE type, IDirect3DQuery9 **query);
		
		// IDirect3DDevice9Ex methods
		long __stdcall SetConvolutionMonoKernel(UINT,UINT,float *,float *);
		long __stdcall ComposeRects(IDirect3DSurface9 *,IDirect3DSurface9 *,IDirect3DVertexBuffer9 *,UINT,IDirect3DVertexBuffer9 *,D3DCOMPOSERECTSOP,int,int);
		long __stdcall PresentEx(const RECT *,const RECT *,HWND,const RGNDATA *,DWORD);
		long __stdcall GetGPUThreadPriority(int *priority);
		long __stdcall SetGPUThreadPriority(int priority);
		long __stdcall WaitForVBlank(unsigned int swapChain);
		long __stdcall CheckResourceResidency(IDirect3DResource9 **resourceArray, unsigned int numResources);
		long __stdcall SetMaximumFrameLatency(unsigned int maxLatency);
		long __stdcall GetMaximumFrameLatency(unsigned int *maxLatency);
		long __stdcall CheckDeviceState(HWND destinationWindow);
		long __stdcall CreateRenderTargetEx(unsigned int width, unsigned int height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multiSampleType, unsigned long multiSampleQuality, int lockable, IDirect3DSurface9 **surface, void **sharedHandle, unsigned long usage);
		long __stdcall CreateOffscreenPlainSurfaceEx(unsigned int width, unsigned int height, D3DFORMAT format, D3DPOOL pool, IDirect3DSurface9 **surface, void **sharedHandle, unsigned long usage);
		long __stdcall CreateDepthStencilSurfaceEx(unsigned int width, unsigned int height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multiSampleType, unsigned long multiSampleQuality, int discard, IDirect3DSurface9 **surface, void **sharedHandle, unsigned long usage);
		long __stdcall ResetEx(D3DPRESENT_PARAMETERS *presentParameters, D3DDISPLAYMODEEX *fullscreenDisplayMode);
		long __stdcall GetDisplayModeEx(unsigned int swapChain, D3DDISPLAYMODEEX *mode, D3DDISPLAYROTATION *rotation);

	private:
		Direct3D9Ex *const d3d9ex;
	};
}

#endif // D3D9_Direct3DDevice9Ex_hpp
