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

#include "VertexRoutine.hpp"

#include "VertexShader.hpp"
#include "Vertex.hpp"
#include "Half.hpp"
#include "Renderer.hpp"
#include "Constants.hpp"
#include "Debug.hpp"

namespace sw
{
	extern bool halfIntegerCoordinates;     // Pixel centers are not at integer coordinates
	extern bool symmetricNormalizedDepth;   // [-1, 1] instead of [0, 1]

	VertexRoutine::VertexRoutine(const VertexProcessor::State &state, const VertexShader *shader) : state(state), shader(shader)
	{
		routine = 0;
	}

	VertexRoutine::~VertexRoutine()
	{
	}

	void VertexRoutine::generate()
	{
		Function<Void, Pointer<Byte>, Pointer<Byte>, Pointer<Byte>, Pointer<Byte> > function;
		{
			Pointer<Byte> vertex(function.arg(0));
			Pointer<Byte> batch(function.arg(1));
			Pointer<Byte> task(function.arg(2));
			Pointer<Byte> data(function.arg(3));

			const bool texldl = state.shaderContainsTexldl;

			Pointer<Byte> cache = task + OFFSET(VertexTask,vertexCache);
			Pointer<Byte> vertexCache = cache + OFFSET(VertexCache,vertex);
			Pointer<Byte> tagCache = cache + OFFSET(VertexCache,tag);

			UInt vertexCount = *Pointer<UInt>(task + OFFSET(VertexTask,vertexCount));

			Registers r(shader);
			r.data = data;
			r.constants = *Pointer<Pointer<Byte> >(data + OFFSET(DrawData,constants));
			if(shader && shader->instanceIdDeclared)
			{
				r.instanceID = *Pointer<Int>(data + OFFSET(DrawData, instanceID));
			}

			Do
			{
				UInt index = *Pointer<UInt>(batch);
				UInt tagIndex = index & 0x0000003C;
				UInt indexQ = !texldl ? UInt(index & 0xFFFFFFFC) : index;   // FIXME: TEXLDL hack to have independent LODs, hurts performance.

				If(*Pointer<UInt>(tagCache + tagIndex) != indexQ)
				{
					*Pointer<UInt>(tagCache + tagIndex) = indexQ;

					readInput(r, indexQ);
					pipeline(r);
					postTransform(r);
					computeClipFlags(r);

					Pointer<Byte> cacheLine0 = vertexCache + tagIndex * UInt((int)sizeof(Vertex));
					writeCache(cacheLine0, r);
				}

				UInt cacheIndex = index & 0x0000003F;
				Pointer<Byte> cacheLine = vertexCache + cacheIndex * UInt((int)sizeof(Vertex));
				writeVertex(vertex, cacheLine);

				vertex += sizeof(Vertex);
				batch += sizeof(unsigned int);
				vertexCount--;
			}
			Until(vertexCount == 0)

			Return();
		}

		routine = function(L"VertexRoutine_%0.8X", state.shaderID);
	}

	Routine *VertexRoutine::getRoutine()
	{
		return routine;
	}

	void VertexRoutine::readInput(Registers &r, UInt &index)
	{
		for(int i = 0; i < VERTEX_ATTRIBUTES; i++)
		{
			Pointer<Byte> input = *Pointer<Pointer<Byte> >(r.data + OFFSET(DrawData,input) + sizeof(void*) * i);
			UInt stride = *Pointer<UInt>(r.data + OFFSET(DrawData,stride) + sizeof(unsigned int) * i);

			r.v[i] = readStream(r, input, stride, state.input[i], index);
		}
	}

