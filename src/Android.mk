LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CLANG := true

LOCAL_MODULE := swiftshader_top
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	Common/CPUID.cpp \
	Common/Configurator.cpp \
	Common/DebugAndroid.cpp \
	Common/GrallocAndroid.cpp \
	Common/Half.cpp \
	Common/Math.cpp \
	Common/Memory.cpp \
	Common/Resource.cpp \
	Common/Socket.cpp \
	Common/Thread.cpp \
	Common/Timer.cpp

LOCAL_SRC_FILES += \
	Main/Config.cpp \
	Main/FrameBuffer.cpp \
	Main/FrameBufferAndroid.cpp \
	Main/Logo.cpp \
	Main/Register.cpp \
	Main/SwiftConfig.cpp \
	Main/crc.cpp \
	Main/serialvalid.cpp \

LOCAL_SRC_FILES += \
	Reactor/Nucleus.cpp \
	Reactor/Routine.cpp \
	Reactor/RoutineManager.cpp

LOCAL_SRC_FILES += \
	Renderer/Blitter.cpp \
	Renderer/Clipper.cpp \
	Renderer/Color.cpp \
	Renderer/Context.cpp \
	Renderer/Matrix.cpp \
	Renderer/PixelProcessor.cpp \
	Renderer/Plane.cpp \
	Renderer/Point.cpp \
	Renderer/QuadRasterizer.cpp \
	Renderer/Rasterizer.cpp \
	Renderer/Renderer.cpp \
	Renderer/Sampler.cpp \
	Renderer/SetupProcessor.cpp \
	Renderer/Surface.cpp \
	Renderer/TextureStage.cpp \
	Renderer/Vector.cpp \
	Renderer/VertexProcessor.cpp \

LOCAL_SRC_FILES += \
	Shader/Constants.cpp \
	Shader/PixelRoutine.cpp \
	Shader/PixelShader.cpp \
	Shader/SamplerCore.cpp \
	Shader/SetupRoutine.cpp \
	Shader/Shader.cpp \
	Shader/ShaderCore.cpp \
	Shader/VertexPipeline.cpp \
	Shader/VertexProgram.cpp \
	Shader/VertexRoutine.cpp \
	Shader/VertexShader.cpp \

LOCAL_SRC_FILES += \
	OpenGL/common/AndroidCommon.cpp \
	OpenGL/common/Image.cpp \
	OpenGL/common/NameSpace.cpp \
	OpenGL/common/Object.cpp \
	OpenGL/common/MatrixStack.cpp \

LOCAL_CFLAGS += -DLOG_TAG=\"swiftshader\" \
	-Wno-unused-parameter \
	-DDISPLAY_LOGO=0 \
	-Wno-implicit-exception-spec-mismatch \
	-Wno-overloaded-virtual

LOCAL_CFLAGS += -fno-operator-names -msse2 -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS
LOCAL_CFLAGS += -std=c++11

# Android's make system also uses NDEBUG, so we need to set/unset it forcefully
# Uncomment for ON:
# LOCAL_CFLAGS += -UNDEBUG -g -O0
# Uncomment for OFF:
LOCAL_CFLAGS += -fomit-frame-pointer -ffunction-sections -fdata-sections -DANGLE_DISABLE_TRACE

LOCAL_C_INCLUDES += \
	bionic \
	$(GCE_STLPORT_INCLUDES) \
        $(LOCAL_PATH)/OpenGL/include \
        $(LOCAL_PATH)/OpenGL/ \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/Renderer/ \
        $(LOCAL_PATH)/Common/ \
        $(LOCAL_PATH)/Shader/ \
        $(LOCAL_PATH)/LLVM/include \
        $(LOCAL_PATH)/Main/

include $(BUILD_STATIC_LIBRARY)
