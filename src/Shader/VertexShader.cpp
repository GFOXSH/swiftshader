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

#include "VertexShader.hpp"

#include "Vertex.hpp"
#include "Debug.hpp"

#include <string.h>

namespace sw
{
	VertexShader::VertexShader(const VertexShader *vs) : Shader()
	{
		version = 0x0300;
		positionRegister = Pos;
		pointSizeRegister = -1;   // No vertex point size
		instanceIdDeclared = false;

		for(int i = 0; i < MAX_INPUT_ATTRIBUTES; i++)
		{
			input[i] = Semantic(-1, -1);
		}

		if(vs)   // Make a copy
		{
			for(size_t i = 0; i < vs->getLength(); i++)
			{
				append(new sw::Shader::Instruction(*vs->getInstruction(i)));
			}

			memcpy(output, vs->output, sizeof(output));
			memcpy(input, vs->input, sizeof(input));
			positionRegister = vs->positionRegister;
			pointSizeRegister = vs->pointSizeRegister;
			instanceIdDeclared = vs->instanceIdDeclared;
			usedSamplers = vs->usedSamplers;

			optimize();
			analyze();
		}
	}

	VertexShader::VertexShader(const unsigned long *token) : Shader()
	{
		parse(token);

		positionRegister = Pos;
		pointSizeRegister = -1;   // No vertex point size
		instanceIdDeclared = false;

		for(int i = 0; i < MAX_INPUT_ATTRIBUTES; i++)
		{
			input[i] = Semantic(-1, -1);
		}

		optimize();
		analyze();
	}

	VertexShader::~VertexShader()
	{
	}

	int VertexShader::validate(const unsigned long *const token)
	{
		if(!token)
		{
			return 0;
		}

		unsigned short version = (unsigned short)(token[0] & 0x0000FFFF);
		unsigned char majorVersion = (unsigned char)((token[0] & 0x0000FF00) >> 8);
		ShaderType shaderType = (ShaderType)((token[0] & 0xFFFF0000) >> 16);

		if(shaderType != SHADER_VERTEX || majorVersion > 3)
		{
			return 0;
		}

		int instructionCount = 1;

		for(int i = 0; token[i] != 0x0000FFFF; i++)
		{
			if((token[i] & 0x0000FFFF) == 0x0000FFFE)   // Comment token
			{
				int length = (token[i] & 0x7FFF0000) >> 16;

				i += length;
			}
			else
			{
				Shader::Opcode opcode = (Shader::Opcode)(token[i] & 0x0000FFFF);

				switch(opcode)
				{
				case Shader::OPCODE_TEXCOORD:
				case Shader::OPCODE_TEXKILL:
				case Shader::OPCODE_TEX:
				case Shader::OPCODE_TEXBEM:
				case Shader::OPCODE_TEXBEML:
				case Shader::OPCODE_TEXREG2AR:
				case Shader::OPCODE_TEXREG2GB:
				case Shader::OPCODE_TEXM3X2PAD:
				case Shader::OPCODE_TEXM3X2TEX:
				case Shader::OPCODE_TEXM3X3PAD:
				case Shader::OPCODE_TEXM3X3TEX:
				case Shader::OPCODE_RESERVED0:
				case Shader::OPCODE_TEXM3X3SPEC:
				case Shader::OPCODE_TEXM3X3VSPEC:
				case Shader::OPCODE_TEXREG2RGB:
				case Shader::OPCODE_TEXDP3TEX:
				case Shader::OPCODE_TEXM3X2DEPTH:
				case Shader::OPCODE_TEXDP3:
				case Shader::OPCODE_TEXM3X3:
				case Shader::OPCODE_TEXDEPTH:
				case Shader::OPCODE_CMP0:
				case Shader::OPCODE_BEM:
				case Shader::OPCODE_DP2ADD:
				case Shader::OPCODE_DFDX:
				case Shader::OPCODE_DFDY:
				case Shader::OPCODE_TEXLDD:
					return 0;   // Unsupported operation
				default:
					instructionCount++;
					break;
				}

				i += size(token[i], version);
			}
		}

		return instructionCount;
	}

	bool VertexShader::containsTexldl() const
	{
		return texldl;
	}

	void VertexShader::analyze()
	{
		analyzeInput();
		analyzeOutput();
		analyzeDirtyConstants();
		analyzeTexldl();
		analyzeDynamicBranching();
		analyzeSamplers();
		analyzeCallSites();
		analyzeDynamicIndexing();
	}