	void VertexRoutine::computeClipFlags(Registers &r)
	{
		int pos = state.positionRegister;

		// Backtransform
		if(state.preTransformed)
		{
			Float4 rhw = Float4(1.0f) / r.o[pos].w;

			Float4 W = *Pointer<Float4>(r.data + OFFSET(DrawData,Wx16)) * Float4(1.0f / 16.0f);
			Float4 H = *Pointer<Float4>(r.data + OFFSET(DrawData,Hx16)) * Float4(1.0f / 16.0f);
			Float4 L = *Pointer<Float4>(r.data + OFFSET(DrawData,X0x16)) * Float4(1.0f / 16.0f);
			Float4 T = *Pointer<Float4>(r.data + OFFSET(DrawData,Y0x16)) * Float4(1.0f / 16.0f);

			r.o[pos].x = (r.o[pos].x - L) / W * rhw;
			r.o[pos].y = (r.o[pos].y - T) / H * rhw;
			r.o[pos].z = r.o[pos].z * rhw;
			r.o[pos].w = rhw;
		}

		if(state.superSampling)
		{
			r.o[pos].x = r.o[pos].x + *Pointer<Float4>(r.data + OFFSET(DrawData,XXXX)) * r.o[pos].w;
			r.o[pos].y = r.o[pos].y + *Pointer<Float4>(r.data + OFFSET(DrawData,YYYY)) * r.o[pos].w;
		}

		Int4 maxX = CmpLT(r.o[pos].w, r.o[pos].x);
		Int4 maxY = CmpLT(r.o[pos].w, r.o[pos].y);
		Int4 maxZ = CmpLT(r.o[pos].w, r.o[pos].z);

		Int4 minX = CmpNLE(-r.o[pos].w, r.o[pos].x);
		Int4 minY = CmpNLE(-r.o[pos].w, r.o[pos].y);
		Int4 minZ = CmpNLE(Float4(0.0f), r.o[pos].z);

		Int flags;

		flags = SignMask(maxX);
		r.clipFlags = *Pointer<Int>(r.constants + OFFSET(Constants,maxX) + flags * 4);   // FIXME: Array indexing
		flags = SignMask(maxY);
		r.clipFlags |= *Pointer<Int>(r.constants + OFFSET(Constants,maxY) + flags * 4);
		flags = SignMask(maxZ);
		r.clipFlags |= *Pointer<Int>(r.constants + OFFSET(Constants,maxZ) + flags * 4);
		flags = SignMask(minX);
		r.clipFlags |= *Pointer<Int>(r.constants + OFFSET(Constants,minX) + flags * 4);
		flags = SignMask(minY);
		r.clipFlags |= *Pointer<Int>(r.constants + OFFSET(Constants,minY) + flags * 4);
		flags = SignMask(minZ);
		r.clipFlags |= *Pointer<Int>(r.constants + OFFSET(Constants,minZ) + flags * 4);

		Int4 finiteX = CmpLE(Abs(r.o[pos].x), *Pointer<Float4>(r.constants + OFFSET(Constants,maxPos)));
		Int4 finiteY = CmpLE(Abs(r.o[pos].y), *Pointer<Float4>(r.constants + OFFSET(Constants,maxPos)));
		Int4 finiteZ = CmpLE(Abs(r.o[pos].z), *Pointer<Float4>(r.constants + OFFSET(Constants,maxPos)));

		flags = SignMask(finiteX & finiteY & finiteZ);
		r.clipFlags |= *Pointer<Int>(r.constants + OFFSET(Constants,fini) + flags * 4);

		if(state.preTransformed)
		{
			r.clipFlags &= 0xFBFBFBFB;   // Don't clip against far clip plane
		}
	}

	Vector4f VertexRoutine::readStream(Registers &r, Pointer<Byte> &buffer, UInt &stride, const Stream &stream, const UInt &index)
	{
		const bool texldl = state.shaderContainsTexldl;

		Vector4f v;

		Pointer<Byte> source0 = buffer + index * stride;
		Pointer<Byte> source1 = source0 + (!texldl ? stride : 0);
		Pointer<Byte> source2 = source1 + (!texldl ? stride : 0);
		Pointer<Byte> source3 = source2 + (!texldl ? stride : 0);

		switch(stream.type)
		{
		case STREAMTYPE_FLOAT:
			{
				if(stream.count == 0)
				{
					// Null stream, all default components
				}
				else if(stream.count == 1)
				{
					v.x.x = *Pointer<Float>(source0);
					v.x.y = *Pointer<Float>(source1);
					v.x.z = *Pointer<Float>(source2);
					v.x.w = *Pointer<Float>(source3);
				}
				else
				{
					v.x = *Pointer<Float4>(source0);
					v.y = *Pointer<Float4>(source1);
					v.z = *Pointer<Float4>(source2);
					v.w = *Pointer<Float4>(source3);

					transpose4xN(v.x, v.y, v.z, v.w, stream.count);
				}
			}
			break;
		case STREAMTYPE_BYTE:
			{
				v.x = Float4(*Pointer<Byte4>(source0));
				v.y = Float4(*Pointer<Byte4>(source1));
				v.z = Float4(*Pointer<Byte4>(source2));
				v.w = Float4(*Pointer<Byte4>(source3));

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);

				if(stream.normalized)
				{
					if(stream.count >= 1) v.x *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleByte));
					if(stream.count >= 2) v.y *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleByte));
					if(stream.count >= 3) v.z *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleByte));
					if(stream.count >= 4) v.w *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleByte));
				}
			}
			break;
		case STREAMTYPE_SBYTE:
			{
				v.x = Float4(*Pointer<SByte4>(source0));
				v.y = Float4(*Pointer<SByte4>(source1));
				v.z = Float4(*Pointer<SByte4>(source2));
				v.w = Float4(*Pointer<SByte4>(source3));

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);

				if(stream.normalized)
				{
					if(stream.count >= 1) v.x *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleSByte));
					if(stream.count >= 2) v.y *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleSByte));
					if(stream.count >= 3) v.z *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleSByte));
					if(stream.count >= 4) v.w *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleSByte));
				}
			}
			break;
		case STREAMTYPE_COLOR:
			{
				v.x = Float4(*Pointer<Byte4>(source0)) * *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleByte));
				v.y = Float4(*Pointer<Byte4>(source1)) * *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleByte));
				v.z = Float4(*Pointer<Byte4>(source2)) * *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleByte));
				v.w = Float4(*Pointer<Byte4>(source3)) * *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleByte));

				transpose4x4(v.x, v.y, v.z, v.w);

				// Swap red and blue
				Float4 t = v.x;
				v.x = v.z;
				v.z = t;
			}
			break;
		case STREAMTYPE_SHORT:
			{
				v.x = Float4(*Pointer<Short4>(source0));
				v.y = Float4(*Pointer<Short4>(source1));
				v.z = Float4(*Pointer<Short4>(source2));
				v.w = Float4(*Pointer<Short4>(source3));
			
				transpose4xN(v.x, v.y, v.z, v.w, stream.count);

				if(stream.normalized)
				{
					if(stream.count >= 1) v.x *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleShort));
					if(stream.count >= 2) v.y *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleShort));
					if(stream.count >= 3) v.z *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleShort));
					if(stream.count >= 4) v.w *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleShort));
				}			
			}
			break;
		case STREAMTYPE_USHORT:
			{
				v.x = Float4(*Pointer<UShort4>(source0));
				v.y = Float4(*Pointer<UShort4>(source1));
				v.z = Float4(*Pointer<UShort4>(source2));
				v.w = Float4(*Pointer<UShort4>(source3));
			
				transpose4xN(v.x, v.y, v.z, v.w, stream.count);

				if(stream.normalized)
				{
					if(stream.count >= 1) v.x *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleUShort));
					if(stream.count >= 2) v.y *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleUShort));
					if(stream.count >= 3) v.z *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleUShort));
					if(stream.count >= 4) v.w *= *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleUShort));
				}
			}
			break;
		case STREAMTYPE_UDEC3:
			{
				// FIXME: Vectorize
				{
					Int x, y, z;
					
					x = y = z = *Pointer<Int>(source0);

					v.x.x = Float(x & 0x000003FF);
					v.x.y = Float(y & 0x000FFC00);
					v.x.z = Float(z & 0x3FF00000);
				}

				{
					Int x, y, z;
					
					x = y = z = *Pointer<Int>(source1);

					v.y.x = Float(x & 0x000003FF);
					v.y.y = Float(y & 0x000FFC00);
					v.y.z = Float(z & 0x3FF00000);
				}

				{
					Int x, y, z;
					
					x = y = z = *Pointer<Int>(source2);

					v.z.x = Float(x & 0x000003FF);
					v.z.y = Float(y & 0x000FFC00);
					v.z.z = Float(z & 0x3FF00000);
				}

				{
					Int x, y, z;
					
					x = y = z = *Pointer<Int>(source3);

					v.w.x = Float(x & 0x000003FF);
					v.w.y = Float(y & 0x000FFC00);
					v.w.z = Float(z & 0x3FF00000);
				}

				transpose4x3(v.x, v.y, v.z, v.w);

				v.y *= Float4(1.0f / 0x00000400);
				v.z *= Float4(1.0f / 0x00100000);
			}
			break;
		case STREAMTYPE_DEC3N:
			{
				// FIXME: Vectorize
				{
					Int x, y, z;
					
					x = y = z = *Pointer<Int>(source0);

					v.x.x = Float((x << 22) & 0xFFC00000);
					v.x.y = Float((y << 12) & 0xFFC00000);
					v.x.z = Float((z << 2)  & 0xFFC00000);
				}

				{
					Int x, y, z;
					
					x = y = z = *Pointer<Int>(source1);

					v.y.x = Float((x << 22) & 0xFFC00000);
					v.y.y = Float((y << 12) & 0xFFC00000);
					v.y.z = Float((z << 2)  & 0xFFC00000);
				}

				{
					Int x, y, z;
					
					x = y = z = *Pointer<Int>(source2);

					v.z.x = Float((x << 22) & 0xFFC00000);
					v.z.y = Float((y << 12) & 0xFFC00000);
					v.z.z = Float((z << 2)  & 0xFFC00000);
				}

				{
					Int x, y, z;
					
					x = y = z = *Pointer<Int>(source3);

					v.w.x = Float((x << 22) & 0xFFC00000);
					v.w.y = Float((y << 12) & 0xFFC00000);
					v.w.z = Float((z << 2)  & 0xFFC00000);
				}

				transpose4x3(v.x, v.y, v.z, v.w);

				v.x *= Float4(1.0f / 0x00400000 / 511.0f);
				v.y *= Float4(1.0f / 0x00400000 / 511.0f);
				v.z *= Float4(1.0f / 0x00400000 / 511.0f);
			}
			break;
		case STREAMTYPE_FIXED:
			{
				v.x = Float4(*Pointer<Int4>(source0)) * *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleFixed));
				v.y = Float4(*Pointer<Int4>(source1)) * *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleFixed));
				v.z = Float4(*Pointer<Int4>(source2)) * *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleFixed));
				v.w = Float4(*Pointer<Int4>(source3)) * *Pointer<Float4>(r.constants + OFFSET(Constants,unscaleFixed));

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);
			}
			break;
		case STREAMTYPE_HALF:
			{
				if(stream.count >= 1)
				{
					UShort x0 = *Pointer<UShort>(source0 + 0);
					UShort x1 = *Pointer<UShort>(source1 + 0);
					UShort x2 = *Pointer<UShort>(source2 + 0);
					UShort x3 = *Pointer<UShort>(source3 + 0);

					v.x.x = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(x0) * 4);
					v.x.y = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(x1) * 4);
					v.x.z = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(x2) * 4);
					v.x.w = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(x3) * 4);
				}

				if(stream.count >= 2)
				{
					UShort y0 = *Pointer<UShort>(source0 + 2);
					UShort y1 = *Pointer<UShort>(source1 + 2);
					UShort y2 = *Pointer<UShort>(source2 + 2);
					UShort y3 = *Pointer<UShort>(source3 + 2);

					v.y.x = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(y0) * 4);
					v.y.y = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(y1) * 4);
					v.y.z = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(y2) * 4);
					v.y.w = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(y3) * 4);
				}

				if(stream.count >= 3)
				{
					UShort z0 = *Pointer<UShort>(source0 + 4);
					UShort z1 = *Pointer<UShort>(source1 + 4);
					UShort z2 = *Pointer<UShort>(source2 + 4);
					UShort z3 = *Pointer<UShort>(source3 + 4);

					v.z.x = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(z0) * 4);
					v.z.y = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(z1) * 4);
					v.z.z = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(z2) * 4);
					v.z.w = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(z3) * 4);
				}

				if(stream.count >= 4)
				{
					UShort w0 = *Pointer<UShort>(source0 + 6);
					UShort w1 = *Pointer<UShort>(source1 + 6);
					UShort w2 = *Pointer<UShort>(source2 + 6);
					UShort w3 = *Pointer<UShort>(source3 + 6);

					v.w.x = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(w0) * 4);
					v.w.y = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(w1) * 4);
					v.w.z = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(w2) * 4);
					v.w.w = *Pointer<Float>(r.constants + OFFSET(Constants,half2float) + Int(w3) * 4);
				}
			}
			break;
		case STREAMTYPE_INDICES:
			{
				v.x.x = *Pointer<Float>(source0);
				v.x.y = *Pointer<Float>(source1);
				v.x.z = *Pointer<Float>(source2);
				v.x.w = *Pointer<Float>(source3);
			}
			break;
		default:
			ASSERT(false);
		}

		if(stream.count < 1) v.x = Float4(0.0f);
		if(stream.count < 2) v.y = Float4(0.0f);
		if(stream.count < 3) v.z = Float4(0.0f);
		if(stream.count < 4) v.w = Float4(1.0f);

		return v;
	}

	void VertexRoutine::postTransform(Registers &r)
	{
		int pos = state.positionRegister;

		if(!halfIntegerCoordinates)
		{
			r.o[pos].x = r.o[pos].x + *Pointer<Float4>(r.data + OFFSET(DrawData,halfPixelX)) * r.o[pos].w;
			r.o[pos].y = r.o[pos].y + *Pointer<Float4>(r.data + OFFSET(DrawData,halfPixelY)) * r.o[pos].w;
		}

		if(symmetricNormalizedDepth && !state.fixedFunction)
		{
			r.o[pos].z = (r.o[pos].z + r.o[pos].w) * Float4(0.5f);
		}
	}

	void VertexRoutine::writeCache(Pointer<Byte> &cacheLine, Registers &r)
	{
		Vector4f v;

		for(int i = 0; i < 12; i++)
		{
			if(state.output[i].write)
			{
				v.x = r.o[i].x;
				v.y = r.o[i].y;
				v.z = r.o[i].z;
				v.w = r.o[i].w;

				if(state.output[i].xClamp)
				{
					v.x = Max(v.x, Float4(0.0f));
					v.x = Min(v.x, Float4(1.0f));
				}

				if(state.output[i].yClamp)
				{
					v.y = Max(v.y, Float4(0.0f));
					v.y = Min(v.y, Float4(1.0f));
				}

				if(state.output[i].zClamp)
				{
					v.z = Max(v.z, Float4(0.0f));
					v.z = Min(v.z, Float4(1.0f));
				}

				if(state.output[i].wClamp)
				{
					v.w = Max(v.w, Float4(0.0f));
					v.w = Min(v.w, Float4(1.0f));
				}

				if(state.output[i].write == 0x01)
				{
					*Pointer<Float>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 0) = v.x.x;
					*Pointer<Float>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 1) = v.x.y;
					*Pointer<Float>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 2) = v.x.z;
					*Pointer<Float>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 3) = v.x.w;
				}
				else
				{
					if(state.output[i].write == 0x02)
					{
						transpose2x4(v.x, v.y, v.z, v.w);
					}
					else
					{
						transpose4x4(v.x, v.y, v.z, v.w);
					}

					*Pointer<Float4>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 0, 16) = v.x;
					*Pointer<Float4>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 1, 16) = v.y;
					*Pointer<Float4>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 2, 16) = v.z;
					*Pointer<Float4>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 3, 16) = v.w;
				}
			}
		}

		*Pointer<Int>(cacheLine + OFFSET(Vertex,clipFlags) + sizeof(Vertex) * 0) = (r.clipFlags >> 0)  & 0x0000000FF;   // FIXME: unsigned char Vertex::clipFlags
		*Pointer<Int>(cacheLine + OFFSET(Vertex,clipFlags) + sizeof(Vertex) * 1) = (r.clipFlags >> 8)  & 0x0000000FF;
		*Pointer<Int>(cacheLine + OFFSET(Vertex,clipFlags) + sizeof(Vertex) * 2) = (r.clipFlags >> 16) & 0x0000000FF;
		*Pointer<Int>(cacheLine + OFFSET(Vertex,clipFlags) + sizeof(Vertex) * 3) = (r.clipFlags >> 24) & 0x0000000FF;

		int pos = state.positionRegister;

		v.x = r.o[pos].x;
		v.y = r.o[pos].y;
		v.z = r.o[pos].z;
		v.w = r.o[pos].w;

		Float4 w = As<Float4>(As<Int4>(v.w) | (As<Int4>(CmpEQ(v.w, Float4(0.0f))) & As<Int4>(Float4(1.0f))));
		Float4 rhw = Float4(1.0f) / w;

		v.x = As<Float4>(RoundInt(*Pointer<Float4>(r.data + OFFSET(DrawData,X0x16)) + v.x * rhw * *Pointer<Float4>(r.data + OFFSET(DrawData,Wx16))));
		v.y = As<Float4>(RoundInt(*Pointer<Float4>(r.data + OFFSET(DrawData,Y0x16)) + v.y * rhw * *Pointer<Float4>(r.data + OFFSET(DrawData,Hx16))));
		v.z = v.z * rhw;
		v.w = rhw;

		transpose4x4(v.x, v.y, v.z, v.w);

		*Pointer<Float4>(cacheLine + OFFSET(Vertex,X) + sizeof(Vertex) * 0, 16) = v.x;
		*Pointer<Float4>(cacheLine + OFFSET(Vertex,X) + sizeof(Vertex) * 1, 16) = v.y;
		*Pointer<Float4>(cacheLine + OFFSET(Vertex,X) + sizeof(Vertex) * 2, 16) = v.z;
		*Pointer<Float4>(cacheLine + OFFSET(Vertex,X) + sizeof(Vertex) * 3, 16) = v.w;
	}

	void VertexRoutine::writeVertex(Pointer<Byte> &vertex, Pointer<Byte> &cache)
	{
		for(int i = 0; i < 12; i++)
		{
			if(state.output[i].write)
			{
				*Pointer<Float4>(vertex + OFFSET(Vertex,v[i])) = *Pointer<Float4>(cache + OFFSET(Vertex,v[i]));
			}
		}

		*Pointer<Int>(vertex + OFFSET(Vertex,clipFlags)) = *Pointer<Int>(cache + OFFSET(Vertex,clipFlags));
		*Pointer<Float4>(vertex + OFFSET(Vertex,X)) = *Pointer<Float4>(cache + OFFSET(Vertex,X));
	}
}