	void VertexShader::analyzeInput()
	{
		for(unsigned int i = 0; i < instruction.size(); i++)
		{
			if(instruction[i]->opcode == Shader::OPCODE_DCL &&
			   instruction[i]->dst.type == Shader::PARAMETER_INPUT)
			{
				int index = instruction[i]->dst.index;

				input[index] = Semantic(instruction[i]->usage, instruction[i]->usageIndex);
			}
		}
	}

	void VertexShader::analyzeOutput()
	{
		if(version < 0x0300)
		{
			output[Pos][0] = Semantic(Shader::USAGE_POSITION, 0);
			output[Pos][1] = Semantic(Shader::USAGE_POSITION, 0);
			output[Pos][2] = Semantic(Shader::USAGE_POSITION, 0);
			output[Pos][3] = Semantic(Shader::USAGE_POSITION, 0);

			for(unsigned int i = 0; i < instruction.size(); i++)
			{
				const DestinationParameter &dst = instruction[i]->dst;

				switch(dst.type)
				{
				case Shader::PARAMETER_RASTOUT:
					switch(dst.index)
					{
					case 0:
						// Position already assumed written
						break;
					case 1:
						output[Fog][0] = Semantic(Shader::USAGE_FOG, 0);
						break;
					case 2:
						output[Pts][1] = Semantic(Shader::USAGE_PSIZE, 0);
						pointSizeRegister = Pts;
						break;
					default: ASSERT(false);
					}
					break;
				case Shader::PARAMETER_ATTROUT:
					if(dst.index == 0)
					{
						if(dst.x) output[D0][0] = Semantic(Shader::USAGE_COLOR, 0);
						if(dst.y) output[D0][1] = Semantic(Shader::USAGE_COLOR, 0);
						if(dst.z) output[D0][2] = Semantic(Shader::USAGE_COLOR, 0);
						if(dst.w) output[D0][3] = Semantic(Shader::USAGE_COLOR, 0);
					}
					else if(dst.index == 1)
					{
						if(dst.x) output[D1][0] = Semantic(Shader::USAGE_COLOR, 1);
						if(dst.y) output[D1][1] = Semantic(Shader::USAGE_COLOR, 1);
						if(dst.z) output[D1][2] = Semantic(Shader::USAGE_COLOR, 1);
						if(dst.w) output[D1][3] = Semantic(Shader::USAGE_COLOR, 1);
					}
					else ASSERT(false);
					break;
				case Shader::PARAMETER_TEXCRDOUT:
					if(dst.x) output[T0 + dst.index][0] = Semantic(Shader::USAGE_TEXCOORD, dst.index);
					if(dst.y) output[T0 + dst.index][1] = Semantic(Shader::USAGE_TEXCOORD, dst.index);
					if(dst.z) output[T0 + dst.index][2] = Semantic(Shader::USAGE_TEXCOORD, dst.index);
					if(dst.w) output[T0 + dst.index][3] = Semantic(Shader::USAGE_TEXCOORD, dst.index);
					break;
				default:
					break;
				}
			}
		}
		else   // Shader Model 3.0 input declaration
		{
			for(unsigned int i = 0; i < instruction.size(); i++)
			{
				if(instruction[i]->opcode == Shader::OPCODE_DCL &&
				   instruction[i]->dst.type == Shader::PARAMETER_OUTPUT)
				{
					unsigned char usage = instruction[i]->usage;
					unsigned char usageIndex = instruction[i]->usageIndex;

					const DestinationParameter &dst = instruction[i]->dst;

					if(dst.x) output[dst.index][0] = Semantic(usage, usageIndex);
					if(dst.y) output[dst.index][1] = Semantic(usage, usageIndex);
					if(dst.z) output[dst.index][2] = Semantic(usage, usageIndex);
					if(dst.w) output[dst.index][3] = Semantic(usage, usageIndex);

					if(usage == Shader::USAGE_POSITION && usageIndex == 0)
					{
						positionRegister = dst.index;
					}

					if(usage == Shader::USAGE_PSIZE && usageIndex == 0)
					{
						pointSizeRegister = dst.index;
					}
				}
			}
		}
	}

	void VertexShader::analyzeTexldl()
	{
		texldl = false;

		for(unsigned int i = 0; i < instruction.size(); i++)
		{
			if(instruction[i]->opcode == Shader::OPCODE_TEXLDL)
			{
				texldl = true;

				break;
			}
		}
	}
}
