﻿/*
================================================================================================

Description	:	Asynchronous Time Warp test utility for OpenGL.
Author		:	J.M.P. van Waveren
Date		:	12/21/2014
Language	:	C99
Format		:	Real tabs with the tab size equal to 4 spaces.
Copyright	:	Copyright (c) 2016 Oculus VR, LLC. All Rights reserved.
			:	Portions copyright (c) 2016 The Brenwill Workshop Ltd. All Rights reserved.


LICENSE
=======

Copyright (c) 2016 Oculus VR, LLC.
Portions of macOS, iOS, functionality copyright (c) 2016 The Brenwill Workshop Ltd.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


DESCRIPTION
===========

This implements the simplest form of time warp transform for OpenGL.
This transform corrects for optical aberration of the optics used in a virtual
reality headset, and only rotates the stereoscopic images based on the very latest
head orientation to reduce the motion-to-photon delay (or end-to-end latency).

This utility can be used to test whether or not a particular combination of hardware,
operating system and graphics driver is capable of rendering stereoscopic pairs of
images, while asynchronously (and ideally concurrently) warping the latest pair of
images onto the display, synchronized with the display refresh without dropping any
frames. Under high system load, the rendering of the stereoscopic images is allowed
to drop frames, but the asynchronous time warp must be able to warp the latest
stereoscopic images onto the display, synchronized with the display refresh
*without ever* dropping any frames.

There is one thread that renders the stereoscopic pairs of images by rendering a scene
to two textures, one for each eye. These eye textures are then handed over to the
asynchronous time warp in a thread safe manner. The asynchronous time warp runs in
another thread and continuously takes the last completed eye textures and warps them
onto the display.

Even though rendering commands are issued concurrently from two separate threads,
most current hardware and drivers serialize these rendering commands because the
hardware cannot actually execute multiple graphics/compute tasks concurrently.
Based on the task switching granularity of the GPU, and on how the rendering
commands are prioritized and serialized, the asynchronous time warp may, or may
not be able to stay synchronized with the display refresh.

On hardware that cannot execute multiple graphics/compute tasks concurrently, the
following is required to keep the asynchronous time warp synchronized with the
display refresh without dropping frames:

	- Context priorities.
	- Fine-grained and low latency priority based task switching.

To significantly reduce the latency in a virtual reality simulation, the asynchronous
time warp needs to be scheduled as close as possible to the display refresh.
In addition to the above requirements, the following is required to achieve this:

	- Accurate timing of the display refresh.
	- Predictable latency and throughput of the time warp execution.


PERFORMANCE
===========

When the frame rate drops, it can be hard to tell whether the stereoscopic rendering,
or the time warp rendering drops frames. Therefore four scrolling bar graphs are
drawn at the bottom left of the screen. Each bar represents a frame. New frames scroll
in on the right and old frames scroll out on the left.

The left-most bar graph represent the frame rate of the stereoscopic rendering (pink).
The next bar graph represents the frame rate of time warp rendering (green). Each bar
that is pink or green respectively reaches the top of the graph and represents a frame
rendered at the display refresh rate. When the frame rate drops, the bars turn red
and become shorter proportional to how much the frame rate drops.

The next two bar graph shows the CPU and GPU time of the stereoscopic rendering (pink),
the time warp rendering (green) and the bar graph rendering (yellow). The times are
stacked in each graph. The full height of a graph represents a full frame time.
For instance, with a 60Hz display refresh rate, the full graph height represents 16.7
milliseconds.


RESOLUTIONS
===========

The rendering resolutions can be changed by adjusting the display resolution, the
eye image resolution, and the eye image MSAA. For each of these there are four levels:

Display Resolution:
	0: 1920 x 1080
	1: 2560 x 1440
	2: 3840 x 2160
	3: 7680 x 4320

Eye image resolution:
	0: 1024 x 1024
	1: 1536 x 1536
	2: 2048 x 2048
	3: 4096 x 4096

Eye image multi-sampling:
	0: 1x MSAA
	1: 2x MSAA
	2: 4x MSAA
	3: 8x MSAA


SCENE WORKLOAD
==============

The graphics work load of the scene that is rendered for each eye can be changed by
adjusting the number of draw calls, the number of triangles per draw call, the fragment
program complexity and the number of samples. For each of these there are four levels:

Number of draw calls:
	0: 8
	1: 64
	2: 512
	3: 4096

Number of triangles per draw call:
	0: 12
	1: 128
	2: 512
	3: 2048

Fragment program complexity:
	0: flat-shaded with 1 light
	1: normal-mapped with 100 lights
	2: normal-mapped with 1000 lights
	3: normal-mapped with 2000 lights

In the lower right corner of the screen there are four indicators that show
the current level for each. The levels are colored: 0 = green, 1 = blue,
2 = yellow and 3 = red.

The scene is normally rendered separately for each eye. However, there is also
an option to render the scene only once for both eyes (multi-view). The left-most
small indicator in the middle-bottom of the screen shows whether or not multi-view
is enabled: gray = off and red = on.


TIMEWARP SETTINGS
=================

The time warp can run in two modes. The first mode only corrects for spatial
aberration and the second mode also corrects for chromatic aberration.
The middle small indicator in the middle-bottom of the screen shows which mode
is used: gray = spatial and red = chromatic.

There are two implementations of the time warp. The first implementation uses
the conventional graphics pipeline and the second implementation uses compute.
The right-most small indicator in the middle-bottom of the screen shows which
implementation is used: gray = graphics and red = compute.


COMMAND-LINE INPUT
==================

The following command-line options can be used to change various settings.

	-f			start fullscreen
	-v <s>		start with V-Sync disabled for this many seconds
	-h			start with head rotation disabled
	-p			start with the simulation paused
	-r <0-3>	set display resolution level
	-b <0-3>	set eye image resolution level
	-s <0-3>	set eye image multi-sampling level
	-q <0-3>	set per eye draw calls level
	-w <0-3>	set per eye triangles per draw call level
	-e <0-3>	set per eye fragment program complexity level
	-m <0-1>	enable/disable multi-view
	-c <0-1>	enable/disable correction for chromatic aberration
	-i <name>	set time warp implementation: graphics, compute
	-z <name>	set the render mode: atw, tw, scene
	-g			hide graphs
	-l <s>		log 10 frames of OpenGL commands after this many seconds
	-d			dump GLSL to files for conversion to SPIR-V


KEYBOARD INPUT
==============

The following keys can be used at run-time to change various settings.

	[F]		= toggle between windowed and fullscreen
	[V]		= toggle V-Sync on/off
	[H]		= toggle head rotation on/off
	[P]		= pause/resume the simulation
	[R]		= cycle screen resolution level
	[B]		= cycle eye buffer resolution level
	[S]		= cycle multi-sampling level
	[Q]		= cycle per eye draw calls level
	[W]		= cycle per eye triangles per draw call level
	[E]		= cycle per eye fragment program complexity level
	[M]		= toggle multi-view
	[C]		= toggle correction for chromatic aberration
	[I]		= toggle time warp implementation: graphics, compute
	[Z]		= cycle the render mode: atw, tw, scene
	[G]		= cycle between showing graphs, showing paused graphs and hiding graphs
	[L]		= log 10 frames of OpenGL commands
	[D]		= dump GLSL to files for conversion to SPIR-V
	[Esc]	= exit


IMPLEMENTATION
==============

The code is written in an object-oriented style with a focus on minimizing state
and side effects. The majority of the functions manipulate self-contained objects
without modifying any global state (except for OpenGL state). The types
introduced in this file have no circular dependencies, and there are no forward
declarations.

Even though an object-oriented style is used, the code is written in straight C99 for
maximum portability and readability. To further improve portability and to simplify
compilation, all source code is in a single file without any dependencies on third-
party code or non-standard libraries. The code does not use an OpenGL loading library
like GLEE, GLEW, GL3W, or an OpenGL toolkit like GLUT, FreeGLUT, GLFW, etc. Instead,
the code provides direct access to window and context creation for driver extension work.

The code is written against version 4.3 of the Core Profile OpenGL Specification,
and version 3.1 of the OpenGL ES Specification.

Supported platforms are:

	- Microsoft Windows 7 or later
	- Apple macOS 10.11 or later
	- Apple iOS 9.0 or later
	- Ubuntu Linux 14.04 or later
	- Android 5.0 or later


GRAPHICS API WRAPPER
====================

The code wraps the OpenGL API with a convenient wrapper that takes care of a
lot of the OpenGL intricacies. This wrapper does not expose the full OpenGL API
but can be easily extended to support more features. Some of the current
limitations are:

- The wrapper is setup for forward rendering with a single render pass. This
  can be easily extended if more complex rendering algorithms are desired.

- A pipeline can only use 256 bytes worth of plain integer and floating-point
  uniforms, including vectors and matrices. If more uniforms are needed then 
  it is advised to use a uniform buffer, which is the preferred approach for
  exposing large amounts of data anyway.

- Graphics programs currently consist of only of a vertex and fragment shader.
  This can be easily extended if there is a need for geometry shaders etc.


COMMAND-LINE COMPILATION
========================

Microsoft Windows: Visual Studio 2013 Compiler:
	include\GL\glext.h  -> https://www.opengl.org/registry/api/GL/glext.h
	include\GL\wglext.h -> https://www.opengl.org/registry/api/GL/wglext.h
	"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
	cl /Zc:wchar_t /Zc:forScope /Wall /MD /GS /Gy /O2 /Oi /arch:SSE2 /Iinclude atw_opengl.c /link user32.lib gdi32.lib Advapi32.lib opengl32.lib

Microsoft Windows: Intel Compiler 14.0
	include\GL\glext.h  -> https://www.opengl.org/registry/api/GL/glext.h
	include\GL\wglext.h -> https://www.opengl.org/registry/api/GL/wglext.h
	"C:\Program Files (x86)\Intel\Composer XE\bin\iclvars.bat" intel64
	icl /Qstd=c99 /Zc:wchar_t /Zc:forScope /Wall /MD /GS /Gy /O2 /Oi /arch:SSE2 /Iinclude atw_opengl.c /link user32.lib gdi32.lib Advapi32.lib opengl32.lib

Apple Mac OS X: Apple LLVM 6.0:
	clang -std=c99 -x objective-c -fno-objc-arc -Wall -g -O2 -o atw_opengl atw_opengl.c -framework Cocoa -framework OpenGL

Linux: GCC 4.8.2 Xlib:
	sudo apt-get install libx11-dev
	sudo apt-get install libxxf86vm-dev
	sudo apt-get install libxrandr-dev
	sudo apt-get install mesa-common-dev
	sudo apt-get install libgl1-mesa-dev
	gcc -std=c99 -Wall -g -O2 -m64 -o atw_opengl atw_opengl.c -lm -lpthread -lX11 -lXxf86vm -lXrandr -lGL

Linux: GCC 4.8.2 XCB:
	sudo apt-get install libxcb1-dev
	sudo apt-get install libxcb-keysyms1-dev
	sudo apt-get install libxcb-icccm4-dev
	sudo apt-get install mesa-common-dev
	sudo apt-get install libgl1-mesa-dev
	gcc -std=c99 -Wall -g -O2 -o -m64 atw_opengl atw_opengl.c -lm -lpthread -lxcb -lxcb-keysyms -lxcb-randr -lxcb-glx -lxcb-dri2 -lGL

Android for ARM from Windows: NDK Revision 11c - Android 21 - ANT/Gradle
	ANT:
		cd projects/android/ant/atw_opengl
		ndk-build
		ant debug
		adb install -r bin/atw_opengl-debug.apk
	Gradle:
		cd projects/android/gradle/atw_opengl
		gradlew build
		adb install -r build/outputs/apk/atw_opengl-all-debug.apk


KNOWN ISSUES
============

OS     : Apple Mac OS X 10.9.5
GPU    : Geforce GT 750M
DRIVER : NVIDIA 310.40.55b01
-----------------------------------------------
- glGetQueryObjectui64v( query, GL_QUERY_RESULT, &time ) always returns zero for a timer query.
- glFlush() after a glFenceSync() stalls the CPU for many milliseconds.
- Creating a context fails when the share context is current on another thread.

OS     : Android 6.0.1
GPU    : Adreno (TM) 530
DRIVER : OpenGL ES 3.1 V@145.0
-----------------------------------------------
- Enabling OVR_multiview hangs the GPU.


WORK ITEMS
==========

- Implement WGL, GLX and NSOpenGL equivalents of EGL_IMG_context_priority.
- Implement an extension that provides accurate display refresh timing (WGL_NV_delay_before_swap, D3DKMTGetScanLine).
- Implement an OpenGL extension that allows rendering directly to the front buffer.
- Implement an OpenGL extension that allows a compute shader to directly write to the front/back buffer images (WGL_AMDX_drawable_view).
- Improve GPU task switching granularity.


VERSION HISTORY
===============

1.0		Initial version.

================================================================================================
*/

#if defined( WIN32 ) || defined( _WIN32 ) || defined( WIN64 ) || defined( _WIN64 )
	#define OS_WINDOWS
#elif defined( __ANDROID__ )
	#define OS_ANDROID
#elif defined( __APPLE__ )
	#define OS_APPLE
	#include <Availability.h>
	#if __IPHONE_OS_VERSION_MAX_ALLOWED
		#define OS_APPLE_IOS
	#elif __MAC_OS_X_VERSION_MAX_ALLOWED
		#define OS_APPLE_MACOS
	#endif
#elif defined( __linux__ )
	#define OS_LINUX
	#define OS_LINUX_XLIB		// Xlib + Xlib GLX 1.3
	//#define OS_LINUX_XCB		// XCB + XCB GLX is limited to OpenGL 2.1
	//#define OS_LINUX_XCB_GLX	// XCB + Xlib GLX 1.3
#else
	#error "unknown platform"
#endif

/*
================================
Platform headers / declarations
================================
*/

#if defined( OS_WINDOWS )

	#if !defined( _CRT_SECURE_NO_WARNINGS )
		#define _CRT_SECURE_NO_WARNINGS
	#endif

	#if defined( _MSC_VER )
		#pragma warning( disable : 4204 )	// nonstandard extension used : non-constant aggregate initializer
		#pragma warning( disable : 4255 )	// '<name>' : no function prototype given: converting '()' to '(void)'
		#pragma warning( disable : 4668 )	// '__cplusplus' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
		#pragma warning( disable : 4710	)	// 'int printf(const char *const ,...)': function not inlined
		#pragma warning( disable : 4711 )	// function '<name>' selected for automatic inline expansion
		#pragma warning( disable : 4738 )	// storing 32-bit float result in memory, possible loss of performance
		#pragma warning( disable : 4820 )	// '<name>' : 'X' bytes padding added after data member '<member>'		
	#endif

	#if _MSC_VER >= 1900
		#pragma warning( disable : 4464	)	// relative include path contains '..'
		#pragma warning( disable : 4774	)	// 'printf' : format string expected in argument 1 is not a string literal
	#endif

	#define OPENGL_VERSION_MAJOR	4
	#define OPENGL_VERSION_MINOR	3
	#define GLSL_PROGRAM_VERSION	"430"
	#define USE_SYNC_OBJECT			0			// 0 = GLsync, 1 = EGLSyncKHR, 2 = storage buffer

	#include <windows.h>
	#include <GL/gl.h>
	#define GL_EXT_color_subtable
	#include <GL/glext.h>
	#include <GL/wglext.h>
	#include <GL/gl_format.h>

	#define OUTPUT_PATH				""

	#define __thread	__declspec( thread )

#elif defined( OS_APPLE )

	// Apple is still at OpenGL 4.1
	#define OPENGL_VERSION_MAJOR	4
	#define OPENGL_VERSION_MINOR	1
	#define GLSL_PROGRAM_VERSION	"410"
	#define USE_SYNC_OBJECT			0			// 0 = GLsync, 1 = EGLSyncKHR, 2 = storage buffer

	#include <sys/param.h>
	#include <sys/sysctl.h>
	#include <sys/time.h>
	#include <pthread.h>
	#include <Cocoa/Cocoa.h>
	#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
	#include <OpenGL/OpenGL.h>
	#include <OpenGL/gl3.h>
	#include <OpenGL/gl3ext.h>
	#include <GL/gl_format.h>

	#define OUTPUT_PATH				""

	// Undocumented CGS and CGL
	typedef void * CGSConnectionID;
	typedef int CGSWindowID;
	typedef int CGSSurfaceID;
    
	CGLError CGLSetSurface( CGLContextObj ctx, CGSConnectionID cid, CGSWindowID wid, CGSSurfaceID sid );
	CGLError CGLGetSurface( CGLContextObj ctx, CGSConnectionID * cid, CGSWindowID * wid, CGSSurfaceID * sid );
	CGLError CGLUpdateContext( CGLContextObj ctx );

	#pragma clang diagnostic ignored "-Wunused-function"
	#pragma clang diagnostic ignored "-Wunused-const-variable"

#elif defined( OS_LINUX )

	#define OPENGL_VERSION_MAJOR	4
	#define OPENGL_VERSION_MINOR	3
	#define GLSL_PROGRAM_VERSION	"430"
	#define USE_SYNC_OBJECT			0			// 0 = GLsync, 1 = EGLSyncKHR, 2 = storage buffer

	#if __STDC_VERSION__ >= 199901L
	#define _XOPEN_SOURCE 600
	#else
	#define _XOPEN_SOURCE 500
	#endif

	#include <time.h>							// for timespec
	#include <sys/time.h>						// for gettimeofday()
	#define __USE_UNIX98						// for pthread_mutexattr_settype
	#include <pthread.h>						// for pthread_create() etc.
	#include <malloc.h>							// for memalign
	#if defined( OS_LINUX_XLIB )
		#include <X11/Xlib.h>
		#include <X11/Xatom.h>
		#include <X11/extensions/xf86vmode.h>	// for fullscreen video mode
		#include <X11/extensions/Xrandr.h>		// for resolution changes
	#elif defined( OS_LINUX_XCB ) || defined( OS_LINUX_XCB_GLX )
		#include <X11/keysym.h>
		#include <xcb/xcb.h>
		#include <xcb/xcb_keysyms.h>
		#include <xcb/xcb_icccm.h>
		#include <xcb/randr.h>
		#include <xcb/glx.h>
		#include <xcb/dri2.h>
	#endif
	#include <GL/glx.h>
	#include <GL/gl_format.h>

	#define OUTPUT_PATH				""

	// These prototypes are only included when __USE_GNU is defined but that causes other compile errors.
	extern int pthread_setname_np( pthread_t __target_thread, __const char *__name );
	extern int pthread_setaffinity_np( pthread_t thread, size_t cpusetsize, const cpu_set_t * cpuset );

	#pragma GCC diagnostic ignored "-Wunused-function"

#elif defined( OS_ANDROID )

	#define OPENGL_VERSION_MAJOR	3
	#define OPENGL_VERSION_MINOR	1
	#define GLSL_PROGRAM_VERSION	"310 es"
	#define USE_SYNC_OBJECT			1			// 0 = GLsync, 1 = EGLSyncKHR, 2 = storage buffer

	#include <time.h>
	#include <unistd.h>
	#include <dirent.h>							// for opendir/closedir
	#include <pthread.h>
	#include <malloc.h>							// for memalign
	#include <dlfcn.h>							// for dlopen
	#include <sys/prctl.h>						// for prctl( PR_SET_NAME )
	#include <sys/stat.h>						// for gettid
	#include <sys/syscall.h>					// for syscall
	#include <android/log.h>					// for __android_log_print
	#include <android/input.h>					// for AKEYCODE_ etc.
	#include <android/window.h>					// for AWINDOW_FLAG_KEEP_SCREEN_ON
	#include <android/native_window_jni.h>		// for native window JNI
	#include <android_native_app_glue.h>
	#include <EGL/egl.h>
	#include <EGL/eglext.h>
	#include <GLES2/gl2ext.h>
	#include <GLES3/gl3.h>
	#if OPENGL_VERSION_MAJOR == 3 && OPENGL_VERSION_MINOR == 1
		#include <GLES3/gl31.h>
	#endif
	#include <GLES3/gl3ext.h>
	#include <GL/gl_format.h>

	#define OUTPUT_PATH		"/sdcard/"

	#pragma GCC diagnostic ignored "-Wunused-function"

	typedef struct
	{
		JavaVM *	vm;			// Java Virtual Machine
		JNIEnv *	env;		// Thread specific environment
		jobject		activity;	// Java activity object
	} Java_t;

#endif

/*
================================
Common headers
================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <string.h>			// for memset
#include <errno.h>			// for EBUSY, ETIMEDOUT etc.
#include <ctype.h>			// for isspace, isdigit

/*
================================
Common defines
================================
*/

#define MATH_PI							3.14159265358979323846f

#define UNUSED_PARM( x )				{ (void)(x); }
#define ARRAY_SIZE( a )					( sizeof( (a) ) / sizeof( (a)[0] ) )
#define OFFSETOF_MEMBER( type, member )	(size_t)&((type *)0)->member
#define SIZEOF_MEMBER( type, member )	sizeof( ((type *)0)->member )
#define BIT( x )						( 1 << (x) )
#define ROUNDUP( x, granularity )		( ( (x) + (granularity) - 1 ) & ~( (granularity) - 1 ) )
#define MAX( x, y )						( ( x > y ) ? ( x ) : ( y ) )
#define MIN( x, y )						( ( x < y ) ? ( x ) : ( y ) )
#define CLAMP( x, min, max )			( ( (x) < (min) ) ? (min) : ( ( (x) > (max) ) ? (max) : (x) ) )
#define STRINGIFY_EXPANDED( a )			#a
#define STRINGIFY( a )					STRINGIFY_EXPANDED(a)

#define APPLICATION_NAME				"OpenGL ATW"
#define WINDOW_TITLE					"Asynchronous Time Warp - OpenGL"

#define GRAPHICS_API_OPENGL				1

#define USE_GLTF						0
#define PROGRAM( name )					name##GLSL

#define GLSL_EXTENSIONS					"#extension GL_EXT_shader_io_blocks : enable\n"
#define GL_FINISH_SYNC					1

#if defined( OS_ANDROID )
#define ES_HIGHP						"highp"	// GLSL "310 es" requires a precision qualifier on a image2D
#else
#define ES_HIGHP						""		// GLSL "430" disallows a precision qualifier on a image2D
#endif

/*
================================================================================================================================

System level functionality

================================================================================================================================
*/

static void * AllocAlignedMemory( size_t size, size_t alignment )
{
	alignment = ( alignment < sizeof( void * ) ) ? sizeof( void * ) : alignment;
#if defined( OS_WINDOWS )
	return _aligned_malloc( size, alignment );
#elif defined( OS_APPLE )
	void * ptr = NULL;
	return ( posix_memalign( &ptr, alignment, size ) == 0 ) ? ptr : NULL;
#else
	return memalign( alignment, size );
#endif
}

static void FreeAlignedMemory( void * ptr )
{
#if defined( OS_WINDOWS )
	_aligned_free( ptr );
#else
	free( ptr );
#endif
}

static void Print( const char * format, ... )
{
#if defined( OS_WINDOWS )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	vsnprintf_s( buffer, 4096, _TRUNCATE, format, args );
	va_end( args );

	OutputDebugString( buffer );
#elif defined( OS_APPLE )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	vsnprintf( buffer, 4096, format, args );
	va_end( args );

	NSLog( @"%s", buffer );
#elif defined( OS_LINUX )
	va_list args;
	va_start( args, format );
	vprintf( format, args );
	va_end( args );
	fflush( stdout );
#elif defined( OS_ANDROID )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	vsnprintf( buffer, 4096, format, args );
	va_end( args );

	__android_log_print( ANDROID_LOG_VERBOSE, "atw", "%s", buffer );
#endif
}

static void Error( const char * format, ... )
{
#if defined( OS_WINDOWS )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	vsnprintf_s( buffer, 4096, _TRUNCATE, format, args );
	va_end( args );

	OutputDebugString( buffer );

	MessageBox( NULL, buffer, "ERROR", MB_OK | MB_ICONINFORMATION );
#elif defined( OS_APPLE_IOS )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	int length = vsnprintf( buffer, 4096, format, args );
	va_end( args );

	NSLog( @"%s\n", buffer );

	if ( [NSThread isMainThread] )
	{
		NSString * string = [[NSString alloc] initWithBytes:buffer length:length encoding:NSASCIIStringEncoding];
		UIAlertController* alert = [UIAlertController alertControllerWithTitle: @"Error"
																	   message: string
																preferredStyle: UIAlertControllerStyleAlert];
		[alert addAction: [UIAlertAction actionWithTitle: @"OK"
												   style: UIAlertActionStyleDefault
												 handler: ^(UIAlertAction * action) {}]];
		[UIApplication.sharedApplication.keyWindow.rootViewController presentViewController: alert animated: YES completion: nil];
	}
#elif defined( OS_APPLE_MACOS )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	int length = vsnprintf( buffer, 4096, format, args );
	va_end( args );

	NSLog( @"%s\n", buffer );

	if ( [NSThread isMainThread] )
	{
		NSString * string = [[NSString alloc] initWithBytes:buffer length:length encoding:NSASCIIStringEncoding];
		NSAlert * alert = [[NSAlert alloc] init];
		[alert addButtonWithTitle:@"OK"];
		[alert setMessageText:@"Error"];
		[alert setInformativeText:string];
		[alert setAlertStyle:NSWarningAlertStyle];
		[alert runModal];
	}
#elif defined( OS_LINUX )
	va_list args;
	va_start( args, format );
	vprintf( format, args );
	va_end( args );
	printf( "\n" );
	fflush( stdout );
#elif defined( OS_ANDROID )
	char buffer[4096];
	va_list args;
	va_start( args, format );
	vsnprintf( buffer, 4096, format, args );
	va_end( args );

	__android_log_print( ANDROID_LOG_ERROR, "atw", "%s", buffer );
#endif
	// Without exiting, the application will likely crash.
	if ( format != NULL )
	{
		exit( 0 );
	}
}

static const char * GetOSVersion()
{
#if defined( OS_WINDOWS )
	HKEY hKey = 0;
	if ( RegOpenKey( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", &hKey ) == ERROR_SUCCESS )
	{
		static char version[1024];
		DWORD version_length = sizeof( version );
		DWORD dwType = REG_SZ;
		if ( RegQueryValueEx( hKey, "ProductName", NULL, &dwType, (LPBYTE)&version, &version_length ) == ERROR_SUCCESS )
		{
			return version;
		}
	}

	return "Microsoft Windows";
#elif defined( OS_APPLE_IOS )
	return [NSString stringWithFormat: @"Apple iOS %@", NSProcessInfo.processInfo.operatingSystemVersionString].UTF8String;
#elif defined( OS_APPLE_MACOS )
	return [NSString stringWithFormat: @"Apple macOS %@", NSProcessInfo.processInfo.operatingSystemVersionString].UTF8String;
#elif defined( OS_LINUX )
	static char buffer[1024];

	FILE * os_release = fopen( "/etc/os-release", "r" );
	if ( os_release != NULL )
	{
		while ( fgets( buffer, sizeof( buffer ), os_release ) )
		{
			if ( strncmp( buffer, "PRETTY_NAME=", 12 ) == 0 )
			{
				char * pretty_name = buffer + 12;

				// remove newline and enclosing quotes
				while(	pretty_name[0] == ' ' ||
						pretty_name[0] == '\t' ||
						pretty_name[0] == ':' ||
						pretty_name[0] == '\'' ||
						pretty_name[0] == '\"' )
				{
					pretty_name++;
				}
				int last = strlen( pretty_name ) - 1;
				while(	last >= 0 && (
						pretty_name[last] == '\n' ||
						pretty_name[last] == '\'' ||
						pretty_name[last] == '\"' ) )
				{
					pretty_name[last--] = '\0';
				}
				return pretty_name;
			}
		}

		fclose( os_release );
	}

	return "Linux";
#elif defined( OS_ANDROID )
	static char version[1024];

	#define PROP_NAME_MAX   32
	#define PROP_VALUE_MAX  92

	char release[PROP_VALUE_MAX] = { 0 };
	char build[PROP_VALUE_MAX] = { 0 };

	void * handle = dlopen( "libc.so", RTLD_NOLOAD );
	if ( handle != NULL )
	{
		typedef int (*PFN_SYSTEM_PROP_GET)(const char *, char *);
		PFN_SYSTEM_PROP_GET __my_system_property_get = (PFN_SYSTEM_PROP_GET)dlsym( handle, "__system_property_get" );
		if ( __my_system_property_get != NULL )
		{
			__my_system_property_get( "ro.build.version.release", release );
			__my_system_property_get( "ro.build.version.incremental", build );
		}
	}

	snprintf( version, sizeof( version ), "Android %s (%s)", release, build );

	return version;
#endif
}

static const char * GetCPUVersion()
{
#if defined( OS_WINDOWS )
	HKEY hKey = 0;
	if ( RegOpenKey( HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", &hKey ) == ERROR_SUCCESS )
	{
		static char processor[1024];
		DWORD processor_length = sizeof( processor );
		DWORD dwType = REG_SZ;
		if ( RegQueryValueEx( hKey, "ProcessorNameString", NULL, &dwType, (LPBYTE)&processor, &processor_length ) == ERROR_SUCCESS )
		{
			return processor;
		}
	}
#elif defined( OS_APPLE )
	static char processor[1024];
	size_t processor_length = sizeof( processor );
	sysctlbyname( "machdep.cpu.brand_string", &processor, &processor_length, NULL, 0 );
	return processor;
#elif defined( OS_LINUX ) || defined( OS_ANDROID )
	struct
	{
		const char * key;
		char value[1024];
	} keyValues[] =
	{
		{ "model name", "" },
		{ "Processor", "" },
		{ "Hardware", "" }
	};
	static char name[1024];

	FILE * cpuinfo = fopen( "/proc/cpuinfo", "r" );
	if ( cpuinfo != NULL )
	{
		char buffer[1024];
		while ( fgets( buffer, sizeof( buffer ), cpuinfo ) )
		{
			for ( int i = 0; i < (int)ARRAY_SIZE( keyValues ); i++ )
			{
				const size_t length = strlen( keyValues[i].key );
				if ( strncmp( buffer, keyValues[i].key, length ) == 0 )
				{
					char * pretty_name = buffer + length;

					// remove newline and enclosing quotes
					while(	pretty_name[0] == ' ' ||
							pretty_name[0] == '\t' ||
							pretty_name[0] == ':' ||
							pretty_name[0] == '\'' ||
							pretty_name[0] == '\"' )
					{
						pretty_name++;
					}
					int last = strlen( pretty_name ) - 1;
					while(	last >= 0 && (
							pretty_name[last] == '\n' ||
							pretty_name[last] == '\'' ||
							pretty_name[last] == '\"' ) )
					{
						pretty_name[last--] = '\0';
					}

					strcpy( keyValues[i].value, pretty_name );
					break;
				}
			}
		}

		fclose( cpuinfo );

		sprintf( name, "%s%s%s", keyValues[2].value,
				( keyValues[2].value[0] != '\0' ) ? " - " : "",
				( keyValues[0].value[0] != '\0' ) ? keyValues[0].value : keyValues[1].value );
		return name;
	}
#endif
	return "unknown";
}

typedef unsigned long long Microseconds_t;

static Microseconds_t GetTimeMicroseconds()
{
#if defined( OS_WINDOWS )
	static Microseconds_t ticksPerSecond = 0;
	static Microseconds_t timeBase = 0;

	if ( ticksPerSecond == 0 )
	{
		LARGE_INTEGER li;
		QueryPerformanceFrequency( &li );
		ticksPerSecond = (Microseconds_t) li.QuadPart;
		QueryPerformanceCounter( &li );
		timeBase = (Microseconds_t) li.LowPart + 0xFFFFFFFFULL * li.HighPart;
	}

	LARGE_INTEGER li;
	QueryPerformanceCounter( &li );
	Microseconds_t counter = (Microseconds_t) li.LowPart + 0xFFFFFFFFULL * li.HighPart;
	return ( counter - timeBase ) * 1000ULL * 1000ULL / ticksPerSecond;
#elif defined( OS_ANDROID )
	struct timespec ts;
	clock_gettime( CLOCK_MONOTONIC, &ts );
	return (Microseconds_t) ts.tv_sec * 1000ULL * 1000ULL + ts.tv_nsec / 1000ULL;
#else
	static Microseconds_t timeBase = 0;

	struct timeval tv;
	gettimeofday( &tv, 0 );

	if ( timeBase == 0 )
	{
		timeBase = (Microseconds_t) tv.tv_sec * 1000ULL * 1000ULL;
	}

	return (Microseconds_t) tv.tv_sec * 1000ULL * 1000ULL + tv.tv_usec - timeBase;
#endif
}

/*
================================================================================================================================

Mutex for mutual exclusion on shared resources within a single process.

Equivalent to a Windows Critical Section Object which allows recursive access. This mutex cannot be
used for mutual-exclusion synchronization between threads from different processes.

Mutex_t

static void Mutex_Create( Mutex_t * mutex );
static void Mutex_Destroy( Mutex_t * mutex );
static bool Mutex_Lock( Mutex_t * mutex, const bool blocking );
static void Mutex_Unlock( Mutex_t * mutex );

================================================================================================================================
*/

typedef struct
{
#if defined( OS_WINDOWS )
	CRITICAL_SECTION	handle;
#else
	pthread_mutex_t		mutex;
#endif
} Mutex_t;

static void Mutex_Create( Mutex_t * mutex )
{
#if defined( OS_WINDOWS )
	InitializeCriticalSection( &mutex->handle );
#else
	pthread_mutexattr_t attr;
	pthread_mutexattr_init( &attr );
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
	pthread_mutex_init( &mutex->mutex, &attr );
#endif
}

static void Mutex_Destroy( Mutex_t * mutex )
{
#if defined( OS_WINDOWS )
	DeleteCriticalSection( &mutex->handle );
#else
	pthread_mutex_destroy( &mutex->mutex );
#endif
}

static bool Mutex_Lock( Mutex_t * mutex, const bool blocking )
{
#if defined( OS_WINDOWS )
	if ( TryEnterCriticalSection( &mutex->handle ) == 0 )
	{
		if ( !blocking )
		{
			return false;
		}
		EnterCriticalSection( &mutex->handle );
	}
	return true;
#else
	if ( pthread_mutex_trylock( &mutex->mutex ) == EBUSY )
	{
		if ( !blocking )
		{
			return false;
		}
		pthread_mutex_lock( &mutex->mutex );
	}
	return true;
#endif
}

static void Mutex_Unlock( Mutex_t * mutex )
{
#if defined( OS_WINDOWS )
	LeaveCriticalSection( &mutex->handle );
#else
	pthread_mutex_unlock( &mutex->mutex );
#endif
}

/*
================================================================================================================================

Signal for thread synchronization, similar to a Windows event object which only supports SetEvent.

Windows event objects come in two types: auto-reset events and manual-reset events. A Windows event object
can be signalled by calling either SetEvent or PulseEvent.

When a manual-reset event is signaled by calling SetEvent, it sets the event into the signaled state and
wakes up all threads waiting on the event. The manual-reset event remains in the signalled state until
the event is manually reset. When an auto-reset event is signaled by calling SetEvent and there are any
threads waiting, it wakes up only one thread and resets the event to the non-signaled state. If there are
no threads waiting for an auto-reset event, then the event remains signaled until a single waiting thread
waits on it and is released.

When a manual-reset event is signaled by calling PulseEvent, it wakes up all waiting threads and atomically
resets the event. When an auto-reset event is signaled by calling PulseEvent, and there are any threads
waiting, it wakes up only one thread and resets the event to the non-signaled state. If there are no threads
waiting, then no threads are released and the event is set to the non-signaled state.

A Windows event object has limited functionality compared to a POSIX condition variable. Unlike a
Windows event object, the expression waited upon by a POSIX condition variable can be arbitrarily complex.
Furthermore, there is no way to release just one waiting thread with a manual-reset Windows event object.
Similarly there is no way to release all waiting threads with an auto-reset Windows event object.
These limitations make it difficult to simulate a POSIX condition variable using Windows event objects.

Windows Vista and later implement PCONDITION_VARIABLE, but as Douglas C. Schmidt and Irfan Pyarali point
out, it is complicated to simulate a POSIX condition variable on prior versions of Windows without causing
unfair or even incorrect behavior:

	1. "Strategies for Implementing POSIX Condition Variables on Win32"
	   http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
	2. "Patterns for POSIX Condition Variables on Win32"
	   http://www.cs.wustl.edu/~schmidt/win32-cv-2.html
	
Even using SignalObjectAndWait is not safe because as per the Microsoft documentation: "Note that the 'signal'
and 'wait' are not guaranteed to be performed as an atomic operation. Threads executing on other processors
can observe the signaled state of the first object before the thread calling SignalObjectAndWait begins its
wait on the second object."

Simulating a Windows event object using a POSIX condition variable is fairly straight forward, which
is done here. However, this implementation does not support the equivalent of PulseEvent, because
PulseEvent is unreliable. On Windows, a thread waiting on an event object can be momentarily removed
from the wait state by a kernel-mode Asynchronous Procedure Call (APC), and then returned to the wait
state after the APC is complete. If a call to PulseEvent occurs during the time when the thread has
been temporarily removed from the wait state, then the thread will not be released, because PulseEvent
releases only those threads that are in the wait state at the moment PulseEvent is called.

Signal_t

static void Signal_Create( Signal_t * signal, const bool autoReset );
static void Signal_Destroy( Signal_t * signal );
static bool Signal_Wait( Signal_t * signal, const int timeOutMilliseconds );
static void Signal_Raise( Signal_t * signal );
static void Signal_Clear( Signal_t * signal );

================================================================================================================================
*/

typedef struct
{
#if defined( OS_WINDOWS )
	HANDLE			handle;
#else
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	int				waitCount;		// number of threads waiting on the signal
	bool			autoReset;		// automatically clear the signalled state when a single thread is released
	bool			signaled;		// in the signalled state if true
#endif
} Signal_t;

static void Signal_Create( Signal_t * signal, const bool autoReset )
{
#if defined( OS_WINDOWS )
	signal->handle = CreateEvent( NULL, !autoReset, FALSE, NULL );
#else
	pthread_mutex_init( &signal->mutex, NULL );
	pthread_cond_init( &signal->cond, NULL );
	signal->waitCount = 0;
	signal->autoReset = autoReset;
	signal->signaled = false;
#endif
}

static void Signal_Destroy( Signal_t * signal )
{
#if defined( OS_WINDOWS )
	CloseHandle( signal->handle );
#else
	pthread_cond_destroy( &signal->cond );
	pthread_mutex_destroy( &signal->mutex );
#endif
}

// Waits for the object to enter the signalled state and returns true if this state is reached within the time-out period.
// If 'autoReset' is true then the first thread that reaches the signalled state within the time-out period will clear the signalled state.
// If 'timeOutMilliseconds' is negative then this will wait indefinitely until the signalled state is reached.
// Returns true if the thread was released because the object entered the signalled state, returns false if the time-out is reached first.
static bool Signal_Wait( Signal_t * signal, const int timeOutMilliseconds )
{
#if defined( OS_WINDOWS )
	DWORD result = WaitForSingleObject( signal->handle, timeOutMilliseconds < 0 ? INFINITE : timeOutMilliseconds );
	assert( result == WAIT_OBJECT_0 || ( timeOutMilliseconds >= 0 && result == WAIT_TIMEOUT ) );
	return ( result == WAIT_OBJECT_0 );
#else
	bool released = false;
	pthread_mutex_lock( &signal->mutex );
	if ( signal->signaled )
	{
		released = true;
	}
	else
	{
		signal->waitCount++;
		if ( timeOutMilliseconds < 0 )
		{
			do
			{
				pthread_cond_wait( &signal->cond, &signal->mutex );
				// Must re-check condition because pthread_cond_wait may spuriously wake up.
			} while ( signal->signaled == false );
		}
		else if ( timeOutMilliseconds > 0 )
		{
			struct timeval tp;
			gettimeofday( &tp, NULL );
			struct timespec ts;
			ts.tv_sec = tp.tv_sec + timeOutMilliseconds / 1000;
			ts.tv_nsec = tp.tv_usec * 1000 + ( timeOutMilliseconds % 1000 ) * 1000000;
			do
			{
				if ( pthread_cond_timedwait( &signal->cond, &signal->mutex, &ts ) == ETIMEDOUT )
				{
					break;
				}
				// Must re-check condition because pthread_cond_timedwait may spuriously wake up.
			} while ( signal->signaled == false );
		}
		released = signal->signaled;
		signal->waitCount--;
	}
	if ( released && signal->autoReset )
	{
		signal->signaled = false;
	}
	pthread_mutex_unlock( &signal->mutex );
	return released;
#endif
}

// Enter the signalled state.
// Note that if 'autoReset' is true then this will only release a single thread.
static void Signal_Raise( Signal_t * signal )
{
#if defined( OS_WINDOWS )
	SetEvent( signal->handle );
#else
	pthread_mutex_lock( &signal->mutex );
	signal->signaled = true;
	if ( signal->waitCount > 0 )
	{
		pthread_cond_broadcast( &signal->cond );
	}
	pthread_mutex_unlock( &signal->mutex );
#endif
}

// Clear the signalled state.
// Should not be needed for auto-reset signals (autoReset == true).
static void Signal_Clear( Signal_t * signal )
{
#if defined( OS_WINDOWS )
	ResetEvent( signal->handle );
#else
	pthread_mutex_lock( &signal->mutex );
	signal->signaled = false;
	pthread_mutex_unlock( &signal->mutex );
#endif
}

/*
================================================================================================================================

Worker thread.

When the thread is first created, it will be in a suspended state. The thread function will be
called as soon as the thread is signalled. If the thread is not signalled again, then the thread
will return to a suspended state as soon as the thread function returns. The thread function will
be called again by signalling the thread again. The thread function will be called again right
away, when the thread is signalled during the execution of the thread function. Signalling the
thread more than once during the execution of the thread function does not cause the thread
function to be called multiple times. The thread can be joined to wait for the thread function
to return.

This worker thread will function as a normal thread by immediately signalling the thread after creation.
Once the thread function returns, the thread can be destroyed. Destroying the thread always waits
for the thread function to return first.

Thread_t

static bool Thread_Create( Thread_t * thread, const char * threadName, threadFunction_t threadFunction, void * threadData );
static void Thread_Destroy( Thread_t * thread );
static void Thread_Signal( Thread_t * thread );
static void Thread_Join( Thread_t * thread );
static void Thread_Submit( Thread_t * thread, threadFunction_t threadFunction, void * threadData );

static void Thread_SetName( const char * name );
static void Thread_SetAffinity( int mask );
static void Thread_SetRealTimePriority( int priority );

================================================================================================================================
*/

typedef void (*threadFunction_t)( void * data );

#if defined( OS_WINDOWS )
#define THREAD_HANDLE			HANDLE
#define THREAD_RETURN_TYPE		int
#define THREAD_RETURN_VALUE		0
#else
#define THREAD_HANDLE			pthread_t
#define THREAD_RETURN_TYPE		void *
#define THREAD_RETURN_VALUE		0
#endif

#define THREAD_AFFINITY_BIG_CORES		-1

typedef struct
{
	char				threadName[128];
	threadFunction_t	threadFunction;
	void *				threadData;

	void *				stack;
	THREAD_HANDLE		handle;
	Signal_t			workIsDone;
	Signal_t			workIsAvailable;
	Mutex_t				workMutex;
	volatile bool		terminate;
} Thread_t;

// Note that on Android AttachCurrentThread will reset the thread name.
static void Thread_SetName( const char * name )
{
#if defined( OS_WINDOWS )
	static const unsigned int MS_VC_EXCEPTION = 0x406D1388;

	typedef struct
	{
		DWORD dwType;		// Must be 0x1000.
		LPCSTR szName;		// Pointer to name (in user address space).
		DWORD dwThreadID;	// Thread ID (-1 = caller thread).
		DWORD dwFlags;		// Reserved for future use, must be zero.
	} THREADNAME_INFO;

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = (DWORD)( -1 );
	info.dwFlags = 0;
	__try
	{
		RaiseException( MS_VC_EXCEPTION, 0, sizeof( info ) / sizeof( DWORD ), (const ULONG_PTR *)&info );
	}
	__except( GetExceptionCode() == MS_VC_EXCEPTION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
	{
		info.dwFlags = 0;
	}
#elif defined( OS_APPLE )
	pthread_setname_np( name );
#elif defined( OS_LINUX )
	pthread_setname_np( pthread_self(), name );
#elif defined( OS_ANDROID )
	prctl( PR_SET_NAME, (long)name, 0, 0, 0 );
#endif
}

static void Thread_SetAffinity( int mask )
{
#if defined( OS_WINDOWS )
	if ( mask == THREAD_AFFINITY_BIG_CORES )
	{
		return;
	}
	HANDLE thread = GetCurrentThread();
	if ( !SetThreadAffinityMask( thread, mask ) )
	{
		char buffer[1024];
		DWORD error = GetLastError();
		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), buffer, sizeof( buffer ), NULL );
		Print( "Failed to set thread %p affinity: %s(%d)\n", thread, buffer, error );
	}
	else
	{
		Print( "Thread %p affinity set to 0x%02X\n", thread, mask );
	}
#elif defined( OS_APPLE )
	// iOS and macOS do not export interfaces that identify processors or control thread placement.
	UNUSED_PARM( mask );
#elif defined( OS_LINUX )
	if ( mask == THREAD_AFFINITY_BIG_CORES )
	{
		return;
	}
	cpu_set_t set;
	memset( &set, 0, sizeof( cpu_set_t ) );
	for ( int bit = 0; bit < 32; bit++ )
	{
		if ( ( mask & ( 1 << bit ) ) != 0 )
		{
			set.__bits[bit / sizeof( set.__bits[0] )] |= 1 << ( bit & ( sizeof( set.__bits[0] ) - 1 ) );
		}
	}
	const int result = pthread_setaffinity_np( pthread_self(), sizeof( cpu_set_t ), &set );
	if ( result != 0 )
	{
		Print( "Failed to set thread %d affinity.\n", (unsigned int)pthread_self() );
	}
	else
	{
		Print( "Thread %d affinity set to 0x%02X\n", (unsigned int)pthread_self(), mask );
	}
#elif defined( OS_ANDROID )
	// Optionally use the faster cores of a heterogeneous CPU.
	if ( mask == THREAD_AFFINITY_BIG_CORES )
	{
		mask = 0;
		unsigned int bestFrequency = 0;
		for ( int i = 0; i < 16; i++ )
		{
			int maxFrequency = 0;
			const char * files[] =
			{
				"scaling_available_frequencies",	// not available on all devices
				"scaling_max_freq",					// no user read permission on all devices
				"cpuinfo_max_freq",					// could be set lower than the actual max, but better than nothing
			};
			for ( int j = 0; j < ARRAY_SIZE( files ); j++ )
			{
				char fileName[1024];
				sprintf( fileName, "/sys/devices/system/cpu/cpu%d/cpufreq/%s", i, files[j] );
				FILE * fp = fopen( fileName, "r" );
				if ( fp == NULL )
				{
					continue;
				}
				char buffer[1024];
				if ( fgets( buffer, sizeof( buffer ), fp ) == NULL )
				{
					fclose( fp );
					continue;
				}
				for ( int index = 0; buffer[index] != '\0'; )
				{
					const unsigned int frequency = atoi( buffer + index );
					maxFrequency = ( frequency > maxFrequency ) ? frequency : maxFrequency;
					while ( isspace( buffer[index] ) ) { index++; }
					while ( isdigit( buffer[index] ) ) { index++; }
				}
				fclose( fp );
				break;
			}
			if ( maxFrequency == 0 )
			{
				break;
			}

			if ( maxFrequency == bestFrequency )
			{
				mask |= ( 1 << i );
			}
			else if ( maxFrequency > bestFrequency )
			{
				mask = ( 1 << i );
				bestFrequency = maxFrequency;
			}
		}

		if ( mask == 0 )
		{
			return;
		}
	}

	// Set the thread affinity.
	pid_t pid = gettid();
	int syscallres = syscall( __NR_sched_setaffinity, pid, sizeof( mask ), &mask );
	if ( syscallres )
	{
		int err = errno;
		Print( "    Error sched_setaffinity(%d): thread=(%d) mask=0x%X err=%s(%d)\n", __NR_sched_setaffinity, pid, mask, strerror( err ), err );
	}
	else
	{
		Print( "    Thread %d affinity 0x%02X\n", pid, mask );
	}
#else
	UNUSED_PARM( mask );
#endif
}

static void Thread_SetRealTimePriority( int priority )
{
#if defined( OS_WINDOWS )
	UNUSED_PARM( priority );
	HANDLE process = GetCurrentProcess();
	if( !SetPriorityClass( process, REALTIME_PRIORITY_CLASS ) )
	{
		char buffer[1024];
		DWORD error = GetLastError();
		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), buffer, sizeof( buffer ), NULL );
		Print( "Failed to set process %p priority class: %s(%d)\n", process, buffer, error );
	}
	else
	{
		Print( "Process %p priority class set to real-time.\n", process );
	}
	HANDLE thread = GetCurrentThread();
	if ( !SetThreadPriority( thread, THREAD_PRIORITY_TIME_CRITICAL ) )
	{
		char buffer[1024];
		DWORD error = GetLastError();
		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), buffer, sizeof( buffer ), NULL );
		Print( "Failed to set thread %p priority: %s(%d)\n", thread, buffer, error );
	}
	else
	{
		Print( "Thread %p priority set to critical.\n", thread );
	}
#elif defined( OS_APPLE ) || defined( OS_LINUX )
	struct sched_param sp;
	memset( &sp, 0, sizeof( struct sched_param ) );
	sp.sched_priority = priority;
	if ( pthread_setschedparam( pthread_self(), SCHED_FIFO, &sp ) == -1 )
	{
		Print( "Failed to change thread %d priority.\n", (unsigned int)pthread_self() );
	}
	else
	{
		Print( "Thread %d set to SCHED_FIFO, priority=%d\n", (unsigned int)pthread_self(), priority );
	}
#elif defined( OS_ANDROID )
	struct sched_attr
	{
		uint32_t size;
		uint32_t sched_policy;
		uint64_t sched_flags;
		int32_t  sched_nice;
		uint32_t sched_priority;
		uint64_t sched_runtime;
		uint64_t sched_deadline;
		uint64_t sched_period;
	} attr;

	memset( &attr, 0, sizeof( attr ) );
	attr.size = sizeof( attr );
	attr.sched_policy = SCHED_FIFO;
	attr.sched_flags = SCHED_FLAG_RESET_ON_FORK;
	attr.sched_nice = 0;				// (SCHED_OTHER, SCHED_BATCH)
	attr.sched_priority = priority;		// (SCHED_FIFO, SCHED_RR)
	attr.sched_runtime = 0;				// (SCHED_DEADLINE)
	attr.sched_deadline = 0;			// (SCHED_DEADLINE)
	attr.sched_period = 0;				// (SCHED_DEADLINE)

	unsigned int flags = 0;

	pid_t pid = gettid();
	int syscallres = syscall( __NR_sched_setattr, pid, &attr, flags );
	if ( syscallres )
	{
		int err = errno;
		Print( "    Error sched_setattr(%d): thread=%d err=%s(%d)\n", __NR_sched_setattr, pid, strerror( err ), err );
	}
	else
	{
		Print( "    Thread %d set to SCHED_FIFO, priority=%d\n", pid, priority );
	}
#else
	UNUSED_PARM( priority );
#endif
}

static THREAD_RETURN_TYPE ThreadFunctionInternal( void * data )
{
	Thread_t * thread = (Thread_t *)data;

	Thread_SetName( thread->threadName );

	for ( ; ; )
	{
		Mutex_Lock( &thread->workMutex, true );
		if ( Signal_Wait( &thread->workIsAvailable, 0 ) )
		{
			Mutex_Unlock( &thread->workMutex );
		}
		else
		{
			Signal_Raise( &thread->workIsDone );
			Mutex_Unlock( &thread->workMutex );
			Signal_Wait( &thread->workIsAvailable, -1 );
		}
		if ( thread->terminate )
		{
			Signal_Raise( &thread->workIsDone );
			break;
		}
		thread->threadFunction( thread->threadData );
	}
	return THREAD_RETURN_VALUE;
}

static bool Thread_Create( Thread_t * thread, const char * threadName, threadFunction_t threadFunction, void * threadData )
{
	strncpy( thread->threadName, threadName, sizeof( thread->threadName ) );
	thread->threadName[sizeof( thread->threadName ) - 1] = '\0';
	thread->threadFunction = threadFunction;
	thread->threadData = threadData;
	thread->stack = NULL;
	Signal_Create( &thread->workIsDone, false );
	Signal_Create( &thread->workIsAvailable, true );
	Mutex_Create( &thread->workMutex );
	thread->terminate = false;

#if defined( OS_WINDOWS )
	const int stackSize = 512 * 1024;
	DWORD threadID;
	thread->handle = CreateThread( NULL, stackSize, (LPTHREAD_START_ROUTINE)ThreadFunctionInternal, thread, STACK_SIZE_PARAM_IS_A_RESERVATION, &threadID );
	if ( thread->handle == 0 )
	{
		return false;
	}
#else
	const int stackSize = 512 * 1024;
	pthread_attr_t attr;
	pthread_attr_init( &attr );
	pthread_attr_setstacksize( &attr, stackSize );
	int ret = pthread_create( &thread->handle, &attr, ThreadFunctionInternal, thread );
	if ( ret != 0 )
	{
		return false;
	}
	pthread_attr_destroy( &attr );
#endif

	Signal_Wait( &thread->workIsDone, -1 );
	return true;
}

static void Thread_Destroy( Thread_t * thread )
{
	Mutex_Lock( &thread->workMutex, true );
	Signal_Clear( &thread->workIsDone );
	thread->terminate = true;
	Signal_Raise( &thread->workIsAvailable );
	Mutex_Unlock( &thread->workMutex );
	Signal_Wait( &thread->workIsDone, -1 );
	Mutex_Destroy( &thread->workMutex );
	Signal_Destroy( &thread->workIsDone );
	Signal_Destroy( &thread->workIsAvailable );
#if defined( OS_WINDOWS )
	WaitForSingleObject( thread->handle, INFINITE );
	CloseHandle( thread->handle );
#else
	pthread_join( thread->handle, NULL );
#endif
}

static void Thread_Signal( Thread_t * thread )
{
	Mutex_Lock( &thread->workMutex, true );
	Signal_Clear( &thread->workIsDone );
	Signal_Raise( &thread->workIsAvailable );
	Mutex_Unlock( &thread->workMutex );
}

static void Thread_Join( Thread_t * thread )
{
	Signal_Wait( &thread->workIsDone, -1 );
}

static void Thread_Submit( Thread_t * thread, threadFunction_t threadFunction, void * threadData )
{
	Thread_Join( thread );
	thread->threadFunction = threadFunction;
	thread->threadData = threadData;
	Thread_Signal( thread );
}

/*
================================================================================================================================

Frame logging.

Each thread that calls FrameLog_Open will open its own log.
A frame log is always opened for a specified number of frames, and will
automatically close after the specified number of frames have been recorded.
The CPU and GPU times for the recorded frames will be listed at the end of the log.

FrameLog_t

static void FrameLog_Open( const char * fileName, const int frameCount );
static void FrameLog_Write( const char * fileName, const int lineNumber, const char * function );
static void FrameLog_BeginFrame();
static void FrameLog_EndFrame( const float cpuTimeMilliseconds, const float gpuTimeMilliseconds, const int gpuTimeFramesDelayed );

================================================================================================================================
*/

typedef struct
{
	FILE *		fp;
	float *		frameCpuTimes;
	float *		frameGpuTimes;
	int			frameCount;
	int			frame;
} FrameLog_t;

__thread FrameLog_t * threadFrameLog;

static FrameLog_t * FrameLog_Get()
{
	FrameLog_t * l = threadFrameLog;
	if ( l == NULL )
	{
		l = (FrameLog_t *) malloc( sizeof( FrameLog_t ) );
		memset( l, 0, sizeof( FrameLog_t ) );
		threadFrameLog = l;
	}
	return l;
}

static void FrameLog_Open( const char * fileName, const int frameCount )
{
	FrameLog_t * l = FrameLog_Get();
	if ( l != NULL && l->fp == NULL )
	{
		l->fp = fopen( fileName, "wb" );
		if ( l->fp == NULL )
		{
			Print( "Failed to open %s\n", fileName );
		}
		else
		{
			Print( "Opened frame log %s for %d frames.\n", fileName, frameCount );
			l->frameCpuTimes = (float *) malloc( frameCount * sizeof( l->frameCpuTimes[0] ) );
			l->frameGpuTimes = (float *) malloc( frameCount * sizeof( l->frameGpuTimes[0] ) );
			memset( l->frameCpuTimes, 0, frameCount * sizeof( l->frameCpuTimes[0] ) );
			memset( l->frameGpuTimes, 0, frameCount * sizeof( l->frameGpuTimes[0] ) );
			l->frameCount = frameCount;
			l->frame = 0;
		}
	}
}

static void FrameLog_Write( const char * fileName, const int lineNumber, const char * function )
{
	FrameLog_t * l = FrameLog_Get();
	if ( l != NULL && l->fp != NULL )
	{
		if ( l->frame < l->frameCount )
		{
			fprintf( l->fp, "%s(%d): %s\r\n", fileName, lineNumber, function );
		}
	}
}

static void FrameLog_BeginFrame()
{
	FrameLog_t * l = FrameLog_Get();
	if ( l != NULL && l->fp != NULL )
	{
		if ( l->frame < l->frameCount )
		{
#if defined( _DEBUG )
			fprintf( l->fp, "================ BEGIN FRAME %d ================\r\n", l->frame );
#endif
		}
	}
}

static void FrameLog_EndFrame( const float cpuTimeMilliseconds, const float gpuTimeMilliseconds, const int gpuTimeFramesDelayed )
{
	FrameLog_t * l = FrameLog_Get();
	if ( l != NULL && l->fp != NULL )
	{
		if ( l->frame < l->frameCount )
		{
			l->frameCpuTimes[l->frame] = cpuTimeMilliseconds;
#if defined( _DEBUG )
			fprintf( l->fp, "================ END FRAME %d ================\r\n", l->frame );
#endif
		}
		if ( l->frame >= gpuTimeFramesDelayed && l->frame < l->frameCount + gpuTimeFramesDelayed )
		{
			l->frameGpuTimes[l->frame - gpuTimeFramesDelayed] = gpuTimeMilliseconds;
		}

		l->frame++;

		if ( l->frame >= l->frameCount + gpuTimeFramesDelayed )
		{
			for ( int i = 0; i < l->frameCount; i++ )
			{
				fprintf( l->fp, "frame %d: CPU = %1.1f ms, GPU = %1.1f ms\r\n", i, l->frameCpuTimes[i], l->frameGpuTimes[i] );
			}

			Print( "Closing frame log file (%d frames).\n", l->frameCount );
			fclose( l->fp );
			free( l->frameCpuTimes );
			free( l->frameGpuTimes );
			memset( l, 0, sizeof( FrameLog_t ) );
		}
	}
}

/*
================================================================================================================================

Vectors and matrices. All matrices are column-major.

Vector2i_t
Vector3i_t
Vector4i_t
Vector2f_t
Vector3f_t
Vector4f_t
Quatf_t
Matrix2x2f_t
Matrix2x3f_t
Matrix2x4f_t
Matrix3x2f_t
Matrix3x3f_t
Matrix3x4f_t
Matrix4x2f_t
Matrix4x3f_t
Matrix4x4f_t

static void Vector3f_Set( Vector3f_t * v, const float value );
static void Vector3f_Add( Vector3f_t * result, const Vector3f_t * a, const Vector3f_t * b );
static void Vector3f_Sub( Vector3f_t * result, const Vector3f_t * a, const Vector3f_t * b );
static void Vector3f_Min( Vector3f_t * result, const Vector3f_t * a, const Vector3f_t * b );
static void Vector3f_Max( Vector3f_t * result, const Vector3f_t * a, const Vector3f_t * b );
static void Vector3f_Decay( Vector3f_t * result, const Vector3f_t * a, const float value );
static void Vector3f_Lerp( Vector3f_t * result, const Vector3f_t * a, const Vector3f_t * b, const float fraction );
static void Vector3f_Normalize( Vector3f_t * v );

static void Quatf_Lerp( Quatf_t * result, const Quatf_t * a, const Quatf_t * b, const float fraction );

static void Matrix3x3f_CreateTransposeFromMatrix4x4f( Matrix3x3f_t * result, const Matrix4x4f_t * src );
static void Matrix3x4f_CreateFromMatrix4x4f( Matrix3x4f_t * result, const Matrix4x4f_t * src );

static void Matrix4x4f_CreateIdentity( Matrix4x4f_t * result );
static void Matrix4x4f_CreateTranslation( Matrix4x4f_t * result, const float x, const float y, const float z );
static void Matrix4x4f_CreateRotation( Matrix4x4f_t * result, const float degreesX, const float degreesY, const float degreesZ );
static void Matrix4x4f_CreateScale( Matrix4x4f_t * result, const float x, const float y, const float z );
static void Matrix4x4f_CreateTranslationRotationScale( Matrix4x4f_t * result, const Vector3f_t * scale, const Quatf_t * rotation, const Vector3f_t * translation );
static void Matrix4x4f_CreateProjection( Matrix4x4f_t * result, const float minX, const float maxX,
											float const minY, const float maxY, const float nearZ, const float farZ );
static void Matrix4x4f_CreateProjectionFov( Matrix4x4f_t * result, const float fovDegreesX, const float fovDegreesY,
											const float offsetX, const float offsetY, const float nearZ, const float farZ );
static void Matrix4x4f_CreateFromQuaternion( Matrix3x4f_t * result, const Quatf_t * src );
static void Matrix4x4f_CreateOffsetScaleForBounds( Matrix4x4f_t * result, const Matrix4x4f_t * matrix, const Vector3f_t * mins, const Vector3f_t * maxs );

static bool Matrix4x4f_IsAffine( const Matrix4x4f_t * matrix, const float epsilon );
static bool Matrix4x4f_IsOrthogonal( const Matrix4x4f_t * matrix, const float epsilon );
static bool Matrix4x4f_IsOrthonormal( const Matrix4x4f_t * matrix, const float epsilon );
static bool Matrix4x4f_IsHomogeneous( const Matrix4x4f_t * matrix, const float epsilon );

static void Matrix4x4f_GetTranslation( Vector3f_t * result, const Matrix4x4f_t * src );
static void Matrix4x4f_GetRotation( Quatf_t * result, const Matrix4x4f_t * src );
static void Matrix4x4f_GetScale( Vector3f_t * result, const Matrix4x4f_t * src );

static void Matrix4x4f_Multiply( Matrix4x4f_t * result, const Matrix4x4f_t * a, const Matrix4x4f_t * b );
static void Matrix4x4f_Transpose( Matrix4x4f_t * result, const Matrix4x4f_t * src );
static void Matrix4x4f_Invert( Matrix4x4f_t * result, const Matrix4x4f_t * src );
static void Matrix4x4f_InvertHomogeneous( Matrix4x4f_t * result, const Matrix4x4f_t * src );

static void Matrix4x4f_TransformVector3f( Vector3f_t * result, const Matrix4x4f_t * m, const Vector3f_t * v );
static void Matrix4x4f_TransformVector4f( Vector4f_t * result, const Matrix4x4f_t * m, const Vector4f_t * v );

static void Matrix4x4f_TransformBounds( Vector3f_t * resultMins, Vector3f_t * resultMaxs, const Matrix4x4f_t * matrix, const Vector3f_t * mins, const Vector3f_t * maxs );
static bool Matrix4x4f_CullBounds( const Matrix4x4f_t * mvp, const Vector3f_t * mins, const Vector3f_t * maxs );

================================================================================================================================
*/

// 2D integer vector
typedef struct
{
	int x;
	int y;
} Vector2i_t;

// 3D integer vector
typedef struct
{
	int x;
	int y;
	int z;
} Vector3i_t;

// 4D integer vector
typedef struct
{
	int x;
	int y;
	int z;
	int w;
} Vector4i_t;

// 2D float vector
typedef struct
{
	float x;
	float y;
} Vector2f_t;

// 3D float vector
typedef struct
{
	float x;
	float y;
	float z;
} Vector3f_t;

// 4D float vector
typedef struct
{
	float x;
	float y;
	float z;
	float w;
} Vector4f_t;

// Quaternion
typedef struct
{
	float x;
	float y;
	float z;
	float w;
} Quatf_t;

// Column-major 2x2 matrix
typedef struct
{
	float m[2][2];
} Matrix2x2f_t;

// Column-major 2x3 matrix
typedef struct
{
	float m[2][3];
} Matrix2x3f_t;

// Column-major 2x4 matrix
typedef struct
{
	float m[2][4];
} Matrix2x4f_t;

// Column-major 3x2 matrix
typedef struct
{
	float m[3][2];
} Matrix3x2f_t;

// Column-major 3x3 matrix
typedef struct
{
	float m[3][3];
} Matrix3x3f_t;

// Column-major 3x4 matrix
typedef struct
{
	float m[3][4];
} Matrix3x4f_t;

// Column-major 4x2 matrix
typedef struct
{
	float m[4][2];
} Matrix4x2f_t;

// Column-major 4x3 matrix
typedef struct
{
	float m[4][3];
} Matrix4x3f_t;

// Column-major 4x4 matrix
typedef struct
{
	float m[4][4];
} Matrix4x4f_t;

static const Vector4f_t colorRed		= { 1.0f, 0.0f, 0.0f, 1.0f };
static const Vector4f_t colorGreen		= { 0.0f, 1.0f, 0.0f, 1.0f };
static const Vector4f_t colorBlue		= { 0.0f, 0.0f, 1.0f, 1.0f };
static const Vector4f_t colorYellow		= { 1.0f, 1.0f, 0.0f, 1.0f };
static const Vector4f_t colorPurple		= { 1.0f, 0.0f, 1.0f, 1.0f };
static const Vector4f_t colorCyan		= { 0.0f, 1.0f, 1.0f, 1.0f };
static const Vector4f_t colorLightGrey	= { 0.7f, 0.7f, 0.7f, 1.0f };
static const Vector4f_t colorDarkGrey	= { 0.3f, 0.3f, 0.3f, 1.0f };

static float RcpSqrt( const float x )
{
	const float SMALLEST_NON_DENORMAL = 1.1754943508222875e-038f;	// ( 1U << 23 )
	const float rcp = ( x >= SMALLEST_NON_DENORMAL ) ? 1.0f / sqrtf( x ) : 1.0f;
	return rcp;
}

static void Vector3f_Set( Vector3f_t * v, const float value )
{
	v->x = value;
	v->y = value;
	v->z = value;
}

static void Vector3f_Add( Vector3f_t * result, const Vector3f_t * a, const Vector3f_t * b )
{
	result->x = a->x + b->x;
	result->y = a->y + b->y;
	result->z = a->z + b->z;
}

static void Vector3f_Sub( Vector3f_t * result, const Vector3f_t * a, const Vector3f_t * b )
{
	result->x = a->x - b->x;
	result->y = a->y - b->y;
	result->z = a->z - b->z;
}

static void Vector3f_Min( Vector3f_t * result, const Vector3f_t * a, const Vector3f_t * b )
{
	result->x = ( a->x < b->x ) ? a->x : b->x;
	result->y = ( a->y < b->y ) ? a->y : b->y;
	result->z = ( a->z < b->z ) ? a->z : b->z;
}

static void Vector3f_Max( Vector3f_t * result, const Vector3f_t * a, const Vector3f_t * b )
{
	result->x = ( a->x > b->x ) ? a->x : b->x;
	result->y = ( a->y > b->y ) ? a->y : b->y;
	result->z = ( a->z > b->z ) ? a->z : b->z;
}

static void Vector3f_Decay( Vector3f_t * result, const Vector3f_t * a, const float value )
{
	result->x = ( fabsf( a->x ) > value ) ? ( ( a->x > 0.0f ) ? ( a->x - value ) : ( a->x + value ) ) : 0.0f;
	result->y = ( fabsf( a->y ) > value ) ? ( ( a->y > 0.0f ) ? ( a->y - value ) : ( a->y + value ) ) : 0.0f;
	result->z = ( fabsf( a->z ) > value ) ? ( ( a->z > 0.0f ) ? ( a->z - value ) : ( a->z + value ) ) : 0.0f;
}

static void Vector3f_Lerp( Vector3f_t * result, const Vector3f_t * a, const Vector3f_t * b, const float fraction )
{
	result->x = a->x + fraction * ( b->x - a->x );
	result->y = a->y + fraction * ( b->y - a->y );
	result->z = a->z + fraction * ( b->z - a->z );
}

static void Vector3f_Normalize( Vector3f_t * v )
{
	const float lengthRcp = RcpSqrt( v->x * v->x + v->y * v->y + v->z * v->z );
	v->x *= lengthRcp;
	v->y *= lengthRcp;
	v->z *= lengthRcp;
}

static void Quatf_Lerp( Quatf_t * result, const Quatf_t * a, const Quatf_t * b, const float fraction )
{
	const float s = a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
	const float fa = 1.0f - fraction;
	const float fb = ( s < 0.0f ) ? -fraction : fraction;
	const float x = a->x * fa + b->x * fb;
	const float y = a->y * fa + b->y * fb;
	const float z = a->z * fa + b->z * fb;
	const float w = a->w * fa + b->w * fb;
	const float lengthRcp = RcpSqrt( x * x + y * y + z * z + w * w );
	result->x = x * lengthRcp;
	result->y = y * lengthRcp;
	result->z = z * lengthRcp;
	result->w = w * lengthRcp;
}

static void Matrix3x3f_CreateTransposeFromMatrix4x4f( Matrix3x3f_t * result, const Matrix4x4f_t * src )
{
	result->m[0][0] = src->m[0][0];
	result->m[0][1] = src->m[1][0];
	result->m[0][2] = src->m[2][0];

	result->m[1][0] = src->m[0][1];
	result->m[1][1] = src->m[1][1];
	result->m[1][2] = src->m[2][1];

	result->m[2][0] = src->m[0][2];
	result->m[2][1] = src->m[1][2];
	result->m[2][2] = src->m[2][2];
}

static void Matrix3x4f_CreateFromMatrix4x4f( Matrix3x4f_t * result, const Matrix4x4f_t * src )
{
	result->m[0][0] = src->m[0][0];
	result->m[0][1] = src->m[1][0];
	result->m[0][2] = src->m[2][0];
	result->m[0][3] = src->m[3][0];
	result->m[1][0] = src->m[0][1];
	result->m[1][1] = src->m[1][1];
	result->m[1][2] = src->m[2][1];
	result->m[1][3] = src->m[3][1];
	result->m[2][0] = src->m[0][2];
	result->m[2][1] = src->m[1][2];
	result->m[2][2] = src->m[2][2];
	result->m[2][3] = src->m[3][2];
}

// Use left-multiplication to accumulate transformations.
static void Matrix4x4f_Multiply( Matrix4x4f_t * result, const Matrix4x4f_t * a, const Matrix4x4f_t * b )
{
	result->m[0][0] = a->m[0][0] * b->m[0][0] + a->m[1][0] * b->m[0][1] + a->m[2][0] * b->m[0][2] + a->m[3][0] * b->m[0][3];
	result->m[0][1] = a->m[0][1] * b->m[0][0] + a->m[1][1] * b->m[0][1] + a->m[2][1] * b->m[0][2] + a->m[3][1] * b->m[0][3];
	result->m[0][2] = a->m[0][2] * b->m[0][0] + a->m[1][2] * b->m[0][1] + a->m[2][2] * b->m[0][2] + a->m[3][2] * b->m[0][3];
	result->m[0][3] = a->m[0][3] * b->m[0][0] + a->m[1][3] * b->m[0][1] + a->m[2][3] * b->m[0][2] + a->m[3][3] * b->m[0][3];

	result->m[1][0] = a->m[0][0] * b->m[1][0] + a->m[1][0] * b->m[1][1] + a->m[2][0] * b->m[1][2] + a->m[3][0] * b->m[1][3];
	result->m[1][1] = a->m[0][1] * b->m[1][0] + a->m[1][1] * b->m[1][1] + a->m[2][1] * b->m[1][2] + a->m[3][1] * b->m[1][3];
	result->m[1][2] = a->m[0][2] * b->m[1][0] + a->m[1][2] * b->m[1][1] + a->m[2][2] * b->m[1][2] + a->m[3][2] * b->m[1][3];
	result->m[1][3] = a->m[0][3] * b->m[1][0] + a->m[1][3] * b->m[1][1] + a->m[2][3] * b->m[1][2] + a->m[3][3] * b->m[1][3];

	result->m[2][0] = a->m[0][0] * b->m[2][0] + a->m[1][0] * b->m[2][1] + a->m[2][0] * b->m[2][2] + a->m[3][0] * b->m[2][3];
	result->m[2][1] = a->m[0][1] * b->m[2][0] + a->m[1][1] * b->m[2][1] + a->m[2][1] * b->m[2][2] + a->m[3][1] * b->m[2][3];
	result->m[2][2] = a->m[0][2] * b->m[2][0] + a->m[1][2] * b->m[2][1] + a->m[2][2] * b->m[2][2] + a->m[3][2] * b->m[2][3];
	result->m[2][3] = a->m[0][3] * b->m[2][0] + a->m[1][3] * b->m[2][1] + a->m[2][3] * b->m[2][2] + a->m[3][3] * b->m[2][3];

	result->m[3][0] = a->m[0][0] * b->m[3][0] + a->m[1][0] * b->m[3][1] + a->m[2][0] * b->m[3][2] + a->m[3][0] * b->m[3][3];
	result->m[3][1] = a->m[0][1] * b->m[3][0] + a->m[1][1] * b->m[3][1] + a->m[2][1] * b->m[3][2] + a->m[3][1] * b->m[3][3];
	result->m[3][2] = a->m[0][2] * b->m[3][0] + a->m[1][2] * b->m[3][1] + a->m[2][2] * b->m[3][2] + a->m[3][2] * b->m[3][3];
	result->m[3][3] = a->m[0][3] * b->m[3][0] + a->m[1][3] * b->m[3][1] + a->m[2][3] * b->m[3][2] + a->m[3][3] * b->m[3][3];
}

// Creates the transpose of the given matrix.
static void Matrix4x4f_Transpose( Matrix4x4f_t * result, const Matrix4x4f_t * src )
{
	result->m[0][0] = src->m[0][0];
	result->m[0][1] = src->m[1][0];
	result->m[0][2] = src->m[2][0];
	result->m[0][3] = src->m[3][0];

	result->m[1][0] = src->m[0][1];
	result->m[1][1] = src->m[1][1];
	result->m[1][2] = src->m[2][1];
	result->m[1][3] = src->m[3][1];

	result->m[2][0] = src->m[0][2];
	result->m[2][1] = src->m[1][2];
	result->m[2][2] = src->m[2][2];
	result->m[2][3] = src->m[3][2];

	result->m[3][0] = src->m[0][3];
	result->m[3][1] = src->m[1][3];
	result->m[3][2] = src->m[2][3];
	result->m[3][3] = src->m[3][3];
}

// Returns a 3x3 minor of a 4x4 matrix.
static float Matrix4x4f_Minor( const Matrix4x4f_t * matrix, int r0, int r1, int r2, int c0, int c1, int c2 )
{
	return	matrix->m[r0][c0] * ( matrix->m[r1][c1] * matrix->m[r2][c2] - matrix->m[r2][c1] * matrix->m[r1][c2] ) -
			matrix->m[r0][c1] * ( matrix->m[r1][c0] * matrix->m[r2][c2] - matrix->m[r2][c0] * matrix->m[r1][c2] ) +
			matrix->m[r0][c2] * ( matrix->m[r1][c0] * matrix->m[r2][c1] - matrix->m[r2][c0] * matrix->m[r1][c1] );
}
 
// Calculates the inverse of a 4x4 matrix.
static void Matrix4x4f_Invert( Matrix4x4f_t * result, const Matrix4x4f_t * src )
{
	const float rcpDet = 1.0f / (	src->m[0][0] * Matrix4x4f_Minor( src, 1, 2, 3, 1, 2, 3 ) -
									src->m[0][1] * Matrix4x4f_Minor( src, 1, 2, 3, 0, 2, 3 ) +
									src->m[0][2] * Matrix4x4f_Minor( src, 1, 2, 3, 0, 1, 3 ) -
									src->m[0][3] * Matrix4x4f_Minor( src, 1, 2, 3, 0, 1, 2 ) );

	result->m[0][0] =  Matrix4x4f_Minor( src, 1, 2, 3, 1, 2, 3 ) * rcpDet;
	result->m[0][1] = -Matrix4x4f_Minor( src, 0, 2, 3, 1, 2, 3 ) * rcpDet;
	result->m[0][2] =  Matrix4x4f_Minor( src, 0, 1, 3, 1, 2, 3 ) * rcpDet;
	result->m[0][3] = -Matrix4x4f_Minor( src, 0, 1, 2, 1, 2, 3 ) * rcpDet;
	result->m[1][0] = -Matrix4x4f_Minor( src, 1, 2, 3, 0, 2, 3 ) * rcpDet;
	result->m[1][1] =  Matrix4x4f_Minor( src, 0, 2, 3, 0, 2, 3 ) * rcpDet;
	result->m[1][2] = -Matrix4x4f_Minor( src, 0, 1, 3, 0, 2, 3 ) * rcpDet;
	result->m[1][3] =  Matrix4x4f_Minor( src, 0, 1, 2, 0, 2, 3 ) * rcpDet;
	result->m[2][0] =  Matrix4x4f_Minor( src, 1, 2, 3, 0, 1, 3 ) * rcpDet;
	result->m[2][1] = -Matrix4x4f_Minor( src, 0, 2, 3, 0, 1, 3 ) * rcpDet;
	result->m[2][2] =  Matrix4x4f_Minor( src, 0, 1, 3, 0, 1, 3 ) * rcpDet;
	result->m[2][3] = -Matrix4x4f_Minor( src, 0, 1, 2, 0, 1, 3 ) * rcpDet;
	result->m[3][0] = -Matrix4x4f_Minor( src, 1, 2, 3, 0, 1, 2 ) * rcpDet;
	result->m[3][1] =  Matrix4x4f_Minor( src, 0, 2, 3, 0, 1, 2 ) * rcpDet;
	result->m[3][2] = -Matrix4x4f_Minor( src, 0, 1, 3, 0, 1, 2 ) * rcpDet;
	result->m[3][3] =  Matrix4x4f_Minor( src, 0, 1, 2, 0, 1, 2 ) * rcpDet;
}

// Calculates the inverse of a 4x4 homogeneous matrix.
static void Matrix4x4f_InvertHomogeneous( Matrix4x4f_t * result, const Matrix4x4f_t * src )
{
	result->m[0][0] = src->m[0][0];
	result->m[0][1] = src->m[1][0];
	result->m[0][2] = src->m[2][0];
	result->m[0][3] = 0.0f;
	result->m[1][0] = src->m[0][1];
	result->m[1][1] = src->m[1][1];
	result->m[1][2] = src->m[2][1];
	result->m[1][3] = 0.0f;
	result->m[2][0] = src->m[0][2];
	result->m[2][1] = src->m[1][2];
	result->m[2][2] = src->m[2][2];
	result->m[2][3] = 0.0f;
	result->m[3][0] = -( src->m[0][0] * src->m[3][0] + src->m[0][1] * src->m[3][1] + src->m[0][2] * src->m[3][2] );
	result->m[3][1] = -( src->m[1][0] * src->m[3][0] + src->m[1][1] * src->m[3][1] + src->m[1][2] * src->m[3][2] );
	result->m[3][2] = -( src->m[2][0] * src->m[3][0] + src->m[2][1] * src->m[3][1] + src->m[2][2] * src->m[3][2] );
	result->m[3][3] = 1.0f;
}

// Creates an identity matrix.
static void Matrix4x4f_CreateIdentity( Matrix4x4f_t * result )
{
	result->m[0][0] = 1.0f; result->m[0][1] = 0.0f; result->m[0][2] = 0.0f; result->m[0][3] = 0.0f;
	result->m[1][0] = 0.0f; result->m[1][1] = 1.0f; result->m[1][2] = 0.0f; result->m[1][3] = 0.0f;
	result->m[2][0] = 0.0f; result->m[2][1] = 0.0f; result->m[2][2] = 1.0f; result->m[2][3] = 0.0f;
	result->m[3][0] = 0.0f; result->m[3][1] = 0.0f; result->m[3][2] = 0.0f; result->m[3][3] = 1.0f;
}

// Creates a translation matrix.
static void Matrix4x4f_CreateTranslation( Matrix4x4f_t * result, const float x, const float y, const float z )
{
	result->m[0][0] = 1.0f; result->m[0][1] = 0.0f; result->m[0][2] = 0.0f; result->m[0][3] = 0.0f;
	result->m[1][0] = 0.0f; result->m[1][1] = 1.0f; result->m[1][2] = 0.0f; result->m[1][3] = 0.0f;
	result->m[2][0] = 0.0f; result->m[2][1] = 0.0f; result->m[2][2] = 1.0f; result->m[2][3] = 0.0f;
	result->m[3][0] =    x; result->m[3][1] =    y; result->m[3][2] =    z; result->m[3][3] = 1.0f;
}

// Creates a rotation matrix.
// If -Z=forward, +Y=up, +X=right, then degreesX=pitch, degreesY=yaw, degreesZ=roll.
static void Matrix4x4f_CreateRotation( Matrix4x4f_t * result, const float degreesX, const float degreesY, const float degreesZ )
{
	const float sinX = sinf( degreesX * ( MATH_PI / 180.0f ) );
	const float cosX = cosf( degreesX * ( MATH_PI / 180.0f ) );
	const Matrix4x4f_t rotationX =
	{ {
		{ 1,     0,    0, 0 },
		{ 0,  cosX, sinX, 0 },
		{ 0, -sinX, cosX, 0 },
		{ 0,     0,    0, 1 }
	} };
	const float sinY = sinf( degreesY * ( MATH_PI / 180.0f ) );
	const float cosY = cosf( degreesY * ( MATH_PI / 180.0f ) );
	const Matrix4x4f_t rotationY =
	{ {
		{ cosY, 0, -sinY, 0 },
		{    0, 1,     0, 0 },
		{ sinY, 0,  cosY, 0 },
		{    0, 0,     0, 1 }
	} };
	const float sinZ = sinf( degreesZ * ( MATH_PI / 180.0f ) );
	const float cosZ = cosf( degreesZ * ( MATH_PI / 180.0f ) );
	const Matrix4x4f_t rotationZ =
	{ {
		{  cosZ, sinZ, 0, 0 },
		{ -sinZ, cosZ, 0, 0 },
		{     0,    0, 1, 0 },
		{     0,    0, 0, 1 }
	} };
	Matrix4x4f_t rotationXY;
	Matrix4x4f_Multiply( &rotationXY, &rotationY, &rotationX );
	Matrix4x4f_Multiply( result, &rotationZ, &rotationXY );
}

// Creates a scale matrix.
static void Matrix4x4f_CreateScale( Matrix4x4f_t * result, const float x, const float y, const float z )
{
	result->m[0][0] =    x; result->m[0][1] = 0.0f; result->m[0][2] = 0.0f; result->m[0][3] = 0.0f;
	result->m[1][0] = 0.0f; result->m[1][1] =    y; result->m[1][2] = 0.0f; result->m[1][3] = 0.0f;
	result->m[2][0] = 0.0f; result->m[2][1] = 0.0f; result->m[2][2] =    z; result->m[2][3] = 0.0f;
	result->m[3][0] = 0.0f; result->m[3][1] = 0.0f; result->m[3][2] = 0.0f; result->m[3][3] = 1.0f;
}

// Creates a matrix from a quaternion.
static void Matrix4x4f_CreateFromQuaternion( Matrix4x4f_t * result, const Quatf_t * quat )
{
	const float x2 = quat->x + quat->x;
	const float y2 = quat->y + quat->y;
	const float z2 = quat->z + quat->z;

	const float xx2 = quat->x * x2;
	const float yy2 = quat->y * y2;
	const float zz2 = quat->z * z2;

	const float yz2 = quat->y * z2;
	const float wx2 = quat->w * x2;
	const float xy2 = quat->x * y2;
	const float wz2 = quat->w * z2;
	const float xz2 = quat->x * z2;
	const float wy2 = quat->w * y2;

	result->m[0][0] = 1.0f - yy2 - zz2;
	result->m[0][1] = xy2 + wz2;
	result->m[0][2] = xz2 - wy2;
	result->m[0][3] = 0.0f;

	result->m[1][0] = xy2 - wz2;
	result->m[1][1] = 1.0f - xx2 - zz2;
	result->m[1][2] = yz2 + wx2;
	result->m[1][3] = 0.0f;

	result->m[2][0] = xz2 + wy2;
	result->m[2][1] = yz2 - wx2;
	result->m[2][2] = 1.0f - xx2 - yy2;
	result->m[2][3] = 0.0f;

	result->m[3][0] = 0.0f;
	result->m[3][1] = 0.0f;
	result->m[3][2] = 0.0f;
	result->m[3][3] = 1.0f;
}

// Creates a combined translation(rotation(scale(object))) matrix.
static void Matrix4x4f_CreateTranslationRotationScale( Matrix4x4f_t * result, const Vector3f_t * scale, const Quatf_t * rotation, const Vector3f_t * translation )
{
	Matrix4x4f_t scaleMatrix;
	Matrix4x4f_CreateScale( &scaleMatrix, scale->x, scale->y, scale->z );

	Matrix4x4f_t rotationMatrix;
	Matrix4x4f_CreateFromQuaternion( &rotationMatrix, rotation );

	Matrix4x4f_t translationMatrix;
	Matrix4x4f_CreateTranslation( &translationMatrix, translation->x, translation->y, translation->z );

	Matrix4x4f_t combinedMatrix;
	Matrix4x4f_Multiply( &combinedMatrix, &rotationMatrix, &scaleMatrix );
	Matrix4x4f_Multiply( result, &translationMatrix, &combinedMatrix );
}

// Creates a projection matrix based on the specified dimensions.
// The projection matrix transforms -Z=forward, +Y=up, +X=right to the appropriate clip space for the graphics API.
// The far plane is placed at infinity if farZ <= nearZ.
// An infinite projection matrix is preferred for rasterization because, except for
// things *right* up against the near plane, it always provides better precision:
//		"Tightening the Precision of Perspective Rendering"
//		Paul Upchurch, Mathieu Desbrun
//		Journal of Graphics Tools, Volume 16, Issue 1, 2012
static void Matrix4x4f_CreateProjection( Matrix4x4f_t * result, const float minX, const float maxX,
											float const minY, const float maxY, const float nearZ, const float farZ )
{
	const float width = maxX - minX;

#if defined( GRAPHICS_API_VULKAN )
	// Set to minY - maxY for a clip space with positive Y down (Vulkan).
	const float height = minY - maxY;
#else
	// Set to maxY - minY for a clip space with positive Y up (OpenGL / D3D).
	const float height = maxY - minY;
#endif

#if defined( GRAPHICS_API_OPENGL )
	// Set to nearZ for a [-1,1] Z clip space (OpenGL).
	const float offsetZ = nearZ;
#else
	// Set to zero for a [0,1] Z clip space (D3D / Vulkan).
	const float offsetZ = 0;
#endif

	if ( farZ <= nearZ )
	{
		// place the far plane at infinity
		result->m[0][0] = 2 * nearZ / width;
		result->m[1][0] = 0;
		result->m[2][0] = ( maxX + minX ) / width;
		result->m[3][0] = 0;

		result->m[0][1] = 0;
		result->m[1][1] = 2 * nearZ / height;
		result->m[2][1] = ( maxY + minY ) / height;
		result->m[3][1] = 0;

		result->m[0][2] = 0;
		result->m[1][2] = 0;
		result->m[2][2] = -1;
		result->m[3][2] = -( nearZ + offsetZ );

		result->m[0][3] = 0;
		result->m[1][3] = 0;
		result->m[2][3] = -1;
		result->m[3][3] = 0;
	}
	else
	{
		// normal projection
		result->m[0][0] = 2 * nearZ / width;
		result->m[1][0] = 0;
		result->m[2][0] = ( maxX + minX ) / width;
		result->m[3][0] = 0;

		result->m[0][1] = 0;
		result->m[1][1] = 2 * nearZ / height;
		result->m[2][1] = ( maxY + minY ) / height;
		result->m[3][1] = 0;

		result->m[0][2] = 0;
		result->m[1][2] = 0;
		result->m[2][2] = -( farZ + offsetZ ) / ( farZ - nearZ );
		result->m[3][2] = -( farZ * ( nearZ + offsetZ ) ) / ( farZ - nearZ );

		result->m[0][3] = 0;
		result->m[1][3] = 0;
		result->m[2][3] = -1;
		result->m[3][3] = 0;
	}
}

// Creates a projection matrix based on the specified FOV.
static void Matrix4x4f_CreateProjectionFov( Matrix4x4f_t * result, const float fovDegreesX, const float fovDegreesY,
												const float offsetX, const float offsetY, const float nearZ, const float farZ )
{
	const float halfWidth = nearZ * tanf( fovDegreesX * ( 0.5f * MATH_PI / 180.0f ) );
	const float halfHeight = nearZ * tanf( fovDegreesY * ( 0.5f * MATH_PI / 180.0f ) );

	const float minX = offsetX - halfWidth;
	const float maxX = offsetX + halfWidth;

	const float minY = offsetY - halfHeight;
	const float maxY = offsetY + halfHeight;

	Matrix4x4f_CreateProjection( result, minX, maxX, minY, maxY, nearZ, farZ );
}

// Creates a matrix that transforms the -1 to 1 cube to cover the given 'mins' and 'maxs' transformed with the given 'matrix'.
static void Matrix4x4f_CreateOffsetScaleForBounds( Matrix4x4f_t * result, const Matrix4x4f_t * matrix, const Vector3f_t * mins, const Vector3f_t * maxs )
{
	const Vector3f_t offset = { ( maxs->x + mins->x ) * 0.5f, ( maxs->y + mins->y ) * 0.5f, ( maxs->z + mins->z ) * 0.5f };
	const Vector3f_t scale = { ( maxs->x - mins->x ) * 0.5f, ( maxs->y - mins->y ) * 0.5f, ( maxs->z - mins->z ) * 0.5f };

	result->m[0][0] = matrix->m[0][0] * scale.x;
	result->m[0][1] = matrix->m[0][1] * scale.x;
	result->m[0][2] = matrix->m[0][2] * scale.x;
	result->m[0][3] = matrix->m[0][3] * scale.x;

	result->m[1][0] = matrix->m[1][0] * scale.y;
	result->m[1][1] = matrix->m[1][1] * scale.y;
	result->m[1][2] = matrix->m[1][2] * scale.y;
	result->m[1][3] = matrix->m[1][3] * scale.y;

	result->m[2][0] = matrix->m[2][0] * scale.z;
	result->m[2][1] = matrix->m[2][1] * scale.z;
	result->m[2][2] = matrix->m[2][2] * scale.z;
	result->m[2][3] = matrix->m[2][3] * scale.z;

	result->m[3][0] = matrix->m[3][0] + matrix->m[0][0] * offset.x + matrix->m[1][0] * offset.y + matrix->m[2][0] * offset.z;
	result->m[3][1] = matrix->m[3][1] + matrix->m[0][1] * offset.x + matrix->m[1][1] * offset.y + matrix->m[2][1] * offset.z;
	result->m[3][2] = matrix->m[3][2] + matrix->m[0][2] * offset.x + matrix->m[1][2] * offset.y + matrix->m[2][2] * offset.z;
	result->m[3][3] = matrix->m[3][3] + matrix->m[0][3] * offset.x + matrix->m[1][3] * offset.y + matrix->m[2][3] * offset.z;
}

// Returns true if the given matrix is affine.
static bool Matrix4x4f_IsAffine( const Matrix4x4f_t * matrix, const float epsilon )
{
	return	fabsf( matrix->m[0][3] ) <= epsilon &&
			fabsf( matrix->m[1][3] ) <= epsilon &&
			fabsf( matrix->m[2][3] ) <= epsilon &&
			fabsf( matrix->m[3][3] - 1.0f ) <= epsilon;
}

// Returns true if the given matrix is orthogonal.
static bool Matrix4x4f_IsOrthogonal( const Matrix4x4f_t * matrix, const float epsilon )
{
	for ( int i = 0; i < 3; i++ )
	{
		for ( int j = 0; j < 3; j++ )
		{
			if ( i != j )
			{
				if ( fabsf( matrix->m[i][0] * matrix->m[j][0] + matrix->m[i][1] * matrix->m[j][1] + matrix->m[i][2] * matrix->m[j][2] ) > epsilon )
				{
					return false;
				}
				if ( fabsf( matrix->m[0][i] * matrix->m[0][j] + matrix->m[1][i] * matrix->m[1][j] + matrix->m[2][i] * matrix->m[2][j] ) > epsilon )
				{
					return false;
				}
			}
		}
	}
	return true;
}

// Returns true if the given matrix is orthonormal.
static bool Matrix4x4f_IsOrthonormal( const Matrix4x4f_t * matrix, const float epsilon )
{
	for ( int i = 0; i < 3; i++ )
	{
		for ( int j = 0; j < 3; j++ )
		{
			const float kd = ( i == j ) ? 1.0f : 0.0f;	// Kronecker delta
			if ( fabsf( kd - ( matrix->m[i][0] * matrix->m[j][0] + matrix->m[i][1] * matrix->m[j][1] + matrix->m[i][2] * matrix->m[j][2] ) ) > epsilon )
			{
				return false;
			}
			if ( fabsf( kd - ( matrix->m[0][i] * matrix->m[0][j] + matrix->m[1][i] * matrix->m[1][j] + matrix->m[2][i] * matrix->m[2][j] ) ) > epsilon )
			{
				return false;
			}
		}
	}
	return true;
}

// Returns true if the given matrix is homogeneous.
static bool Matrix4x4f_IsHomogeneous( const Matrix4x4f_t * matrix, const float epsilon )
{
	return Matrix4x4f_IsAffine( matrix, epsilon ) && Matrix4x4f_IsOrthonormal( matrix, epsilon );
}

// Get the translation from a combined translation(rotation(scale(object))) matrix.
static void Matrix4x4f_GetTranslation( Vector3f_t * result, const Matrix4x4f_t * src )
{
	assert( Matrix4x4f_IsAffine( src, 1e-4f ) );
	assert( Matrix4x4f_IsOrthogonal( src, 1e-4f ) );

	result->x = src->m[3][0];
	result->y = src->m[3][1];
	result->z = src->m[3][2];
}

// Get the rotation from a combined translation(rotation(scale(object))) matrix.
static void Matrix4x4f_GetRotation( Quatf_t * result, const Matrix4x4f_t * src )
{
	assert( Matrix4x4f_IsAffine( src, 1e-4f ) );
	assert( Matrix4x4f_IsOrthogonal( src, 1e-4f ) );

	const float scaleX = RcpSqrt( src->m[0][0] * src->m[0][0] + src->m[0][1] * src->m[0][1] + src->m[0][2] * src->m[0][2] );
	const float scaleY = RcpSqrt( src->m[1][0] * src->m[1][0] + src->m[1][1] * src->m[1][1] + src->m[1][2] * src->m[1][2] );
	const float scaleZ = RcpSqrt( src->m[2][0] * src->m[2][0] + src->m[2][1] * src->m[2][1] + src->m[2][2] * src->m[2][2] );
	const float m[9] =
	{
		src->m[0][0] * scaleX, src->m[0][1] * scaleX, src->m[0][2] * scaleX,
		src->m[1][0] * scaleY, src->m[1][1] * scaleY, src->m[1][2] * scaleY,
		src->m[2][0] * scaleZ, src->m[2][1] * scaleZ, src->m[2][2] * scaleZ
	};
	if ( m[0 * 3 + 0] + m[1 * 3 + 1] + m[2 * 3 + 2] > 0.0f )
	{
		float t = + m[0 * 3 + 0] + m[1 * 3 + 1] + m[2 * 3 + 2] + 1.0f;
		float s = RcpSqrt( t ) * 0.5f;
		result->w = s * t;
		result->z = ( m[0 * 3 + 1] - m[1 * 3 + 0] ) * s;
		result->y = ( m[2 * 3 + 0] - m[0 * 3 + 2] ) * s;
		result->x = ( m[1 * 3 + 2] - m[2 * 3 + 1] ) * s;
	}
	else if ( m[0 * 3 + 0] > m[1 * 3 + 1] && m[0 * 3 + 0] > m[2 * 3 + 2] )
	{
		float t = + m[0 * 3 + 0] - m[1 * 3 + 1] - m[2 * 3 + 2] + 1.0f;
		float s = RcpSqrt( t ) * 0.5f;
		result->x = s * t;
		result->y = ( m[0 * 3 + 1] + m[1 * 3 + 0] ) * s; 
		result->z = ( m[2 * 3 + 0] + m[0 * 3 + 2] ) * s;
		result->w = ( m[1 * 3 + 2] - m[2 * 3 + 1] ) * s;
	}
	else if ( m[1 * 3 + 1] > m[2 * 3 + 2] )
	{
		float t = - m[0 * 3 + 0] + m[1 * 3 + 1] - m[2 * 3 + 2] + 1.0f;
		float s = RcpSqrt( t ) * 0.5f;
		result->y = s * t;
		result->x = ( m[0 * 3 + 1] + m[1 * 3 + 0] ) * s;
		result->w = ( m[2 * 3 + 0] - m[0 * 3 + 2] ) * s;
		result->z = ( m[1 * 3 + 2] + m[2 * 3 + 1] ) * s;
	}
	else
	{
		float t = - m[0 * 3 + 0] - m[1 * 3 + 1] + m[2 * 3 + 2] + 1.0f;
		float s = RcpSqrt( t ) * 0.5f;
		result->z = s * t;
		result->w = ( m[0 * 3 + 1] - m[1 * 3 + 0] ) * s;
		result->x = ( m[2 * 3 + 0] + m[0 * 3 + 2] ) * s;
		result->y = ( m[1 * 3 + 2] + m[2 * 3 + 1] ) * s;
	}
}

// Get the scale from a combined translation(rotation(scale(object))) matrix.
static void Matrix4x4f_GetScale( Vector3f_t * result, const Matrix4x4f_t * src )
{
	assert( Matrix4x4f_IsAffine( src, 1e-4f ) );
	assert( Matrix4x4f_IsOrthogonal( src, 1e-4f ) );

	result->x = sqrtf( src->m[0][0] * src->m[0][0] + src->m[0][1] * src->m[0][1] + src->m[0][2] * src->m[0][2] );
	result->y = sqrtf( src->m[1][0] * src->m[1][0] + src->m[1][1] * src->m[1][1] + src->m[1][2] * src->m[1][2] );
	result->z = sqrtf( src->m[2][0] * src->m[2][0] + src->m[2][1] * src->m[2][1] + src->m[2][2] * src->m[2][2] );
}

// Transforms a 3D vector.
static void Matrix4x4f_TransformVector3f( Vector3f_t * result, const Matrix4x4f_t * m, const Vector3f_t * v )
{
	const float w = m->m[0][3] * v->x + m->m[1][3] * v->y + m->m[2][3] * v->z + m->m[3][3];
	const float rcpW = 1.0f / w;
	result->x = ( m->m[0][0] * v->x + m->m[1][0] * v->y + m->m[2][0] * v->z + m->m[3][0] ) * rcpW;
	result->y = ( m->m[0][1] * v->x + m->m[1][1] * v->y + m->m[2][1] * v->z + m->m[3][1] ) * rcpW;
	result->z = ( m->m[0][2] * v->x + m->m[1][2] * v->y + m->m[2][2] * v->z + m->m[3][2] ) * rcpW;
}

// Transforms a 4D vector.
static void Matrix4x4f_TransformVector4f( Vector4f_t * result, const Matrix4x4f_t * m, const Vector4f_t * v )
{
	result->x = m->m[0][0] * v->x + m->m[1][0] * v->y + m->m[2][0] * v->z + m->m[3][0];
	result->y = m->m[0][1] * v->x + m->m[1][1] * v->y + m->m[2][1] * v->z + m->m[3][1];
	result->z = m->m[0][2] * v->x + m->m[1][2] * v->y + m->m[2][2] * v->z + m->m[3][2];
	result->w = m->m[0][3] * v->x + m->m[1][3] * v->y + m->m[2][3] * v->z + m->m[3][3];
}

// Transforms the 'mins' and 'maxs' bounds with the given 'matrix'.
static void Matrix4x4f_TransformBounds( Vector3f_t * resultMins, Vector3f_t * resultMaxs, const Matrix4x4f_t * matrix, const Vector3f_t * mins, const Vector3f_t * maxs )
{
	assert( Matrix4x4f_IsAffine( matrix, 1e-4f ) );

	const Vector3f_t center = { ( mins->x + maxs->x ) * 0.5f, ( mins->y + maxs->y ) * 0.5f, ( mins->z + maxs->z ) * 0.5f };
	const Vector3f_t extents = { maxs->x - center.x, maxs->y - center.y, maxs->z - center.z };
	const Vector3f_t newCenter =
	{
		matrix->m[0][0] * center.x + matrix->m[1][0] * center.y + matrix->m[2][0] * center.z + matrix->m[3][0],
		matrix->m[0][1] * center.x + matrix->m[1][1] * center.y + matrix->m[2][1] * center.z + matrix->m[3][1],
		matrix->m[0][2] * center.x + matrix->m[1][2] * center.y + matrix->m[2][2] * center.z + matrix->m[3][2]
	};
	const Vector3f_t newExtents =
	{
		fabsf( extents.x * matrix->m[0][0] ) + fabsf( extents.y * matrix->m[1][0] ) + fabsf( extents.z * matrix->m[2][0] ),
		fabsf( extents.x * matrix->m[0][1] ) + fabsf( extents.y * matrix->m[1][1] ) + fabsf( extents.z * matrix->m[2][1] ),
		fabsf( extents.x * matrix->m[0][2] ) + fabsf( extents.y * matrix->m[1][2] ) + fabsf( extents.z * matrix->m[2][2] )
	};
	Vector3f_Sub( resultMins, &newCenter, &newExtents );
	Vector3f_Add( resultMaxs, &newCenter, &newExtents );
}

// Returns true if the 'mins' and 'maxs' bounds is completely off to one side of the projection matrix.
static bool Matrix4x4f_CullBounds( const Matrix4x4f_t * mvp, const Vector3f_t * mins, const Vector3f_t * maxs )
{
	if ( maxs->x <= mins->x && maxs->y <= mins->y && maxs->z <= mins->z )
	{
		return false;
	}

	Vector4f_t c[8];
	for ( int i = 0; i < 8; i++ )
	{
		const Vector4f_t corner =
		{
			( i & 1 ) ? maxs->x : mins->x,
			( i & 2 ) ? maxs->y : mins->y,
			( i & 4 ) ? maxs->z : mins->z,
			1.0f
		};
		Matrix4x4f_TransformVector4f( &c[i], mvp, &corner );
	}

	int i;
	for ( i = 0; i < 8; i++ )
	{
		if ( c[i].x > -c[i].w )
		{
			break;
		}
	}
	if ( i == 8 )
	{
		return true;
	}
	for ( i = 0; i < 8; i++ )
	{
		if ( c[i].x < c[i].w )
		{
			break;
		}
	}
	if ( i == 8 )
	{
		return true;
	}

	for ( i = 0; i < 8; i++ )
	{
		if ( c[i].y > -c[i].w )
		{
			break;
		}
	}
	if ( i == 8 )
	{
		return true;
	}
	for ( i = 0; i < 8; i++ )
	{
		if ( c[i].y < c[i].w )
		{
			break;
		}
	}
	if ( i == 8 )
	{
		return true;
	}
	for ( i = 0; i < 8; i++ )
	{
		if ( c[i].z > -c[i].w )
		{
			break;
		}
	}
	if ( i == 8 )
	{
		return true;
	}
	for ( i = 0; i < 8; i++ )
	{
		if ( c[i].z < c[i].w )
		{
			break;
		}
	}
	if ( i == 8 )
	{
		return true;
	}

	return false;
}

/*
================================================================================================================================
 
Rectangles.

ScreenRect_t
ClipRect_t

ScreenRect_t is specified in pixels with 0,0 at the left-bottom.
ClipRect_t is specified in clip space in the range [-1,1], with -1,-1 at the left-bottom.

static ClipRect_t ScreenRect_ToClipRect( const ScreenRect_t * screenRect, const int resolutionX, const int resolutionY );
static ScreenRect_t ClipRect_ToScreenRect( const ClipRect_t * clipRect, const int resolutionX, const int resolutionY );

================================================================================================================================
*/

typedef struct
{
	int x;
	int y;
	int width;
	int height;
} ScreenRect_t;

typedef struct
{
	float x;
	float y;
	float width;
	float height;
} ClipRect_t;

static ClipRect_t ScreenRect_ToClipRect( const ScreenRect_t * screenRect, const int resolutionX, const int resolutionY )
{
	ClipRect_t clipRect;
	clipRect.x = 2.0f * screenRect->x / resolutionX - 1.0f;
	clipRect.y = 2.0f * screenRect->y / resolutionY - 1.0f;
	clipRect.width = 2.0f * screenRect->width / resolutionX;
	clipRect.height = 2.0f * screenRect->height / resolutionY;
	return clipRect;
}

static ScreenRect_t ClipRect_ToScreenRect( const ClipRect_t * clipRect, const int resolutionX, const int resolutionY )
{
	ScreenRect_t screenRect;
	screenRect.x = (int)( ( clipRect->x * 0.5f + 0.5f ) * resolutionX + 0.5f );
	screenRect.y = (int)( ( clipRect->y * 0.5f + 0.5f ) * resolutionY + 0.5f );
	screenRect.width = (int)( clipRect->width * 0.5f * resolutionX + 0.5f );
	screenRect.height = (int)( clipRect->height * 0.5f * resolutionY + 0.5f );
	return screenRect;
}

/*
================================================================================================================================

OpenGL error checking.

================================================================================================================================
*/

#if defined( _DEBUG )
	#define GL( func )		func; FrameLog_Write( __FILE__, __LINE__, #func ); GlCheckErrors( #func );
#else
	#define GL( func )		func;
#endif

#if defined( _DEBUG )
	#define EGL( func )		FrameLog_Write( __FILE__, __LINE__, #func ); if ( func == EGL_FALSE ) { Error( #func " failed: %s", EglErrorString( eglGetError() ) ); }
#else
	#define EGL( func )		if ( func == EGL_FALSE ) { Error( #func " failed: %s", EglErrorString( eglGetError() ) ); }
#endif

#if defined( OS_ANDROID )
static const char * EglErrorString( const EGLint error )
{
	switch ( error )
	{
		case EGL_SUCCESS:				return "EGL_SUCCESS";
		case EGL_NOT_INITIALIZED:		return "EGL_NOT_INITIALIZED";
		case EGL_BAD_ACCESS:			return "EGL_BAD_ACCESS";
		case EGL_BAD_ALLOC:				return "EGL_BAD_ALLOC";
		case EGL_BAD_ATTRIBUTE:			return "EGL_BAD_ATTRIBUTE";
		case EGL_BAD_CONTEXT:			return "EGL_BAD_CONTEXT";
		case EGL_BAD_CONFIG:			return "EGL_BAD_CONFIG";
		case EGL_BAD_CURRENT_SURFACE:	return "EGL_BAD_CURRENT_SURFACE";
		case EGL_BAD_DISPLAY:			return "EGL_BAD_DISPLAY";
		case EGL_BAD_SURFACE:			return "EGL_BAD_SURFACE";
		case EGL_BAD_MATCH:				return "EGL_BAD_MATCH";
		case EGL_BAD_PARAMETER:			return "EGL_BAD_PARAMETER";
		case EGL_BAD_NATIVE_PIXMAP:		return "EGL_BAD_NATIVE_PIXMAP";
		case EGL_BAD_NATIVE_WINDOW:		return "EGL_BAD_NATIVE_WINDOW";
		case EGL_CONTEXT_LOST:			return "EGL_CONTEXT_LOST";
		default:						return "unknown";
	}
}
#endif

static const char * GlErrorString( GLenum error )
{
	switch ( error )
	{
		case GL_NO_ERROR:						return "GL_NO_ERROR";
		case GL_INVALID_ENUM:					return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:					return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:				return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION:	return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY:					return "GL_OUT_OF_MEMORY";
#if !defined( OS_APPLE_MACOS ) && !defined( OS_ANDROID )
		case GL_STACK_UNDERFLOW:				return "GL_STACK_UNDERFLOW";
		case GL_STACK_OVERFLOW:					return "GL_STACK_OVERFLOW";
#endif
		default: return "unknown";
	}
}

static const char * GlFramebufferStatusString( GLenum status )
{
	switch ( status )
	{
		case GL_FRAMEBUFFER_UNDEFINED:						return "GL_FRAMEBUFFER_UNDEFINED";
		case GL_FRAMEBUFFER_UNSUPPORTED:					return "GL_FRAMEBUFFER_UNSUPPORTED";
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:			return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:	return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:			return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
#if !defined( OS_ANDROID )
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:			return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:			return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:		return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
#endif
		default: return "unknown";
	}
}

static void GlCheckErrors( const char * function )
{
	for ( int i = 0; i < 10; i++ )
	{
		const GLenum error = glGetError();
		if ( error == GL_NO_ERROR )
		{
			break;
		}
		Error( "GL error: %s: %s", function, GlErrorString( error ) );
	}
}

/*
================================================================================================================================

OpenGL extensions.

================================================================================================================================
*/

typedef struct
{
	bool timer_query;						// GL_ARB_timer_query, GL_EXT_disjoint_timer_query
	bool texture_clamp_to_border;			// GL_EXT_texture_border_clamp, GL_OES_texture_border_clamp
	bool buffer_storage;					// GL_ARB_buffer_storage
	bool multi_sampled_storage;				// GL_ARB_texture_storage_multisample
	bool multi_view;						// GL_OVR_multiview, GL_OVR_multiview2
	bool multi_sampled_resolve;				// GL_EXT_multisampled_render_to_texture
	bool multi_view_multi_sampled_resolve;	// GL_OVR_multiview_multisampled_render_to_texture

	int texture_clamp_to_border_id;
} OpenGLExtensions_t;

OpenGLExtensions_t glExtensions;

/*
================================
Multi-view support
================================
*/

#if !defined( GL_OVR_multiview )
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_NUM_VIEWS_OVR			0x9630
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_BASE_VIEW_INDEX_OVR	0x9632
#define GL_MAX_VIEWS_OVR										0x9631

typedef void (* PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC) (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint baseViewIndex, GLsizei numViews);
#endif

/*
================================
Multi-sampling support
================================
*/

#if !defined( GL_EXT_framebuffer_multisample )
typedef void (* PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
#endif

#if !defined( GL_EXT_multisampled_render_to_texture )
typedef void (* PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);
#endif

#if !defined( GL_OVR_multiview_multisampled_render_to_texture )
typedef void (* PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC) (GLenum target, GLenum attachment, GLuint texture, GLint level, GLsizei samples, GLint baseViewIndex, GLsizei numViews);
#endif

/*
================================
Get proc address / extensions
================================
*/

#if defined( OS_WINDOWS )
PROC GetExtension( const char * functionName )
{
	return wglGetProcAddress( functionName );
}
#elif defined( OS_APPLE )
void ( *GetExtension( const char * functionName ) )()
{
	return NULL;
}
#elif defined( OS_LINUX )
void ( *GetExtension( const char * functionName ) )()
{
	return glXGetProcAddress( (const GLubyte *)functionName );
}
#elif defined( OS_ANDROID )
void ( *GetExtension( const char * functionName ) )()
{
	return eglGetProcAddress( functionName );
}
#endif

GLint glGetInteger( GLenum pname )
{
	GLint i;
	GL( glGetIntegerv( pname, &i ) );
	return i;
}

static bool GlCheckExtension( const char * extension )
{
#if defined( OS_WINDOWS ) || defined( OS_LINUX )
	PFNGLGETSTRINGIPROC glGetStringi = (PFNGLGETSTRINGIPROC) GetExtension( "glGetStringi" );
#endif
	GL( const GLint numExtensions = glGetInteger( GL_NUM_EXTENSIONS ) );
	for ( int i = 0; i < numExtensions; i++ )
	{
		GL( const GLubyte * string = glGetStringi( GL_EXTENSIONS, i ) );
		if ( strcmp( (const char *)string, extension ) == 0 )
		{
			return true;
		}
	}
	return false;
}

#if defined( OS_WINDOWS ) || defined( OS_LINUX )

PFNGLGENFRAMEBUFFERSPROC							glGenFramebuffers;
PFNGLDELETEFRAMEBUFFERSPROC							glDeleteFramebuffers;
PFNGLBINDFRAMEBUFFERPROC							glBindFramebuffer;
PFNGLBLITFRAMEBUFFERPROC							glBlitFramebuffer;
PFNGLGENRENDERBUFFERSPROC							glGenRenderbuffers;
PFNGLDELETERENDERBUFFERSPROC						glDeleteRenderbuffers;
PFNGLBINDRENDERBUFFERPROC							glBindRenderbuffer;
PFNGLISRENDERBUFFERPROC								glIsRenderbuffer;
PFNGLRENDERBUFFERSTORAGEPROC						glRenderbufferStorage;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC				glRenderbufferStorageMultisample;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC			glRenderbufferStorageMultisampleEXT;
PFNGLFRAMEBUFFERRENDERBUFFERPROC					glFramebufferRenderbuffer;
PFNGLFRAMEBUFFERTEXTURE2DPROC						glFramebufferTexture2D;
PFNGLFRAMEBUFFERTEXTURELAYERPROC					glFramebufferTextureLayer;
PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC			glFramebufferTexture2DMultisampleEXT;
PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC				glFramebufferTextureMultiviewOVR;
PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC	glFramebufferTextureMultisampleMultiviewOVR;
PFNGLCHECKFRAMEBUFFERSTATUSPROC						glCheckFramebufferStatus;

PFNGLGENBUFFERSPROC									glGenBuffers;
PFNGLDELETEBUFFERSPROC								glDeleteBuffers;
PFNGLBINDBUFFERPROC									glBindBuffer;
PFNGLBINDBUFFERBASEPROC								glBindBufferBase;
PFNGLBUFFERDATAPROC									glBufferData;
PFNGLBUFFERSTORAGEPROC								glBufferStorage;
PFNGLMAPBUFFERPROC									glMapBuffer;
PFNGLMAPBUFFERRANGEPROC								glMapBufferRange;
PFNGLUNMAPBUFFERPROC								glUnmapBuffer;

PFNGLGENVERTEXARRAYSPROC							glGenVertexArrays;
PFNGLDELETEVERTEXARRAYSPROC							glDeleteVertexArrays;
PFNGLBINDVERTEXARRAYPROC							glBindVertexArray;
PFNGLVERTEXATTRIBPOINTERPROC						glVertexAttribPointer;
PFNGLVERTEXATTRIBDIVISORPROC						glVertexAttribDivisor;
PFNGLDISABLEVERTEXATTRIBARRAYPROC					glDisableVertexAttribArray;
PFNGLENABLEVERTEXATTRIBARRAYPROC					glEnableVertexAttribArray;

#if defined( OS_WINDOWS )
PFNGLACTIVETEXTUREPROC								glActiveTexture;
PFNGLTEXIMAGE3DPROC									glTexImage3D;
PFNGLCOMPRESSEDTEXIMAGE2DPROC						glCompressedTexImage2D;
PFNGLCOMPRESSEDTEXIMAGE3DPROC						glCompressedTexImage3D;
PFNGLTEXSUBIMAGE3DPROC								glTexSubImage3D;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC					glCompressedTexSubImage2D;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC					glCompressedTexSubImage3D;
#endif
PFNGLTEXSTORAGE2DPROC								glTexStorage2D;
PFNGLTEXSTORAGE3DPROC								glTexStorage3D;
PFNGLTEXIMAGE2DMULTISAMPLEPROC						glTexImage2DMultisample;
PFNGLTEXIMAGE3DMULTISAMPLEPROC						glTexImage3DMultisample;
PFNGLTEXSTORAGE2DMULTISAMPLEPROC					glTexStorage2DMultisample;
PFNGLTEXSTORAGE3DMULTISAMPLEPROC					glTexStorage3DMultisample;
PFNGLGENERATEMIPMAPPROC								glGenerateMipmap;
PFNGLBINDIMAGETEXTUREPROC							glBindImageTexture;

PFNGLCREATEPROGRAMPROC								glCreateProgram;
PFNGLDELETEPROGRAMPROC								glDeleteProgram;
PFNGLCREATESHADERPROC								glCreateShader;
PFNGLDELETESHADERPROC								glDeleteShader;
PFNGLSHADERSOURCEPROC								glShaderSource;
PFNGLCOMPILESHADERPROC								glCompileShader;
PFNGLGETSHADERIVPROC								glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC							glGetShaderInfoLog;
PFNGLUSEPROGRAMPROC									glUseProgram;
PFNGLATTACHSHADERPROC								glAttachShader;
PFNGLLINKPROGRAMPROC								glLinkProgram;
PFNGLGETPROGRAMIVPROC								glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC							glGetProgramInfoLog;
PFNGLGETATTRIBLOCATIONPROC							glGetAttribLocation;
PFNGLBINDATTRIBLOCATIONPROC							glBindAttribLocation;
PFNGLGETUNIFORMLOCATIONPROC							glGetUniformLocation;
PFNGLGETUNIFORMBLOCKINDEXPROC						glGetUniformBlockIndex;
PFNGLGETPROGRAMRESOURCEINDEXPROC					glGetProgramResourceIndex;
PFNGLUNIFORMBLOCKBINDINGPROC						glUniformBlockBinding;
PFNGLSHADERSTORAGEBLOCKBINDINGPROC					glShaderStorageBlockBinding;
PFNGLPROGRAMUNIFORM1IPROC							glProgramUniform1i;
PFNGLUNIFORM1IPROC									glUniform1i;
PFNGLUNIFORM1IVPROC									glUniform1iv;
PFNGLUNIFORM2IVPROC									glUniform2iv;
PFNGLUNIFORM3IVPROC									glUniform3iv;
PFNGLUNIFORM4IVPROC									glUniform4iv;
PFNGLUNIFORM1FPROC									glUniform1f;
PFNGLUNIFORM1FVPROC									glUniform1fv;
PFNGLUNIFORM2FVPROC									glUniform2fv;
PFNGLUNIFORM3FVPROC									glUniform3fv;
PFNGLUNIFORM4FVPROC									glUniform4fv;
PFNGLUNIFORMMATRIX2FVPROC							glUniformMatrix2fv;
PFNGLUNIFORMMATRIX2X3FVPROC							glUniformMatrix2x3fv;
PFNGLUNIFORMMATRIX2X4FVPROC							glUniformMatrix2x4fv;
PFNGLUNIFORMMATRIX3X2FVPROC							glUniformMatrix3x2fv;
PFNGLUNIFORMMATRIX3FVPROC							glUniformMatrix3fv;
PFNGLUNIFORMMATRIX3X4FVPROC							glUniformMatrix3x4fv;
PFNGLUNIFORMMATRIX4X2FVPROC							glUniformMatrix4x2fv;
PFNGLUNIFORMMATRIX4X3FVPROC							glUniformMatrix4x3fv;
PFNGLUNIFORMMATRIX4FVPROC							glUniformMatrix4fv;

PFNGLDRAWELEMENTSINSTANCEDPROC						glDrawElementsInstanced;
PFNGLDISPATCHCOMPUTEPROC							glDispatchCompute;
PFNGLMEMORYBARRIERPROC								glMemoryBarrier;

PFNGLGENQUERIESPROC									glGenQueries;
PFNGLDELETEQUERIESPROC								glDeleteQueries;
PFNGLISQUERYPROC									glIsQuery;
PFNGLBEGINQUERYPROC									glBeginQuery;
PFNGLENDQUERYPROC									glEndQuery;
PFNGLQUERYCOUNTERPROC								glQueryCounter;
PFNGLGETQUERYIVPROC									glGetQueryiv;
PFNGLGETQUERYOBJECTIVPROC							glGetQueryObjectiv;
PFNGLGETQUERYOBJECTUIVPROC							glGetQueryObjectuiv;
PFNGLGETQUERYOBJECTI64VPROC							glGetQueryObjecti64v;
PFNGLGETQUERYOBJECTUI64VPROC						glGetQueryObjectui64v;

PFNGLFENCESYNCPROC									glFenceSync;
PFNGLCLIENTWAITSYNCPROC								glClientWaitSync;
PFNGLDELETESYNCPROC									glDeleteSync;
PFNGLISSYNCPROC										glIsSync;

PFNGLBLENDFUNCSEPARATEPROC							glBlendFuncSeparate;
PFNGLBLENDEQUATIONSEPARATEPROC						glBlendEquationSeparate;

#if defined( OS_WINDOWS )
PFNGLBLENDCOLORPROC									glBlendColor;
PFNWGLCHOOSEPIXELFORMATARBPROC						wglChoosePixelFormatARB;
PFNWGLCREATECONTEXTATTRIBSARBPROC					wglCreateContextAttribsARB;
PFNWGLSWAPINTERVALEXTPROC							wglSwapIntervalEXT;
PFNWGLDELAYBEFORESWAPNVPROC							wglDelayBeforeSwapNV;
#elif defined( OS_LINUX )
PFNGLXCREATECONTEXTATTRIBSARBPROC					glXCreateContextAttribsARB;
PFNGLXSWAPINTERVALEXTPROC							glXSwapIntervalEXT;
PFNGLXDELAYBEFORESWAPNVPROC							glXDelayBeforeSwapNV;
#endif

static void GlInitExtensions()
{
	glGenFramebuffers							= (PFNGLGENFRAMEBUFFERSPROC)							GetExtension( "glGenFramebuffers" );
	glDeleteFramebuffers						= (PFNGLDELETEFRAMEBUFFERSPROC)							GetExtension( "glDeleteFramebuffers" );
	glBindFramebuffer							= (PFNGLBINDFRAMEBUFFERPROC)							GetExtension( "glBindFramebuffer" );
	glBlitFramebuffer							= (PFNGLBLITFRAMEBUFFERPROC)							GetExtension( "glBlitFramebuffer" );
	glGenRenderbuffers							= (PFNGLGENRENDERBUFFERSPROC)							GetExtension( "glGenRenderbuffers" );
	glDeleteRenderbuffers						= (PFNGLDELETERENDERBUFFERSPROC)						GetExtension( "glDeleteRenderbuffers" );
	glBindRenderbuffer							= (PFNGLBINDRENDERBUFFERPROC)							GetExtension( "glBindRenderbuffer" );
	glIsRenderbuffer							= (PFNGLISRENDERBUFFERPROC)								GetExtension( "glIsRenderbuffer" );
	glRenderbufferStorage						= (PFNGLRENDERBUFFERSTORAGEPROC)						GetExtension( "glRenderbufferStorage" );
	glRenderbufferStorageMultisample			= (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)				GetExtension( "glRenderbufferStorageMultisample" );
	glRenderbufferStorageMultisampleEXT			= (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)			GetExtension( "glRenderbufferStorageMultisampleEXT" );
	glFramebufferRenderbuffer					= (PFNGLFRAMEBUFFERRENDERBUFFERPROC)					GetExtension( "glFramebufferRenderbuffer" );
	glFramebufferTexture2D						= (PFNGLFRAMEBUFFERTEXTURE2DPROC)						GetExtension( "glFramebufferTexture2D" );
	glFramebufferTextureLayer					= (PFNGLFRAMEBUFFERTEXTURELAYERPROC)					GetExtension( "glFramebufferTextureLayer" );
	glFramebufferTexture2DMultisampleEXT		= (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)			GetExtension( "glFramebufferTexture2DMultisampleEXT" );
	glFramebufferTextureMultiviewOVR			= (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)				GetExtension( "glFramebufferTextureMultiviewOVR" );
	glFramebufferTextureMultisampleMultiviewOVR = (PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)	GetExtension( "glFramebufferTextureMultisampleMultiviewOVR" );
	glCheckFramebufferStatus					= (PFNGLCHECKFRAMEBUFFERSTATUSPROC)						GetExtension( "glCheckFramebufferStatus" );

	glGenBuffers								= (PFNGLGENBUFFERSPROC)					GetExtension( "glGenBuffers" );
	glDeleteBuffers								= (PFNGLDELETEBUFFERSPROC)				GetExtension( "glDeleteBuffers" );
	glBindBuffer								= (PFNGLBINDBUFFERPROC)					GetExtension( "glBindBuffer" );
	glBindBufferBase							= (PFNGLBINDBUFFERBASEPROC)				GetExtension( "glBindBufferBase" );
	glBufferData								= (PFNGLBUFFERDATAPROC)					GetExtension( "glBufferData" );
	glBufferStorage								= (PFNGLBUFFERSTORAGEPROC)				GetExtension( "glBufferStorage" );
	glMapBuffer									= (PFNGLMAPBUFFERPROC)					GetExtension( "glMapBuffer" );
	glMapBufferRange							= (PFNGLMAPBUFFERRANGEPROC)				GetExtension( "glMapBufferRange" );
	glUnmapBuffer								= (PFNGLUNMAPBUFFERPROC)				GetExtension( "glUnmapBuffer" );

	glGenVertexArrays							= (PFNGLGENVERTEXARRAYSPROC)			GetExtension( "glGenVertexArrays" );
	glDeleteVertexArrays						= (PFNGLDELETEVERTEXARRAYSPROC)			GetExtension( "glDeleteVertexArrays" );
	glBindVertexArray							= (PFNGLBINDVERTEXARRAYPROC)			GetExtension( "glBindVertexArray" );
	glVertexAttribPointer						= (PFNGLVERTEXATTRIBPOINTERPROC)		GetExtension( "glVertexAttribPointer" );
	glVertexAttribDivisor						= (PFNGLVERTEXATTRIBDIVISORPROC)		GetExtension( "glVertexAttribDivisor" );
	glDisableVertexAttribArray					= (PFNGLDISABLEVERTEXATTRIBARRAYPROC)	GetExtension( "glDisableVertexAttribArray" );
	glEnableVertexAttribArray					= (PFNGLENABLEVERTEXATTRIBARRAYPROC)	GetExtension( "glEnableVertexAttribArray" );

#if defined( OS_WINDOWS )
	glActiveTexture								= (PFNGLACTIVETEXTUREPROC)				GetExtension( "glActiveTexture" );
	glTexImage3D								= (PFNGLTEXIMAGE3DPROC)					GetExtension( "glTexImage3D" );
	glCompressedTexImage2D						= (PFNGLCOMPRESSEDTEXIMAGE2DPROC)		GetExtension( "glCompressedTexImage2D ");
	glCompressedTexImage3D						= (PFNGLCOMPRESSEDTEXIMAGE3DPROC)		GetExtension( "glCompressedTexImage3D ");
	glTexSubImage3D								= (PFNGLTEXSUBIMAGE3DPROC)				GetExtension( "glTexSubImage3D" );
	glCompressedTexSubImage2D					= (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)	GetExtension( "glCompressedTexSubImage2D" );
	glCompressedTexSubImage3D					= (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)	GetExtension( "glCompressedTexSubImage3D" );
#endif
	glTexStorage2D								= (PFNGLTEXSTORAGE2DPROC)				GetExtension( "glTexStorage2D" );
	glTexStorage3D								= (PFNGLTEXSTORAGE3DPROC)				GetExtension( "glTexStorage3D" );
	glTexImage2DMultisample						= (PFNGLTEXIMAGE2DMULTISAMPLEPROC)		GetExtension( "glTexImage2DMultisample" );
	glTexImage3DMultisample						= (PFNGLTEXIMAGE3DMULTISAMPLEPROC)		GetExtension( "glTexImage3DMultisample" );
	glTexStorage2DMultisample					= (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)	GetExtension( "glTexStorage2DMultisample" );
	glTexStorage3DMultisample					= (PFNGLTEXSTORAGE3DMULTISAMPLEPROC)	GetExtension( "glTexStorage3DMultisample" );
	glGenerateMipmap							= (PFNGLGENERATEMIPMAPPROC)				GetExtension( "glGenerateMipmap" );
	glBindImageTexture							= (PFNGLBINDIMAGETEXTUREPROC)			GetExtension( "glBindImageTexture" );

	glCreateProgram								= (PFNGLCREATEPROGRAMPROC)				GetExtension( "glCreateProgram" );
	glDeleteProgram								= (PFNGLDELETEPROGRAMPROC)				GetExtension( "glDeleteProgram" );
	glCreateShader								= (PFNGLCREATESHADERPROC)				GetExtension( "glCreateShader" );
	glDeleteShader								= (PFNGLDELETESHADERPROC)				GetExtension( "glDeleteShader" );
	glShaderSource								= (PFNGLSHADERSOURCEPROC)				GetExtension( "glShaderSource" );
	glCompileShader								= (PFNGLCOMPILESHADERPROC)				GetExtension( "glCompileShader" );
	glGetShaderiv								= (PFNGLGETSHADERIVPROC)				GetExtension( "glGetShaderiv" );
	glGetShaderInfoLog							= (PFNGLGETSHADERINFOLOGPROC)			GetExtension( "glGetShaderInfoLog" );
	glUseProgram								= (PFNGLUSEPROGRAMPROC)					GetExtension( "glUseProgram" );
	glAttachShader								= (PFNGLATTACHSHADERPROC)				GetExtension( "glAttachShader" );
	glLinkProgram								= (PFNGLLINKPROGRAMPROC)				GetExtension( "glLinkProgram" );
	glGetProgramiv								= (PFNGLGETPROGRAMIVPROC)				GetExtension( "glGetProgramiv" );
	glGetProgramInfoLog							= (PFNGLGETPROGRAMINFOLOGPROC)			GetExtension( "glGetProgramInfoLog" );
	glGetAttribLocation							= (PFNGLGETATTRIBLOCATIONPROC)			GetExtension( "glGetAttribLocation" );
	glBindAttribLocation						= (PFNGLBINDATTRIBLOCATIONPROC)			GetExtension( "glBindAttribLocation" );
	glGetUniformLocation						= (PFNGLGETUNIFORMLOCATIONPROC)			GetExtension( "glGetUniformLocation" );
	glGetUniformBlockIndex						= (PFNGLGETUNIFORMBLOCKINDEXPROC)		GetExtension( "glGetUniformBlockIndex" );
	glProgramUniform1i							= (PFNGLPROGRAMUNIFORM1IPROC)			GetExtension( "glProgramUniform1i" );
	glUniform1i									= (PFNGLUNIFORM1IPROC)					GetExtension( "glUniform1i" );
	glUniform1iv								= (PFNGLUNIFORM1IVPROC)					GetExtension( "glUniform1iv" );
	glUniform2iv								= (PFNGLUNIFORM2IVPROC)					GetExtension( "glUniform2iv" );
	glUniform3iv								= (PFNGLUNIFORM3IVPROC)					GetExtension( "glUniform3iv" );
	glUniform4iv								= (PFNGLUNIFORM4IVPROC)					GetExtension( "glUniform4iv" );
	glUniform1f									= (PFNGLUNIFORM1FPROC)					GetExtension( "glUniform1f" );
	glUniform1fv								= (PFNGLUNIFORM1FVPROC)					GetExtension( "glUniform1fv" );
	glUniform2fv								= (PFNGLUNIFORM2FVPROC)					GetExtension( "glUniform2fv" );
	glUniform3fv								= (PFNGLUNIFORM3FVPROC)					GetExtension( "glUniform3fv" );
	glUniform4fv								= (PFNGLUNIFORM4FVPROC)					GetExtension( "glUniform4fv" );
	glUniformMatrix2fv							= (PFNGLUNIFORMMATRIX2FVPROC)			GetExtension( "glUniformMatrix3fv" );
	glUniformMatrix2x3fv						= (PFNGLUNIFORMMATRIX2X3FVPROC)			GetExtension( "glUniformMatrix2x3fv" );
	glUniformMatrix2x4fv						= (PFNGLUNIFORMMATRIX2X4FVPROC)			GetExtension( "glUniformMatrix2x4fv" );
	glUniformMatrix3x2fv						= (PFNGLUNIFORMMATRIX3X2FVPROC)			GetExtension( "glUniformMatrix3x2fv" );
	glUniformMatrix3fv							= (PFNGLUNIFORMMATRIX3FVPROC)			GetExtension( "glUniformMatrix3fv" );
	glUniformMatrix3x4fv						= (PFNGLUNIFORMMATRIX3X4FVPROC)			GetExtension( "glUniformMatrix3x4fv" );
	glUniformMatrix4x2fv						= (PFNGLUNIFORMMATRIX4X2FVPROC)			GetExtension( "glUniformMatrix4x2fv" );
	glUniformMatrix4x3fv						= (PFNGLUNIFORMMATRIX4X3FVPROC)			GetExtension( "glUniformMatrix4x3fv" );
	glUniformMatrix4fv							= (PFNGLUNIFORMMATRIX4FVPROC)			GetExtension( "glUniformMatrix4fv" );
	glGetProgramResourceIndex					= (PFNGLGETPROGRAMRESOURCEINDEXPROC)	GetExtension( "glGetProgramResourceIndex" );
	glUniformBlockBinding						= (PFNGLUNIFORMBLOCKBINDINGPROC)		GetExtension( "glUniformBlockBinding" );
	glShaderStorageBlockBinding					= (PFNGLSHADERSTORAGEBLOCKBINDINGPROC)	GetExtension( "glShaderStorageBlockBinding" );

	glDrawElementsInstanced						= (PFNGLDRAWELEMENTSINSTANCEDPROC)		GetExtension( "glDrawElementsInstanced" );
	glDispatchCompute							= (PFNGLDISPATCHCOMPUTEPROC)			GetExtension( "glDispatchCompute" );
	glMemoryBarrier								= (PFNGLMEMORYBARRIERPROC)				GetExtension( "glMemoryBarrier" );

	glGenQueries								= (PFNGLGENQUERIESPROC)					GetExtension( "glGenQueries" );
	glDeleteQueries								= (PFNGLDELETEQUERIESPROC)				GetExtension( "glDeleteQueries" );
	glIsQuery									= (PFNGLISQUERYPROC)					GetExtension( "glIsQuery" );
	glBeginQuery								= (PFNGLBEGINQUERYPROC)					GetExtension( "glBeginQuery" );
	glEndQuery									= (PFNGLENDQUERYPROC)					GetExtension( "glEndQuery" );
	glQueryCounter								= (PFNGLQUERYCOUNTERPROC)				GetExtension( "glQueryCounter" );
	glGetQueryiv								= (PFNGLGETQUERYIVPROC)					GetExtension( "glGetQueryiv" );
	glGetQueryObjectiv							= (PFNGLGETQUERYOBJECTIVPROC)			GetExtension( "glGetQueryObjectiv" );
	glGetQueryObjectuiv							= (PFNGLGETQUERYOBJECTUIVPROC)			GetExtension( "glGetQueryObjectuiv" );
	glGetQueryObjecti64v						= (PFNGLGETQUERYOBJECTI64VPROC)			GetExtension( "glGetQueryObjecti64v" );
	glGetQueryObjectui64v						= (PFNGLGETQUERYOBJECTUI64VPROC)		GetExtension( "glGetQueryObjectui64v" );

	glFenceSync									= (PFNGLFENCESYNCPROC)					GetExtension( "glFenceSync" );
	glClientWaitSync							= (PFNGLCLIENTWAITSYNCPROC)				GetExtension( "glClientWaitSync" );
	glDeleteSync								= (PFNGLDELETESYNCPROC)					GetExtension( "glDeleteSync" );
	glIsSync									= (PFNGLISSYNCPROC)						GetExtension( "glIsSync" );

	glBlendFuncSeparate							= (PFNGLBLENDFUNCSEPARATEPROC)			GetExtension( "glBlendFuncSeparate" );
	glBlendEquationSeparate						= (PFNGLBLENDEQUATIONSEPARATEPROC)		GetExtension( "glBlendEquationSeparate" );

#if defined( OS_WINDOWS )
	glBlendColor								= (PFNGLBLENDCOLORPROC)					GetExtension( "glBlendColor" );
	wglChoosePixelFormatARB						= (PFNWGLCHOOSEPIXELFORMATARBPROC)		GetExtension( "wglChoosePixelFormatARB" );
	wglCreateContextAttribsARB					= (PFNWGLCREATECONTEXTATTRIBSARBPROC)	GetExtension( "wglCreateContextAttribsARB" );
	wglSwapIntervalEXT							= (PFNWGLSWAPINTERVALEXTPROC)			GetExtension( "wglSwapIntervalEXT" );
	wglDelayBeforeSwapNV						= (PFNWGLDELAYBEFORESWAPNVPROC)			GetExtension( "wglDelayBeforeSwapNV" );
#elif defined( OS_LINUX )
	glXCreateContextAttribsARB					= (PFNGLXCREATECONTEXTATTRIBSARBPROC)	GetExtension( "glXCreateContextAttribsARB" );
	glXSwapIntervalEXT							= (PFNGLXSWAPINTERVALEXTPROC)			GetExtension( "glXSwapIntervalEXT" );
	glXDelayBeforeSwapNV						= (PFNGLXDELAYBEFORESWAPNVPROC)			GetExtension( "glXDelayBeforeSwapNV" );
#endif

	glExtensions.timer_query						= GlCheckExtension( "GL_EXT_timer_query" );
	glExtensions.texture_clamp_to_border			= true; // always available
	glExtensions.buffer_storage						= GlCheckExtension( "GL_EXT_buffer_storage" ) || ( OPENGL_VERSION_MAJOR * 10 + OPENGL_VERSION_MINOR >= 44 );
	glExtensions.multi_sampled_storage				= GlCheckExtension( "GL_ARB_texture_storage_multisample" ) || ( OPENGL_VERSION_MAJOR * 10 + OPENGL_VERSION_MINOR >= 43 );
	glExtensions.multi_view							= GlCheckExtension( "GL_OVR_multiview2" );
	glExtensions.multi_sampled_resolve				= GlCheckExtension( "GL_EXT_multisampled_render_to_texture" );
	glExtensions.multi_view_multi_sampled_resolve	= GlCheckExtension( "GL_OVR_multiview_multisampled_render_to_texture" );

	glExtensions.texture_clamp_to_border_id			= GL_CLAMP_TO_BORDER;
}

#elif defined( OS_APPLE_MACOS )

PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC				glFramebufferTextureMultiviewOVR;
PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC	glFramebufferTextureMultisampleMultiviewOVR;
PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC			glFramebufferTexture2DMultisampleEXT;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC			glRenderbufferStorageMultisampleEXT;

static void GlInitExtensions()
{
	glExtensions.timer_query						= GlCheckExtension( "GL_EXT_timer_query" );
	glExtensions.texture_clamp_to_border			= true; // always available
	glExtensions.buffer_storage						= GlCheckExtension( "GL_EXT_buffer_storage" ) || ( OPENGL_VERSION_MAJOR * 10 + OPENGL_VERSION_MINOR >= 44 );
	glExtensions.multi_sampled_storage				= GlCheckExtension( "GL_ARB_texture_storage_multisample" ) || ( OPENGL_VERSION_MAJOR * 10 + OPENGL_VERSION_MINOR >= 43 );
	glExtensions.multi_view							= GlCheckExtension( "GL_OVR_multiview2" );
	glExtensions.multi_sampled_resolve				= GlCheckExtension( "GL_EXT_multisampled_render_to_texture" );
	glExtensions.multi_view_multi_sampled_resolve	= GlCheckExtension( "GL_OVR_multiview_multisampled_render_to_texture" );

	glExtensions.texture_clamp_to_border_id			= GL_CLAMP_TO_BORDER;
}

#elif defined( OS_ANDROID )

// GL_EXT_disjoint_timer_query without _EXT
#if !defined( GL_TIMESTAMP )
#define GL_QUERY_COUNTER_BITS				GL_QUERY_COUNTER_BITS_EXT
#define GL_TIME_ELAPSED						GL_TIME_ELAPSED_EXT
#define GL_TIMESTAMP						GL_TIMESTAMP_EXT
#define GL_GPU_DISJOINT						GL_GPU_DISJOINT_EXT
#endif

// GL_EXT_buffer_storage without _EXT
#if !defined( GL_BUFFER_STORAGE_FLAGS )
#define GL_MAP_READ_BIT						0x0001		// GL_MAP_READ_BIT_EXT
#define GL_MAP_WRITE_BIT					0x0002		// GL_MAP_WRITE_BIT_EXT
#define GL_MAP_PERSISTENT_BIT				0x0040		// GL_MAP_PERSISTENT_BIT_EXT
#define GL_MAP_COHERENT_BIT					0x0080		// GL_MAP_COHERENT_BIT_EXT
#define GL_DYNAMIC_STORAGE_BIT				0x0100		// GL_DYNAMIC_STORAGE_BIT_EXT
#define GL_CLIENT_STORAGE_BIT				0x0200		// GL_CLIENT_STORAGE_BIT_EXT
#define GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT	0x00004000	// GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT_EXT
#define GL_BUFFER_IMMUTABLE_STORAGE			0x821F		// GL_BUFFER_IMMUTABLE_STORAGE_EXT
#define GL_BUFFER_STORAGE_FLAGS				0x8220		// GL_BUFFER_STORAGE_FLAGS_EXT
#endif

typedef void (GL_APIENTRY * PFNGLBUFFERSTORAGEEXTPROC) (GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);
typedef void (GL_APIENTRY * PFNGLTEXSTORAGE3DMULTISAMPLEPROC) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);

// EGL_KHR_fence_sync, GL_OES_EGL_sync, VG_KHR_EGL_sync
PFNEGLCREATESYNCKHRPROC								eglCreateSyncKHR;
PFNEGLDESTROYSYNCKHRPROC							eglDestroySyncKHR;
PFNEGLCLIENTWAITSYNCKHRPROC							eglClientWaitSyncKHR;
PFNEGLGETSYNCATTRIBKHRPROC							eglGetSyncAttribKHR;

// GL_EXT_disjoint_timer_query
PFNGLQUERYCOUNTEREXTPROC							glQueryCounter;
PFNGLGETQUERYOBJECTI64VEXTPROC						glGetQueryObjecti64v;
PFNGLGETQUERYOBJECTUI64VEXTPROC						glGetQueryObjectui64v;

// GL_EXT_buffer_storage
PFNGLBUFFERSTORAGEEXTPROC							glBufferStorage;

// GL_OVR_multiview
PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC				glFramebufferTextureMultiviewOVR;

// GL_EXT_multisampled_render_to_texture
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC			glRenderbufferStorageMultisampleEXT;
PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC			glFramebufferTexture2DMultisampleEXT;

// GL_OVR_multiview_multisampled_render_to_texture
PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC	glFramebufferTextureMultisampleMultiviewOVR;

PFNGLTEXSTORAGE3DMULTISAMPLEPROC					glTexStorage3DMultisample;

#if !defined( EGL_OPENGL_ES3_BIT )
#define EGL_OPENGL_ES3_BIT					0x0040
#endif

// GL_EXT_texture_cube_map_array
#if !defined( GL_TEXTURE_CUBE_MAP_ARRAY )
#define GL_TEXTURE_CUBE_MAP_ARRAY			0x9009
#endif

// GL_EXT_texture_filter_anisotropic
#if !defined( GL_TEXTURE_MAX_ANISOTROPY_EXT )
#define GL_TEXTURE_MAX_ANISOTROPY_EXT		0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT	0x84FF
#endif

// GL_EXT_texture_border_clamp or GL_OES_texture_border_clamp
#if !defined( GL_CLAMP_TO_BORDER )
#define GL_CLAMP_TO_BORDER					0x812D
#endif

// No 1D textures in OpenGL ES.
#if !defined( GL_TEXTURE_1D )
#define GL_TEXTURE_1D						0x0DE0
#endif

// No 1D texture arrays in OpenGL ES.
#if !defined( GL_TEXTURE_1D_ARRAY )
#define GL_TEXTURE_1D_ARRAY					0x8C18
#endif

// No multi-sampled texture arrays in OpenGL ES.
#if !defined( GL_TEXTURE_2D_MULTISAMPLE_ARRAY )
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY		0x9102
#endif

static void GlInitExtensions()
{
	eglCreateSyncKHR								= (PFNEGLCREATESYNCKHRPROC)			GetExtension( "eglCreateSyncKHR" );
	eglDestroySyncKHR								= (PFNEGLDESTROYSYNCKHRPROC)		GetExtension( "eglDestroySyncKHR" );
	eglClientWaitSyncKHR							= (PFNEGLCLIENTWAITSYNCKHRPROC)		GetExtension( "eglClientWaitSyncKHR" );
	eglGetSyncAttribKHR								= (PFNEGLGETSYNCATTRIBKHRPROC)		GetExtension( "eglGetSyncAttribKHR" );

	glQueryCounter									= (PFNGLQUERYCOUNTEREXTPROC)		GetExtension( "glQueryCounterEXT" );
	glGetQueryObjecti64v							= (PFNGLGETQUERYOBJECTI64VEXTPROC)	GetExtension( "glGetQueryObjecti64vEXT" );
	glGetQueryObjectui64v							= (PFNGLGETQUERYOBJECTUI64VEXTPROC)	GetExtension( "glGetQueryObjectui64vEXT" );

	glBufferStorage									= (PFNGLBUFFERSTORAGEEXTPROC)		GetExtension( "glBufferStorageEXT" );

	glRenderbufferStorageMultisampleEXT				= (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)			GetExtension( "glRenderbufferStorageMultisampleEXT" );
	glFramebufferTexture2DMultisampleEXT			= (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)			GetExtension( "glFramebufferTexture2DMultisampleEXT" );
	glFramebufferTextureMultiviewOVR				= (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)				GetExtension( "glFramebufferTextureMultiviewOVR" );
	glFramebufferTextureMultisampleMultiviewOVR		= (PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)	GetExtension( "glFramebufferTextureMultisampleMultiviewOVR" );

	glTexStorage3DMultisample						= (PFNGLTEXSTORAGE3DMULTISAMPLEPROC)					GetExtension( "glTexStorage3DMultisample" );
	
	glExtensions.timer_query						= GlCheckExtension( "GL_EXT_disjoint_timer_query" );
	glExtensions.texture_clamp_to_border			= GlCheckExtension( "GL_EXT_texture_border_clamp" ) || GlCheckExtension( "GL_OES_texture_border_clamp" );
	glExtensions.buffer_storage						= GlCheckExtension( "GL_EXT_buffer_storage" );
	glExtensions.multi_view							= GlCheckExtension( "GL_OVR_multiview2" );
	glExtensions.multi_sampled_resolve				= GlCheckExtension( "GL_EXT_multisampled_render_to_texture" );
	glExtensions.multi_view_multi_sampled_resolve	= GlCheckExtension( "GL_OVR_multiview_multisampled_render_to_texture" );

	glExtensions.texture_clamp_to_border_id			=	( GlCheckExtension( "GL_OES_texture_border_clamp" ) ? GL_CLAMP_TO_BORDER :
														( GlCheckExtension( "GL_EXT_texture_border_clamp" ) ? GL_CLAMP_TO_BORDER :
														( GL_CLAMP_TO_EDGE ) ) );
}

#endif

/*
================================
Compute support
================================
*/

#if defined( OS_APPLE_MACOS ) && ( OPENGL_VERSION_MAJOR * 10 + OPENGL_VERSION_MINOR < 43 )

	#define OPENGL_COMPUTE_ENABLED	0

	#define GL_SHADER_STORAGE_BUFFER			0x90D2
	#define GL_COMPUTE_SHADER					0x91B9
	#define GL_UNIFORM_BLOCK					0x92E2
	#define GL_SHADER_STORAGE_BLOCK				0x92E6

	#define GL_TEXTURE_FETCH_BARRIER_BIT		0x00000008
	#define GL_TEXTURE_UPDATE_BARRIER_BIT		0x00000100
	#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT	0x00000020
	#define GL_FRAMEBUFFER_BARRIER_BIT			0x00000400
	#define GL_ALL_BARRIER_BITS					0xFFFFFFFF

	static GLuint glGetProgramResourceIndex( GLuint program, GLenum programInterface, const GLchar *name ) { assert( false ); return 0; }
	static void glShaderStorageBlockBinding( GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding ) { assert( false ); }
	static void glBindImageTexture( GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format ) { assert( false ); }
	static void glDispatchCompute( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z ) { assert( false ); }

	static void glMemoryBarrier( GLbitfield barriers ) { assert( false ); }

	static void glTexStorage2DMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations ) { assert( false ); }
	static void glTexStorage3DMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations ) { assert( false ); }

#elif defined( OS_ANDROID ) && ( OPENGL_VERSION_MAJOR * 10 + OPENGL_VERSION_MINOR < 31 )

	#define OPENGL_COMPUTE_ENABLED	0

	#define GL_SHADER_STORAGE_BUFFER			0x90D2
	#define GL_COMPUTE_SHADER					0x91B9
	#define GL_UNIFORM_BLOCK					0x92E2
	#define GL_SHADER_STORAGE_BLOCK				0x92E6

	#define GL_READ_ONLY						0x88B8
	#define GL_WRITE_ONLY						0x88B9
	#define GL_READ_WRITE						0x88BA

	#define GL_TEXTURE_FETCH_BARRIER_BIT		0x00000008
	#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT	0x00000020
	#define GL_FRAMEBUFFER_BARRIER_BIT			0x00000400
	#define GL_ALL_BARRIER_BITS					0xFFFFFFFF

	static GLuint glGetProgramResourceIndex( GLuint program, GLenum programInterface, const GLchar *name ) { assert( false ); return 0; }
	static void glShaderStorageBlockBinding( GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding ) { assert( false ); }
	static void glBindImageTexture( GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format ) { assert( false ); }
	static void glDispatchCompute( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z ) { assert( false ); }

	static void glProgramUniform1i( GLuint program, GLint location, GLint v0 ) { assert( false ); }
	static void glMemoryBarrier( GLbitfield barriers ) { assert( false ); }

#else

	#define OPENGL_COMPUTE_ENABLED	1

#endif

#if !defined( GL_SR8_EXT )
#define GL_SR8_EXT							0x8FBD
#endif

#if !defined( GL_SRG8_EXT )
#define GL_SRG8_EXT							0x8FBE
#endif

/*
================================================================================================================================

Driver Instance.

DriverInstance_t

static bool DriverInstance_Create( DriverInstance_t * intance );
static void DriverInstance_Destroy( DriverInstance_t * instance );

================================================================================================================================
*/

typedef struct
{
	int dummy;
} DriverInstance_t;

static bool DriverInstance_Create( DriverInstance_t * instance )
{
	memset( instance, 0, sizeof( DriverInstance_t ) );
	return true;
}

static void DriverInstance_Destroy( DriverInstance_t * instance )
{
	memset( instance, 0, sizeof( DriverInstance_t ) );
}

/*
================================================================================================================================

GPU device.

GpuQueueProperty_t
GpuQueuePriority_t
GpuQueueInfo_t
GpuDevice_t

static bool GpuDevice_Create( GpuDevice_t * device, DriverInstance_t * instance,
							const GpuQueueInfo_t * queueInfo, const VkSurfaceKHR presentSurface );
static void GpuDevice_Destroy( GpuDevice_t * device );

================================================================================================================================
*/

typedef enum
{
	GPU_QUEUE_PROPERTY_GRAPHICS		= BIT( 0 ),
	GPU_QUEUE_PROPERTY_COMPUTE		= BIT( 1 ),
	GPU_QUEUE_PROPERTY_TRANSFER		= BIT( 2 )
} GpuQueueProperty_t;

typedef enum
{
	GPU_QUEUE_PRIORITY_LOW,
	GPU_QUEUE_PRIORITY_MEDIUM,
	GPU_QUEUE_PRIORITY_HIGH
} GpuQueuePriority_t;

#define MAX_QUEUES	16

typedef struct
{
	int					queueCount;						// number of queues
	GpuQueueProperty_t	queueProperties;				// desired queue family properties
	GpuQueuePriority_t	queuePriorities[MAX_QUEUES];	// individual queue priorities
} GpuQueueInfo_t;

typedef struct
{
	DriverInstance_t *	instance;
	GpuQueueInfo_t		queueInfo;
} GpuDevice_t;

static bool GpuDevice_Create( GpuDevice_t * device, DriverInstance_t * instance,
								const GpuQueueInfo_t * queueInfo )
{
	/*
		Use an extensions to select the appropriate device:
		https://www.opengl.org/registry/specs/NV/gpu_affinity.txt
		https://www.opengl.org/registry/specs/AMD/wgl_gpu_association.txt
		https://www.opengl.org/registry/specs/AMD/glx_gpu_association.txt

		On Linux configure each GPU to use a separate X screen and then select
		the X screen to render to.
	*/

	memset( device, 0, sizeof( GpuDevice_t ) );

	device->instance = instance;
	device->queueInfo = *queueInfo;

	return true;
}

static void GpuDevice_Destroy( GpuDevice_t * device )
{
	memset( device, 0, sizeof( GpuDevice_t ) );
}

/*
================================================================================================================================

GPU context.

A context encapsulates a queue that is used to submit command buffers.
A context can only be used by a single thread.
For optimal performance a context should only be created at load time, not at runtime.

GpuContext_t
GpuSurfaceColorFormat_t
GpuSurfaceDepthFormat_t
GpuSampleCount_t

static bool GpuContext_CreateShared( GpuContext_t * context, const GpuContext_t * other, const int queueIndex );
static void GpuContext_Destroy( GpuContext_t * context );
static void GpuContext_WaitIdle( GpuContext_t * context );
static void GpuContext_SetCurrent( GpuContext_t * context );
static void GpuContext_UnsetCurrent( GpuContext_t * context );
static bool GpuContext_CheckCurrent( GpuContext_t * context );

static bool GpuContext_CreateForSurface( GpuContext_t * context, const GpuDevice_t * device, const int queueIndex,
										const GpuSurfaceColorFormat_t colorFormat,
										const GpuSurfaceDepthFormat_t depthFormat,
										const GpuSampleCount_t sampleCount,
										... );

================================================================================================================================
*/

typedef enum
{
	GPU_SURFACE_COLOR_FORMAT_R5G6B5,
	GPU_SURFACE_COLOR_FORMAT_B5G6R5,
	GPU_SURFACE_COLOR_FORMAT_R8G8B8A8,
	GPU_SURFACE_COLOR_FORMAT_B8G8R8A8,
	GPU_SURFACE_COLOR_FORMAT_MAX
} GpuSurfaceColorFormat_t;

typedef enum
{
	GPU_SURFACE_DEPTH_FORMAT_NONE,
	GPU_SURFACE_DEPTH_FORMAT_D16,
	GPU_SURFACE_DEPTH_FORMAT_D24,
	GPU_SURFACE_DEPTH_FORMAT_MAX
} GpuSurfaceDepthFormat_t;

typedef enum
{
	GPU_SAMPLE_COUNT_1		= 1,
	GPU_SAMPLE_COUNT_2		= 2,
	GPU_SAMPLE_COUNT_4		= 4,
	GPU_SAMPLE_COUNT_8		= 8,
	GPU_SAMPLE_COUNT_16		= 16,
	GPU_SAMPLE_COUNT_32		= 32,
	GPU_SAMPLE_COUNT_64		= 64,
} GpuSampleCount_t;

typedef struct
{
#if defined( OS_WINDOWS )
	HDC						hDC;
	HGLRC					hGLRC;
#elif defined( OS_APPLE_MACOS )
	NSOpenGLContext *		nsContext;
	CGLContextObj			cglContext;
#elif defined( OS_LINUX_XLIB ) || defined( OS_LINUX_XCB_GLX )
	Display *				xDisplay;
	uint32_t				visualid;
	GLXFBConfig				glxFBConfig;
	GLXDrawable				glxDrawable;
	GLXContext				glxContext;
#elif defined( OS_LINUX_XCB )
	xcb_connection_t *		connection;
	uint32_t				screen_number;
	xcb_glx_fbconfig_t		fbconfigid;
	xcb_visualid_t			visualid;
	xcb_glx_drawable_t		glxDrawable;
	xcb_glx_context_t		glxContext;
	xcb_glx_context_tag_t	glxContextTag;
#elif defined( OS_ANDROID )
	EGLDisplay				display;
	EGLConfig				config;
	EGLSurface				tinySurface;
	EGLSurface				mainSurface;
	EGLContext				context;
#endif
} GpuContext_t;

typedef struct
{
	unsigned char	redBits;
	unsigned char	greenBits;
	unsigned char	blueBits;
	unsigned char	alphaBits;
	unsigned char	colorBits;
	unsigned char	depthBits;
} GpuSurfaceBits_t;

GpuSurfaceBits_t GpuContext_BitsForSurfaceFormat( const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat )
{
	GpuSurfaceBits_t bits;
	bits.redBits =		( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 ) ? 8 :
						( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 ) ? 8 :
						( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_R5G6B5 ) ? 5 :
						( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_B5G6R5 ) ? 5 :
						8 ) ) ) );
	bits.greenBits =	( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 ) ? 8 :
						( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 ) ? 8 :
						( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_R5G6B5 ) ? 6 :
						( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_B5G6R5 ) ? 6 :
						8 ) ) ) );
	bits.blueBits =		( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 ) ? 8 :
						( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 ) ? 8 :
						( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_R5G6B5 ) ? 5 :
						( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_B5G6R5 ) ? 5 :
						8 ) ) ) );
	bits.alphaBits =	( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 ) ? 8 :
						( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 ) ? 8 :
						( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_R5G6B5 ) ? 0 :
						( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_B5G6R5 ) ? 0 :
						8 ) ) ) );
	bits.colorBits =	bits.redBits + bits.greenBits + bits.blueBits + bits.alphaBits;
	bits.depthBits =	( ( depthFormat == GPU_SURFACE_DEPTH_FORMAT_D16 ) ? 16 :
						( ( depthFormat == GPU_SURFACE_DEPTH_FORMAT_D24 ) ? 24 :
						0 ) );
	return bits;
}

GLenum GpuContext_InternalSurfaceColorFormat( const GpuSurfaceColorFormat_t colorFormat )
{
	return	( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_R8G8B8A8 ) ? GL_RGBA8 :
			( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_B8G8R8A8 ) ? GL_RGBA8 :
			( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_R5G6B5 ) ? GL_RGB565 :
			( ( colorFormat == GPU_SURFACE_COLOR_FORMAT_B5G6R5 ) ? GL_RGB565 :
			GL_RGBA8 ) ) ) );
}

GLenum GpuContext_InternalSurfaceDepthFormat( const GpuSurfaceDepthFormat_t depthFormat )
{
	return	( ( depthFormat == GPU_SURFACE_DEPTH_FORMAT_D16 ) ? GL_DEPTH_COMPONENT16 :
			( ( depthFormat == GPU_SURFACE_DEPTH_FORMAT_D24 ) ? GL_DEPTH_COMPONENT24 :
			GL_DEPTH_COMPONENT24 ) );
}

#if defined( OS_WINDOWS )

static bool GpuContext_CreateForSurface( GpuContext_t * context, const GpuDevice_t * device, const int queueIndex,
										const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
										const GpuSampleCount_t sampleCount, HINSTANCE hInstance, HDC hDC )
{
	UNUSED_PARM( device );
	UNUSED_PARM( queueIndex );

	const GpuSurfaceBits_t bits = GpuContext_BitsForSurfaceFormat( colorFormat, depthFormat );

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof( PIXELFORMATDESCRIPTOR ),
		1,						// version
		PFD_DRAW_TO_WINDOW |	// must support windowed
		PFD_SUPPORT_OPENGL |	// must support OpenGL
		PFD_DOUBLEBUFFER,		// must support double buffering
		PFD_TYPE_RGBA,			// iPixelType
		bits.colorBits,			// cColorBits
		0, 0,					// cRedBits, cRedShift
		0, 0,					// cGreenBits, cGreenShift
		0, 0,					// cBlueBits, cBlueShift
		0, 0,					// cAlphaBits, cAlphaShift
		0,						// cAccumBits
		0,						// cAccumRedBits
		0,						// cAccumGreenBits
		0,						// cAccumBlueBits
		0,						// cAccumAlphaBits
		bits.depthBits,			// cDepthBits
		0,						// cStencilBits
		0,						// cAuxBuffers
		PFD_MAIN_PLANE,			// iLayerType
		0,						// bReserved
		0,						// dwLayerMask
		0,						// dwVisibleMask
		0						// dwDamageMask
	};

	HWND localWnd = NULL;
	HDC localDC = hDC;

	if ( sampleCount > GPU_SAMPLE_COUNT_1 )
	{
		// A valid OpenGL context is needed to get OpenGL extensions including wglChoosePixelFormatARB
		// and wglCreateContextAttribsARB. A device context with a valid pixel format is needed to create
		// an OpenGL context. However, once a pixel format is set on a device context it is final.
		// Therefore a pixel format is set on the device context of a temporary window to create a context
		// to get the extensions for multi-sampling.
		localWnd = CreateWindow( APPLICATION_NAME, "temp", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL );
		localDC = GetDC( localWnd );
	}

	int pixelFormat = ChoosePixelFormat( localDC, &pfd );
	if ( pixelFormat == 0 )
	{
		Error( "Failed to find a suitable pixel format." );
		return false;
	}

	if ( !SetPixelFormat( localDC, pixelFormat, &pfd ) )
	{
		Error( "Failed to set the pixel format." );
		return false;
	}

	// Now that the pixel format is set, create a temporary context to get the extensions.
	{
		HGLRC hGLRC = wglCreateContext( localDC );
		wglMakeCurrent( localDC, hGLRC );

		GlInitExtensions();

		wglMakeCurrent( NULL, NULL );
		wglDeleteContext( hGLRC );
	}

	if ( sampleCount > GPU_SAMPLE_COUNT_1 )
	{
		// Release the device context and destroy the window that were created to get extensions.
		ReleaseDC( localWnd, localDC );
		DestroyWindow( localWnd );

		int pixelFormatAttribs[] =
		{
			WGL_DRAW_TO_WINDOW_ARB,				GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB,				GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB,				GL_TRUE,
			WGL_PIXEL_TYPE_ARB,					WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB,					bits.colorBits,
			WGL_DEPTH_BITS_ARB,					bits.depthBits,
			WGL_SAMPLE_BUFFERS_ARB,				1,
			WGL_SAMPLES_ARB,					sampleCount,
			0
		};

		unsigned int numPixelFormats = 0;

		if ( !wglChoosePixelFormatARB( hDC, pixelFormatAttribs, NULL, 1, &pixelFormat, &numPixelFormats ) || numPixelFormats == 0 )
		{
			Error( "Failed to find MSAA pixel format." );
			return false;
		}

		memset( &pfd, 0, sizeof( pfd ) );

		if ( !DescribePixelFormat( hDC, pixelFormat, sizeof( PIXELFORMATDESCRIPTOR ), &pfd ) )
		{
			Error( "Failed to describe the pixel format." );
			return false;
		}

		if ( !SetPixelFormat( hDC, pixelFormat, &pfd ) )
		{
			Error( "Failed to set the pixel format." );
			return false;
		}
	}

	int contextAttribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB,	OPENGL_VERSION_MAJOR,
		WGL_CONTEXT_MINOR_VERSION_ARB,	OPENGL_VERSION_MINOR,
		WGL_CONTEXT_PROFILE_MASK_ARB,	WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		WGL_CONTEXT_FLAGS_ARB,			WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

	context->hDC = hDC;
	context->hGLRC = wglCreateContextAttribsARB( hDC, NULL, contextAttribs );
	if ( !context->hGLRC )
	{
		Error( "Failed to create GL context." );
		return false;
	}

	return true;
}

#elif defined( OS_APPLE_MACOS )

static bool GpuContext_CreateForSurface( GpuContext_t * context, const GpuDevice_t * device, const int queueIndex,
										const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
										const GpuSampleCount_t sampleCount, CGDirectDisplayID display )
{
	UNUSED_PARM( device );
	UNUSED_PARM( queueIndex );

	const GpuSurfaceBits_t bits = GpuContext_BitsForSurfaceFormat( colorFormat, depthFormat );

	NSOpenGLPixelFormatAttribute pixelFormatAttributes[] =
	{
		NSOpenGLPFAMinimumPolicy,	1,
		NSOpenGLPFAScreenMask,		CGDisplayIDToOpenGLDisplayMask( display ),
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAOpenGLProfile,	NSOpenGLProfileVersion3_2Core,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize,		bits.colorBits,
		NSOpenGLPFADepthSize,		bits.depthBits,
		NSOpenGLPFASampleBuffers,	( sampleCount > GPU_SAMPLE_COUNT_1 ),
		NSOpenGLPFASamples,			sampleCount,
		0
	};

	NSOpenGLPixelFormat * pixelFormat = [[[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes] autorelease];
	if ( pixelFormat == nil )
	{
		return false;
	}
	context->nsContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
	if ( context->nsContext == nil )
	{
		return false;
	}

	context->cglContext = [context->nsContext CGLContextObj];

	GlInitExtensions();

	return true;
}

#elif defined( OS_LINUX_XLIB ) || defined( OS_LINUX_XCB_GLX )

static int glxGetFBConfigAttrib2( Display * dpy, GLXFBConfig config, int attribute )
{
	int value;
	glXGetFBConfigAttrib( dpy, config, attribute, &value );
	return value;
}

static bool GpuContext_CreateForSurface( GpuContext_t * context, const GpuDevice_t * device, const int queueIndex,
										const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
										const GpuSampleCount_t sampleCount, Display * xDisplay, int xScreen )
{
	UNUSED_PARM( device );
	UNUSED_PARM( queueIndex );

	GlInitExtensions();

	int glxErrorBase;
	int glxEventBase;
	if ( !glXQueryExtension( xDisplay, &glxErrorBase, &glxEventBase ) )
	{
		Error( "X display does not support the GLX extension." );
		return false;
	}

	int glxVersionMajor;
	int glxVersionMinor;
	if ( !glXQueryVersion( xDisplay, &glxVersionMajor, &glxVersionMinor ) )
	{
		Error( "Unable to retrieve GLX version." );
		return false;
	}

	int fbConfigCount = 0;
	GLXFBConfig * fbConfigs = glXGetFBConfigs( xDisplay, xScreen, &fbConfigCount );
	if ( fbConfigCount == 0 )
	{
		Error( "No valid framebuffer configurations found." );
		return false;
	}

	const GpuSurfaceBits_t bits = GpuContext_BitsForSurfaceFormat( colorFormat, depthFormat );

	bool foundFbConfig = false;
	for ( int i = 0; i < fbConfigCount; i++ )
	{
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_FBCONFIG_ID ) == 0 ) { continue; }
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_VISUAL_ID ) == 0 ) { continue; }
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_DOUBLEBUFFER ) == 0 ) { continue; }
		if ( ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_RENDER_TYPE ) & GLX_RGBA_BIT ) == 0 ) { continue; }
		if ( ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_DRAWABLE_TYPE ) & GLX_WINDOW_BIT ) == 0 ) { continue; }
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_RED_SIZE )   != bits.redBits ) { continue; }
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_GREEN_SIZE ) != bits.greenBits ) { continue; }
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_BLUE_SIZE )  != bits.blueBits ) { continue; }
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_ALPHA_SIZE ) != bits.alphaBits ) { continue; }
		if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_DEPTH_SIZE ) != bits.depthBits ) { continue; }
		if ( sampleCount > GPU_SAMPLE_COUNT_1 )
		{
			if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_SAMPLE_BUFFERS ) != 1 ) { continue; }
			if ( glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_SAMPLES ) != sampleCount ) { continue; }
		}

		context->visualid = glxGetFBConfigAttrib2( xDisplay, fbConfigs[i], GLX_VISUAL_ID );
		context->glxFBConfig = fbConfigs[i];
		foundFbConfig = true;
		break;
	}

	XFree( fbConfigs );

	if ( !foundFbConfig )
	{
		Error( "Failed to to find desired framebuffer configuration." );
		return false;
	}

	context->xDisplay = xDisplay;

	int attribs[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB,	OPENGL_VERSION_MAJOR,
		GLX_CONTEXT_MINOR_VERSION_ARB,	OPENGL_VERSION_MINOR,
		GLX_CONTEXT_PROFILE_MASK_ARB,	GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		GLX_CONTEXT_FLAGS_ARB,			GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

	context->glxContext = glXCreateContextAttribsARB( xDisplay,					// Display *	dpy
														context->glxFBConfig,	// GLXFBConfig	config
														NULL,					// GLXContext	share_context
														True,					// Bool			direct
														attribs );				// const int *	attrib_list

	if ( context->glxContext == NULL )
	{
		Error( "Unable to create GLX context." );
		return false;
	}

	if ( !glXIsDirect( xDisplay, context->glxContext ) )
	{
		Error( "Unable to create direct rendering context." );
		return false;
	}

	return true;
}

#elif defined( OS_LINUX_XCB )

static uint32_t xcb_glx_get_property( const uint32_t * properties, const uint32_t numProperties, uint32_t propertyName )
{
	for ( int i = 0; i < numProperties; i++ )
	{
		if ( properties[i * 2 + 0] == propertyName )
		{
			return properties[i * 2 + 1];
		}
	}
	return 0;
}

static bool GpuContext_CreateForSurface( GpuContext_t * context, const GpuDevice_t * device, const int queueIndex,
										const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
										const GpuSampleCount_t sampleCount, xcb_connection_t * connection, int screen_number )
{
	UNUSED_PARM( device );
	UNUSED_PARM( queueIndex );

	GlInitExtensions();

	xcb_glx_query_version_cookie_t glx_query_version_cookie = xcb_glx_query_version( connection, OPENGL_VERSION_MAJOR, OPENGL_VERSION_MINOR );
	xcb_glx_query_version_reply_t * glx_query_version_reply = xcb_glx_query_version_reply( connection, glx_query_version_cookie, NULL );
	if ( glx_query_version_reply == NULL )
	{
		Error( "Unable to retrieve GLX version." );
		return false;
	}
	free( glx_query_version_reply );

	xcb_glx_get_fb_configs_cookie_t get_fb_configs_cookie = xcb_glx_get_fb_configs( connection, screen_number );
	xcb_glx_get_fb_configs_reply_t * get_fb_configs_reply = xcb_glx_get_fb_configs_reply( connection, get_fb_configs_cookie, NULL );

	if ( get_fb_configs_reply == NULL || get_fb_configs_reply->num_FB_configs == 0 )
	{
		Error( "No valid framebuffer configurations found." );
		return false;
	}

	const GpuSurfaceBits_t bits = GpuContext_BitsForSurfaceFormat( colorFormat, depthFormat );

	const uint32_t * fb_configs_properties = xcb_glx_get_fb_configs_property_list( get_fb_configs_reply );
	const uint32_t fb_configs_num_properties = get_fb_configs_reply->num_properties;

	bool foundFbConfig = false;
	for ( int i = 0; i < get_fb_configs_reply->num_FB_configs; i++ )
	{
		const uint32_t * fb_config = fb_configs_properties + i * fb_configs_num_properties * 2;

		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_FBCONFIG_ID ) == 0 ) { continue; }
		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_VISUAL_ID ) == 0 ) { continue; }
		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_DOUBLEBUFFER ) == 0 ) { continue; }
		if ( ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_RENDER_TYPE ) & GLX_RGBA_BIT ) == 0 ) { continue; }
		if ( ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_DRAWABLE_TYPE ) & GLX_WINDOW_BIT ) == 0 ) { continue; }
		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_RED_SIZE )   != bits.redBits ) { continue; }
		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_GREEN_SIZE ) != bits.greenBits ) { continue; }
		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_BLUE_SIZE )  != bits.blueBits ) { continue; }
		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_ALPHA_SIZE ) != bits.alphaBits ) { continue; }
		if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_DEPTH_SIZE ) != bits.depthBits ) { continue; }
		if ( sampleCount > GPU_SAMPLE_COUNT_1 )
		{
			if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_SAMPLE_BUFFERS ) != 1 ) { continue; }
			if ( xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_SAMPLES ) != sampleCount ) { continue; }
		}

		context->fbconfigid = xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_FBCONFIG_ID );
		context->visualid = xcb_glx_get_property( fb_config, fb_configs_num_properties, GLX_VISUAL_ID );
		foundFbConfig = true;
		break;
	}

	free( get_fb_configs_reply );

	if ( !foundFbConfig )
	{
		Error( "Failed to to find desired framebuffer configuration." );
		return false;
	}

	context->connection = connection;
	context->screen_number = screen_number;

	// Create the context.
	uint32_t attribs[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB,	OPENGL_VERSION_MAJOR,
		GLX_CONTEXT_MINOR_VERSION_ARB,	OPENGL_VERSION_MINOR,
		GLX_CONTEXT_PROFILE_MASK_ARB,	GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		GLX_CONTEXT_FLAGS_ARB,			GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

	context->glxContext = xcb_generate_id( connection );
	xcb_glx_create_context_attribs_arb( connection,				// xcb_connection_t *	connection
										context->glxContext,	// xcb_glx_context_t	context
										context->fbconfigid,	// xcb_glx_fbconfig_t	fbconfig
										screen_number,			// uint32_t				screen
										0,						// xcb_glx_context_t	share_list
										1,						// uint8_t				is_direct
										4,						// uint32_t				num_attribs
										attribs );				// const uint32_t *		attribs

	// Make sure the context is direct.
	xcb_generic_error_t * error;
	xcb_glx_is_direct_cookie_t glx_is_direct_cookie = xcb_glx_is_direct_unchecked( connection, context->glxContext );
	xcb_glx_is_direct_reply_t * glx_is_direct_reply = xcb_glx_is_direct_reply( connection, glx_is_direct_cookie, &error );
	const bool is_direct = ( glx_is_direct_reply != NULL && glx_is_direct_reply->is_direct );
	free( glx_is_direct_reply );

	if ( !is_direct )
	{
		Error( "Unable to create direct rendering context." );
		return false;
	}

	return true;
}

#elif defined( OS_ANDROID )

static bool GpuContext_CreateForSurface( GpuContext_t * context, const GpuDevice_t * device, const int queueIndex,
										const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
										const GpuSampleCount_t sampleCount, EGLDisplay display )
{
	context->display = display;

	// Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
	// flags in eglChooseConfig when the user has selected the "force 4x MSAA" option in
	// settings, and that is completely wasted on the time warped frontbuffer.
	const int MAX_CONFIGS = 1024;
	EGLConfig configs[MAX_CONFIGS];
	EGLint numConfigs = 0;
	EGL( eglGetConfigs( display, configs, MAX_CONFIGS, &numConfigs ) );

	const GpuSurfaceBits_t bits = GpuContext_BitsForSurfaceFormat( colorFormat, depthFormat );

	const EGLint configAttribs[] =
	{
		EGL_RED_SIZE,		bits.greenBits,
		EGL_GREEN_SIZE,		bits.redBits,
		EGL_BLUE_SIZE,		bits.blueBits,
		EGL_ALPHA_SIZE,		bits.alphaBits,
		EGL_DEPTH_SIZE,		bits.depthBits,
		//EGL_STENCIL_SIZE,	0,
		EGL_SAMPLE_BUFFERS,	( sampleCount > GPU_SAMPLE_COUNT_1 ),
		EGL_SAMPLES,		sampleCount,
		EGL_NONE
	};

	context->config = 0;
	for ( int i = 0; i < numConfigs; i++ )
	{
		EGLint value = 0;

		eglGetConfigAttrib( display, configs[i], EGL_RENDERABLE_TYPE, &value );
		if ( ( value & EGL_OPENGL_ES3_BIT ) != EGL_OPENGL_ES3_BIT )
		{
			continue;
		}

		// Without EGL_KHR_surfaceless_context, the config needs to support both pbuffers and window surfaces.
		eglGetConfigAttrib( display, configs[i], EGL_SURFACE_TYPE, &value );
		if ( ( value & ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) ) != ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) )
		{
			continue;
		}

		int	j = 0;
		for ( ; configAttribs[j] != EGL_NONE; j += 2 )
		{
			eglGetConfigAttrib( display, configs[i], configAttribs[j], &value );
			if ( value != configAttribs[j + 1] )
			{
				break;
			}
		}
		if ( configAttribs[j] == EGL_NONE )
		{
			context->config = configs[i];
			break;
		}
	}
	if ( context->config == 0 )
	{
		Error( "Failed to find EGLConfig" );
		return false;
	}

	EGLint contextAttribs[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, OPENGL_VERSION_MAJOR,
		EGL_NONE, EGL_NONE,
		EGL_NONE
	};
	// Use the default priority if GPU_QUEUE_PRIORITY_MEDIUM is selected.
	const GpuQueuePriority_t priority = device->queueInfo.queuePriorities[queueIndex];
	if ( priority != GPU_QUEUE_PRIORITY_MEDIUM )
	{
		contextAttribs[2] = EGL_CONTEXT_PRIORITY_LEVEL_IMG;
		contextAttribs[3] = ( priority == GPU_QUEUE_PRIORITY_LOW ) ? EGL_CONTEXT_PRIORITY_LOW_IMG : EGL_CONTEXT_PRIORITY_HIGH_IMG;
	}
	context->context = eglCreateContext( display, context->config, EGL_NO_CONTEXT, contextAttribs );
	if ( context->context == EGL_NO_CONTEXT )
	{
		Error( "eglCreateContext() failed: %s", EglErrorString( eglGetError() ) );
		return false;
	}

	const EGLint surfaceAttribs[] =
	{
		EGL_WIDTH, 16,
		EGL_HEIGHT, 16,
		EGL_NONE
	};
	context->tinySurface = eglCreatePbufferSurface( display, context->config, surfaceAttribs );
	if ( context->tinySurface == EGL_NO_SURFACE )
	{
		Error( "eglCreatePbufferSurface() failed: %s", EglErrorString( eglGetError() ) );
		eglDestroyContext( display, context->context );
		context->context = EGL_NO_CONTEXT;
		return false;
	}
	context->mainSurface = context->tinySurface;

	return true;
}

#endif

static bool GpuContext_CreateShared( GpuContext_t * context, const GpuContext_t * other, const int queueIndex )
{
	UNUSED_PARM( queueIndex );

	memset( context, 0, sizeof( GpuContext_t ) );

#if defined( OS_WINDOWS )
	context->hDC = other->hDC;
	context->hGLRC = wglCreateContext( other->hDC );
	if ( !wglShareLists( other->hGLRC, context->hGLRC ) )
	{
		return false;
	}
#elif defined( OS_APPLE_MACOS )
	context->nsContext = NULL;
	CGLPixelFormatObj pf = CGLGetPixelFormat( other->cglContext );
	if ( CGLCreateContext( pf, other->cglContext, &context->cglContext ) != kCGLNoError )
	{
		return false;
	}
	CGSConnectionID cid;
	CGSWindowID wid;
	CGSSurfaceID sid;
	if ( CGLGetSurface( other->cglContext, &cid, &wid, &sid ) != kCGLNoError )
	{
		return false;
	}
	if ( CGLSetSurface( context->cglContext, cid, wid, sid ) != kCGLNoError )
	{
		return false;
	}
#elif defined( OS_LINUX_XLIB ) || defined( OS_LINUX_XCB_GLX )
	context->xDisplay = other->xDisplay;
	context->visualid = other->visualid;
	context->glxFBConfig = other->glxFBConfig;
	context->glxDrawable = other->glxDrawable;
	context->glxContext = glXCreateNewContext( other->xDisplay, other->glxFBConfig, GLX_RGBA_TYPE, other->glxContext, True );
	if ( context->glxContext == NULL )
	{
		return false;
	}
#elif defined( OS_LINUX_XCB )
	context->connection = other->connection;
	context->screen_number = other->screen_number;
	context->fbconfigid = other->fbconfigid;
	context->visualid = other->visualid;
	context->glxDrawable = other->glxDrawable;
	context->glxContext = xcb_generate_id( other->connection );
	xcb_glx_create_context( other->connection, context->glxContext, other->visualid, other->screen_number, other->glxContext, 1 );
	context->glxContextTag = 0;
#elif defined( OS_ANDROID )
	context->display = other->display;
	EGLint configID;
	if ( !eglQueryContext( context->display, other->context, EGL_CONFIG_ID, &configID ) )
	{
    	Error( "eglQueryContext EGL_CONFIG_ID failed: %s", EglErrorString( eglGetError() ) );
		return false;
	}
	const int MAX_CONFIGS = 1024;
	EGLConfig configs[MAX_CONFIGS];
	EGLint numConfigs = 0;
	EGL( eglGetConfigs( context->display, configs, MAX_CONFIGS, &numConfigs ) );
	context->config = 0;
	for ( int i = 0; i < numConfigs; i++ )
	{
		EGLint value = 0;
		eglGetConfigAttrib( context->display, configs[i], EGL_CONFIG_ID, &value );
		if ( value == configID )
		{
			context->config = configs[i];
			break;
		}
	}
	if ( context->config == 0 )
	{
		Error( "Failed to find share context config." );
		return false;
	}
	EGLint surfaceType = 0;
	eglGetConfigAttrib( context->display, context->config, EGL_SURFACE_TYPE, &surfaceType );
	if ( ( surfaceType & EGL_PBUFFER_BIT ) == 0 )
	{
		Error( "Share context config does have EGL_PBUFFER_BIT." );
		return false;
	}
	EGLint contextAttribs[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, OPENGL_VERSION_MAJOR,
		EGL_NONE
	};
	context->context = eglCreateContext( context->display, context->config, other->context, contextAttribs );
	if ( context->context == EGL_NO_CONTEXT )
	{
		Error( "eglCreateContext() failed: %s", EglErrorString( eglGetError() ) );
		return false;
	}
	const EGLint surfaceAttribs[] =
	{
		EGL_WIDTH, 16,
		EGL_HEIGHT, 16,
		EGL_NONE
	};
	context->tinySurface = eglCreatePbufferSurface( context->display, context->config, surfaceAttribs );
	if ( context->tinySurface == EGL_NO_SURFACE )
	{
		Error( "eglCreatePbufferSurface() failed: %s", EglErrorString( eglGetError() ) );
		eglDestroyContext( context->display, context->context );
		context->context = EGL_NO_CONTEXT;
		return false;
	}
	context->mainSurface = context->tinySurface;
#endif
	return true;
}

static void GpuContext_Destroy( GpuContext_t * context )
{
#if defined( OS_WINDOWS )
	if ( context->hGLRC )
	{
		if ( !wglMakeCurrent( NULL, NULL ) )
		{
			Error( "Failed to release context." );
		}

		if ( !wglDeleteContext( context->hGLRC ) )
		{
			Error( "Failed to delete context." );
		}
		context->hGLRC = NULL;
	}
	context->hDC = NULL;
#elif defined( OS_APPLE_MACOS )
	CGLSetCurrentContext( NULL );
	if ( context->nsContext != NULL )
	{
		[context->nsContext clearDrawable];
		[context->nsContext release];
		context->nsContext = nil;
	}
	else
	{
		CGLDestroyContext( context->cglContext );
	}
	context->cglContext = nil;
#elif defined( OS_LINUX_XLIB ) || defined( OS_LINUX_XCB_GLX )
	glXDestroyContext( context->xDisplay, context->glxContext );
	context->xDisplay = NULL;
	context->visualid = 0;
	context->glxFBConfig = NULL;
	context->glxDrawable = 0;
	context->glxContext = NULL;
#elif defined( OS_LINUX_XCB )
	xcb_glx_destroy_context( context->connection, context->glxContext );
	context->connection = NULL;
	context->screen_number = 0;
	context->fbconfigid = 0;
	context->visualid = 0;
	context->glxDrawable = 0;
	context->glxContext = 0;
	context->glxContextTag = 0;
#elif defined( OS_ANDROID )
	if ( context->display != 0 )
	{
		EGL( eglMakeCurrent( context->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT ) );
	}
	if ( context->context != EGL_NO_CONTEXT )
	{
		EGL( eglDestroyContext( context->display, context->context ) );
	}
	if ( context->mainSurface != context->tinySurface )
	{
		EGL( eglDestroySurface( context->display, context->mainSurface ) );
	}
	if ( context->tinySurface != EGL_NO_SURFACE )
	{
		EGL( eglDestroySurface( context->display, context->tinySurface ) );
	}
	context->display = 0;
	context->config = 0;
	context->tinySurface = EGL_NO_SURFACE;
	context->mainSurface = EGL_NO_SURFACE;
	context->context = EGL_NO_CONTEXT;
#endif
}

static void GpuContext_WaitIdle( GpuContext_t * context )
{
	UNUSED_PARM( context );

	GL( glFinish() );
}

static void GpuContext_SetCurrent( GpuContext_t * context )
{
#if defined( OS_WINDOWS )
	wglMakeCurrent( context->hDC, context->hGLRC );
#elif defined( OS_APPLE_MACOS )
	CGLSetCurrentContext( context->cglContext );
#elif defined( OS_LINUX_XLIB ) || defined( OS_LINUX_XCB_GLX )
	glXMakeCurrent( context->xDisplay, context->glxDrawable, context->glxContext );
#elif defined( OS_LINUX_XCB )
	xcb_glx_make_current_cookie_t glx_make_current_cookie = xcb_glx_make_current( context->connection, context->glxDrawable, context->glxContext, 0 );
	xcb_glx_make_current_reply_t * glx_make_current_reply = xcb_glx_make_current_reply( context->connection, glx_make_current_cookie, NULL );
	context->glxContextTag = glx_make_current_reply->context_tag;
	free( glx_make_current_reply );
#elif defined( OS_ANDROID )
	EGL( eglMakeCurrent( context->display, context->mainSurface, context->mainSurface, context->context ) );
#endif
}

static void GpuContext_UnsetCurrent( GpuContext_t * context )
{
#if defined( OS_WINDOWS )
	wglMakeCurrent( context->hDC, NULL );
#elif defined( OS_APPLE_MACOS )
	CGLSetCurrentContext( NULL );
#elif defined( OS_LINUX_XLIB ) || defined( OS_LINUX_XCB_GLX )
	glXMakeCurrent( context->xDisplay, None, NULL );
#elif defined( OS_LINUX_XCB )
	xcb_glx_make_current( context->connection, 0, 0, 0 );
#elif defined( OS_ANDROID )
	EGL( eglMakeCurrent( context->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT ) );
#endif
}

static bool GpuContext_CheckCurrent( GpuContext_t * context )
{
#if defined( OS_WINDOWS )
	return ( wglGetCurrentContext() == context->hGLRC );
#elif defined( OS_APPLE_MACOS )
	return ( CGLGetCurrentContext() == context->cglContext );
#elif defined( OS_LINUX_XLIB ) || defined( OS_LINUX_XCB_GLX )
	return ( glXGetCurrentContext() == context->glxContext );
#elif defined( OS_LINUX_XCB )
	return true;
#elif defined( OS_ANDROID )
	return ( eglGetCurrentContext() == context->context );
#endif
}

/*
================================================================================================================================

GPU Window.

Window with associated GPU context for GPU accelerated rendering.
For optimal performance a window should only be created at load time, not at runtime.
Because on some platforms the OS/drivers use thread local storage, GpuWindow_t *must* be created
and destroyed on the same thread that will actually render to the window and swap buffers.

GpuWindow_t
GpuWindowEvent_t
KeyboardKey_t
MouseButton_t

static bool GpuWindow_SupportedResolution( const int width, const int height );
static bool GpuWindow_Create( GpuWindow_t * window, DriverInstance_t * instance,
								const GpuQueueInfo_t * queueInfo, const int queueIndex,
								const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
								const GpuSampleCount_t sampleCount, const int width, const int height, const bool fullscreen );
static void GpuWindow_Destroy( GpuWindow_t * window );
static void GpuWindow_Exit( GpuWindow_t * window );
static GpuWindowEvent_t GpuWindow_ProcessEvents( GpuWindow_t * window );
static void GpuWindow_SwapInterval( GpuWindow_t * window, const int swapInterval );
static void GpuWindow_SwapBuffers( GpuWindow_t * window );
static Microseconds_t GpuWindow_GetNextSwapTimeMicroseconds( GpuWindow_t * window );
static Microseconds_t GpuWindow_GetFrameTimeMicroseconds( GpuWindow_t * window );
static bool GpuWindowInput_ConsumeKeyboardKey( GpuWindow_t * window, const KeyboardKey_t key );
static bool GpuWindowInput_ConsumeMouseButton( GpuWindow_t * window, const MouseButton_t button );
static bool GpuWindowInput_CheckKeyboardKey( GpuWindow_t * window, const KeyboardKey_t key );

================================================================================================================================
*/

typedef enum
{
	GPU_WINDOW_EVENT_NONE,
	GPU_WINDOW_EVENT_ACTIVATED,
	GPU_WINDOW_EVENT_DEACTIVATED,
	GPU_WINDOW_EVENT_EXIT
} GpuWindowEvent_t;

typedef struct
{
	bool					keyInput[256];
	bool					mouseInput[8];
	int						mouseInputX[8];
	int						mouseInputY[8];
} GpuWindowInput_t;

typedef struct
{
	GpuDevice_t				device;
	GpuContext_t			context;
	GpuSurfaceColorFormat_t	colorFormat;
	GpuSurfaceDepthFormat_t	depthFormat;
	GpuSampleCount_t		sampleCount;
	int						windowWidth;
	int						windowHeight;
	int						windowSwapInterval;
	float					windowRefreshRate;
	bool					windowFullscreen;
	bool					windowActive;
	bool					windowExit;
	GpuWindowInput_t		input;
	Microseconds_t			lastSwapTime;

#if defined( OS_WINDOWS )
	HINSTANCE				hInstance;
	HDC						hDC;
	HWND					hWnd;
	bool					windowActiveState;
#elif defined( OS_APPLE_IOS )
	UIWindow *				uiWindow;
	UIView *				uiView;
#elif defined( OS_APPLE_MACOS )
	CGDirectDisplayID		display;
	CGDisplayModeRef		desktopDisplayMode;
	NSWindow *				nsWindow;
	NSView *				nsView;
#elif defined( OS_LINUX_XLIB )
	Display *				xDisplay;
	int						xScreen;
	Window					xRoot;
	XVisualInfo *			xVisual;
	Colormap				xColormap;
	Window					xWindow;
	int						desktopWidth;
	int						desktopHeight;
	float					desktopRefreshRate;
#elif defined( OS_LINUX_XCB ) || defined( OS_LINUX_XCB_GLX )
	Display *				xDisplay;
	xcb_connection_t *		connection;
	xcb_screen_t *			screen;
	xcb_colormap_t			colormap;
	xcb_window_t			window;
	xcb_atom_t				wm_delete_window_atom;
	xcb_key_symbols_t *		key_symbols;
	xcb_glx_window_t		glxWindow;
	int						desktopWidth;
	int						desktopHeight;
	float					desktopRefreshRate;
#elif defined( OS_ANDROID )
	EGLDisplay				display;
	EGLint					majorVersion;
	EGLint					minorVersion;
	struct android_app *	app;
	Java_t					java;
	ANativeWindow *			nativeWindow;
	bool					resumed;
#endif
} GpuWindow_t;

#if defined( OS_WINDOWS )

typedef enum
{
	KEY_A				= 0x41,
	KEY_B				= 0x42,
	KEY_C				= 0x43,
	KEY_D				= 0x44,
	KEY_E				= 0x45,
	KEY_F				= 0x46,
	KEY_G				= 0x47,
	KEY_H				= 0x48,
	KEY_I				= 0x49,
	KEY_J				= 0x4A,
	KEY_K				= 0x4B,
	KEY_L				= 0x4C,
	KEY_M				= 0x4D,
	KEY_N				= 0x4E,
	KEY_O				= 0x4F,
	KEY_P				= 0x50,
	KEY_Q				= 0x51,
	KEY_R				= 0x52,
	KEY_S				= 0x53,
	KEY_T				= 0x54,
	KEY_U				= 0x55,
	KEY_V				= 0x56,
	KEY_W				= 0x57,
	KEY_X				= 0x58,
	KEY_Y				= 0x59,
	KEY_Z				= 0x5A,
	KEY_RETURN			= VK_RETURN,
	KEY_TAB				= VK_TAB,
	KEY_ESCAPE			= VK_ESCAPE,
	KEY_SHIFT_LEFT		= VK_LSHIFT,
	KEY_CTRL_LEFT		= VK_LCONTROL,
	KEY_ALT_LEFT		= VK_LMENU,
	KEY_CURSOR_UP		= VK_UP,
	KEY_CURSOR_DOWN		= VK_DOWN,
	KEY_CURSOR_LEFT		= VK_LEFT,
	KEY_CURSOR_RIGHT	= VK_RIGHT
} KeyboardKey_t;

typedef enum
{
	MOUSE_LEFT		= 0,
	MOUSE_RIGHT		= 1
} MouseButton_t;

LRESULT APIENTRY WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	GpuWindow_t * window = (GpuWindow_t *) GetWindowLongPtr( hWnd, GWLP_USERDATA );

	switch ( message )
	{
		case WM_SIZE:
		{
			if ( window != NULL )
			{
				window->windowWidth = (int) LOWORD( lParam );
				window->windowHeight = (int) HIWORD( lParam );
			}
			return 0;
		}
		case WM_ACTIVATE:
		{
			if ( window != NULL )
			{
				window->windowActiveState = !HIWORD( wParam );
			}
			return 0;
		}
		case WM_ERASEBKGND:
		{
			return 0;
		}
		case WM_CLOSE:
		{
			PostQuitMessage( 0 );
			return 0;
		}
		case WM_KEYDOWN:
		{
			if ( window != NULL )
			{
				if ( (int)wParam >= 0 && (int)wParam < 256 )
				{
					if ( 	(int)wParam != KEY_SHIFT_LEFT &&
							(int)wParam != KEY_CTRL_LEFT &&
							(int)wParam != KEY_ALT_LEFT &&
							(int)wParam != KEY_CURSOR_UP &&
							(int)wParam != KEY_CURSOR_DOWN &&
							(int)wParam != KEY_CURSOR_LEFT &&
							(int)wParam != KEY_CURSOR_RIGHT )
					{
						window->input.keyInput[(int)wParam] = true;
					}
				}
			}
			break;
		}
		case WM_LBUTTONDOWN:
		{
			window->input.mouseInput[MOUSE_LEFT] = true;
			window->input.mouseInputX[MOUSE_LEFT] = LOWORD( lParam );
			window->input.mouseInputY[MOUSE_LEFT] = window->windowHeight - HIWORD( lParam );
			break;
		}
		case WM_RBUTTONDOWN:
		{
			window->input.mouseInput[MOUSE_RIGHT] = true;
			window->input.mouseInputX[MOUSE_RIGHT] = LOWORD( lParam );
			window->input.mouseInputY[MOUSE_RIGHT] = window->windowHeight - HIWORD( lParam );
			break;
		}
	}
	return DefWindowProc( hWnd, message, wParam, lParam );
}

static void GpuWindow_Destroy( GpuWindow_t * window )
{
	GpuContext_Destroy( &window->context );
	GpuDevice_Destroy( &window->device );

	if ( window->windowFullscreen )
	{
		ChangeDisplaySettings( NULL, 0 );
		ShowCursor( TRUE );
	}

	if ( window->hDC )
	{
		if ( !ReleaseDC( window->hWnd, window->hDC ) )
		{
			Error( "Failed to release device context." );
		}
		window->hDC = NULL;
	}

	if ( window->hWnd )
	{
		if ( !DestroyWindow( window->hWnd ) )
		{
			Error( "Failed to destroy the window." );
		}
		window->hWnd = NULL;
	}

	if ( window->hInstance )
	{
		if ( !UnregisterClass( APPLICATION_NAME, window->hInstance ) )
		{
			Error( "Failed to unregister window class." );
		}
		window->hInstance = NULL;
	}
}

static bool GpuWindow_Create( GpuWindow_t * window, DriverInstance_t * instance,
								const GpuQueueInfo_t * queueInfo, const int queueIndex,
								const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
								const GpuSampleCount_t sampleCount, const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( GpuWindow_t ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->sampleCount = sampleCount;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = fullscreen;
	window->windowActive = false;
	window->windowExit = false;
	window->windowActiveState = false;
	window->lastSwapTime = GetTimeMicroseconds();

	if ( window->windowFullscreen )
	{
		DEVMODE dmScreenSettings;
		memset( &dmScreenSettings, 0, sizeof( dmScreenSettings ) );
		dmScreenSettings.dmSize			= sizeof( dmScreenSettings );
		dmScreenSettings.dmPelsWidth	= width;
		dmScreenSettings.dmPelsHeight	= height;
		dmScreenSettings.dmBitsPerPel	= 32;
		dmScreenSettings.dmFields		= DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

		if ( ChangeDisplaySettings( &dmScreenSettings, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL )
		{
			Error( "The requested fullscreen mode is not supported." );
			return false;
		}
	}

	DEVMODE lpDevMode;
	memset( &lpDevMode, 0, sizeof( DEVMODE ) );
	lpDevMode.dmSize = sizeof( DEVMODE );
	lpDevMode.dmDriverExtra = 0;

	if ( EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &lpDevMode ) != FALSE )
	{
		window->windowRefreshRate = (float)lpDevMode.dmDisplayFrequency;
	}

	window->hInstance = GetModuleHandle( NULL );

	WNDCLASS wc;
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc		= (WNDPROC) WndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= window->hInstance;
	wc.hIcon			= LoadIcon( NULL, IDI_WINLOGO );
	wc.hCursor			= LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground	= NULL;
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= APPLICATION_NAME;

	if ( !RegisterClass( &wc ) )
	{
		Error( "Failed to register window class." );
		return false;
	}
	
	DWORD dwExStyle = 0;
	DWORD dwStyle = 0;
	if ( window->windowFullscreen )
	{
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP;
		ShowCursor( FALSE );
	}
	else
	{
		// Fixed size window.
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	}

	RECT windowRect;
	windowRect.left = (long)0;
	windowRect.right = (long)width;
	windowRect.top = (long)0;
	windowRect.bottom = (long)height;

	AdjustWindowRectEx( &windowRect, dwStyle, FALSE, dwExStyle );

	if ( !window->windowFullscreen )
	{
		RECT desktopRect;
		GetWindowRect( GetDesktopWindow(), &desktopRect );

		const int offsetX = ( desktopRect.right - ( windowRect.right - windowRect.left ) ) / 2;
		const int offsetY = ( desktopRect.bottom - ( windowRect.bottom - windowRect.top ) ) / 2;

		windowRect.left += offsetX;
		windowRect.right += offsetX;
		windowRect.top += offsetY;
		windowRect.bottom += offsetY;
	}

	window->hWnd = CreateWindowEx( dwExStyle,						// Extended style for the window
								APPLICATION_NAME,					// Class name
								WINDOW_TITLE,						// Window title
								dwStyle |							// Defined window style
								WS_CLIPSIBLINGS |					// Required window style
								WS_CLIPCHILDREN,					// Required window style
								windowRect.left,					// Window X position
								windowRect.top,						// Window Y position
								windowRect.right - windowRect.left,	// Window width
								windowRect.bottom - windowRect.top,	// Window height
								NULL,								// No parent window
								NULL,								// No menu
								window->hInstance,					// Instance
								NULL );
	if ( !window->hWnd )
	{
		GpuWindow_Destroy( window );
		Error( "Failed to create window." );
		return false;
	}

	SetWindowLongPtr( window->hWnd, GWLP_USERDATA, (LONG_PTR) window );

	window->hDC = GetDC( window->hWnd );
	if ( !window->hDC )
	{
		GpuWindow_Destroy( window );
		Error( "Failed to acquire device context." );
		return false;
	}

	GpuDevice_Create( &window->device, instance, queueInfo );
	GpuContext_CreateForSurface( &window->context, &window->device, queueIndex, colorFormat, depthFormat, sampleCount, window->hInstance, window->hDC );
	GpuContext_SetCurrent( &window->context );

	ShowWindow( window->hWnd, SW_SHOW );
	SetForegroundWindow( window->hWnd );
	SetFocus( window->hWnd );

	return true;
}

static bool GpuWindow_SupportedResolution( const int width, const int height )
{
	DEVMODE dm = { 0 };
	dm.dmSize = sizeof( dm );
	for ( int modeIndex = 0; EnumDisplaySettings( NULL, modeIndex, &dm ) != 0; modeIndex++ )
	{
		if ( dm.dmPelsWidth == (DWORD)width && dm.dmPelsHeight == (DWORD)height )
		{
			return true;
		}
	}
	return false;
}

static void GpuWindow_Exit( GpuWindow_t * window )
{
	window->windowExit = true;
}

static GpuWindowEvent_t GpuWindow_ProcessEvents( GpuWindow_t * window )
{
	MSG msg;
	while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) > 0 )
	{
		if ( msg.message == WM_QUIT )
		{
			window->windowExit = true;
		}
		else
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}

	window->input.keyInput[KEY_SHIFT_LEFT]		= GetAsyncKeyState( KEY_SHIFT_LEFT );
	window->input.keyInput[KEY_CTRL_LEFT]		= GetAsyncKeyState( KEY_CTRL_LEFT );
	window->input.keyInput[KEY_ALT_LEFT]		= GetAsyncKeyState( KEY_ALT_LEFT );
	window->input.keyInput[KEY_CURSOR_UP]		= GetAsyncKeyState( KEY_CURSOR_UP );
	window->input.keyInput[KEY_CURSOR_DOWN]		= GetAsyncKeyState( KEY_CURSOR_DOWN );
	window->input.keyInput[KEY_CURSOR_LEFT]		= GetAsyncKeyState( KEY_CURSOR_LEFT );
	window->input.keyInput[KEY_CURSOR_RIGHT]	= GetAsyncKeyState( KEY_CURSOR_RIGHT );

	if ( window->windowExit )
	{
		return GPU_WINDOW_EVENT_EXIT;
	}
	if ( window->windowActiveState != window->windowActive )
	{
		window->windowActive = window->windowActiveState;
		return ( window->windowActiveState ) ? GPU_WINDOW_EVENT_ACTIVATED : GPU_WINDOW_EVENT_DEACTIVATED;
	}
	return GPU_WINDOW_EVENT_NONE;
}

#elif defined( OS_APPLE_IOS )

typedef enum
{
	KEY_A				= 0x00,
	KEY_B				= 0x0B,
	KEY_C				= 0x08,
	KEY_D				= 0x02,
	KEY_E				= 0x0E,
	KEY_F				= 0x03,
	KEY_G				= 0x05,
	KEY_H				= 0x04,
	KEY_I				= 0x22,
	KEY_J				= 0x26,
	KEY_K				= 0x28,
	KEY_L				= 0x25,
	KEY_M				= 0x2E,
	KEY_N				= 0x2D,
	KEY_O				= 0x1F,
	KEY_P				= 0x23,
	KEY_Q				= 0x0C,
	KEY_R				= 0x0F,
	KEY_S				= 0x01,
	KEY_T				= 0x11,
	KEY_U				= 0x20,
	KEY_V				= 0x09,
	KEY_W				= 0x0D,
	KEY_X				= 0x07,
	KEY_Y				= 0x10,
	KEY_Z				= 0x06,
	KEY_RETURN			= 0x24,
	KEY_TAB				= 0x30,
	KEY_ESCAPE			= 0x35,
	KEY_SHIFT_LEFT		= 0x38,
	KEY_CTRL_LEFT		= 0x3B,
	KEY_ALT_LEFT		= 0x3A,
	KEY_CURSOR_UP		= 0x7E,
	KEY_CURSOR_DOWN		= 0x7D,
	KEY_CURSOR_LEFT		= 0x7B,
	KEY_CURSOR_RIGHT	= 0x7C
} KeyboardKey_t;

typedef enum
{
	MOUSE_LEFT			= 0,
	MOUSE_RIGHT			= 1
} MouseButton_t;

static NSAutoreleasePool * autoReleasePool;
static UIView* myUIView;
static UIWindow* myUIWindow;

@interface MyUIView : UIView
@end

@implementation MyUIView

-(instancetype) initWithFrame:(CGRect)frameRect {
	self = [super initWithFrame: frameRect];
	if ( self ) {
		self.contentScaleFactor = UIScreen.mainScreen.nativeScale;
	}
	return self;
}

+(Class) layerClass { return [CAMetalLayer class]; }

@end

@interface MyUIViewController : UIViewController
@end

@implementation MyUIViewController

-(UIInterfaceOrientationMask) supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscape; }

-(BOOL) shouldAutorotate { return TRUE; }

@end

static void GpuWindow_Destroy( GpuWindow_t * window )
{
	GpuWindow_DestroySurface( window );
	GpuContext_Destroy( &window->context );
	GpuDevice_Destroy( &window->device );

	if ( window->uiWindow )
	{
		[window->uiWindow release];
		window->uiWindow = nil;
	}
	if ( window->uiView )
	{
		[window->uiView release];
		window->uiView = nil;
	}
}

static bool GpuWindow_Create( GpuWindow_t * window, DriverInstance_t * instance,
								const GpuQueueInfo_t * queueInfo, const int queueIndex,
								const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
								const GpuSampleCount_t sampleCount, const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( GpuWindow_t ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->sampleCount = sampleCount;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = fullscreen;
	window->windowActive = false;
	window->windowExit = false;
	window->lastSwapTime = GetTimeMicroseconds();
	window->uiView = myUIView;
	window->uiWindow = myUIWindow;

	GpuDevice_Create( &window->device, instance, queueInfo );
	GpuContext_CreateForSurface( &window->context, &window->device, queueIndex, colorFormat, depthFormat, sampleCount, window->display );

	return true;
}

static bool GpuWindow_SupportedResolution( const int width, const int height )
{
	UNUSED_PARM( width );
	UNUSED_PARM( height );

	return true;
}

static void GpuWindow_Exit( GpuWindow_t * window )
{
	window->windowExit = true;
}

static GpuWindowEvent_t GpuWindow_ProcessEvents( GpuWindow_t * window )
{
	[autoReleasePool release];
	autoReleasePool = [[NSAutoreleasePool alloc] init];

	if ( window->windowExit )
	{
		return GPU_WINDOW_EVENT_EXIT;
	}

	if ( window->windowActive == false )
	{
		window->windowActive = true;
		return GPU_WINDOW_EVENT_ACTIVATED;
	}
	
	return GPU_WINDOW_EVENT_NONE;
}

#elif defined( OS_APPLE_MACOS )

typedef enum
{
	KEY_A				= 0x00,
	KEY_B				= 0x0B,
	KEY_C				= 0x08,
	KEY_D				= 0x02,
	KEY_E				= 0x0E,
	KEY_F				= 0x03,
	KEY_G				= 0x05,
	KEY_H				= 0x04,
	KEY_I				= 0x22,
	KEY_J				= 0x26,
	KEY_K				= 0x28,
	KEY_L				= 0x25,
	KEY_M				= 0x2E,
	KEY_N				= 0x2D,
	KEY_O				= 0x1F,
	KEY_P				= 0x23,
	KEY_Q				= 0x0C,
	KEY_R				= 0x0F,
	KEY_S				= 0x01,
	KEY_T				= 0x11,
	KEY_U				= 0x20,
	KEY_V				= 0x09,
	KEY_W				= 0x0D,
	KEY_X				= 0x07,
	KEY_Y				= 0x10,
	KEY_Z				= 0x06,
	KEY_RETURN			= 0x24,
	KEY_TAB				= 0x30,
	KEY_ESCAPE			= 0x35,
	KEY_SHIFT_LEFT		= 0x38,
	KEY_CTRL_LEFT		= 0x3B,
	KEY_ALT_LEFT		= 0x3A,
	KEY_CURSOR_UP		= 0x7E,
	KEY_CURSOR_DOWN		= 0x7D,
	KEY_CURSOR_LEFT		= 0x7B,
	KEY_CURSOR_RIGHT	= 0x7C
} KeyboardKey_t;

typedef enum
{
	MOUSE_LEFT			= 0,
	MOUSE_RIGHT			= 1
} MouseButton_t;

NSAutoreleasePool * autoReleasePool;

@interface MyNSWindow : NSWindow
- (BOOL)canBecomeMainWindow;
- (BOOL)canBecomeKeyWindow;
- (BOOL)acceptsFirstResponder;
- (void)keyDown:(NSEvent *)event;
@end

@implementation MyNSWindow
- (BOOL)canBecomeMainWindow { return YES; }
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }
- (void)keyDown:(NSEvent *)event {}
@end

@interface MyNSView : NSView
- (BOOL)acceptsFirstResponder;
- (void)keyDown:(NSEvent *)event;
@end

@implementation MyNSView
- (BOOL)acceptsFirstResponder { return YES; }
- (void)keyDown:(NSEvent *)event {}
@end

static void GpuWindow_Destroy( GpuWindow_t * window )
{
	GpuContext_Destroy( &window->context );
	GpuDevice_Destroy( &window->device );

	if ( window->windowFullscreen )
	{
		CGDisplaySetDisplayMode( window->display, window->desktopDisplayMode, NULL );
		CGDisplayModeRelease( window->desktopDisplayMode );
		window->desktopDisplayMode = NULL;
	}
	if ( window->nsWindow )
	{
		[window->nsWindow release];
		window->nsWindow = nil;
	}
	if ( window->nsView )
	{
		[window->nsView release];
		window->nsView = nil;
	}
}

static bool GpuWindow_Create( GpuWindow_t * window, DriverInstance_t * instance,
								const GpuQueueInfo_t * queueInfo, const int queueIndex,
								const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
								const GpuSampleCount_t sampleCount, const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( GpuWindow_t ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->sampleCount = sampleCount;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = fullscreen;
	window->windowActive = false;
	window->windowExit = false;
	window->lastSwapTime = GetTimeMicroseconds();

	// Get a list of all available displays.
	CGDirectDisplayID displays[32];
	CGDisplayCount displayCount = 0;
	CGDisplayErr err = CGGetActiveDisplayList( 32, displays, &displayCount );
	if ( err != CGDisplayNoErr )
	{
		return false;
	}
	// Use the main display.
	window->display = displays[0];
	window->desktopDisplayMode = CGDisplayCopyDisplayMode( window->display );

	// If fullscreen then switch to the best matching display mode.
	if ( window->windowFullscreen )
	{
		CFArrayRef displayModes = CGDisplayCopyAllDisplayModes( window->display, NULL );
		CFIndex displayModeCount = CFArrayGetCount( displayModes );
		CGDisplayModeRef bestDisplayMode = nil;
		size_t bestDisplayWidth = 0;
		size_t bestDisplayHeight = 0;
		float bestDisplayRefreshRate = 0;
		size_t bestError = 0x7FFFFFFF;
		for ( CFIndex i = 0; i < displayModeCount; i++ )
		{
			CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex( displayModes, i );

			const size_t modeWidth = CGDisplayModeGetWidth( mode );
			const size_t modeHeight = CGDisplayModeGetHeight( mode );
			const double modeRefreshRate = CGDisplayModeGetRefreshRate( mode );
			CFStringRef modePixelEncoding = CGDisplayModeCopyPixelEncoding( mode );
			const bool modeBitsPerPixelIs32 = ( CFStringCompare( modePixelEncoding, CFSTR( IO32BitDirectPixels ), 0) == kCFCompareEqualTo );
			CFRelease( modePixelEncoding );

			if ( modeBitsPerPixelIs32 )
			{
				const size_t dw = modeWidth - width;
				const size_t dh = modeHeight - height;
				const size_t error = dw * dw + dh * dh;
				if ( error < bestError )
				{
					bestError = error;
					bestDisplayMode = mode;
					bestDisplayWidth = modeWidth;
					bestDisplayHeight = modeHeight;
					bestDisplayRefreshRate = (float)modeRefreshRate;
				}
			}
		}
		CGDisplayErr err = CGDisplaySetDisplayMode( window->display, bestDisplayMode, NULL );
		if ( err != CGDisplayNoErr )
		{
			CFRelease( displayModes );
			return false;
		}
		CFRelease( displayModes );
		window->windowWidth = (int)bestDisplayWidth;
		window->windowHeight = (int)bestDisplayHeight;
		window->windowRefreshRate = ( bestDisplayRefreshRate > 0.0f ) ? bestDisplayRefreshRate : 60.0f;
	}
	else
	{
		const float desktopDisplayRefreshRate = (float)CGDisplayModeGetRefreshRate( window->desktopDisplayMode );
		window->windowRefreshRate = ( desktopDisplayRefreshRate > 0.0f ) ? desktopDisplayRefreshRate : 60.0f;
	}

	if ( window->windowFullscreen )
	{
		NSScreen * screen = [NSScreen mainScreen];
		NSRect screenRect = [screen frame];
		
		window->nsView = [MyNSView alloc];
		[window->nsView initWithFrame:screenRect];

		const int style = NSBorderlessWindowMask;

		window->nsWindow = [MyNSWindow alloc];
		[window->nsWindow initWithContentRect:screenRect styleMask:style backing:NSBackingStoreBuffered defer:NO screen:screen];
		[window->nsWindow setOpaque:YES];
		[window->nsWindow setLevel:NSMainMenuWindowLevel+1];
		[window->nsWindow setContentView:window->nsView];
		[window->nsWindow makeMainWindow];
		[window->nsWindow makeKeyAndOrderFront:nil];
		[window->nsWindow makeFirstResponder:nil];
	}
	else
	{
		NSScreen * screen = [NSScreen mainScreen];
		NSRect screenRect = [screen frame];

		NSRect windowRect;
		windowRect.origin.x = ( screenRect.size.width - width ) / 2;
		windowRect.origin.y = ( screenRect.size.height - height ) / 2;
		windowRect.size.width = width;
		windowRect.size.height = height;

		window->nsView = [MyNSView alloc];
		[window->nsView initWithFrame:windowRect];

		// Fixed size window.
		const int style = NSTitledWindowMask;// | NSClosableWindowMask | NSResizableWindowMask;

		window->nsWindow = [MyNSWindow alloc];
		[window->nsWindow initWithContentRect:windowRect styleMask:style backing:NSBackingStoreBuffered defer:NO screen:screen];
		[window->nsWindow setTitle:@WINDOW_TITLE];
		[window->nsWindow setOpaque:YES];
		[window->nsWindow setContentView:window->nsView];
		[window->nsWindow makeMainWindow];
		[window->nsWindow makeKeyAndOrderFront:nil];
		[window->nsWindow makeFirstResponder:nil];
	}

	GpuDevice_Create( &window->device, instance, queueInfo );
	GpuContext_CreateForSurface( &window->context, &window->device, queueIndex, colorFormat, depthFormat, sampleCount, window->display );

	[window->context.nsContext setView:window->nsView];

	GpuContext_SetCurrent( &window->context );

	// The color buffers are not cleared by default.
	for ( int i = 0; i < 2; i++ )
	{
		GL( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );
		GL( glClear( GL_COLOR_BUFFER_BIT ) );
		CGLFlushDrawable( window->context.cglContext );
	}

	return true;
}

static bool GpuWindow_SupportedResolution( const int width, const int height )
{
	UNUSED_PARM( width );
	UNUSED_PARM( height );

	return true;
}

static void GpuWindow_Exit( GpuWindow_t * window )
{
	window->windowExit = true;
}

static GpuWindowEvent_t GpuWindow_ProcessEvents( GpuWindow_t * window )
{
	[autoReleasePool release];
	autoReleasePool = [[NSAutoreleasePool alloc] init];

	for ( ; ; )
	{
		NSEvent * event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
		if ( event == nil )
		{
			break;
		}

		if ( event.type == NSKeyDown )
		{
			unsigned short key = [event keyCode];
			if ( key >= 0 && key < 256 )
			{
				window->input.keyInput[key] = true;
			}
		}
		else if ( event.type == NSLeftMouseDown )
		{
			NSPoint point = [event locationInWindow];
			window->input.mouseInput[MOUSE_LEFT] = true;
			window->input.mouseInputX[MOUSE_LEFT] = point.x;
			window->input.mouseInputY[MOUSE_LEFT] = point.y - 1;	// change to zero-based
		}
		else if ( event.type == NSRightMouseDown )
		{
			NSPoint point = [event locationInWindow];
			window->input.mouseInput[MOUSE_RIGHT] = true;
			window->input.mouseInputX[MOUSE_RIGHT] = point.x;
			window->input.mouseInputY[MOUSE_RIGHT] = point.y - 1;	// change to zero-based
		}

		[NSApp sendEvent:event];
	}

	if ( window->windowExit )
	{
		return GPU_WINDOW_EVENT_EXIT;
	}

	if ( window->windowActive == false )
	{
		window->windowActive = true;
		return GPU_WINDOW_EVENT_ACTIVATED;
	}

	return GPU_WINDOW_EVENT_NONE;
}

#elif defined( OS_LINUX_XLIB )

typedef enum	// keysym.h
{
	KEY_A				= XK_a,
	KEY_B				= XK_b,
	KEY_C				= XK_c,
	KEY_D				= XK_d,
	KEY_E				= XK_e,
	KEY_F				= XK_f,
	KEY_G				= XK_g,
	KEY_H				= XK_h,
	KEY_I				= XK_i,
	KEY_J				= XK_j,
	KEY_K				= XK_k,
	KEY_L				= XK_l,
	KEY_M				= XK_m,
	KEY_N				= XK_n,
	KEY_O				= XK_o,
	KEY_P				= XK_p,
	KEY_Q				= XK_q,
	KEY_R				= XK_r,
	KEY_S				= XK_s,
	KEY_T				= XK_t,
	KEY_U				= XK_u,
	KEY_V				= XK_v,
	KEY_W				= XK_w,
	KEY_X				= XK_x,
	KEY_Y				= XK_y,
	KEY_Z				= XK_z,
	KEY_RETURN			= ( XK_Return & 0xFF ),
	KEY_TAB				= ( XK_Tab & 0xFF ),
	KEY_ESCAPE			= ( XK_Escape & 0xFF ),
	KEY_SHIFT_LEFT		= ( XK_Shift_L & 0xFF ),
	KEY_CTRL_LEFT		= ( XK_Control_L & 0xFF ),
	KEY_ALT_LEFT		= ( XK_Alt_L & 0xFF ),
	KEY_CURSOR_UP		= ( XK_Up & 0xFF ),
	KEY_CURSOR_DOWN		= ( XK_Down & 0xFF ),
	KEY_CURSOR_LEFT		= ( XK_Left & 0xFF ),
	KEY_CURSOR_RIGHT	= ( XK_Right & 0xFF )
} KeyboardKey_t;

typedef enum
{
	MOUSE_LEFT		= Button1,
	MOUSE_RIGHT		= Button2
} MouseButton_t;

/*
	Change video mode using the XFree86-VidMode X extension.

	While the XFree86-VidMode X extension should be superseded by the XRandR X extension,
	this still appears to be the most reliable way to change video modes for a single
	monitor configuration.
*/
static bool ChangeVideoMode_XF86VidMode( Display * xDisplay, int xScreen, Window xWindow,
								int * currentWidth, int * currentHeight, float * currentRefreshRate,
								int * desiredWidth, int * desiredHeight, float * desiredRefreshRate )
{
	int videoModeCount;
	XF86VidModeModeInfo ** videoModeInfos;

	XF86VidModeGetAllModeLines( xDisplay, xScreen, &videoModeCount, &videoModeInfos );

	if ( currentWidth != NULL && currentHeight != NULL && currentRefreshRate != NULL )
	{
		XF86VidModeModeInfo * mode = videoModeInfos[0];
		*currentWidth = mode->hdisplay;
		*currentHeight = mode->vdisplay;
		*currentRefreshRate = ( mode->dotclock * 1000.0f ) / ( mode->htotal * mode->vtotal );
	}

	if ( desiredWidth != NULL && desiredHeight != NULL && desiredRefreshRate != NULL )
	{
		XF86VidModeModeInfo * bestMode = NULL;
		int bestModeWidth = 0;
		int bestModeHeight = 0;
		float bestModeRefreshRate = 0.0f;
		int bestSizeError = 0x7FFFFFFF;
		float bestRefreshRateError = 1e6f;
		for ( int j = 0; j < videoModeCount; j++ )
		{
			XF86VidModeModeInfo * mode = videoModeInfos[j];
			const int modeWidth = mode->hdisplay;
			const int modeHeight = mode->vdisplay;
			const float modeRefreshRate = ( mode->dotclock * 1000.0f ) / ( mode->htotal * mode->vtotal );

			const int dw = modeWidth - *desiredWidth;
			const int dh = modeHeight - *desiredHeight;
			const int sizeError = dw * dw + dh * dh;
			const float refreshRateError = fabs( modeRefreshRate - *desiredRefreshRate );
			if ( sizeError < bestSizeError || ( sizeError == bestSizeError && refreshRateError < bestRefreshRateError ) )
			{
				bestSizeError = sizeError;
				bestRefreshRateError = refreshRateError;
				bestMode = mode;
				bestModeWidth = modeWidth;
				bestModeHeight = modeHeight;
				bestModeRefreshRate = modeRefreshRate;
			}
		}

		XF86VidModeSwitchToMode( xDisplay, xScreen, bestMode );
		XF86VidModeSetViewPort( xDisplay, xScreen, 0, 0 );

		*desiredWidth = bestModeWidth;
		*desiredHeight = bestModeHeight;
		*desiredRefreshRate = bestModeRefreshRate;
	}

	for ( int i = 0; i < videoModeCount; i++ )
	{
		if ( videoModeInfos[i]->privsize > 0 )
		{
			XFree( videoModeInfos[i]->private );
		}
	}
	XFree( videoModeInfos );

	return true;
}

/*
	Change video mode using the XRandR X extension version 1.1

	This does not work using NVIDIA drivers because the NVIDIA drivers by default dynamically
	configure TwinView, known as DynamicTwinView. When DynamicTwinView is enabled (the default),
	the refresh rate of a mode reported through XRandR is not the actual refresh rate, but
	instead is an unique number such that each MetaMode has a different value. This is to
	guarantee that MetaModes can be uniquely identified by XRandR.

	To get XRandR to report accurate refresh rates, DynamicTwinView needs to be disabled, but
	then NV-CONTROL clients, such as nvidia-settings, will not be able to dynamically manipulate
	the X screen's MetaModes.
*/
static bool ChangeVideoMode_XRandR_1_1( Display * xDisplay, int xScreen, Window xWindow,
								int * currentWidth, int * currentHeight, float * currentRefreshRate,
								int * desiredWidth, int * desiredHeight, float * desiredRefreshRate )
{
	int major_version;
	int minor_version;
	XRRQueryVersion( xDisplay, &major_version, &minor_version );

	XRRScreenConfiguration * screenInfo = XRRGetScreenInfo( xDisplay, xWindow );
	if ( screenInfo == NULL )
	{
		Error( "Cannot get screen info." );
		return false;
	}

	if ( currentWidth != NULL && currentHeight != NULL && currentRefreshRate != NULL )
	{
		XRRScreenConfiguration * screenInfo = XRRGetScreenInfo( xDisplay, xWindow );

		Rotation rotation;
		int size_index = XRRConfigCurrentConfiguration( screenInfo, &rotation );

		int nsizes;
		XRRScreenSize * sizes = XRRConfigSizes( screenInfo, &nsizes );

		*currentWidth = sizes[size_index].width;
		*currentHeight = sizes[size_index].height;
		*currentRefreshRate = XRRConfigCurrentRate( screenInfo );
	}

	if ( desiredWidth != NULL && desiredHeight != NULL && desiredRefreshRate != NULL )
	{
		int nsizes = 0;
		XRRScreenSize * sizes = XRRConfigSizes( screenInfo, &nsizes );

		int size_index = -1;
		int bestSizeError = 0x7FFFFFFF;
		for ( int i = 0; i < nsizes; i++ )
		{
			const int dw = sizes[i].width - *desiredWidth;
			const int dh = sizes[i].height - *desiredHeight;
			const int error = dw * dw + dh * dh;
			if ( error < bestSizeError )
			{
				bestSizeError = error;
				size_index = i;
			}
		}
		if ( size_index == -1 )
		{
			Error( "%dx%d resolution not available.", *desiredWidth, *desiredHeight );
			XRRFreeScreenConfigInfo( screenInfo );
			return false;
		}

		int nrates = 0;
		short * rates = XRRConfigRates( screenInfo, size_index, &nrates );

		int rate_index = -1;
		float bestRateError = 1e6f;
		for ( int i = 0; i < nrates; i++ )
		{
			const float error = fabs( rates[i] - *desiredRefreshRate );
			if ( error < bestRateError )
			{
				bestRateError = error;
				rate_index = i;
			}
		}

		*desiredWidth = sizes[size_index].width;
		*desiredHeight = sizes[size_index].height;
		*desiredRefreshRate = rates[rate_index];

		XSelectInput( xDisplay, xWindow, StructureNotifyMask );
		XRRSelectInput( xDisplay, xWindow, RRScreenChangeNotifyMask );

		Rotation rotation = 1;
		int reflection = 0;

		Status status = XRRSetScreenConfigAndRate( xDisplay, screenInfo, xWindow,
							(SizeID) size_index,
							(Rotation) (rotation | reflection),
							rates[rate_index],
							CurrentTime );

		if ( status != RRSetConfigSuccess)
		{
			Error( "Failed to change resolution to %dx%d", *desiredWidth, *desiredHeight );
			XRRFreeScreenConfigInfo( screenInfo );
			return false;
		}

		int eventbase;
		int errorbase;
		XRRQueryExtension( xDisplay, &eventbase, &errorbase );

		bool receivedScreenChangeNotify = false;
		bool receivedConfigNotify = false;
		while ( 1 )
		{
			XEvent event;
			XNextEvent( xDisplay, (XEvent *) &event );
			XRRUpdateConfiguration( &event );
			if ( event.type - eventbase == RRScreenChangeNotify )
			{
				receivedScreenChangeNotify = true;
			}
			else if ( event.type == ConfigureNotify )
			{
				receivedConfigNotify = true ;
			}
			if ( receivedScreenChangeNotify && receivedConfigNotify )
			{
				break;
			}
		}
	}

	XRRFreeScreenConfigInfo( screenInfo );

	return true;
}

/*
	Change video mode using the XRandR X extension version 1.2

	The following code does not necessarily work out of the box, because on
	some configurations the modes list returned by XRRGetScreenResources()
	is populated with nothing other than the maximum display resolution,
	even though XF86VidModeGetAllModeLines() and XRRConfigSizes() *will*
	list all resolutions for the same display.

	The user can manually add new modes from the command-line using the
	xrandr utility:

	xrandr --newmode <modeline>

	Where <modeline> is generated with a utility that implements either
	the General Timing Formula (GTF) or the Coordinated Video Timing (CVT)
	standard put forth by the Video Electronics Standards Association (VESA):

	gft <width> <height> <Hz>	// http://gtf.sourceforge.net/
	cvt <width> <height> <Hz>	// http://www.uruk.org/~erich/projects/cvt/

	Alternatively, new modes can be added in code using XRRCreateMode().
	However, this requires calculating all the timing information in code
	because there is no standard library that implements the GTF or CVT.
*/
static bool ChangeVideoMode_XRandR_1_2( Display * xDisplay, int xScreen, Window xWindow,
								int * currentWidth, int * currentHeight, float * currentRefreshRate,
								int * desiredWidth, int * desiredHeight, float * desiredRefreshRate )
{
	int major_version;
	int minor_version;
	XRRQueryVersion( xDisplay, &major_version, &minor_version );

	/*
		Screen	- virtual screenspace which may be covered by multiple CRTCs
		CRTC	- display controller
		Output	- display/monitor connected to a CRTC
		Clones	- outputs that are simultaneously connected to the same CRTC
	*/

	const int PRIMARY_CRTC_INDEX = 0;
	const int PRIMARY_OUTPUT_INDEX = 0;

	XRRScreenResources * screenResources = XRRGetScreenResources( xDisplay, xWindow );
	XRRCrtcInfo * primaryCrtcInfo = XRRGetCrtcInfo( xDisplay, screenResources, screenResources->crtcs[PRIMARY_CRTC_INDEX] );
	XRROutputInfo * primaryOutputInfo = XRRGetOutputInfo( xDisplay, screenResources, primaryCrtcInfo->outputs[PRIMARY_OUTPUT_INDEX] );

	if ( currentWidth != NULL && currentHeight != NULL && currentRefreshRate != NULL )
	{
		for ( int i = 0; i < screenResources->nmode; i++ )
		{
			const XRRModeInfo * modeInfo = &screenResources->modes[i];
			if ( modeInfo->id == primaryCrtcInfo->mode )
			{
				*currentWidth = modeInfo->width;
				*currentHeight = modeInfo->height;
				*currentRefreshRate = modeInfo->dotClock / ( (float)modeInfo->hTotal * (float)modeInfo->vTotal );
				break;
			}
		}
	}

	if ( desiredWidth != NULL && desiredHeight != NULL && desiredRefreshRate != NULL )
	{
		RRMode bestMode = 0;
		int bestModeWidth = 0;
		int bestModeHeight = 0;
		float bestModeRefreshRate = 0.0f;
		int bestSizeError = 0x7FFFFFFF;
		float bestRefreshRateError = 1e6f;

		for ( int i = 0; i < screenResources->nmode; i++ )
		{
			const XRRModeInfo * modeInfo = &screenResources->modes[i];

			if ( modeInfo->modeFlags & RR_Interlace )
			{
				continue;
			}

			bool validOutputMode = false;
			for ( int j = 0; j < primaryOutputInfo->nmode; j++ )
			{
				if ( modeInfo->id == primaryOutputInfo->modes[j] )
				{
					validOutputMode = true;
					break;
				}
			}
			if ( !validOutputMode )
			{
				continue;
			}

			const int modeWidth = modeInfo->width;
			const int modeHeight = modeInfo->height;
			const float modeRefreshRate = modeInfo->dotClock / ( (float)modeInfo->hTotal * (float)modeInfo->vTotal );

			const int dw = modeWidth - *desiredWidth;
			const int dh = modeHeight - *desiredHeight;
			const int sizeError = dw * dw + dh * dh;
			const float refreshRateError = fabs( modeRefreshRate - *desiredRefreshRate );
			if ( sizeError < bestSizeError || ( sizeError == bestSizeError && refreshRateError < bestRefreshRateError ) )
			{
				bestSizeError = sizeError;
				bestRefreshRateError = refreshRateError;
				bestMode = modeInfo->id;
				bestModeWidth = modeWidth;
				bestModeHeight = modeHeight;
				bestModeRefreshRate = modeRefreshRate;
			}
		}

		XRRSetCrtcConfig( xDisplay, screenResources, primaryOutputInfo->crtc, CurrentTime,
							primaryCrtcInfo->x, primaryCrtcInfo->y, bestMode, primaryCrtcInfo->rotation,
							primaryCrtcInfo->outputs, primaryCrtcInfo->noutput );

		*desiredWidth = bestModeWidth;
		*desiredHeight = bestModeHeight;
		*desiredRefreshRate = bestModeRefreshRate;
	}

	XRRFreeOutputInfo( primaryOutputInfo );
	XRRFreeCrtcInfo( primaryCrtcInfo );
	XRRFreeScreenResources( screenResources );

	return true;
}

static void GpuWindow_Destroy( GpuWindow_t * window )
{
	GpuContext_Destroy( &window->context );
	GpuDevice_Destroy( &window->device );

	if ( window->windowFullscreen )
	{
		ChangeVideoMode_XF86VidMode( window->xDisplay, window->xScreen, window->xRoot,
									NULL, NULL, NULL,
									&window->desktopWidth, &window->desktopHeight, &window->desktopRefreshRate );

		XUngrabPointer( window->xDisplay, CurrentTime );
		XUngrabKeyboard( window->xDisplay, CurrentTime );
	}

	if ( window->xWindow )
	{
		XUnmapWindow( window->xDisplay, window->xWindow );
		XDestroyWindow( window->xDisplay, window->xWindow );
		window->xWindow = 0;
	}

	if ( window->xColormap )
	{
		XFreeColormap( window->xDisplay, window->xColormap );
		window->xColormap = 0;
	}

	if ( window->xVisual )
	{
		XFree( window->xVisual );
		window->xVisual = NULL;
	}

	XFlush( window->xDisplay );
	XCloseDisplay( window->xDisplay );
	window->xDisplay = NULL;
}

static bool GpuWindow_Create( GpuWindow_t * window, DriverInstance_t * instance,
								const GpuQueueInfo_t * queueInfo, const int queueIndex,
								const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
								const GpuSampleCount_t sampleCount, const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( GpuWindow_t ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->sampleCount = sampleCount;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = fullscreen;
	window->windowActive = false;
	window->windowExit = false;
	window->lastSwapTime = GetTimeMicroseconds();

	const char * displayName = NULL;
	window->xDisplay = XOpenDisplay( displayName );
	if ( !window->xDisplay )
	{
		Error( "Unable to open X Display." );
		return false;
	}

	window->xScreen = XDefaultScreen( window->xDisplay );
	window->xRoot = XRootWindow( window->xDisplay, window->xScreen );

	if ( window->windowFullscreen )
	{
		ChangeVideoMode_XF86VidMode( window->xDisplay, window->xScreen, window->xRoot,
									&window->desktopWidth, &window->desktopHeight, &window->desktopRefreshRate,
									&window->windowWidth, &window->windowHeight, &window->windowRefreshRate );
	}
	else
	{
		ChangeVideoMode_XF86VidMode( window->xDisplay, window->xScreen, window->xRoot,
									&window->desktopWidth, &window->desktopHeight, &window->desktopRefreshRate,
									NULL, NULL, NULL );
		window->windowRefreshRate = window->desktopRefreshRate;
	}

	GpuDevice_Create( &window->device, instance, queueInfo );
	GpuContext_CreateForSurface( &window->context, &window->device, queueIndex, colorFormat, depthFormat, sampleCount, window->xDisplay, window->xScreen );

	window->xVisual = glXGetVisualFromFBConfig( window->xDisplay, window->context.glxFBConfig );
	if ( window->xVisual == NULL )
	{
		Error( "Failed to retrieve visual for framebuffer config." );
		GpuWindow_Destroy( window );
		return false;
	}

	window->xColormap = XCreateColormap( window->xDisplay, window->xRoot, window->xVisual->visual, AllocNone );

	const unsigned long wamask = CWColormap | CWEventMask | ( window->windowFullscreen ? 0 : CWBorderPixel );

	XSetWindowAttributes wa;
	memset( &wa, 0, sizeof( wa ) );
	wa.colormap = window->xColormap;
	wa.border_pixel = 0;
	wa.event_mask = StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask |
					KeyPressMask | KeyReleaseMask |
					ButtonPressMask | ButtonReleaseMask |
					FocusChangeMask | ExposureMask | VisibilityChangeMask |
					EnterWindowMask | LeaveWindowMask;

	window->xWindow = XCreateWindow(	window->xDisplay,		// Display * display
										window->xRoot,			// Window parent
										0,						// int x
										0,						// int y
										window->windowWidth,	// unsigned int width
										window->windowHeight,	// unsigned int height
										0,						// unsigned int border_width
										window->xVisual->depth,	// int depth
										InputOutput,			// unsigned int class
										window->xVisual->visual,// Visual * visual
										wamask,					// unsigned long valuemask
										&wa );					// XSetWindowAttributes * attributes

	if ( !window->xWindow )
	{
		Error( "Failed to create window." );
		GpuWindow_Destroy( window );
		return false;
	}

	// Change the window title.
	Atom _NET_WM_NAME = XInternAtom( window->xDisplay, "_NET_WM_NAME", False );
	XChangeProperty( window->xDisplay, window->xWindow, _NET_WM_NAME,
						XA_STRING, 8, PropModeReplace,
						(const unsigned char *)WINDOW_TITLE, strlen( WINDOW_TITLE ) );

	if ( window->windowFullscreen )
	{
		// Bypass the compositor in fullscreen mode.
		const unsigned long bypass = 1;
		Atom _NET_WM_BYPASS_COMPOSITOR = XInternAtom( window->xDisplay, "_NET_WM_BYPASS_COMPOSITOR", False );
		XChangeProperty( window->xDisplay, window->xWindow, _NET_WM_BYPASS_COMPOSITOR,
							XA_CARDINAL, 32, PropModeReplace, (const unsigned char*)&bypass, 1 );

		// Completely dissasociate window from window manager.
		XSetWindowAttributes attributes;
		attributes.override_redirect = True;
		XChangeWindowAttributes( window->xDisplay, window->xWindow, CWOverrideRedirect, &attributes );

		// Make the window visible.
		XMapRaised( window->xDisplay, window->xWindow );
		XMoveResizeWindow( window->xDisplay, window->xWindow, 0, 0, window->windowWidth, window->windowHeight );
		XFlush( window->xDisplay );

		// Grab mouse and keyboard input now that the window is disassociated from the window manager.
		XGrabPointer( window->xDisplay, window->xWindow, True, 0, GrabModeAsync, GrabModeAsync, window->xWindow, 0L, CurrentTime );
		XGrabKeyboard( window->xDisplay, window->xWindow, True, GrabModeAsync, GrabModeAsync, CurrentTime );
	}
	else
	{
		// Make the window fixed size.
		XSizeHints * hints = XAllocSizeHints();
		hints->flags = ( PMinSize | PMaxSize );
		hints->min_width = window->windowWidth;
		hints->max_width = window->windowWidth;
		hints->min_height = window->windowHeight;
		hints->max_height = window->windowHeight;
		XSetWMNormalHints( window->xDisplay, window->xWindow, hints );
		XFree( hints );

		// First map the window and then center the window on the screen.
		XMapRaised( window->xDisplay, window->xWindow );
		const int x = ( window->desktopWidth - window->windowWidth ) / 2;
		const int y = ( window->desktopHeight - window->windowHeight ) / 2;
		XMoveResizeWindow( window->xDisplay, window->xWindow, x, y, window->windowWidth, window->windowHeight );
		XFlush( window->xDisplay );
	}

	window->context.glxDrawable = window->xWindow;

	GpuContext_SetCurrent( &window->context );

	return true;
}

static bool GpuWindow_SupportedResolution( const int width, const int height )
{
	UNUSED_PARM( width );
	UNUSED_PARM( height );

	return true;
}

static void GpuWindow_Exit( GpuWindow_t * window )
{
	window->windowExit = true;
}

static GpuWindowEvent_t GpuWindow_ProcessEvents( GpuWindow_t * window )
{
	int count = XPending( window->xDisplay );
	for ( int i = 0; i < count; i++ )
	{
		XEvent event;
		XNextEvent( window->xDisplay, &event );

		switch ( event.type )
		{
			case KeyPress:
			{
				KeySym key = XLookupKeysym( &event.xkey, 0 );
				if ( key < 256 || key == XK_Escape )
				{
					window->input.keyInput[key & 255] = true;
				}
				break;
			}
			case KeyRelease:
			{
				break;
			}
			case ButtonPress:
			{
				window->input.mouseInput[event.xbutton.button] = true;
				window->input.mouseInputX[event.xbutton.button] = event.xbutton.x;
				window->input.mouseInputY[event.xbutton.button] = event.xbutton.y;
			}
			case ButtonRelease:
			{
				break;
			}
			// StructureNotifyMask
			case ConfigureNotify:
			case MapNotify:
			case UnmapNotify:
			case DestroyNotify:
			// PropertyChangeMask
			case PropertyNotify:
			// ResizeRedirectMask
			case ResizeRequest:
			// EnterWindowMask | LeaveWindowMask
			case EnterNotify:
			case LeaveNotify:
			// FocusChangeMask
			case FocusIn:
			case FocusOut:
			// ExposureMask
			case Expose:
			// VisibilityChangeMask
			case VisibilityNotify:

			case GenericEvent:
			default: break;
		}
	}

	if ( window->windowExit )
	{
		return GPU_WINDOW_EVENT_EXIT;
	}

	if ( window->windowActive == false )
	{
		window->windowActive = true;
		return GPU_WINDOW_EVENT_ACTIVATED;
	}

	return GPU_WINDOW_EVENT_NONE;
}

#elif defined( OS_LINUX_XCB ) || defined( OS_LINUX_XCB_GLX )

typedef enum	// keysym.h
{
	KEY_A				= XK_a,
	KEY_B				= XK_b,
	KEY_C				= XK_c,
	KEY_D				= XK_d,
	KEY_E				= XK_e,
	KEY_F				= XK_f,
	KEY_G				= XK_g,
	KEY_H				= XK_h,
	KEY_I				= XK_i,
	KEY_J				= XK_j,
	KEY_K				= XK_k,
	KEY_L				= XK_l,
	KEY_M				= XK_m,
	KEY_N				= XK_n,
	KEY_O				= XK_o,
	KEY_P				= XK_p,
	KEY_Q				= XK_q,
	KEY_R				= XK_r,
	KEY_S				= XK_s,
	KEY_T				= XK_t,
	KEY_U				= XK_u,
	KEY_V				= XK_v,
	KEY_W				= XK_w,
	KEY_X				= XK_x,
	KEY_Y				= XK_y,
	KEY_Z				= XK_z,
	KEY_RETURN			= ( XK_Return & 0xFF ),
	KEY_TAB				= ( XK_Tab & 0xFF ),
	KEY_ESCAPE			= ( XK_Escape & 0xFF ),
	KEY_SHIFT_LEFT		= ( XK_Shift_L & 0xFF ),
	KEY_CTRL_LEFT		= ( XK_Control_L & 0xFF ),
	KEY_ALT_LEFT		= ( XK_Alt_L & 0xFF ),
	KEY_CURSOR_UP		= ( XK_Up & 0xFF ),
	KEY_CURSOR_DOWN		= ( XK_Down & 0xFF ),
	KEY_CURSOR_LEFT		= ( XK_Left & 0xFF ),
	KEY_CURSOR_RIGHT	= ( XK_Right & 0xFF )
} KeyboardKey_t;

typedef enum
{
	MOUSE_LEFT		= 0,
	MOUSE_RIGHT		= 1
} MouseButton_t;

typedef enum
{
	XCB_SIZE_HINT_US_POSITION	= 1 << 0,
	XCB_SIZE_HINT_US_SIZE		= 1 << 1,
	XCB_SIZE_HINT_P_POSITION	= 1 << 2,
	XCB_SIZE_HINT_P_SIZE		= 1 << 3,
	XCB_SIZE_HINT_P_MIN_SIZE	= 1 << 4,
	XCB_SIZE_HINT_P_MAX_SIZE	= 1 << 5,
	XCB_SIZE_HINT_P_RESIZE_INC	= 1 << 6,
	XCB_SIZE_HINT_P_ASPECT		= 1 << 7,
	XCB_SIZE_HINT_BASE_SIZE		= 1 << 8,
	XCB_SIZE_HINT_P_WIN_GRAVITY	= 1 << 9
} xcb_size_hints_flags_t;

static const int _NET_WM_STATE_REMOVE	= 0;	// remove/unset property
static const int _NET_WM_STATE_ADD		= 1;	// add/set property
static const int _NET_WM_STATE_TOGGLE	= 2;	// toggle property

/*
	Change video mode using the RandR X extension version 1.4

	The following code does not necessarily work out of the box, because on
	some configurations the modes list returned by XRRGetScreenResources()
	is populated with nothing other than the maximum display resolution,
	even though XF86VidModeGetAllModeLines() and XRRConfigSizes() *will*
	list all resolutions for the same display.

	The user can manually add new modes from the command-line using the
	xrandr utility:

	xrandr --newmode <modeline>

	Where <modeline> is generated with a utility that implements either
	the General Timing Formula (GTF) or the Coordinated Video Timing (CVT)
	standard put forth by the Video Electronics Standards Association (VESA):

	gft <width> <height> <Hz>	// http://gtf.sourceforge.net/
	cvt <width> <height> <Hz>	// http://www.uruk.org/~erich/projects/cvt/

	Alternatively, new modes can be added in code using XRRCreateMode().
	However, this requires calculating all the timing information in code
	because there is no standard library that implements the GTF or CVT.
*/
static bool ChangeVideoMode_XcbRandR_1_4( xcb_connection_t * connection, xcb_screen_t * screen,
								int * currentWidth, int * currentHeight, float * currentRefreshRate,
								int * desiredWidth, int * desiredHeight, float * desiredRefreshRate )
{
	/*
		Screen	- virtual screenspace which may be covered by multiple CRTCs
		CRTC	- display controller
		Output	- display/monitor connected to a CRTC
		Clones	- outputs that are simultaneously connected to the same CRTC
	*/

	xcb_randr_get_screen_resources_cookie_t screen_resources_cookie = xcb_randr_get_screen_resources( connection, screen->root );
	xcb_randr_get_screen_resources_reply_t * screen_resources_reply = xcb_randr_get_screen_resources_reply( connection, screen_resources_cookie, 0 );
	if ( screen_resources_reply == NULL )
	{
		return false;
	}

	xcb_randr_mode_info_t * mode_info = xcb_randr_get_screen_resources_modes( screen_resources_reply );
	const int modes_length = xcb_randr_get_screen_resources_modes_length( screen_resources_reply );
	assert( modes_length > 0 );
	
	xcb_randr_crtc_t * crtcs = xcb_randr_get_screen_resources_crtcs( screen_resources_reply );
	const int crtcs_length = xcb_randr_get_screen_resources_crtcs_length( screen_resources_reply );
	assert( crtcs_length > 0 );
	UNUSED_PARM( crtcs_length );

	const int PRIMARY_CRTC_INDEX = 0;
	const int PRIMARY_OUTPUT_INDEX = 0;

	xcb_randr_get_crtc_info_cookie_t primary_crtc_info_cookie = xcb_randr_get_crtc_info( connection, crtcs[PRIMARY_CRTC_INDEX], 0 );
	xcb_randr_get_crtc_info_reply_t * primary_crtc_info_reply = xcb_randr_get_crtc_info_reply( connection, primary_crtc_info_cookie, NULL );

	xcb_randr_output_t * crtc_outputs = xcb_randr_get_crtc_info_outputs( primary_crtc_info_reply );

	xcb_randr_get_output_info_cookie_t primary_output_info_cookie = xcb_randr_get_output_info( connection, crtc_outputs[PRIMARY_OUTPUT_INDEX], 0 );
	xcb_randr_get_output_info_reply_t * primary_output_info_reply = xcb_randr_get_output_info_reply( connection, primary_output_info_cookie, NULL );

	if ( currentWidth != NULL && currentHeight != NULL && currentRefreshRate != NULL )
	{
		for ( int i = 0; i < modes_length; i++ )
		{
			if ( mode_info[i].id == primary_crtc_info_reply->mode )
			{
				*currentWidth = mode_info[i].width;
				*currentHeight = mode_info[i].height;
				*currentRefreshRate = mode_info[i].dot_clock / ( (float)mode_info[i].htotal * (float)mode_info[i].vtotal );
				break;
			}
		}
	}

	if ( desiredWidth != NULL && desiredHeight != NULL && desiredRefreshRate != NULL )
	{
		xcb_randr_mode_t bestMode = 0;
		int bestModeWidth = 0;
		int bestModeHeight = 0;
		float bestModeRefreshRate = 0.0f;
		int bestSizeError = 0x7FFFFFFF;
		float bestRefreshRateError = 1e6f;
		for ( int i = 0; i < modes_length; i++ )
		{
			if ( mode_info[i].mode_flags & XCB_RANDR_MODE_FLAG_INTERLACE )
			{
				continue;
			}

			xcb_randr_mode_t * primary_output_info_modes = xcb_randr_get_output_info_modes( primary_output_info_reply );
			int primary_output_info_modes_length = xcb_randr_get_output_info_modes_length( primary_output_info_reply );

			bool validOutputMode = false;
			for ( int j = 0; j < primary_output_info_modes_length; j++ )
			{
				if ( mode_info[i].id == primary_output_info_modes[j] )
				{
					validOutputMode = true;
					break;
				}
			}
			if ( !validOutputMode )
			{
				continue;
			}

			const int modeWidth = mode_info[i].width;
			const int modeHeight = mode_info[i].height;
			const float modeRefreshRate = mode_info[i].dot_clock / ( (float)mode_info[i].htotal * (float)mode_info[i].vtotal );

			const int dw = modeWidth - *desiredWidth;
			const int dh = modeHeight - *desiredHeight;
			const int sizeError = dw * dw + dh * dh;
			const float refreshRateError = fabs( modeRefreshRate - *desiredRefreshRate );
			if ( sizeError < bestSizeError || ( sizeError == bestSizeError && refreshRateError < bestRefreshRateError ) )
			{
				bestSizeError = sizeError;
				bestRefreshRateError = refreshRateError;
				bestMode = mode_info[i].id;
				bestModeWidth = modeWidth;
				bestModeHeight = modeHeight;
				bestModeRefreshRate = modeRefreshRate;
			}
		}

		xcb_randr_output_t * primary_crtc_info_outputs = xcb_randr_get_crtc_info_outputs( primary_crtc_info_reply );
		int primary_crtc_info_outputs_length = xcb_randr_get_crtc_info_outputs_length( primary_crtc_info_reply );

		xcb_randr_set_crtc_config( connection, primary_output_info_reply->crtc, XCB_TIME_CURRENT_TIME, XCB_TIME_CURRENT_TIME,
									primary_crtc_info_reply->x, primary_crtc_info_reply->y, bestMode, primary_crtc_info_reply->rotation,
									primary_crtc_info_outputs_length, primary_crtc_info_outputs );

		*desiredWidth = bestModeWidth;
		*desiredHeight = bestModeHeight;
		*desiredRefreshRate = bestModeRefreshRate;
	}

	free( primary_output_info_reply );
	free( primary_crtc_info_reply );
	free( screen_resources_reply );

	return true;
}

static void GpuWindow_Destroy( GpuWindow_t * window )
{
	GpuContext_Destroy( &window->context );
	GpuDevice_Destroy( &window->device );

#if defined( OS_LINUX_XCB_GLX )
	glXDestroyWindow( window->xDisplay, window->glxWindow );
	XFlush( window->xDisplay );
	XCloseDisplay( window->xDisplay );
	window->xDisplay = NULL;
#else
	xcb_glx_delete_window( window->connection, window->glxWindow );
#endif

	if ( window->windowFullscreen )
	{
		ChangeVideoMode_XcbRandR_1_4( window->connection, window->screen,
									NULL, NULL, NULL,
									&window->desktopWidth, &window->desktopHeight, &window->desktopRefreshRate );
	}

	xcb_destroy_window( window->connection, window->window );
	xcb_free_colormap( window->connection, window->colormap );
	xcb_flush( window->connection );
	xcb_disconnect( window->connection );
	xcb_key_symbols_free( window->key_symbols );
}

static bool GpuWindow_Create( GpuWindow_t * window, DriverInstance_t * instance,
								const GpuQueueInfo_t * queueInfo, const int queueIndex,
								const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
								const GpuSampleCount_t sampleCount, const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( GpuWindow_t ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->sampleCount = sampleCount;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = fullscreen;
	window->windowActive = false;
	window->windowExit = false;
	window->lastSwapTime = GetTimeMicroseconds();

	const char * displayName = NULL;
	int screen_number = 0;
	window->connection = xcb_connect( displayName, &screen_number );
	if ( xcb_connection_has_error( window->connection ) )
	{
		GpuWindow_Destroy( window );
		Error( "Failed to open XCB connection." );
		return false;
	}

	const xcb_setup_t * setup = xcb_get_setup( window->connection );
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator( setup );
	for ( int i = 0; i < screen_number; i++ )
	{
		xcb_screen_next( &iter );
	}
	window->screen = iter.data;

	if ( window->windowFullscreen )
	{
		ChangeVideoMode_XcbRandR_1_4( window->connection, window->screen,
									&window->desktopWidth, &window->desktopHeight, &window->desktopRefreshRate,
									&window->windowWidth, &window->windowHeight, &window->windowRefreshRate );
	}
	else
	{
		ChangeVideoMode_XcbRandR_1_4( window->connection, window->screen,
									&window->desktopWidth, &window->desktopHeight, &window->desktopRefreshRate,
									NULL, NULL, NULL );
		window->windowRefreshRate = window->desktopRefreshRate;
	}

	GpuDevice_Create( &window->device, instance, queueInfo );
#if defined( OS_LINUX_XCB_GLX )
	window->xDisplay = XOpenDisplay( displayName );
	GpuContext_CreateForSurface( &window->context, &window->device, queueIndex, colorFormat, depthFormat, sampleCount, window->xDisplay, screen_number );
#else
	GpuContext_CreateForSurface( &window->context, &window->device, queueIndex, colorFormat, depthFormat, sampleCount, window->connection, screen_number );
#endif

	// Create the color map.
	window->colormap = xcb_generate_id( window->connection );
	xcb_create_colormap( window->connection, XCB_COLORMAP_ALLOC_NONE, window->colormap, window->screen->root, window->context.visualid );

	// Create the window.
	uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
	uint32_t value_list[5];
	value_list[0] = window->screen->black_pixel;
	value_list[1] = 0;
	value_list[2] = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_BUTTON_PRESS;
	value_list[3] = window->colormap;
	value_list[4] = 0;

	window->window = xcb_generate_id( window->connection );
	xcb_create_window(	window->connection,				// xcb_connection_t *	connection
						XCB_COPY_FROM_PARENT,			// uint8_t				depth
						window->window,					// xcb_window_t			wid
						window->screen->root,			// xcb_window_t			parent
						0,								// int16_t				x
						0,								// int16_t				y
						window->windowWidth,			// uint16_t				width
						window->windowHeight,			// uint16_t				height
						0,								// uint16_t				border_width
						XCB_WINDOW_CLASS_INPUT_OUTPUT,	// uint16_t				_class
						window->context.visualid,		// xcb_visualid_t		visual
						value_mask,						// uint32_t				value_mask
						value_list );					// const uint32_t *		value_list

	// Change the window title.
	xcb_change_property( window->connection, XCB_PROP_MODE_REPLACE, window->window,
						XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
						8, strlen( WINDOW_TITLE ), WINDOW_TITLE );

	// Setup code that will send a notification when the window is destroyed.
	xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom( window->connection, 1, 12, "WM_PROTOCOLS" );
	xcb_intern_atom_cookie_t wm_delete_window_cookie = xcb_intern_atom( window->connection, 0, 16, "WM_DELETE_WINDOW" );
	xcb_intern_atom_reply_t * wm_protocols_reply = xcb_intern_atom_reply( window->connection, wm_protocols_cookie, 0 );
	xcb_intern_atom_reply_t * wm_delete_window_reply = xcb_intern_atom_reply( window->connection, wm_delete_window_cookie, 0 );

	window->wm_delete_window_atom = wm_delete_window_reply->atom;
	xcb_change_property( window->connection, XCB_PROP_MODE_REPLACE, window->window,
						wm_protocols_reply->atom, XCB_ATOM_ATOM,
						32, 1, &wm_delete_window_reply->atom );

	free( wm_protocols_reply );
	free( wm_delete_window_reply );

	if ( window->windowFullscreen )
	{
		// Change the window to fullscreen
		xcb_intern_atom_cookie_t wm_state_cookie = xcb_intern_atom( window->connection, 0, 13, "_NET_WM_STATE" );
		xcb_intern_atom_cookie_t wm_state_fullscreen_cookie = xcb_intern_atom( window->connection, 0, 24, "_NET_WM_STATE_FULLSCREEN" );
		xcb_intern_atom_reply_t * wm_state_reply = xcb_intern_atom_reply( window->connection, wm_state_cookie, 0 );
		xcb_intern_atom_reply_t * wm_state_fullscreen_reply = xcb_intern_atom_reply( window->connection, wm_state_fullscreen_cookie, 0 );

		xcb_client_message_event_t ev;
		ev.response_type = XCB_CLIENT_MESSAGE;
		ev.format = 32;
		ev.sequence = 0;
		ev.window = window->window;
		ev.type = wm_state_reply->atom;
		ev.data.data32[0] = _NET_WM_STATE_ADD;
		ev.data.data32[1] = wm_state_fullscreen_reply->atom;
		ev.data.data32[2] = XCB_ATOM_NONE;
		ev.data.data32[3] = 0;
		ev.data.data32[4] = 0;

		xcb_send_event(	window->connection, 1, window->window,
						XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
						(const char*)(&ev) );

		free( wm_state_reply );
		free( wm_state_fullscreen_reply );

		xcb_map_window( window->connection, window->window );
		xcb_flush( window->connection );
	}
	else
	{
		// Make the window fixed size.
		xcb_size_hints_t hints;
		memset( &hints, 0, sizeof( hints ) );
		hints.flags = XCB_SIZE_HINT_US_SIZE | XCB_SIZE_HINT_P_SIZE | XCB_SIZE_HINT_P_MIN_SIZE | XCB_SIZE_HINT_P_MAX_SIZE;
		hints.min_width = window->windowWidth;
		hints.max_width = window->windowWidth;
		hints.min_height = window->windowHeight;
		hints.max_height = window->windowHeight;

		xcb_change_property( window->connection, XCB_PROP_MODE_REPLACE, window->window,
							XCB_ATOM_WM_NORMAL_HINTS, XCB_ATOM_WM_SIZE_HINTS,
							32, sizeof( hints ) / 4, &hints );

		// First map the window and then center the window on the screen.
		xcb_map_window( window->connection, window->window );
		const uint32_t coords[] =
		{
			( window->desktopWidth - window->windowWidth ) / 2,
			( window->desktopHeight - window->windowHeight ) / 2
		};
		xcb_configure_window( window->connection, window->window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords );
		xcb_flush( window->connection );
	}

	window->key_symbols = xcb_key_symbols_alloc( window->connection );

#if defined( OS_LINUX_XCB_GLX )
	window->glxWindow = glXCreateWindow( window->xDisplay, window->context.glxFBConfig, window->window, NULL );
#else
	window->glxWindow = xcb_generate_id( window->connection );
	xcb_glx_create_window( window->connection, screen_number, window->context.fbconfigid, window->window, window->glxWindow, 0, NULL );
#endif

	window->context.glxDrawable = window->glxWindow;

	GpuContext_SetCurrent( &window->context );

	return true;
}

static bool GpuWindow_SupportedResolution( const int width, const int height )
{
	UNUSED_PARM( width );
	UNUSED_PARM( height );

	return true;
}

static void GpuWindow_Exit( GpuWindow_t * window )
{
	window->windowExit = true;
}

static GpuWindowEvent_t GpuWindow_ProcessEvents( GpuWindow_t * window )
{
	xcb_generic_event_t * event = xcb_poll_for_event( window->connection );
	if ( event != NULL )
	{
		const uint8_t event_code = ( event->response_type & 0x7f );
		switch ( event_code )
		{
			case XCB_CLIENT_MESSAGE:
			{
				const xcb_client_message_event_t * client_message_event = (const xcb_client_message_event_t *) event;
				if ( client_message_event->data.data32[0] == window->wm_delete_window_atom )
				{
					free( event );
					return GPU_WINDOW_EVENT_EXIT;
				}
				break;
			}
			case XCB_KEY_PRESS:
			{
				xcb_key_press_event_t * key_press_event = (xcb_key_press_event_t *) event;
				const xcb_keysym_t keysym = xcb_key_press_lookup_keysym( window->key_symbols, key_press_event, 0 );
				if ( keysym < 256 || keysym == XK_Escape )
				{
					window->input.keyInput[keysym & 255] = true;
				}
				break;
			}
			case XCB_BUTTON_PRESS:
			{
				const xcb_button_press_event_t * button_press_event = (const xcb_button_press_event_t *) event;
				const int masks[5] = { XCB_BUTTON_MASK_1, XCB_BUTTON_MASK_2, XCB_BUTTON_MASK_3, XCB_BUTTON_MASK_4, XCB_BUTTON_MASK_5 };
				for ( int i = 0; i < 5; i++ )
				{
					if ( ( button_press_event->state & masks[i] ) != 0 )
					{
						window->input.mouseInput[i] = true;
						window->input.mouseInputX[i] = button_press_event->event_x;
						window->input.mouseInputY[i] = button_press_event->event_y;
					}
				}
				break;
			}
			default:
			{
				break;
			}
		}
		free( event );
	}

	if ( window->windowExit )
	{
		return GPU_WINDOW_EVENT_EXIT;
	}

	if ( window->windowActive == false )
	{
		window->windowActive = true;
		return GPU_WINDOW_EVENT_ACTIVATED;
	}

	return GPU_WINDOW_EVENT_NONE;
}

#elif defined( OS_ANDROID )

typedef enum	// https://developer.android.com/ndk/reference/group___input.html
{
	KEY_A				= AKEYCODE_A,
	KEY_B				= AKEYCODE_B,
	KEY_C				= AKEYCODE_C,
	KEY_D				= AKEYCODE_D,
	KEY_E				= AKEYCODE_E,
	KEY_F				= AKEYCODE_F,
	KEY_G				= AKEYCODE_G,
	KEY_H				= AKEYCODE_H,
	KEY_I				= AKEYCODE_I,
	KEY_J				= AKEYCODE_J,
	KEY_K				= AKEYCODE_K,
	KEY_L				= AKEYCODE_L,
	KEY_M				= AKEYCODE_M,
	KEY_N				= AKEYCODE_N,
	KEY_O				= AKEYCODE_O,
	KEY_P				= AKEYCODE_P,
	KEY_Q				= AKEYCODE_Q,
	KEY_R				= AKEYCODE_R,
	KEY_S				= AKEYCODE_S,
	KEY_T				= AKEYCODE_T,
	KEY_U				= AKEYCODE_U,
	KEY_V				= AKEYCODE_V,
	KEY_W				= AKEYCODE_W,
	KEY_X				= AKEYCODE_X,
	KEY_Y				= AKEYCODE_Y,
	KEY_Z				= AKEYCODE_Z,
	KEY_RETURN			= AKEYCODE_ENTER,
	KEY_TAB				= AKEYCODE_TAB,
	KEY_ESCAPE			= AKEYCODE_ESCAPE,
	KEY_SHIFT_LEFT		= AKEYCODE_SHIFT_LEFT,
	KEY_CTRL_LEFT		= AKEYCODE_CTRL_LEFT,
	KEY_ALT_LEFT		= AKEYCODE_ALT_LEFT,
	KEY_CURSOR_UP		= AKEYCODE_DPAD_UP,
	KEY_CURSOR_DOWN		= AKEYCODE_DPAD_DOWN,
	KEY_CURSOR_LEFT		= AKEYCODE_DPAD_LEFT,
	KEY_CURSOR_RIGHT	= AKEYCODE_DPAD_RIGHT
} KeyboardKey_t;

typedef enum
{
	MOUSE_LEFT		= 0,
	MOUSE_RIGHT		= 1
} MouseButton_t;

static void app_handle_cmd( struct android_app * app, int32_t cmd )
{
	GpuWindow_t * window = (GpuWindow_t *)app->userData;

	switch ( cmd )
	{
		// There is no APP_CMD_CREATE. The ANativeActivity creates the
		// application thread from onCreate(). The application thread
		// then calls android_main().
		case APP_CMD_START:
		{
			Print( "onStart()" );
			Print( "    APP_CMD_START" );
			break;
		}
		case APP_CMD_RESUME:
		{
			Print( "onResume()" );
			Print( "    APP_CMD_RESUME" );
			window->resumed = true;
			break;
		}
		case APP_CMD_PAUSE:
		{
			Print( "onPause()" );
			Print( "    APP_CMD_PAUSE" );
			window->resumed = false;
			break;
		}
		case APP_CMD_STOP:
		{
			Print( "onStop()" );
			Print( "    APP_CMD_STOP" );
			break;
		}
		case APP_CMD_DESTROY:
		{
			Print( "onDestroy()" );
			Print( "    APP_CMD_DESTROY" );
			window->nativeWindow = NULL;
			break;
		}
		case APP_CMD_INIT_WINDOW:
		{
			Print( "surfaceCreated()" );
			Print( "    APP_CMD_INIT_WINDOW" );
			window->nativeWindow = app->window;
			break;
		}
		case APP_CMD_TERM_WINDOW:
		{
			Print( "surfaceDestroyed()" );
			Print( "    APP_CMD_TERM_WINDOW" );
			window->nativeWindow = NULL;
			break;
		}
    }
}

static int32_t app_handle_input( struct android_app * app, AInputEvent * event )
{
	GpuWindow_t * window = (GpuWindow_t *)app->userData;

	const int type = AInputEvent_getType( event );
	if ( type == AINPUT_EVENT_TYPE_KEY )
	{
		int keyCode = AKeyEvent_getKeyCode( event );
		const int action = AKeyEvent_getAction( event );
		if ( action == AKEY_EVENT_ACTION_DOWN )
		{
			// Translate controller input to useful keys.
			switch ( keyCode )
			{
				case AKEYCODE_BUTTON_A: keyCode = AKEYCODE_Q; break;
				case AKEYCODE_BUTTON_B: keyCode = AKEYCODE_W; break;
				case AKEYCODE_BUTTON_X: keyCode = AKEYCODE_E; break;
				case AKEYCODE_BUTTON_Y: keyCode = AKEYCODE_M; break;
				case AKEYCODE_BUTTON_START: keyCode = AKEYCODE_L; break;
				case AKEYCODE_BUTTON_SELECT: keyCode = AKEYCODE_ESCAPE; break;
			}
			if ( keyCode >= 0 && keyCode < 256 )
			{
				window->input.keyInput[keyCode] = true;
				return 1;
			}
		}
		return 0;
	}
	else if ( type == AINPUT_EVENT_TYPE_MOTION )
	{
		const int source = AInputEvent_getSource( event );
		// Events with source == AINPUT_SOURCE_TOUCHSCREEN come from the phone's builtin touch screen.
		// Events with source == AINPUT_SOURCE_MOUSE come from the trackpad on the right side of the GearVR.
		if ( source == AINPUT_SOURCE_TOUCHSCREEN || source == AINPUT_SOURCE_MOUSE )
		{
			const int action = AKeyEvent_getAction( event ) & AMOTION_EVENT_ACTION_MASK;
			const float x = AMotionEvent_getRawX( event, 0 );
			const float y = AMotionEvent_getRawY( event, 0 );
			if ( action == AMOTION_EVENT_ACTION_UP )
			{
				window->input.mouseInput[MOUSE_LEFT] = true;
				window->input.mouseInputX[MOUSE_LEFT] = (int)x;
				window->input.mouseInputY[MOUSE_LEFT] = (int)y;
				return 1;
			}
			return 0;
		}
	}
	return 0;
}

static void GpuWindow_Destroy( GpuWindow_t * window )
{
	GpuContext_Destroy( &window->context );
	GpuDevice_Destroy( &window->device );

	if ( window->display != 0 )
	{
		EGL( eglTerminate( window->display ) );
		window->display = 0;
	}

	if ( window->app != NULL )
	{
		(*window->java.vm)->DetachCurrentThread( window->java.vm );
		window->java.vm = NULL;
		window->java.env = NULL;
		window->java.activity = 0;
	}
}

static float GetDisplayRefreshRate( const Java_t * java )
{
	// Retrieve Context.WINDOW_SERVICE.
	jclass contextClass = (*java->env)->FindClass( java->env, "android/content/Context" );
	jfieldID field_WINDOW_SERVICE = (*java->env)->GetStaticFieldID( java->env, contextClass, "WINDOW_SERVICE", "Ljava/lang/String;" );
	jobject WINDOW_SERVICE = (*java->env)->GetStaticObjectField( java->env, contextClass, field_WINDOW_SERVICE );
	(*java->env)->DeleteLocalRef( java->env, contextClass );

	// WindowManager windowManager = (WindowManager) activity.getSystemService( Context.WINDOW_SERVICE );
	const jclass activityClass = (*java->env)->GetObjectClass( java->env, java->activity );
	const jmethodID getSystemServiceMethodId = (*java->env)->GetMethodID( java->env, activityClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
	const jobject windowManager = (*java->env)->CallObjectMethod( java->env, java->activity, getSystemServiceMethodId, WINDOW_SERVICE );
	(*java->env)->DeleteLocalRef( java->env, activityClass );

	// Display display = windowManager.getDefaultDisplay();
	const jclass windowManagerClass = (*java->env)->GetObjectClass( java->env, windowManager );
	const jmethodID getDefaultDisplayMethodId = (*java->env)->GetMethodID( java->env, windowManagerClass, "getDefaultDisplay", "()Landroid/view/Display;" );
	const jobject display = (*java->env)->CallObjectMethod( java->env, windowManager, getDefaultDisplayMethodId );
	(*java->env)->DeleteLocalRef( java->env, windowManagerClass );

	// float refreshRate = display.getRefreshRate();
	const jclass displayClass = (*java->env)->GetObjectClass( java->env, display );
	const jmethodID getRefreshRateMethodId = (*java->env)->GetMethodID( java->env, displayClass, "getRefreshRate", "()F" );
	const float refreshRate = (*java->env)->CallFloatMethod( java->env, display, getRefreshRateMethodId );
	(*java->env)->DeleteLocalRef( java->env, displayClass );

	(*java->env)->DeleteLocalRef( java->env, display );
	(*java->env)->DeleteLocalRef( java->env, windowManager );
	(*java->env)->DeleteLocalRef( java->env, WINDOW_SERVICE );

	return refreshRate;
}

struct android_app * global_app;

static bool GpuWindow_Create( GpuWindow_t * window, DriverInstance_t * instance,
								const GpuQueueInfo_t * queueInfo, const int queueIndex,
								const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
								const GpuSampleCount_t sampleCount, const int width, const int height, const bool fullscreen )
{
	memset( window, 0, sizeof( GpuWindow_t ) );

	window->colorFormat = colorFormat;
	window->depthFormat = depthFormat;
	window->sampleCount = sampleCount;
	window->windowWidth = width;
	window->windowHeight = height;
	window->windowSwapInterval = 1;
	window->windowRefreshRate = 60.0f;
	window->windowFullscreen = true;
	window->windowActive = false;
	window->windowExit = false;
	window->lastSwapTime = GetTimeMicroseconds();

	window->app = global_app;
	window->nativeWindow = NULL;
	window->resumed = false;

	if ( window->app != NULL )
	{
		window->app->userData = window;
		window->app->onAppCmd = app_handle_cmd;
		window->app->onInputEvent = app_handle_input;
		window->java.vm = window->app->activity->vm;
		(*window->java.vm)->AttachCurrentThread( window->java.vm, &window->java.env, NULL );
		window->java.activity = window->app->activity->clazz;

		window->windowRefreshRate = GetDisplayRefreshRate( &window->java );

		// Keep the display on and bright.
		// Also make sure there is only one "HWC" next to the "FB TARGET" (adb shell dumpsys SurfaceFlinger).
		ANativeActivity_setWindowFlags( window->app->activity, AWINDOW_FLAG_FULLSCREEN | AWINDOW_FLAG_KEEP_SCREEN_ON, 0 );
	}

	window->display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
	EGL( eglInitialize( window->display, &window->majorVersion, &window->minorVersion ) );

	GpuDevice_Create( &window->device, instance, queueInfo );
	GpuContext_CreateForSurface( &window->context, &window->device, queueIndex, colorFormat, depthFormat, sampleCount, window->display );
	GpuContext_SetCurrent( &window->context );

	GlInitExtensions();

	return true;
}

static bool GpuWindow_SupportedResolution( const int width, const int height )
{
	UNUSED_PARM( width );
	UNUSED_PARM( height );

	// Assume the HWC can handle any window size.
	return true;
}

static void GpuWindow_Exit( GpuWindow_t * window )
{
	// Call finish() on the activity and GpuWindow_ProcessEvents will handle the rest.
	ANativeActivity_finish( window->app->activity );
}

static GpuWindowEvent_t GpuWindow_ProcessEvents( GpuWindow_t * window )
{
	if ( window->app == NULL )
	{
		return GPU_WINDOW_EVENT_NONE;
	}

	const bool windowWasActive = window->windowActive;

	for ( ; ; )
	{
		int events;
		struct android_poll_source * source;
		const int timeoutMilliseconds = ( window->windowActive == false && window->app->destroyRequested == 0 ) ? -1 : 0;
		if ( ALooper_pollAll( timeoutMilliseconds, NULL, &events, (void **)&source ) < 0 )
		{
			break;
		}

		if ( source != NULL )
		{
			source->process( window->app, source );
		}

		if ( window->nativeWindow != NULL && window->context.mainSurface == window->context.tinySurface )
		{
			Print( "        ANativeWindow_setBuffersGeometry %d x %d", window->windowWidth, window->windowHeight );
			ANativeWindow_setBuffersGeometry( window->nativeWindow, window->windowWidth, window->windowHeight, 0 );

			const EGLint surfaceAttribs[] = { EGL_NONE };
			Print( "        mainSurface = eglCreateWindowSurface( nativeWindow )" );
			window->context.mainSurface = eglCreateWindowSurface( window->context.display, window->context.config, window->nativeWindow, surfaceAttribs );
			if ( window->context.mainSurface == EGL_NO_SURFACE )
			{
				Error( "        eglCreateWindowSurface() failed: %s", EglErrorString( eglGetError() ) );
				return GPU_WINDOW_EVENT_EXIT;
			}
			Print( "        eglMakeCurrent( mainSurface )" );
			EGL( eglMakeCurrent( window->context.display, window->context.mainSurface, window->context.mainSurface, window->context.context ) );

			eglQuerySurface( window->context.display, window->context.mainSurface, EGL_WIDTH, &window->windowWidth );
			eglQuerySurface( window->context.display, window->context.mainSurface, EGL_HEIGHT, &window->windowHeight );
		}

		if ( window->resumed != false && window->nativeWindow != NULL )
		{
			window->windowActive = true;
		}
		else
		{
			window->windowActive = false;
		}

		if ( window->nativeWindow == NULL && window->context.mainSurface != window->context.tinySurface )
		{
			Print( "        eglMakeCurrent( tinySurface )" );
			EGL( eglMakeCurrent( window->context.display, window->context.tinySurface, window->context.tinySurface, window->context.context ) );
			Print( "        eglDestroySurface( mainSurface )" );
			EGL( eglDestroySurface( window->context.display, window->context.mainSurface ) );
			window->context.mainSurface = window->context.tinySurface;
		}
	}

	if ( window->app->destroyRequested != 0 )
	{
		return GPU_WINDOW_EVENT_EXIT;
	}
	if ( windowWasActive != window->windowActive )
	{
		return ( window->windowActive ) ? GPU_WINDOW_EVENT_ACTIVATED : GPU_WINDOW_EVENT_DEACTIVATED;
	}
	return GPU_WINDOW_EVENT_NONE;
}

#endif

static void GpuWindow_SwapInterval( GpuWindow_t * window, const int swapInterval )
{
	if ( swapInterval != window->windowSwapInterval )
	{
#if defined( OS_WINDOWS )
		wglSwapIntervalEXT( swapInterval );
#elif defined( OS_APPLE_MACOS )
		CGLSetParameter( window->context.cglContext, kCGLCPSwapInterval, &swapInterval );
#elif defined( OS_LINUX_XLIB )
		glXSwapIntervalEXT( window->context.xDisplay, window->xWindow, swapInterval );
#elif defined( OS_LINUX_XCB )
		xcb_dri2_swap_interval( window->context.connection, window->context.glxDrawable, swapInterval );
#elif defined( OS_LINUX_XCB_GLX )
		glXSwapIntervalEXT( window->context.xDisplay, window->glxWindow, swapInterval );
#elif defined( OS_ANDROID )
		EGL( eglSwapInterval( window->context.display, swapInterval ) );
#endif
		window->windowSwapInterval = swapInterval;
	}
}

static void GpuWindow_SwapBuffers( GpuWindow_t * window )
{
#if defined( OS_WINDOWS )
	SwapBuffers( window->context.hDC );
#elif defined( OS_APPLE_MACOS )
	CGLFlushDrawable( window->context.cglContext );
#elif defined( OS_LINUX_XLIB )
	glXSwapBuffers( window->context.xDisplay, window->xWindow );
#elif defined( OS_LINUX_XCB )
	xcb_glx_swap_buffers( window->context.connection, window->context.glxContextTag, window->glxWindow );
#elif defined( OS_LINUX_XCB_GLX )
	glXSwapBuffers( window->context.xDisplay, window->glxWindow );
#elif defined( OS_ANDROID )
	EGL( eglSwapBuffers( window->context.display, window->context.mainSurface ) );
#endif

	Microseconds_t newTimeMicroseconds = GetTimeMicroseconds();

	// Even with smoothing, this is not particularly accurate.
	const float frameTimeMicroseconds = 1000.0f * 1000.0f / window->windowRefreshRate;
	const float deltaTimeMicroseconds = (float)newTimeMicroseconds - window->lastSwapTime - frameTimeMicroseconds;
	if ( fabs( deltaTimeMicroseconds ) < frameTimeMicroseconds * 0.75f )
	{
		newTimeMicroseconds = (Microseconds_t)( window->lastSwapTime + frameTimeMicroseconds + 0.025f * deltaTimeMicroseconds );
	}
	//const float smoothDeltaMicroseconds = (float)( newTimeMicroseconds - window->lastSwapTime );
	//Print( "frame delta = %1.3f (error = %1.3f)\n", smoothDeltaMicroseconds * ( 1.0f / 1000.0f ),
	//					( smoothDeltaMicroseconds - frameTimeMicroseconds ) * ( 1.0f / 1000.0f ) );
	window->lastSwapTime = newTimeMicroseconds;
}

static Microseconds_t GpuWindow_GetNextSwapTimeMicroseconds( GpuWindow_t * window )
{
	const float frameTimeMicroseconds = 1000.0f * 1000.0f / window->windowRefreshRate;
	return window->lastSwapTime + (Microseconds_t)( frameTimeMicroseconds );
}

static Microseconds_t GpuWindow_GetFrameTimeMicroseconds( GpuWindow_t * window )
{
	const float frameTimeMicroseconds = 1000.0f * 1000.0f / window->windowRefreshRate;
	return (Microseconds_t)( frameTimeMicroseconds );
}

static void GpuWindow_DelayBeforeSwap( GpuWindow_t * window, const Microseconds_t delay )
{
	UNUSED_PARM( window );
	UNUSED_PARM( delay );

	// FIXME: this appears to not only stall the calling context but also other contexts.
/*
#if defined( OS_WINDOWS )
	if ( wglDelayBeforeSwapNV != NULL )
	{
		wglDelayBeforeSwapNV( window->hDC, delay * 1e-6f );
	}
#elif defined( OS_LINUX_XLIB )
	if ( glXDelayBeforeSwapNV != NULL )
	{
		glXDelayBeforeSwapNV( window->hDC, delay * 1e-6f );
	}
#endif
*/
}

static bool GpuWindowInput_ConsumeKeyboardKey( GpuWindowInput_t * input, const KeyboardKey_t key )
{
	if ( input->keyInput[key] )
	{
		input->keyInput[key] = false;
		return true;
	}
	return false;
}

static bool GpuWindowInput_ConsumeMouseButton( GpuWindowInput_t * input, const MouseButton_t button )
{
	if ( input->mouseInput[button] )
	{
		input->mouseInput[button] = false;
		return true;
	}
	return false;
}

static bool GpuWindowInput_CheckKeyboardKey( GpuWindowInput_t * input, const KeyboardKey_t key )
{
	return ( input->keyInput[key] != false );
}

/*
================================================================================================================================

GPU buffer.

A buffer maintains a block of memory for a specific use by GPU programs (vertex, index, uniform, storage).
For optimal performance a buffer should only be created at load time, not at runtime.
The best performance is typically achieved when the buffer is not host visible.

GpuBufferType_t
GpuBuffer_t

static bool GpuBuffer_Create( GpuContext_t * context, GpuBuffer_t * buffer, const GpuBufferType_t type,
							const size_t dataSize, const void * data, const bool hostVisible );
static void GpuBuffer_Destroy( GpuContext_t * context, GpuBuffer_t * buffer );

================================================================================================================================
*/

typedef enum
{
	GPU_BUFFER_TYPE_VERTEX,
	GPU_BUFFER_TYPE_INDEX,
	GPU_BUFFER_TYPE_UNIFORM,
	GPU_BUFFER_TYPE_STORAGE
} GpuBufferType_t;

typedef struct
{
	GLuint			target;
	GLuint			buffer;
	size_t			size;
} GpuBuffer_t;

static bool GpuBuffer_Create( GpuContext_t * context, GpuBuffer_t * buffer, const GpuBufferType_t type,
							const size_t dataSize, const void * data, const bool hostVisible )
{
	UNUSED_PARM( context );
	UNUSED_PARM( hostVisible );

	buffer->target =	( ( type == GPU_BUFFER_TYPE_VERTEX ) ?	GL_ARRAY_BUFFER :
						( ( type == GPU_BUFFER_TYPE_INDEX ) ?	GL_ELEMENT_ARRAY_BUFFER :
						( ( type == GPU_BUFFER_TYPE_UNIFORM ) ?	GL_UNIFORM_BUFFER :
						( ( type == GPU_BUFFER_TYPE_STORAGE ) ?	GL_SHADER_STORAGE_BUFFER :
																0 ) ) ) );
	buffer->size = dataSize;

	GL( glGenBuffers( 1, &buffer->buffer ) );
	GL( glBindBuffer( buffer->target, buffer->buffer) );
	GL( glBufferData( buffer->target, dataSize, data, GL_STATIC_DRAW ) );
	GL( glBindBuffer( buffer->target, 0 ) );

	return true;
}

static void GpuBuffer_Destroy( GpuContext_t * context, GpuBuffer_t * buffer )
{
	UNUSED_PARM( context );

	GL( glDeleteBuffers( 1, &buffer->buffer ) );
	buffer->buffer = 0;
}

/*
================================================================================================================================

GPU texture.

Supports loading textures from raw data or KTX container files.
Textures are always created as immutable textures.
For optimal performance a texture should only be created or modified at load time, not at runtime.
Note that the geometry code assumes the texture origin 0,0 = left-top as opposed to left-bottom.
In other words, textures are expected to be stored top-down as opposed to bottom-up.

GpuTextureFormat_t
GpuTextureUsage_t
GpuTextureWrapMode_t
GpuTextureFilter_t
GpuTextureDefault_t
GpuTexture_t

static bool GpuTexture_Create2D( GpuContext_t * context, GpuTexture_t * texture,
								const GpuTextureFormat_t format, const GpuSampleCount_t sampleCount,
								const int width, const int height, const int mipCount,
								const GpuTextureUsageFlags_t usageFlags, const void * data, const size_t dataSize );
static bool GpuTexture_Create2DArray( GpuContext_t * context, GpuTexture_t * texture,
								const GpuTextureFormat_t format, const GpuSampleCount_t sampleCount,
								const int width, const int height, const int layerCount, const int mipCount,
								const GpuTextureUsageFlags_t usageFlags, const void * data, const size_t dataSize );
static bool GpuTexture_CreateDefault( GpuContext_t * context, GpuTexture_t * texture, const GpuTextureDefault_t defaultType,
								const int width, const int height, const int depth,
								const int layerCount, const int faceCount, const bool mipmaps, const bool border );
static bool GpuTexture_CreateFromSwapChain( GpuContext_t * context, GpuTexture_t * texture, const GpuWindow_t * window, int index );
static bool GpuTexture_CreateFromFile( GpuContext_t * context, GpuTexture_t * texture, const char * fileName );
static void GpuTexture_Destroy( GpuContext_t * context, GpuTexture_t * texture );

static void GpuTexture_SetFilter( GpuContext_t * context, GpuTexture_t * texture, const GpuTextureFilter_t filter );
static void GpuTexture_SetAniso( GpuContext_t * context, GpuTexture_t * texture, const float maxAniso );
static void GpuTexture_SetWrapMode( GpuContext_t * context, GpuTexture_t * texture, const GpuTextureWrapMode_t wrapMode );

================================================================================================================================
*/

// Note that the channel listed first in the name shall occupy the least significant bit.
typedef enum
{
	//
	// 8 bits per component
	//
	GPU_TEXTURE_FORMAT_R8_UNORM				= GL_R8,											// 1-component, 8-bit unsigned normalized
	GPU_TEXTURE_FORMAT_R8G8_UNORM			= GL_RG8,											// 2-component, 8-bit unsigned normalized
	GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM		= GL_RGBA8,											// 4-component, 8-bit unsigned normalized

	GPU_TEXTURE_FORMAT_R8_SNORM				= GL_R8_SNORM,										// 1-component, 8-bit signed normalized
	GPU_TEXTURE_FORMAT_R8G8_SNORM			= GL_RG8_SNORM,										// 2-component, 8-bit signed normalized
	GPU_TEXTURE_FORMAT_R8G8B8A8_SNORM		= GL_RGBA8_SNORM,									// 4-component, 8-bit signed normalized

	GPU_TEXTURE_FORMAT_R8_UINT				= GL_R8UI,											// 1-component, 8-bit unsigned integer
	GPU_TEXTURE_FORMAT_R8G8_UINT			= GL_RG8UI,											// 2-component, 8-bit unsigned integer
	GPU_TEXTURE_FORMAT_R8G8B8A8_UINT		= GL_RGBA8UI,										// 4-component, 8-bit unsigned integer

	GPU_TEXTURE_FORMAT_R8_SINT				= GL_R8I,											// 1-component, 8-bit signed integer
	GPU_TEXTURE_FORMAT_R8G8_SINT			= GL_RG8I,											// 2-component, 8-bit signed integer
	GPU_TEXTURE_FORMAT_R8G8B8A8_SINT		= GL_RGBA8I,										// 4-component, 8-bit signed integer

	GPU_TEXTURE_FORMAT_R8_SRGB				= GL_SR8_EXT,										// 1-component, 8-bit sRGB
	GPU_TEXTURE_FORMAT_R8G8_SRGB			= GL_SRG8_EXT,										// 2-component, 8-bit sRGB
	GPU_TEXTURE_FORMAT_R8G8B8A8_SRGB		= GL_SRGB8_ALPHA8,									// 4-component, 8-bit sRGB

	//
	// 16 bits per component
	//
#if defined( GL_R16 )
	GPU_TEXTURE_FORMAT_R16_UNORM			= GL_R16,											// 1-component, 16-bit unsigned normalized
	GPU_TEXTURE_FORMAT_R16G16_UNORM			= GL_RG16,											// 2-component, 16-bit unsigned normalized
	GPU_TEXTURE_FORMAT_R16G16B16A16_UNORM	= GL_RGBA16,										// 4-component, 16-bit unsigned normalized
#elif defined( GL_R16_EXT )
	GPU_TEXTURE_FORMAT_R16_UNORM			= GL_R16_EXT,										// 1-component, 16-bit unsigned normalized
	GPU_TEXTURE_FORMAT_R16G16_UNORM			= GL_RG16_EXT,										// 2-component, 16-bit unsigned normalized
	GPU_TEXTURE_FORMAT_R16G16B16A16_UNORM	= GL_RGBA16_EXT,									// 4-component, 16-bit unsigned normalized
#endif

#if defined( GL_R16_SNORM )
	GPU_TEXTURE_FORMAT_R16_SNORM			= GL_R16_SNORM,										// 1-component, 16-bit signed normalized
	GPU_TEXTURE_FORMAT_R16G16_SNORM			= GL_RG16_SNORM,									// 2-component, 16-bit signed normalized
	GPU_TEXTURE_FORMAT_R16G16B16A16_SNORM	= GL_RGBA16_SNORM,									// 4-component, 16-bit signed normalized
#elif defined( GL_R16_SNORM_EXT )
	GPU_TEXTURE_FORMAT_R16_SNORM			= GL_R16_SNORM_EXT,									// 1-component, 16-bit signed normalized
	GPU_TEXTURE_FORMAT_R16G16_SNORM			= GL_RG16_SNORM_EXT,								// 2-component, 16-bit signed normalized
	GPU_TEXTURE_FORMAT_R16G16B16A16_SNORM	= GL_RGBA16_SNORM_EXT,								// 4-component, 16-bit signed normalized
#endif

	GPU_TEXTURE_FORMAT_R16_UINT				= GL_R16UI,											// 1-component, 16-bit unsigned integer
	GPU_TEXTURE_FORMAT_R16G16_UINT			= GL_RG16UI,										// 2-component, 16-bit unsigned integer
	GPU_TEXTURE_FORMAT_R16G16B16A16_UINT	= GL_RGBA16UI,										// 4-component, 16-bit unsigned integer

	GPU_TEXTURE_FORMAT_R16_SINT				= GL_R16I,											// 1-component, 16-bit signed integer
	GPU_TEXTURE_FORMAT_R16G16_SINT			= GL_RG16I,											// 2-component, 16-bit signed integer
	GPU_TEXTURE_FORMAT_R16G16B16A16_SINT	= GL_RGBA16I,										// 4-component, 16-bit signed integer

	GPU_TEXTURE_FORMAT_R16_SFLOAT			= GL_R16F,											// 1-component, 16-bit floating-point
	GPU_TEXTURE_FORMAT_R16G16_SFLOAT		= GL_RG16F,											// 2-component, 16-bit floating-point
	GPU_TEXTURE_FORMAT_R16G16B16A16_SFLOAT	= GL_RGBA16F,										// 4-component, 16-bit floating-point

	//
	// 32 bits per component
	//
	GPU_TEXTURE_FORMAT_R32_UINT				= GL_R32UI,											// 1-component, 32-bit unsigned integer
	GPU_TEXTURE_FORMAT_R32G32_UINT			= GL_RG32UI,										// 2-component, 32-bit unsigned integer
	GPU_TEXTURE_FORMAT_R32G32B32A32_UINT	= GL_RGBA32UI,										// 4-component, 32-bit unsigned integer

	GPU_TEXTURE_FORMAT_R32_SINT				= GL_R32I,											// 1-component, 32-bit signed integer
	GPU_TEXTURE_FORMAT_R32G32_SINT			= GL_RG32I,											// 2-component, 32-bit signed integer
	GPU_TEXTURE_FORMAT_R32G32B32A32_SINT	= GL_RGBA32I,										// 4-component, 32-bit signed integer

	GPU_TEXTURE_FORMAT_R32_SFLOAT			= GL_R32F,											// 1-component, 32-bit floating-point
	GPU_TEXTURE_FORMAT_R32G32_SFLOAT		= GL_RG32F,											// 2-component, 32-bit floating-point
	GPU_TEXTURE_FORMAT_R32G32B32A32_SFLOAT	= GL_RGBA32F,										// 4-component, 32-bit floating-point

	//
	// S3TC/DXT/BC
	//
#if defined( GL_COMPRESSED_RGB_S3TC_DXT1_EXT )
	GPU_TEXTURE_FORMAT_BC1_R8G8B8_UNORM		= GL_COMPRESSED_RGB_S3TC_DXT1_EXT,					// 3-component, line through 3D space, unsigned normalized
	GPU_TEXTURE_FORMAT_BC1_R8G8B8A1_UNORM	= GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,					// 4-component, line through 3D space plus 1-bit alpha, unsigned normalized
	GPU_TEXTURE_FORMAT_BC2_R8G8B8A8_UNORM	= GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,					// 4-component, line through 3D space plus line through 1D space, unsigned normalized
	GPU_TEXTURE_FORMAT_BC3_R8G8B8A4_UNORM	= GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,					// 4-component, line through 3D space plus 4-bit alpha, unsigned normalized
#endif

#if defined( GL_COMPRESSED_SRGB_S3TC_DXT1_EXT )
	GPU_TEXTURE_FORMAT_BC1_R8G8B8_SRGB		= GL_COMPRESSED_SRGB_S3TC_DXT1_EXT,					// 3-component, line through 3D space, sRGB
	GPU_TEXTURE_FORMAT_BC1_R8G8B8A1_SRGB	= GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,			// 4-component, line through 3D space plus 1-bit alpha, sRGB
	GPU_TEXTURE_FORMAT_BC2_R8G8B8A8_SRGB	= GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT,			// 4-component, line through 3D space plus line through 1D space, sRGB
	GPU_TEXTURE_FORMAT_BC3_R8G8B8A4_SRGB	= GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,			// 4-component, line through 3D space plus 4-bit alpha, sRGB
#endif

#if defined( GL_COMPRESSED_LUMINANCE_LATC1_EXT )
	GPU_TEXTURE_FORMAT_BC4_R8_UNORM			= GL_COMPRESSED_LUMINANCE_LATC1_EXT,				// 1-component, line through 1D space, unsigned normalized
	GPU_TEXTURE_FORMAT_BC5_R8G8_UNORM		= GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT,			// 2-component, two lines through 1D space, unsigned normalized
	GPU_TEXTURE_FORMAT_BC4_R8_SNORM			= GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT,			// 1-component, line through 1D space, signed normalized
	GPU_TEXTURE_FORMAT_BC5_R8G8_SNORM		= GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT,	// 2-component, two lines through 1D space, signed normalized
#endif

	//
	// ETC
	//
#if defined( GL_COMPRESSED_RGB8_ETC2 )
	GPU_TEXTURE_FORMAT_ETC2_R8G8B8_UNORM	= GL_COMPRESSED_RGB8_ETC2,							// 3-component ETC2, unsigned normalized
	GPU_TEXTURE_FORMAT_ETC2_R8G8B8A1_UNORM	= GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,		// 3-component with 1-bit alpha ETC2, unsigned normalized
	GPU_TEXTURE_FORMAT_ETC2_R8G8B8A8_UNORM	= GL_COMPRESSED_RGBA8_ETC2_EAC,						// 4-component ETC2, unsigned normalized

	GPU_TEXTURE_FORMAT_ETC2_R8G8B8_SRGB		= GL_COMPRESSED_SRGB8_ETC2,							// 3-component ETC2, sRGB
	GPU_TEXTURE_FORMAT_ETC2_R8G8B8A1_SRGB	= GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,		// 3-component with 1-bit alpha ETC2, sRGB
	GPU_TEXTURE_FORMAT_ETC2_R8G8B8A8_SRGB	= GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,				// 4-component ETC2, sRGB
#endif

#if defined( GL_COMPRESSED_R11_EAC )
	GPU_TEXTURE_FORMAT_EAC_R11_UNORM		= GL_COMPRESSED_R11_EAC,							// 1-component ETC, line through 1D space, unsigned normalized
	GPU_TEXTURE_FORMAT_EAC_R11G11_UNORM		= GL_COMPRESSED_RG11_EAC,							// 2-component ETC, two lines through 1D space, unsigned normalized
	GPU_TEXTURE_FORMAT_EAC_R11_SNORM		= GL_COMPRESSED_SIGNED_R11_EAC,						// 1-component ETC, line through 1D space, signed normalized
	GPU_TEXTURE_FORMAT_EAC_R11G11_SNORM		= GL_COMPRESSED_SIGNED_RG11_EAC,					// 2-component ETC, two lines through 1D space, signed normalized
#endif

	//
	// ASTC
	//
#if defined( GL_COMPRESSED_RGBA_ASTC_4x4_KHR )
	GPU_TEXTURE_FORMAT_ASTC_4x4_UNORM		= GL_COMPRESSED_RGBA_ASTC_4x4_KHR,					// 4-component ASTC, 4x4 blocks, unsigned normalized
	GPU_TEXTURE_FORMAT_ASTC_5x4_UNORM		= GL_COMPRESSED_RGBA_ASTC_5x4_KHR,					// 4-component ASTC, 5x4 blocks, unsigned normalized
	GPU_TEXTURE_FORMAT_ASTC_5x5_UNORM		= GL_COMPRESSED_RGBA_ASTC_5x5_KHR,					// 4-component ASTC, 5x5 blocks, unsigned normalized
	GPU_TEXTURE_FORMAT_ASTC_6x5_UNORM		= GL_COMPRESSED_RGBA_ASTC_6x5_KHR,					// 4-component ASTC, 6x5 blocks, unsigned normalized
	GPU_TEXTURE_FORMAT_ASTC_6x6_UNORM		= GL_COMPRESSED_RGBA_ASTC_6x6_KHR,					// 4-component ASTC, 6x6 blocks, unsigned normalized
	GPU_TEXTURE_FORMAT_ASTC_8x5_UNORM		= GL_COMPRESSED_RGBA_ASTC_8x5_KHR,					// 4-component ASTC, 8x5 blocks, unsigned normalized
	GPU_TEXTURE_FORMAT_ASTC_8x6_UNORM		= GL_COMPRESSED_RGBA_ASTC_8x6_KHR,					// 4-component ASTC, 8x6 blocks, unsigned normalized
	GPU_TEXTURE_FORMAT_ASTC_8x8_UNORM		= GL_COMPRESSED_RGBA_ASTC_8x8_KHR,					// 4-component ASTC, 8x8 blocks, unsigned normalized
	GPU_TEXTURE_FORMAT_ASTC_10x5_UNORM		= GL_COMPRESSED_RGBA_ASTC_10x5_KHR,					// 4-component ASTC, 10x5 blocks, unsigned normalized
	GPU_TEXTURE_FORMAT_ASTC_10x6_UNORM		= GL_COMPRESSED_RGBA_ASTC_10x6_KHR,					// 4-component ASTC, 10x6 blocks, unsigned normalized
	GPU_TEXTURE_FORMAT_ASTC_10x8_UNORM		= GL_COMPRESSED_RGBA_ASTC_10x8_KHR,					// 4-component ASTC, 10x8 blocks, unsigned normalized
	GPU_TEXTURE_FORMAT_ASTC_10x10_UNORM		= GL_COMPRESSED_RGBA_ASTC_10x10_KHR,				// 4-component ASTC, 10x10 blocks, unsigned normalized
	GPU_TEXTURE_FORMAT_ASTC_12x10_UNORM		= GL_COMPRESSED_RGBA_ASTC_12x10_KHR,				// 4-component ASTC, 12x10 blocks, unsigned normalized
	GPU_TEXTURE_FORMAT_ASTC_12x12_UNORM		= GL_COMPRESSED_RGBA_ASTC_12x12_KHR,				// 4-component ASTC, 12x12 blocks, unsigned normalized

	GPU_TEXTURE_FORMAT_ASTC_4x4_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR,			// 4-component ASTC, 4x4 blocks, sRGB
	GPU_TEXTURE_FORMAT_ASTC_5x4_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR,			// 4-component ASTC, 5x4 blocks, sRGB
	GPU_TEXTURE_FORMAT_ASTC_5x5_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR,			// 4-component ASTC, 5x5 blocks, sRGB
	GPU_TEXTURE_FORMAT_ASTC_6x5_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR,			// 4-component ASTC, 6x5 blocks, sRGB
	GPU_TEXTURE_FORMAT_ASTC_6x6_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR,			// 4-component ASTC, 6x6 blocks, sRGB
	GPU_TEXTURE_FORMAT_ASTC_8x5_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR,			// 4-component ASTC, 8x5 blocks, sRGB
	GPU_TEXTURE_FORMAT_ASTC_8x6_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR,			// 4-component ASTC, 8x6 blocks, sRGB
	GPU_TEXTURE_FORMAT_ASTC_8x8_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR,			// 4-component ASTC, 8x8 blocks, sRGB
	GPU_TEXTURE_FORMAT_ASTC_10x5_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR,			// 4-component ASTC, 10x5 blocks, sRGB
	GPU_TEXTURE_FORMAT_ASTC_10x6_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR,			// 4-component ASTC, 10x6 blocks, sRGB
	GPU_TEXTURE_FORMAT_ASTC_10x8_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR,			// 4-component ASTC, 10x8 blocks, sRGB
	GPU_TEXTURE_FORMAT_ASTC_10x10_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR,		// 4-component ASTC, 10x10 blocks, sRGB
	GPU_TEXTURE_FORMAT_ASTC_12x10_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR,		// 4-component ASTC, 12x10 blocks, sRGB
	GPU_TEXTURE_FORMAT_ASTC_12x12_SRGB		= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR,		// 4-component ASTC, 12x12 blocks, sRGB
#endif
} GpuTextureFormat_t;

typedef enum
{
	GPU_TEXTURE_USAGE_UNDEFINED			= BIT( 0 ),
	GPU_TEXTURE_USAGE_GENERAL			= BIT( 1 ),
	GPU_TEXTURE_USAGE_TRANSFER_SRC		= BIT( 2 ),
	GPU_TEXTURE_USAGE_TRANSFER_DST		= BIT( 3 ),
	GPU_TEXTURE_USAGE_SAMPLED			= BIT( 4 ),
	GPU_TEXTURE_USAGE_STORAGE			= BIT( 5 ),
	GPU_TEXTURE_USAGE_COLOR_ATTACHMENT	= BIT( 6 ),
	GPU_TEXTURE_USAGE_PRESENTATION		= BIT( 7 )
} GpuTextureUsage_t;

typedef unsigned int GpuTextureUsageFlags_t;

typedef enum
{
	GPU_TEXTURE_WRAP_MODE_REPEAT,
	GPU_TEXTURE_WRAP_MODE_CLAMP_TO_EDGE,
	GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER
} GpuTextureWrapMode_t;

typedef enum
{
	GPU_TEXTURE_FILTER_NEAREST,
	GPU_TEXTURE_FILTER_LINEAR,
	GPU_TEXTURE_FILTER_BILINEAR
} GpuTextureFilter_t;

typedef enum
{
	GPU_TEXTURE_DEFAULT_CHECKERBOARD,	// 32x32 checkerboard pattern (GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM)
	GPU_TEXTURE_DEFAULT_PYRAMIDS,		// 32x32 block pattern of pyramids (GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM)
	GPU_TEXTURE_DEFAULT_CIRCLES			// 32x32 block pattern with circles (GPU_TEXTURE_FORMAT_R8G8B8A8_UNORM)
} GpuTextureDefault_t;

typedef struct
{
	int						width;
	int						height;
	int						depth;
	int						layerCount;
	int						mipCount;
	GpuSampleCount_t		sampleCount;
	GpuTextureUsage_t		usage;
	GpuTextureUsageFlags_t	usageFlags;
	GpuTextureWrapMode_t	wrapMode;
	GpuTextureFilter_t		filter;
	float					maxAnisotropy;
	GLenum					format;
	GLuint					target;
	GLuint					texture;
} GpuTexture_t;

static int IntegerLog2( int i )
{
	int r = 0;
	int t;
	t = ( (~( ( i >> 16 ) + ~0U ) ) >> 27 ) & 0x10; r |= t; i >>= t;
	t = ( (~( ( i >>  8 ) + ~0U ) ) >> 28 ) & 0x08; r |= t; i >>= t;
	t = ( (~( ( i >>  4 ) + ~0U ) ) >> 29 ) & 0x04; r |= t; i >>= t;
	t = ( (~( ( i >>  2 ) + ~0U ) ) >> 30 ) & 0x02; r |= t; i >>= t;
	return ( r | ( i >> 1 ) );
}

// 'width' must be >= 1 and <= 32768.
// 'height' must be >= 1 and <= 32768.
// 'depth' must be >= 0 and <= 32768.
// 'layerCount' must be >= 0.
// 'faceCount' must be either 1 or 6.
// 'mipCount' must be -1 or >= 1.
// 'mipCount' includes the finest level.
// 'mipCount' set to -1 will allocate the full mip chain.
// 'data' may be NULL to allocate a texture without initialization.
// 'dataSize' is the full data size in bytes.
// The 'data' is expected to be stored packed on a per mip level basis.
// If 'data' != NULL and 'mipCount' <= 0, then the full mip chain will be generated from the finest data level.
static bool GpuTexture_CreateInternal( GpuContext_t * context, GpuTexture_t * texture, const char * fileName,
										const GLenum glInternalFormat, const GpuSampleCount_t sampleCount,
										const int width, const int height, const int depth,
										const int layerCount, const int faceCount, const int mipCount,
										const GpuTextureUsageFlags_t usageFlags,
										const void * data, const size_t dataSize, const bool mipSizeStored )
{
	UNUSED_PARM( context );

	memset( texture, 0, sizeof( GpuTexture_t ) );

	assert( depth >= 0 );
	assert( layerCount >= 0 );
	assert( faceCount == 1 || faceCount == 6 );

	if ( width < 1 || width > 32768 || height < 1 || height > 32768 || depth < 0 || depth > 32768 )
	{
		Error( "%s: Invalid texture size (%dx%dx%d)", fileName, width, height, depth );
		return false;
	}

	if ( faceCount != 1 && faceCount != 6 )
	{
		Error( "%s: Cube maps must have 6 faces (%d)", fileName, faceCount );
		return false;
	}

	if ( faceCount == 6 && width != height )
	{
		Error( "%s: Cube maps must be square (%dx%d)", fileName, width, height );
		return false;
	}

	if ( depth > 0 && layerCount > 0 )
	{
		Error( "%s: 3D array textures not supported", fileName );
		return false;
	}

	const int maxDimension = width > height ? ( width > depth ? width : depth ) : ( height > depth ? height : depth );
	const int maxMipLevels = ( 1 + IntegerLog2( maxDimension ) );

	if ( mipCount > maxMipLevels )
	{
		Error( "%s: Too many mip levels (%d > %d)", fileName, mipCount, maxMipLevels );
		return false;
	}

	const GLenum glTarget = ( ( depth > 0 ) ? GL_TEXTURE_3D :
							( ( faceCount == 6 ) ?
							( ( layerCount > 0 ) ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP ) :
							( ( height > 0 ) ?
							( ( layerCount > 0 ) ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D ) :
							( ( layerCount > 0 ) ? GL_TEXTURE_1D_ARRAY : GL_TEXTURE_1D ) ) ) );

	const int numStorageLevels = ( mipCount >= 1 ) ? mipCount : maxMipLevels;

	GL( glGenTextures( 1, &texture->texture ) );
	GL( glBindTexture( glTarget, texture->texture ) );
	if ( depth <= 0 && layerCount <= 0 )
	{
		if ( sampleCount > GPU_SAMPLE_COUNT_1 )
		{
			GL( glTexStorage2DMultisample( glTarget, sampleCount, glInternalFormat, width, height, GL_TRUE ) );
		}
		else
		{
			GL( glTexStorage2D( glTarget, numStorageLevels, glInternalFormat, width, height ) );
		}
	}
	else
	{
		if ( sampleCount > GPU_SAMPLE_COUNT_1 )
		{
			GL( glTexStorage3DMultisample( glTarget, sampleCount, glInternalFormat, width, height, MAX( depth, 1 ) * MAX( layerCount, 1 ), GL_TRUE ) );
		}
		else
		{
			GL( glTexStorage3D( glTarget, numStorageLevels, glInternalFormat, width, height, MAX( depth, 1 ) * MAX( layerCount, 1 ) ) );
		}
	}

	texture->target = glTarget;
	texture->format = glInternalFormat;
	texture->width = width;
	texture->height = height;
	texture->depth = depth;
	texture->layerCount = layerCount;
	texture->mipCount = numStorageLevels;
	texture->sampleCount = sampleCount;
	texture->usage = GPU_TEXTURE_USAGE_UNDEFINED;
	texture->usageFlags = usageFlags;
	texture->wrapMode = GPU_TEXTURE_WRAP_MODE_REPEAT;
	texture->filter = ( numStorageLevels > 1 ) ? GPU_TEXTURE_FILTER_BILINEAR : GPU_TEXTURE_FILTER_LINEAR;
	texture->maxAnisotropy = 1.0f;

	if ( data != NULL )
	{
		assert( sampleCount == GPU_SAMPLE_COUNT_1 );

		const int numDataLevels = ( mipCount >= 1 ) ? mipCount : 1;
		const unsigned char * levelData = (const unsigned char *)data;
		const unsigned char * endOfBuffer = levelData + dataSize;
		bool compressed = false;

		for ( int mipLevel = 0; mipLevel < numDataLevels; mipLevel++ )
		{
			const int mipWidth = ( width >> mipLevel ) >= 1 ? ( width >> mipLevel ) : 1;
			const int mipHeight = ( height >> mipLevel ) >= 1 ? ( height >> mipLevel ) : 1;
			const int mipDepth = ( depth >> mipLevel ) >= 1 ? ( depth >> mipLevel ) : 1;

			size_t mipSize = 0;
			GLenum glFormat = GL_RGBA;
			GLenum glDataType = GL_UNSIGNED_BYTE;
			switch ( glInternalFormat )
			{
				//
				// 8 bits per component
				//
				case GL_R8:				{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned char ); glFormat = GL_RED;  glDataType = GL_UNSIGNED_BYTE; break; }
				case GL_RG8:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned char ); glFormat = GL_RG;   glDataType = GL_UNSIGNED_BYTE; break; }
				case GL_RGBA8:			{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned char ); glFormat = GL_RGBA; glDataType = GL_UNSIGNED_BYTE; break; }

				case GL_R8_SNORM:		{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( char ); glFormat = GL_RED;  glDataType = GL_BYTE; break; }
				case GL_RG8_SNORM:		{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( char ); glFormat = GL_RG;   glDataType = GL_BYTE; break; }
				case GL_RGBA8_SNORM:	{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( char ); glFormat = GL_RGBA; glDataType = GL_BYTE; break; }

				case GL_R8UI:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned char ); glFormat = GL_RED;  glDataType = GL_UNSIGNED_BYTE; break; }
				case GL_RG8UI:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned char ); glFormat = GL_RG;   glDataType = GL_UNSIGNED_BYTE; break; }
				case GL_RGBA8UI:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned char ); glFormat = GL_RGBA; glDataType = GL_UNSIGNED_BYTE; break; }

				case GL_R8I:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( char ); glFormat = GL_RED;  glDataType = GL_BYTE; break; }
				case GL_RG8I:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( char ); glFormat = GL_RG;   glDataType = GL_BYTE; break; }
				case GL_RGBA8I:			{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( char ); glFormat = GL_RGBA; glDataType = GL_BYTE; break; }

				case GL_SR8_EXT:		{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned char ); glFormat = GL_RED;  glDataType = GL_UNSIGNED_BYTE; break; }
				case GL_SRG8_EXT:		{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned char ); glFormat = GL_RG;   glDataType = GL_UNSIGNED_BYTE; break; }
				case GL_SRGB8_ALPHA8:	{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned char ); glFormat = GL_RGBA; glDataType = GL_UNSIGNED_BYTE; break; }

				//
				// 16 bits per component
				//
#if defined( GL_R16 )
				case GL_R16:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned short ); glFormat = GL_RED;  glDataType = GL_UNSIGNED_SHORT; break; }
				case GL_RG16:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned short ); glFormat = GL_RG;   glDataType = GL_UNSIGNED_SHORT; break; }
				case GL_RGBA16:			{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned short ); glFormat = GL_RGBA; glDataType = GL_UNSIGNED_SHORT; break; }
#elif defined( GL_R16_EXT )
				case GL_R16_EXT:		{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned short ); glFormat = GL_RED;  glDataType = GL_UNSIGNED_SHORT; break; }
				case GL_RG16_EXT:		{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned short ); glFormat = GL_RG;   glDataType = GL_UNSIGNED_SHORT; break; }
				case GL_RGB16_EXT:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned short ); glFormat = GL_RGBA; glDataType = GL_UNSIGNED_SHORT; break; }
#endif

#if defined( GL_R16_SNORM )
				case GL_R16_SNORM:		{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( short ); glFormat = GL_RED;  glDataType = GL_SHORT; break; }
				case GL_RG16_SNORM:		{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( short ); glFormat = GL_RG;   glDataType = GL_SHORT; break; }
				case GL_RGBA16_SNORM:	{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( short ); glFormat = GL_RGBA; glDataType = GL_SHORT; break; }
#elif defined( GL_R16_SNORM_EXT )
				case GL_R16_SNORM_EXT:	{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( short ); glFormat = GL_RED;  glDataType = GL_SHORT; break; }
				case GL_RG16_SNORM_EXT:	{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( short ); glFormat = GL_RG;   glDataType = GL_SHORT; break; }
				case GL_RGBA16_SNORM_EXT:{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( short ); glFormat = GL_RGBA; glDataType = GL_SHORT; break; }
#endif

				case GL_R16UI:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned short ); glFormat = GL_RED;  glDataType = GL_UNSIGNED_SHORT; break; }
				case GL_RG16UI:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned short ); glFormat = GL_RG;   glDataType = GL_UNSIGNED_SHORT; break; }
				case GL_RGBA16UI:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned short ); glFormat = GL_RGBA; glDataType = GL_UNSIGNED_SHORT; break; }

				case GL_R16I:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( short ); glFormat = GL_RED;  glDataType = GL_SHORT; break; }
				case GL_RG16I:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( short ); glFormat = GL_RG;   glDataType = GL_SHORT; break; }
				case GL_RGBA16I:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( short ); glFormat = GL_RGBA; glDataType = GL_SHORT; break; }

				case GL_R16F:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned short ); glFormat = GL_RED;  glDataType = GL_HALF_FLOAT; break; }
				case GL_RG16F:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned short ); glFormat = GL_RG;   glDataType = GL_HALF_FLOAT; break; }
				case GL_RGBA16F:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned short ); glFormat = GL_RGBA; glDataType = GL_HALF_FLOAT; break; }

				//
				// 32 bits per component
				//
				case GL_R32UI:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( unsigned int ); glFormat = GL_RED;  glDataType = GL_UNSIGNED_INT; break; }
				case GL_RG32UI:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( unsigned int ); glFormat = GL_RG;   glDataType = GL_UNSIGNED_INT; break; }
				case GL_RGBA32UI:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( unsigned int ); glFormat = GL_RGBA; glDataType = GL_UNSIGNED_INT; break; }

				case GL_R32I:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( int ); glFormat = GL_RED;  glDataType = GL_INT; break; }
				case GL_RG32I:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( int ); glFormat = GL_RG;   glDataType = GL_INT; break; }
				case GL_RGBA32I:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( int ); glFormat = GL_RGBA; glDataType = GL_INT; break; }

				case GL_R32F:			{ mipSize = mipWidth * mipHeight * mipDepth * 1 * sizeof( float ); glFormat = GL_RED;  glDataType = GL_FLOAT; break; }
				case GL_RG32F:			{ mipSize = mipWidth * mipHeight * mipDepth * 2 * sizeof( float ); glFormat = GL_RG;   glDataType = GL_FLOAT; break; }
				case GL_RGBA32F:		{ mipSize = mipWidth * mipHeight * mipDepth * 4 * sizeof( float ); glFormat = GL_RGBA; glDataType = GL_FLOAT; break; }

				//
				// S3TC/DXT/BC
				//
#if defined( GL_COMPRESSED_RGB_S3TC_DXT1_EXT )
				case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:				{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:				{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:				{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:				{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
#endif

#if defined( GL_COMPRESSED_SRGB_S3TC_DXT1_EXT )
				case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:				{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:		{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:		{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:		{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
#endif

#if defined( GL_COMPRESSED_LUMINANCE_LATC1_EXT )
				case GL_COMPRESSED_LUMINANCE_LATC1_EXT:				{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:		{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }

				case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:		{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
#endif

				//
				// ETC
				//
#if defined( GL_COMPRESSED_RGB8_ETC2 )
				case GL_COMPRESSED_RGB8_ETC2:						{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:	{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_RGBA8_ETC2_EAC:					{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }

				case GL_COMPRESSED_SRGB8_ETC2:						{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:	{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:			{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
#endif

#if defined( GL_COMPRESSED_R11_EAC )
				case GL_COMPRESSED_R11_EAC:							{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_RG11_EAC:						{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }

				case GL_COMPRESSED_SIGNED_R11_EAC:					{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 8; compressed = true; break; }
				case GL_COMPRESSED_SIGNED_RG11_EAC:					{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
#endif

				//
				// ASTC
				//
#if defined( GL_COMPRESSED_RGBA_ASTC_4x4_KHR )
				case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:				{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:				{ mipSize = ((mipWidth+4)/5) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:				{ mipSize = ((mipWidth+4)/5) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:				{ mipSize = ((mipWidth+5)/6) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:				{ mipSize = ((mipWidth+5)/6) * ((mipHeight+5)/6) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:				{ mipSize = ((mipWidth+7)/8) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:				{ mipSize = ((mipWidth+7)/8) * ((mipHeight+5)/6) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:				{ mipSize = ((mipWidth+7)/8) * ((mipHeight+7)/8) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:				{ mipSize = ((mipWidth+9)/10) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:				{ mipSize = ((mipWidth+9)/10) * ((mipHeight+5)/6) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:				{ mipSize = ((mipWidth+9)/10) * ((mipHeight+7)/8) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:				{ mipSize = ((mipWidth+9)/10) * ((mipHeight+9)/10) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:				{ mipSize = ((mipWidth+11)/12) * ((mipHeight+9)/10) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:				{ mipSize = ((mipWidth+11)/12) * ((mipHeight+11)/12) * mipDepth * 16; compressed = true; break; }

				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:		{ mipSize = ((mipWidth+3)/4) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:		{ mipSize = ((mipWidth+4)/5) * ((mipHeight+3)/4) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:		{ mipSize = ((mipWidth+4)/5) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:		{ mipSize = ((mipWidth+5)/6) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:		{ mipSize = ((mipWidth+5)/6) * ((mipHeight+5)/6) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:		{ mipSize = ((mipWidth+7)/8) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:		{ mipSize = ((mipWidth+7)/8) * ((mipHeight+5)/6) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:		{ mipSize = ((mipWidth+7)/8) * ((mipHeight+7)/8) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:		{ mipSize = ((mipWidth+9)/10) * ((mipHeight+4)/5) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:		{ mipSize = ((mipWidth+9)/10) * ((mipHeight+5)/6) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:		{ mipSize = ((mipWidth+9)/10) * ((mipHeight+7)/8) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:		{ mipSize = ((mipWidth+9)/10) * ((mipHeight+9)/10) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:		{ mipSize = ((mipWidth+11)/12) * ((mipHeight+9)/10) * mipDepth * 16; compressed = true; break; }
				case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:		{ mipSize = ((mipWidth+11)/12) * ((mipHeight+11)/12) * mipDepth * 16; compressed = true; break; }
#endif
				default:
				{
					Error( "%s: Unsupported image format %d", fileName, glInternalFormat );
					GL( glBindTexture( glTarget, 0 ) );
					return false;
				}
			}

			if ( layerCount > 0 )
			{
				mipSize = mipSize * layerCount * faceCount;
			}

			if ( mipSizeStored )
			{
				if ( levelData + 4 > endOfBuffer )
				{
					Error( "%s: Image data exceeds buffer size", fileName );
					GL( glBindTexture( glTarget, 0 ) );
					return false;
				}
				mipSize = (size_t) *(const unsigned int *)levelData;
				levelData += 4;
			}

			if ( depth <= 0 && layerCount <= 0 )
			{
				for ( int face = 0; face < faceCount; face++ )
				{
					if ( mipSize <= 0 || mipSize > (size_t)( endOfBuffer - levelData ) )
					{
						Error( "%s: Mip %mipDepth data exceeds buffer size (%lld > %lld)", fileName, mipLevel, (uint64_t)mipSize, (uint64_t)( endOfBuffer - levelData ) );
						GL( glBindTexture( glTarget, 0 ) );
						return false;
					}

					const GLenum uploadTarget = ( glTarget == GL_TEXTURE_CUBE_MAP ) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : GL_TEXTURE_2D;
					if ( compressed )
					{
						GL( glCompressedTexSubImage2D( uploadTarget + face, mipLevel, 0, 0, mipWidth, mipHeight, glInternalFormat, (GLsizei)mipSize, levelData ) );
					}
					else
					{
						GL( glTexSubImage2D( uploadTarget + face, mipLevel, 0, 0, mipWidth, mipHeight, glFormat, glDataType, levelData ) );
					}

					levelData += mipSize;

					if ( mipSizeStored )
					{
						levelData += 3 - ( ( mipSize + 3 ) % 4 );
						if ( levelData > endOfBuffer )
						{
							Error( "%s: Image data exceeds buffer size", fileName );
							GL( glBindTexture( glTarget, 0 ) );
							return false;
						}
					}
				}
			}
			else
			{
				if ( mipSize <= 0 || mipSize > (size_t)( endOfBuffer - levelData ) )
				{
					Error( "%s: Mip %mipDepth data exceeds buffer size (%lld > %lld)", fileName, mipLevel, (uint64_t)mipSize, (uint64_t)( endOfBuffer - levelData ) );
					GL( glBindTexture( glTarget, 0 ) );
					return false;
				}

				if ( compressed )
				{
					GL( glCompressedTexSubImage3D( glTarget, mipLevel, 0, 0, 0, mipWidth, mipHeight, mipDepth * MAX( layerCount, 1 ), glInternalFormat, (GLsizei)mipSize, levelData ) );
				}
				else
				{
					GL( glTexSubImage3D( glTarget, mipLevel, 0, 0, 0, mipWidth, mipHeight, mipDepth * MAX( layerCount, 1 ), glFormat, glDataType, levelData ) );
				}

				levelData += mipSize;

				if ( mipSizeStored )
				{
					levelData += 3 - ( ( mipSize + 3 ) % 4 );
					if ( levelData > endOfBuffer )
					{
						Error( "%s: Image data exceeds buffer size", fileName );
						GL( glBindTexture( glTarget, 0 ) );
						return false;
					}
				}
			}
		}

		if ( mipCount < 1 )
		{
			// Can ony generate mip levels for uncompressed textures.
			assert( compressed == false );

			GL( glGenerateMipmap( glTarget ) );
		}
	}

	GL( glTexParameteri( glTarget, GL_TEXTURE_MIN_FILTER, ( numStorageLevels > 1 ) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR ) );
	GL( glTexParameteri( glTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );

	GL( glBindTexture( glTarget, 0 ) );

	texture->usage = GPU_TEXTURE_USAGE_SAMPLED;

	return true;
}

static bool GpuTexture_Create2D( GpuContext_t * context, GpuTexture_t * texture,
								const GpuTextureFormat_t format, const GpuSampleCount_t sampleCount,
								const int width, const int height, const int mipCount,
								const GpuTextureUsageFlags_t usageFlags, const void * data, const size_t dataSize )
{
	const int depth = 0;
	const int layerCount = 0;
	const int faceCount = 1;
	return GpuTexture_CreateInternal( context, texture, "data", (GLenum)format, sampleCount, width, height, depth,
										layerCount, faceCount, mipCount,
										usageFlags, data, dataSize, false );
}

static bool GpuTexture_Create2DArray( GpuContext_t * context, GpuTexture_t * texture,
								const GpuTextureFormat_t format, const GpuSampleCount_t sampleCount,
								const int width, const int height, const int layerCount, const int mipCount,
								const GpuTextureUsageFlags_t usageFlags, const void * data, const size_t dataSize )
{
	const int depth = 0;
	const int faceCount = 1;
	return GpuTexture_CreateInternal( context, texture, "data", (GLenum)format, sampleCount, width, height, depth,
										layerCount, faceCount, mipCount,
										usageFlags, data, dataSize, false );
}

static bool GpuTexture_CreateDefault( GpuContext_t * context, GpuTexture_t * texture, const GpuTextureDefault_t defaultType,
										const int width, const int height, const int depth,
										const int layerCount, const int faceCount,
										const bool mipmaps, const bool border )
{
	const int TEXEL_SIZE = 4;
	const int layerSize = width * height * TEXEL_SIZE;
	const int dataSize = MAX( depth, 1 ) * MAX( layerCount, 1 ) * faceCount * layerSize;
	unsigned char * data = (unsigned char *) malloc( dataSize );

	if ( defaultType == GPU_TEXTURE_DEFAULT_CHECKERBOARD )
	{
		const int blockSize = 32;	// must be a power of two
		for ( int layer = 0; layer < MAX( depth, 1 ) * MAX( layerCount, 1 ) * faceCount; layer++ )
		{
			for ( int y = 0; y < height; y++ )
			{
				for ( int x = 0; x < width; x++ )
				{
					if ( ( ( ( x / blockSize ) ^ ( y / blockSize ) ) & 1 ) == 0 )
					{
						data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 0] = ( layer & 1 ) == 0 ? 96 : 160;
						data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 1] = 64;
						data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 2] = ( layer & 1 ) == 0 ? 255 : 96;
					}
					else
					{
						data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 0] = ( layer & 1 ) == 0 ? 64 : 160;
						data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 1] = 32;
						data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 2] = ( layer & 1 ) == 0 ? 255 : 64;
					}
					data[layer * layerSize + ( y * 128 + x ) * TEXEL_SIZE + 3] = 255;
				}
			}
		}
	}
	else if ( defaultType == GPU_TEXTURE_DEFAULT_PYRAMIDS )
	{
		const int blockSize = 32;	// must be a power of two
		for ( int layer = 0; layer < MAX( depth, 1 ) * MAX( layerCount, 1 ) * faceCount; layer++ )
		{
			for ( int y = 0; y < height; y++ )
			{
				for ( int x = 0; x < width; x++ )
				{
					const int mask = blockSize - 1;
					const int lx = x & mask;
					const int ly = y & mask;
					const int rx = mask - lx;
					const int ry = mask - ly;

					char cx = 0;
					char cy = 0;
					if ( lx != ly && lx != ry )
					{
						int m = blockSize;
						if ( lx < m ) { m = lx; cx = -96; cy =   0; }
						if ( ly < m ) { m = ly; cx =   0; cy = -96; }
						if ( rx < m ) { m = rx; cx = +96; cy =   0; }
						if ( ry < m ) { m = ry; cx =   0; cy = +96; }
					}
					data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 0] = 128 + cx;
					data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 1] = 128 + cy;
					data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 2] = 128 + 85;
					data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + 3] = 255;
				}
			}
		}
	}
	else if ( defaultType == GPU_TEXTURE_DEFAULT_CIRCLES )
	{
		const int blockSize = 32;	// must be a power of two
		const int radius = 10;
		const unsigned char colors[4][4] =
		{
			{ 0xFF, 0x00, 0x00, 0xFF },
			{ 0x00, 0xFF, 0x00, 0xFF },
			{ 0x00, 0x00, 0xFF, 0xFF },
			{ 0xFF, 0xFF, 0x00, 0xFF }
		};
		for ( int layer = 0; layer < MAX( depth, 1 ) * MAX( layerCount, 1 ) * faceCount; layer++ )
		{
			for ( int y = 0; y < height; y++ )
			{
				for ( int x = 0; x < width; x++ )
				{
					// Pick a color per block of texels.
					const int index =	( ( ( y / ( blockSize / 2 ) ) & 2 ) ^ ( ( x / ( blockSize * 1 ) ) & 2 ) ) |
										( ( ( x / ( blockSize * 1 ) ) & 1 ) ^ ( ( y / ( blockSize * 2 ) ) & 1 ) );

					// Draw a circle with radius 10 centered inside each 32x32 block of texels.
					const int dX = ( x & ~( blockSize - 1 ) ) + ( blockSize / 2 ) - x;
					const int dY = ( y & ~( blockSize - 1 ) ) + ( blockSize / 2 ) - y;
					const int dS = abs( dX * dX + dY * dY - radius * radius );
					const int scale = ( dS <= blockSize ) ? dS : blockSize;

					for ( int c = 0; c < TEXEL_SIZE - 1; c++ )
					{
						data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + c] = (unsigned char)( ( colors[index][c] * scale ) / blockSize );
					}
					data[layer * layerSize + ( y * width + x ) * TEXEL_SIZE + TEXEL_SIZE - 1] = 255;
				}
			}
		}
	}

	if ( border )
	{
		for ( int layer = 0; layer < MAX( depth, 1 ) * MAX( layerCount, 1 ) * faceCount; layer++ )
		{
			for ( int x = 0; x < width; x++ )
			{
				data[layer * layerSize + ( 0 * width + x ) * TEXEL_SIZE + 0] = 0;
				data[layer * layerSize + ( 0 * width + x ) * TEXEL_SIZE + 1] = 0;
				data[layer * layerSize + ( 0 * width + x ) * TEXEL_SIZE + 2] = 0;
				data[layer * layerSize + ( 0 * width + x ) * TEXEL_SIZE + 3] = 255;

				data[layer * layerSize + ( ( height - 1 ) * width + x ) * TEXEL_SIZE + 0] = 0;
				data[layer * layerSize + ( ( height - 1 ) * width + x ) * TEXEL_SIZE + 1] = 0;
				data[layer * layerSize + ( ( height - 1 ) * width + x ) * TEXEL_SIZE + 2] = 0;
				data[layer * layerSize + ( ( height - 1 ) * width + x ) * TEXEL_SIZE + 3] = 255;
			}
			for ( int y = 0; y < height; y++ )
			{
				data[layer * layerSize + ( y * width + 0 ) * TEXEL_SIZE + 0] = 0;
				data[layer * layerSize + ( y * width + 0 ) * TEXEL_SIZE + 1] = 0;
				data[layer * layerSize + ( y * width + 0 ) * TEXEL_SIZE + 2] = 0;
				data[layer * layerSize + ( y * width + 0 ) * TEXEL_SIZE + 3] = 255;

				data[layer * layerSize + ( y * width + width - 1 ) * TEXEL_SIZE + 0] = 0;
				data[layer * layerSize + ( y * width + width - 1 ) * TEXEL_SIZE + 1] = 0;
				data[layer * layerSize + ( y * width + width - 1 ) * TEXEL_SIZE + 2] = 0;
				data[layer * layerSize + ( y * width + width - 1 ) * TEXEL_SIZE + 3] = 255;
			}
		}
	}

	const int mipCount = ( mipmaps ) ? -1 : 1;
	bool success = GpuTexture_CreateInternal( context, texture, "data", GL_RGBA8, GPU_SAMPLE_COUNT_1,
												width, height, depth,
												layerCount, faceCount, mipCount,
												GPU_TEXTURE_USAGE_SAMPLED, data, dataSize, false );

	free( data );

	return success;
}

static bool GpuTexture_CreateFromSwapChain( GpuContext_t * context, GpuTexture_t * texture, const GpuWindow_t * window, int index )
{
	UNUSED_PARM( context );
	UNUSED_PARM( index );

	memset( texture, 0, sizeof( GpuTexture_t ) );

	texture->width = window->windowWidth;
	texture->height = window->windowHeight;
	texture->depth = 1;
	texture->layerCount = 1;
	texture->mipCount = 1;
	texture->sampleCount = GPU_SAMPLE_COUNT_1;
	texture->usage = GPU_TEXTURE_USAGE_UNDEFINED;
	texture->wrapMode = GPU_TEXTURE_WRAP_MODE_REPEAT;
	texture->filter = GPU_TEXTURE_FILTER_LINEAR;
	texture->maxAnisotropy = 1.0f;
	texture->format = GpuContext_InternalSurfaceColorFormat( window->colorFormat );
	texture->target = 0;
	texture->texture = 0;

	return true;
}

// This KTX loader does not do any conversions. In other words, the format
// stored in the KTX file must be the same as the glInternaltFormat.
static bool GpuTexture_CreateFromKTX( GpuContext_t * context, GpuTexture_t * texture, const char * fileName,
									const unsigned char * buffer, const size_t bufferSize )
{
	memset( texture, 0, sizeof( GpuTexture_t ) );

#pragma pack(1)
	typedef struct
	{
		unsigned char	identifier[12];
		unsigned int	endianness;
		unsigned int	glType;
		unsigned int	glTypeSize;
		unsigned int	glFormat;
		unsigned int	glInternalFormat;
		unsigned int	glBaseInternalFormat;
		unsigned int	pixelWidth;
		unsigned int	pixelHeight;
		unsigned int	pixelDepth;
		unsigned int	numberOfArrayElements;
		unsigned int	numberOfFaces;
		unsigned int	numberOfMipmapLevels;
		unsigned int	bytesOfKeyValueData;
	} GlHeaderKTX_t;
#pragma pack()

	if ( bufferSize < sizeof( GlHeaderKTX_t ) )
	{
    	Error( "%s: Invalid KTX file", fileName );
        return false;
	}

	const unsigned char fileIdentifier[12] =
	{
		(unsigned char)'\xAB', 'K', 'T', 'X', ' ', '1', '1', (unsigned char)'\xBB', '\r', '\n', '\x1A', '\n'
	};

	const GlHeaderKTX_t * header = (GlHeaderKTX_t *)buffer;
	if ( memcmp( header->identifier, fileIdentifier, sizeof( fileIdentifier ) ) != 0 )
	{
		Error( "%s: Invalid KTX file", fileName );
		return false;
	}
	// only support little endian
	if ( header->endianness != 0x04030201 )
	{
		Error( "%s: KTX file has wrong endianess", fileName );
		return false;
	}
	// skip the key value data
	const size_t startTex = sizeof( GlHeaderKTX_t ) + header->bytesOfKeyValueData;
	if ( ( startTex < sizeof( GlHeaderKTX_t ) ) || ( startTex >= bufferSize ) )
	{
		Error( "%s: Invalid KTX header sizes", fileName );
		return false;
	}

	const GLenum derivedFormat = glGetFormatFromInternalFormat( header->glInternalFormat );
	const GLenum derivedType = glGetTypeFromInternalFormat( header->glInternalFormat );

	UNUSED_PARM( derivedFormat );
	UNUSED_PARM( derivedType );

	// The glFormat and glType must either both be zero or both be non-zero.
	assert( ( header->glFormat == 0 ) == ( header->glType == 0 ) );
	// Uncompressed glTypeSize must be 1, 2, 4 or 8.
	assert( header->glFormat == 0 || header->glTypeSize == 1 || header->glTypeSize == 2 || header->glTypeSize == 4 || header->glTypeSize == 8 );
	// Uncompressed glFormat must match the format derived from glInternalFormat.
	assert( header->glFormat == 0 || header->glFormat == derivedFormat );
	// Uncompressed glType must match the type derived from glInternalFormat.
	assert( header->glFormat == 0 || header->glType == derivedType );
	// Uncompressed glBaseInternalFormat must be the same as glFormat.
	assert( header->glFormat == 0 || header->glBaseInternalFormat == header->glFormat );
	// Compressed glTypeSize must be 1.
	assert( header->glFormat != 0 || header->glTypeSize == 1 );
	// Compressed glBaseInternalFormat must match the format drived from glInternalFormat.
	assert( header->glFormat != 0 || header->glBaseInternalFormat == derivedFormat );

	const int numberOfFaces = ( header->numberOfFaces >= 1 ) ? header->numberOfFaces : 1;
	const GLenum format = header->glInternalFormat;

	return GpuTexture_CreateInternal( context, texture, fileName,
									format, GPU_SAMPLE_COUNT_1,
									header->pixelWidth, header->pixelHeight, header->pixelDepth,
									header->numberOfArrayElements, numberOfFaces, header->numberOfMipmapLevels,
									GPU_TEXTURE_USAGE_SAMPLED, buffer + startTex, bufferSize - startTex, true );
}

static bool GpuTexture_CreateFromFile( GpuContext_t * context, GpuTexture_t * texture, const char * fileName )
{
	memset( texture, 0, sizeof( GpuTexture_t ) );

	FILE * fp = fopen( fileName, "rb" );
	if ( fp == NULL )
	{
		Error( "Failed to open %s", fileName );
		return false;
	}

	fseek( fp, 0L, SEEK_END );
	size_t bufferSize = ftell( fp );
	fseek( fp, 0L, SEEK_SET );

	unsigned char * buffer = (unsigned char *) malloc( bufferSize );
	if ( fread( buffer, 1, bufferSize, fp ) != bufferSize )
	{
		Error( "Failed to read %s", fileName );
		free( buffer );
		fclose( fp );
		return false;
	}
	fclose( fp );

	bool success = GpuTexture_CreateFromKTX( context, texture, fileName, buffer, bufferSize );

	free( buffer );

	return success;
}

static void GpuTexture_Destroy( GpuContext_t * context, GpuTexture_t * texture )
{
	UNUSED_PARM( context );

	if ( texture->texture )
	{
		GL( glDeleteTextures( 1, &texture->texture ) );
	}
	memset( texture, 0, sizeof( GpuTexture_t ) );
}

static void GpuTexture_SetWrapMode( GpuContext_t * context, GpuTexture_t * texture, const GpuTextureWrapMode_t wrapMode )
{
	UNUSED_PARM( context );

	texture->wrapMode = wrapMode;

	const GLint wrap =  ( ( wrapMode == GPU_TEXTURE_WRAP_MODE_CLAMP_TO_EDGE ) ? GL_CLAMP_TO_EDGE :
						( ( wrapMode == GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER ) ? glExtensions.texture_clamp_to_border_id :
						( GL_REPEAT ) ) );

	GL( glBindTexture( texture->target, texture->texture ) );
	GL( glTexParameteri( texture->target, GL_TEXTURE_WRAP_S, wrap ) );
	GL( glTexParameteri( texture->target, GL_TEXTURE_WRAP_T, wrap ) );
	GL( glBindTexture( texture->target, 0 ) );
}

static void GpuTexture_SetFilter( GpuContext_t * context, GpuTexture_t * texture, const GpuTextureFilter_t filter )
{
	UNUSED_PARM( context );

	texture->filter = filter;

	GL( glBindTexture( texture->target, texture->texture ) );
	if ( filter == GPU_TEXTURE_FILTER_NEAREST )
	{
   		GL( glTexParameteri( texture->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST ) );
		GL( glTexParameteri( texture->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST ) );
	}
	else if ( filter == GPU_TEXTURE_FILTER_LINEAR )
	{
   		GL( glTexParameteri( texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
		GL( glTexParameteri( texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );
	}
	else if ( filter == GPU_TEXTURE_FILTER_BILINEAR )
	{
   		GL( glTexParameteri( texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR ) );
		GL( glTexParameteri( texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );
	}
	GL( glBindTexture( texture->target, 0 ) );
}

static void GpuTexture_SetAniso( GpuContext_t * context, GpuTexture_t * texture, const float maxAniso )
{
	UNUSED_PARM( context );

	texture->maxAnisotropy = maxAniso;

	GL( glBindTexture( texture->target, texture->texture ) );
   	GL( glTexParameterf( texture->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso ) );
	GL( glBindTexture( texture->target, 0 ) );
}

/*
================================================================================================================================

GPU vertex attributes.

GpuTriangleIndex_t
GpuVertexAttribute_t
GpuVertexAttributeArraysBase_t

================================================================================================================================
*/

typedef unsigned short GpuTriangleIndex_t;

typedef struct
{
	int				attributeFlag;		// VERTEX_ATTRIBUTE_FLAG_
	size_t			attributeOffset;	// Offset in bytes to the pointer in GpuVertexAttributeArraysBase_t
	size_t			attributeSize;		// Size in bytes of a single attribute
	int				componentType;		// OpenGL type of a single component
	int				componentCount;		// Number of components per location
	int				locationCount;		// Number of attribute locations
	const char *	name;				// Name in vertex program
} GpuVertexAttribute_t;

typedef struct
{
	const GpuVertexAttribute_t *	layout;
} GpuVertexAttributeArraysBase_t;

static size_t GpuVertexAttributeArrays_GetDataSize( const GpuVertexAttribute_t * layout, const int vertexCount, const int attribsFlags )
{
	size_t totalSize = 0;
	for ( int i = 0; layout[i].attributeFlag != 0; i++ )
	{
		const GpuVertexAttribute_t * v = &layout[i];
		if ( ( v->attributeFlag & attribsFlags ) != 0 )
		{
			totalSize += v->attributeSize;
		}
	}
	return vertexCount * totalSize;
}

static void * GpuVertexAttributeArrays_GetDataPointer( const GpuVertexAttributeArraysBase_t * attribs )
{
	for ( int i = 0; attribs->layout[i].attributeFlag != 0; i++ )
	{
		const GpuVertexAttribute_t * v = &attribs->layout[i];
		void * attribPtr = *(void **) ( ((char *)attribs) + v->attributeOffset );
		if ( attribPtr != NULL )
		{
			return attribPtr;
		}
	}
	return NULL;
}

static int GpuVertexAttributeArrays_GetAttribsFlags( const GpuVertexAttributeArraysBase_t * attribs )
{
	int attribsFlags = 0;
	for ( int i = 0; attribs->layout[i].attributeFlag != 0; i++ )
	{
		const GpuVertexAttribute_t * v = &attribs->layout[i];
		void * attribPtr = *(void **) ( ((char *)attribs) + v->attributeOffset );
		if ( attribPtr != NULL )
		{
			attribsFlags |= v->attributeFlag;
		}
	}
	return attribsFlags;
}

static void GpuVertexAttributeArrays_Map( GpuVertexAttributeArraysBase_t * attribs, void * data, const size_t dataSize, const int vertexCount, const int attribsFlags )
{
	unsigned char * dataBytePtr = (unsigned char *) data;
	size_t offset = 0;

	for ( int i = 0; attribs->layout[i].attributeFlag != 0; i++ )
	{
		const GpuVertexAttribute_t * v = &attribs->layout[i];
		void ** attribPtr = (void **) ( ((char *)attribs) + v->attributeOffset );
		if ( ( v->attributeFlag & attribsFlags ) != 0 )
		{
			*attribPtr = ( dataBytePtr + offset );
			offset += vertexCount * v->attributeSize;
		}
		else
		{
			*attribPtr = NULL;
		}
	}

	assert( offset == dataSize );
	UNUSED_PARM( dataSize );
}

static void GpuVertexAttributeArrays_Alloc( GpuVertexAttributeArraysBase_t * attribs, const GpuVertexAttribute_t * layout, const int vertexCount, const int attribsFlags )
{
	const size_t dataSize = GpuVertexAttributeArrays_GetDataSize( layout, vertexCount, attribsFlags );
	void * data = malloc( dataSize );
	attribs->layout = layout;
	GpuVertexAttributeArrays_Map( attribs, data, dataSize, vertexCount, attribsFlags );
}

static void GpuVertexAttributeArrays_Free( GpuVertexAttributeArraysBase_t * attribs )
{
	void * data = GpuVertexAttributeArrays_GetDataPointer( attribs );
	free( data );
}

static void * GpuVertexAttributeArrays_FindAtribute( GpuVertexAttributeArraysBase_t * attribs, const char * name )
{
	for ( int i = 0; attribs->layout[i].attributeFlag != 0; i++ )
	{
		const GpuVertexAttribute_t * v = &attribs->layout[i];
		if ( strcmp( v->name, name ) == 0 ) 
		{
			void ** attribPtr = (void **) ( ((char *)attribs) + v->attributeOffset );
			return *attribPtr;
		}
	}
	return NULL;
}

static void GpuVertexAttributeArrays_CalculateTangents( GpuVertexAttributeArraysBase_t * attribs, const int vertexCount,
														const GpuTriangleIndex_t * indices, const int indexCount )
{
	Vector3f_t * vertexPosition	= (Vector3f_t *)GpuVertexAttributeArrays_FindAtribute( attribs, "vertexPosition" );
	Vector3f_t * vertexNormal	= (Vector3f_t *)GpuVertexAttributeArrays_FindAtribute( attribs, "vertexNormal" );
	Vector3f_t * vertexTangent	= (Vector3f_t *)GpuVertexAttributeArrays_FindAtribute( attribs, "vertexTangent" );
	Vector3f_t * vertexBinormal	= (Vector3f_t *)GpuVertexAttributeArrays_FindAtribute( attribs, "vertexBinormal" );
	Vector2f_t * vertexUv0		= (Vector2f_t *)GpuVertexAttributeArrays_FindAtribute( attribs, "vertexUv0" );

	if ( vertexPosition == NULL || vertexNormal == NULL || vertexTangent == NULL || vertexBinormal == NULL || vertexUv0 == NULL )
	{
		return;
	}

	for ( int i = 0; i < vertexCount; i++ )
	{
		Vector3f_Set( &vertexTangent[i], 0.0f );
		Vector3f_Set( &vertexBinormal[i], 0.0f );
	}

	for ( int i = 0; i < indexCount; i += 3 )
	{
		const GpuTriangleIndex_t * v = indices + i;
		const Vector3f_t * pos = vertexPosition;
		const Vector2f_t * uv0 = vertexUv0;

		const Vector3f_t delta0 = { pos[v[1]].x - pos[v[0]].x, pos[v[1]].y - pos[v[0]].y, pos[v[1]].z - pos[v[0]].z };
		const Vector3f_t delta1 = { pos[v[2]].x - pos[v[1]].x, pos[v[2]].y - pos[v[1]].y, pos[v[2]].z - pos[v[1]].z };
		const Vector3f_t delta2 = { pos[v[0]].x - pos[v[2]].x, pos[v[0]].y - pos[v[2]].y, pos[v[0]].z - pos[v[2]].z };

		const float l0 = delta0.x * delta0.x + delta0.y * delta0.y + delta0.z * delta0.z;
		const float l1 = delta1.x * delta1.x + delta1.y * delta1.y + delta1.z * delta1.z;
		const float l2 = delta2.x * delta2.x + delta2.y * delta2.y + delta2.z * delta2.z;

		const int i0 = ( l0 > l1 ) ? ( l0 > l2 ? 2 : 1 ) : ( l1 > l2 ? 0 : 1 );
		const int i1 = ( i0 + 1 ) % 3;
		const int i2 = ( i0 + 2 ) % 3;

		const Vector3f_t d0 = { pos[v[i1]].x - pos[v[i0]].x, pos[v[i1]].y - pos[v[i0]].y, pos[v[i1]].z - pos[v[i0]].z };
		const Vector3f_t d1 = { pos[v[i2]].x - pos[v[i0]].x, pos[v[i2]].y - pos[v[i0]].y, pos[v[i2]].z - pos[v[i0]].z };

		const Vector2f_t s0 = { uv0[v[i1]].x - uv0[v[i0]].x, uv0[v[i1]].y - uv0[v[i0]].y };
		const Vector2f_t s1 = { uv0[v[i2]].x - uv0[v[i0]].x, uv0[v[i2]].y - uv0[v[i0]].y };

		const float sign = ( s0.x * s1.y - s0.y * s1.x ) < 0.0f ? -1.0f : 1.0f;

		Vector3f_t tangent  = { ( d0.x * s1.y - d1.x * s0.y ) * sign, ( d0.y * s1.y - d1.y * s0.y ) * sign, ( d0.z * s1.y - d1.z * s0.y ) * sign };
		Vector3f_t binormal = { ( d1.x * s0.x - d0.x * s1.x ) * sign, ( d1.y * s0.x - d0.y * s1.x ) * sign, ( d1.z * s0.x - d0.z * s1.x ) * sign };

		Vector3f_Normalize( &tangent );
		Vector3f_Normalize( &binormal );

		for ( int j = 0; j < 3; j++ )
		{
			vertexTangent[v[j]].x += tangent.x;
			vertexTangent[v[j]].y += tangent.y;
			vertexTangent[v[j]].z += tangent.z;

			vertexBinormal[v[j]].x += binormal.x;
			vertexBinormal[v[j]].y += binormal.y;
			vertexBinormal[v[j]].z += binormal.z;
		}
	}

	for ( int i = 0; i < vertexCount; i++ )
	{
		Vector3f_Normalize( &vertexTangent[i] );
		Vector3f_Normalize( &vertexBinormal[i] );
	}
}

/*
================================================================================================================================

GPU default vertex attribute layout.

GpuVertexAttributeFlags_t
GpuVertexAttributeArrays_t

================================================================================================================================
*/

typedef enum
{
	VERTEX_ATTRIBUTE_FLAG_POSITION		= BIT( 0 ),		// vec3 vertexPosition
	VERTEX_ATTRIBUTE_FLAG_NORMAL		= BIT( 1 ),		// vec3 vertexNormal
	VERTEX_ATTRIBUTE_FLAG_TANGENT		= BIT( 2 ),		// vec3 vertexTangent
	VERTEX_ATTRIBUTE_FLAG_BINORMAL		= BIT( 3 ),		// vec3 vertexBinormal
	VERTEX_ATTRIBUTE_FLAG_COLOR			= BIT( 4 ),		// vec4 vertexColor
	VERTEX_ATTRIBUTE_FLAG_UV0			= BIT( 5 ),		// vec2 vertexUv0
	VERTEX_ATTRIBUTE_FLAG_UV1			= BIT( 6 ),		// vec2 vertexUv1
	VERTEX_ATTRIBUTE_FLAG_UV2			= BIT( 7 ),		// vec2 vertexUv2
	VERTEX_ATTRIBUTE_FLAG_JOINT_INDICES	= BIT( 8 ),		// vec4 jointIndices
	VERTEX_ATTRIBUTE_FLAG_JOINT_WEIGHTS	= BIT( 9 ),		// vec4 jointWeights
	VERTEX_ATTRIBUTE_FLAG_TRANSFORM		= BIT( 10 )		// mat4 vertexTransform (NOTE this mat4 takes up 4 attribute locations)
} GpuVertexAttributeFlags_t;

typedef struct
{
	GpuVertexAttributeArraysBase_t	base;
	Vector3f_t *					position;
	Vector3f_t *					normal;
	Vector3f_t *					tangent;
	Vector3f_t *					binormal;
	Vector4f_t *					color;
	Vector2f_t *					uv0;
	Vector2f_t *					uv1;
	Vector2f_t *					uv2;
	Vector4f_t *					jointIndices;
	Vector4f_t *					jointWeights;
	Matrix4x4f_t *					transform;
} GpuVertexAttributeArrays_t;

static const GpuVertexAttribute_t DefaultVertexAttributeLayout[] =
{
	{ VERTEX_ATTRIBUTE_FLAG_POSITION,		OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, position ),	SIZEOF_MEMBER( GpuVertexAttributeArrays_t, position[0] ),		GL_FLOAT,	3,	1,	"vertexPosition" },
	{ VERTEX_ATTRIBUTE_FLAG_NORMAL,			OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, normal ),		SIZEOF_MEMBER( GpuVertexAttributeArrays_t, normal[0] ),			GL_FLOAT,	3,	1,	"vertexNormal" },
	{ VERTEX_ATTRIBUTE_FLAG_TANGENT,		OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, tangent ),		SIZEOF_MEMBER( GpuVertexAttributeArrays_t, tangent[0] ),		GL_FLOAT,	3,	1,	"vertexTangent" },
	{ VERTEX_ATTRIBUTE_FLAG_BINORMAL,		OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, binormal ),	SIZEOF_MEMBER( GpuVertexAttributeArrays_t, binormal[0] ),		GL_FLOAT,	3,	1,	"vertexBinormal" },
	{ VERTEX_ATTRIBUTE_FLAG_COLOR,			OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, color ),		SIZEOF_MEMBER( GpuVertexAttributeArrays_t, color[0] ),			GL_FLOAT,	4,	1,	"vertexColor" },
	{ VERTEX_ATTRIBUTE_FLAG_UV0,			OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, uv0 ),			SIZEOF_MEMBER( GpuVertexAttributeArrays_t, uv0[0] ),			GL_FLOAT,	2,	1,	"vertexUv0" },
	{ VERTEX_ATTRIBUTE_FLAG_UV1,			OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, uv1 ),			SIZEOF_MEMBER( GpuVertexAttributeArrays_t, uv1[0] ),			GL_FLOAT,	2,	1,	"vertexUv1" },
	{ VERTEX_ATTRIBUTE_FLAG_UV2,			OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, uv2 ),			SIZEOF_MEMBER( GpuVertexAttributeArrays_t, uv2[0] ),			GL_FLOAT,	2,	1,	"vertexUv2" },
	{ VERTEX_ATTRIBUTE_FLAG_JOINT_INDICES,	OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, jointIndices ),SIZEOF_MEMBER( GpuVertexAttributeArrays_t, jointIndices[0] ),	GL_FLOAT,	4,	1,	"vertexJointIndices" },
	{ VERTEX_ATTRIBUTE_FLAG_JOINT_WEIGHTS,	OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, jointWeights ),SIZEOF_MEMBER( GpuVertexAttributeArrays_t, jointWeights[0] ),	GL_FLOAT,	4,	1,	"vertexJointWeights" },
	{ VERTEX_ATTRIBUTE_FLAG_TRANSFORM,		OFFSETOF_MEMBER( GpuVertexAttributeArrays_t, transform ),	SIZEOF_MEMBER( GpuVertexAttributeArrays_t, transform[0] ),		GL_FLOAT,	4,	4,	"vertexTransform" },
	{ 0, 0, 0, 0, 0, 0, "" }
};

/*
================================================================================================================================

GPU geometry.

For optimal performance geometry should only be created at load time, not at runtime.
The vertex attributes are not packed. Each attribute is stored in a separate array for
optimal binning on tiling GPUs that only transform the vertex position for the binning pass.
Storing each attribute in a saparate array is preferred even on immediate-mode GPUs to avoid
wasting cache space for attributes that are not used by a particular vertex shader.

GpuGeometry_t

static void GpuGeometry_Create( GpuContext_t * context, GpuGeometry_t * geometry,
								const GpuVertexAttributeArraysBase_t * attribs, const int vertexCount,
								const GpuTriangleIndex_t * indices, const int indexCount );
static void GpuGeometry_CreateQuad( GpuContext_t * context, GpuGeometry_t * geometry, const float offset, const float scale );
static void GpuGeometry_CreateCube( GpuContext_t * context, GpuGeometry_t * geometry, const float offset, const float scale );
static void GpuGeometry_CreateTorus( GpuContext_t * context, GpuGeometry_t * geometry, const int tesselation, const float offset, const float scale );
static void GpuGeometry_Destroy( GpuContext_t * context, GpuGeometry_t * geometry );

static void GpuGeometry_AddInstanceAttributes( GpuContext_t * context, GpuGeometry_t * geometry, const int numInstances, const int instanceAttribsFlags );

================================================================================================================================
*/

typedef struct
{
	const GpuVertexAttribute_t *	layout;
	int								vertexCount;
	int								instanceCount;
	int 							indexCount;
	int								vertexAttribsFlags;
	int								instanceAttribsFlags;
	GpuBuffer_t						vertexBuffer;
	GpuBuffer_t						instanceBuffer;
	GpuBuffer_t						indexBuffer;
} GpuGeometry_t;

static void GpuGeometry_Create( GpuContext_t * context, GpuGeometry_t * geometry,
								const GpuVertexAttributeArraysBase_t * attribs, const int vertexCount,
								const GpuTriangleIndex_t * indices, const int indexCount )
{
	memset( geometry, 0, sizeof( GpuGeometry_t ) );

	geometry->layout = attribs->layout;
	geometry->vertexCount = vertexCount;
	geometry->indexCount = indexCount;
	geometry->vertexAttribsFlags = GpuVertexAttributeArrays_GetAttribsFlags( attribs );

	const void * data = GpuVertexAttributeArrays_GetDataPointer( attribs );
	const size_t dataSize = GpuVertexAttributeArrays_GetDataSize( attribs->layout, geometry->vertexCount, geometry->vertexAttribsFlags );

	GpuBuffer_Create( context, &geometry->vertexBuffer, GPU_BUFFER_TYPE_VERTEX, dataSize, data, false );
	GpuBuffer_Create( context, &geometry->indexBuffer, GPU_BUFFER_TYPE_INDEX, indexCount * sizeof( indices[0] ), indices, false );
}

// The quad is centered about the origin and without offset/scale spans the [-1, 1] X-Y range.
static void GpuGeometry_CreateQuad( GpuContext_t * context, GpuGeometry_t * geometry, const float offset, const float scale )
{
	const Vector3f_t quadPositions[4] =
	{
		{ -1.0f, -1.0f, 0.0f }, { +1.0f, -1.0f, 0.0f }, { +1.0f, +1.0f, 0.0f }, { -1.0f, +1.0f, 0.0f }
	};

	const Vector3f_t quadNormals[4] =
	{
		{ 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }
	};

	const Vector2f_t quadUvs[4] =
	{
		{ 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f },	{ 0.0f, 0.0f }
	};

	const GpuTriangleIndex_t quadIndices[6] =
	{
		 0,  1,  2,  2,  3,  0
	};

	GpuVertexAttributeArrays_t quadAttribs;
	GpuVertexAttributeArrays_Alloc( &quadAttribs.base,
									DefaultVertexAttributeLayout, 4,
									VERTEX_ATTRIBUTE_FLAG_POSITION |
									VERTEX_ATTRIBUTE_FLAG_NORMAL |
									VERTEX_ATTRIBUTE_FLAG_TANGENT |
									VERTEX_ATTRIBUTE_FLAG_BINORMAL |
									VERTEX_ATTRIBUTE_FLAG_UV0 );

	for ( int i = 0; i < 4; i++ )
	{
		quadAttribs.position[i].x = ( quadPositions[i].x + offset ) * scale;
		quadAttribs.position[i].y = ( quadPositions[i].y + offset ) * scale;
		quadAttribs.position[i].z = ( quadPositions[i].z + offset ) * scale;
		quadAttribs.normal[i].x = quadNormals[i].x;
		quadAttribs.normal[i].y = quadNormals[i].y;
		quadAttribs.normal[i].z = quadNormals[i].z;
		quadAttribs.uv0[i].x = quadUvs[i].x;
		quadAttribs.uv0[i].y = quadUvs[i].y;
	}

	GpuVertexAttributeArrays_CalculateTangents( &quadAttribs.base, 4, quadIndices, 6 );

	GpuGeometry_Create( context, geometry, &quadAttribs.base, 4, quadIndices, 6 );

	GpuVertexAttributeArrays_Free( &quadAttribs.base );
}

// The cube is centered about the origin and without offset/scale spans the [-1, 1] X-Y-Z range.
static void GpuGeometry_CreateCube( GpuContext_t * context, GpuGeometry_t * geometry, const float offset, const float scale )
{
	const Vector3f_t cubePositions[24] =
	{
		{ +1.0f, -1.0f, -1.0f }, { +1.0f, +1.0f, -1.0f }, { +1.0f, +1.0f, +1.0f }, { +1.0f, -1.0f, +1.0f },
		{ -1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f, +1.0f }, { -1.0f, +1.0f, +1.0f }, { -1.0f, +1.0f, -1.0f },

		{ -1.0f, +1.0f, -1.0f }, { +1.0f, +1.0f, -1.0f }, { +1.0f, +1.0f, +1.0f }, { -1.0f, +1.0f, +1.0f },
		{ -1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f, +1.0f }, { +1.0f, -1.0f, +1.0f }, { +1.0f, -1.0f, -1.0f },

		{ -1.0f, -1.0f, +1.0f }, { +1.0f, -1.0f, +1.0f }, { +1.0f, +1.0f, +1.0f }, { -1.0f, +1.0f, +1.0f },
		{ -1.0f, -1.0f, -1.0f }, { -1.0f, +1.0f, -1.0f }, { +1.0f, +1.0f, -1.0f }, { +1.0f, -1.0f, -1.0f }
	};

	const Vector3f_t cubeNormals[24] =
	{
		{ +1.0f, 0.0f, 0.0f }, { +1.0f, 0.0f, 0.0f }, { +1.0f, 0.0f, 0.0f }, { +1.0f, 0.0f, 0.0f },
		{ -1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f },

		{ 0.0f, +1.0f, 0.0f }, { 0.0f, +1.0f, 0.0f }, { 0.0f, +1.0f, 0.0f }, { 0.0f, +1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f },

		{ 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, { 0.0f, 0.0f, +1.0f },
		{ 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }
	};

	const Vector2f_t cubeUvs[24] =
	{
		{ 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f },
		{ 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f },	{ 0.0f, 1.0f },

		{ 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f },	{ 0.0f, 0.0f },
		{ 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f },	{ 0.0f, 1.0f },

		{ 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f },	{ 0.0f, 0.0f },
		{ 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f },	{ 0.0f, 1.0f },
	};

	const GpuTriangleIndex_t cubeIndices[36] =
	{
		 0,  1,  2,  2,  3,  0,
		 4,  5,  6,  6,  7,  4,
		 8, 10,  9, 10,  8, 11,
		12, 14, 13, 14, 12, 15,
		16, 17, 18, 18, 19, 16,
		20, 21, 22, 22, 23, 20
	};

	GpuVertexAttributeArrays_t cubeAttribs;
	GpuVertexAttributeArrays_Alloc( &cubeAttribs.base,
									DefaultVertexAttributeLayout, 24,
									VERTEX_ATTRIBUTE_FLAG_POSITION |
									VERTEX_ATTRIBUTE_FLAG_NORMAL |
									VERTEX_ATTRIBUTE_FLAG_TANGENT |
									VERTEX_ATTRIBUTE_FLAG_BINORMAL |
									VERTEX_ATTRIBUTE_FLAG_UV0 );

	for ( int i = 0; i < 24; i++ )
	{
		cubeAttribs.position[i].x = ( cubePositions[i].x + offset ) * scale;
		cubeAttribs.position[i].y = ( cubePositions[i].y + offset ) * scale;
		cubeAttribs.position[i].z = ( cubePositions[i].z + offset ) * scale;
		cubeAttribs.normal[i].x = cubeNormals[i].x;
		cubeAttribs.normal[i].y = cubeNormals[i].y;
		cubeAttribs.normal[i].z = cubeNormals[i].z;
		cubeAttribs.uv0[i].x = cubeUvs[i].x;
		cubeAttribs.uv0[i].y = cubeUvs[i].y;
	}

	GpuVertexAttributeArrays_CalculateTangents( &cubeAttribs.base, 24, cubeIndices, 36 );

	GpuGeometry_Create( context, geometry, &cubeAttribs.base, 24, cubeIndices, 36 );

	GpuVertexAttributeArrays_Free( &cubeAttribs.base );
}

// The torus is centered about the origin and without offset/scale spans the [-1, 1] X-Y range and the [-0.3, 0.3] Z range.
static void GpuGeometry_CreateTorus( GpuContext_t * context, GpuGeometry_t * geometry, const int tesselation, const float offset, const float scale )
{
	const int minorTesselation = tesselation;
	const int majorTesselation = tesselation;
	const float tubeRadius = 0.3f;
	const float tubeCenter = 0.7f;
	const int vertexCount = ( majorTesselation + 1 ) * ( minorTesselation + 1 );
	const int indexCount = majorTesselation * minorTesselation * 6;

	GpuVertexAttributeArrays_t torusAttribs;
	GpuVertexAttributeArrays_Alloc( &torusAttribs.base,
									DefaultVertexAttributeLayout, vertexCount,
									VERTEX_ATTRIBUTE_FLAG_POSITION |
									VERTEX_ATTRIBUTE_FLAG_NORMAL |
									VERTEX_ATTRIBUTE_FLAG_TANGENT |
									VERTEX_ATTRIBUTE_FLAG_BINORMAL |
									VERTEX_ATTRIBUTE_FLAG_UV0 );

	GpuTriangleIndex_t * torusIndices = (GpuTriangleIndex_t *) malloc( indexCount * sizeof( torusIndices[0] ) );

	for ( int u = 0; u <= majorTesselation; u++ )
	{
		const float ua = 2.0f * MATH_PI * u / majorTesselation;
		const float majorCos = cosf( ua );
		const float majorSin = sinf( ua );

		for ( int v = 0; v <= minorTesselation; v++ )
		{
			const float va = MATH_PI + 2.0f * MATH_PI * v / minorTesselation;
			const float minorCos = cosf( va );
			const float minorSin = sinf( va );

			const float minorX = tubeCenter + tubeRadius * minorCos;
			const float minorZ = tubeRadius * minorSin;

			const int index = u * ( minorTesselation + 1 ) + v;
			torusAttribs.position[index].x = ( minorX * majorCos * scale ) + offset;
			torusAttribs.position[index].y = ( minorX * majorSin * scale ) + offset;
			torusAttribs.position[index].z = ( minorZ * scale ) + offset;
			torusAttribs.normal[index].x = minorCos * majorCos;
			torusAttribs.normal[index].y = minorCos * majorSin;
			torusAttribs.normal[index].z = minorSin;
			torusAttribs.uv0[index].x = (float) u / majorTesselation;
			torusAttribs.uv0[index].y = (float) v / minorTesselation;
		}
	}

	for ( int u = 0; u < majorTesselation; u++ )
	{
		for ( int v = 0; v < minorTesselation; v++ )
		{
			const int index = ( u * minorTesselation + v ) * 6;
			torusIndices[index + 0] = (GpuTriangleIndex_t)( ( u + 0 ) * ( minorTesselation + 1 ) + ( v + 0 ) );
			torusIndices[index + 1] = (GpuTriangleIndex_t)( ( u + 1 ) * ( minorTesselation + 1 ) + ( v + 0 ) );
			torusIndices[index + 2] = (GpuTriangleIndex_t)( ( u + 1 ) * ( minorTesselation + 1 ) + ( v + 1 ) );
			torusIndices[index + 3] = (GpuTriangleIndex_t)( ( u + 1 ) * ( minorTesselation + 1 ) + ( v + 1 ) );
			torusIndices[index + 4] = (GpuTriangleIndex_t)( ( u + 0 ) * ( minorTesselation + 1 ) + ( v + 1 ) );
			torusIndices[index + 5] = (GpuTriangleIndex_t)( ( u + 0 ) * ( minorTesselation + 1 ) + ( v + 0 ) );
		}
	}

	GpuVertexAttributeArrays_CalculateTangents( &torusAttribs.base, vertexCount, torusIndices, indexCount );

	GpuGeometry_Create( context, geometry, &torusAttribs.base, vertexCount, torusIndices, indexCount );

	GpuVertexAttributeArrays_Free( &torusAttribs.base );
	free( torusIndices );
}

static void GpuGeometry_Destroy( GpuContext_t * context, GpuGeometry_t * geometry )
{
	GpuBuffer_Destroy( context, &geometry->indexBuffer );
	GpuBuffer_Destroy( context, &geometry->vertexBuffer );
	if ( geometry->instanceBuffer.size != 0 )
	{
		GpuBuffer_Destroy( context, &geometry->instanceBuffer );
	}

	memset( geometry, 0, sizeof( GpuGeometry_t ) );
}

static void GpuGeometry_AddInstanceAttributes( GpuContext_t * context, GpuGeometry_t * geometry, const int numInstances, const int instanceAttribsFlags )
{
	assert( geometry->layout != NULL );
	assert( ( geometry->vertexAttribsFlags & instanceAttribsFlags ) == 0 );

	geometry->instanceCount = numInstances;
	geometry->instanceAttribsFlags = instanceAttribsFlags;

	const size_t dataSize = GpuVertexAttributeArrays_GetDataSize( geometry->layout, numInstances, geometry->instanceAttribsFlags );

	GpuBuffer_Create( context, &geometry->instanceBuffer, GPU_BUFFER_TYPE_VERTEX, dataSize, NULL, false );
}

/*
================================================================================================================================

GPU render pass.

A render pass encapsulates a sequence of graphics commands that can be executed in a single tiling pass.
For optimal performance a render pass should only be created at load time, not at runtime.
Render passes cannot overlap and cannot be nested.

GpuRenderPassType_t
GpuRenderPassFlags_t
GpuRenderPass_t

static bool GpuRenderPass_Create( GpuContext_t * context, GpuRenderPass_t * renderPass,
									const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
									const GpuSampleCount_t sampleCount, const GpuRenderPassType_t type, const uint32_t flags );
static void GpuRenderPass_Destroy( GpuContext_t * context, GpuRenderPass_t * renderPass );

================================================================================================================================
*/

typedef enum
{
	GPU_RENDERPASS_TYPE_INLINE,
	GPU_RENDERPASS_TYPE_SECONDARY_COMMAND_BUFFERS
} GpuRenderPassType_t;

typedef enum
{
	GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER		= BIT( 0 ),
	GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER		= BIT( 1 )
} GpuRenderPassFlags_t;

typedef struct
{
	GpuRenderPassType_t			type;
	int							flags;
	GpuSurfaceColorFormat_t		colorFormat;
	GpuSurfaceDepthFormat_t		depthFormat;
	GpuSampleCount_t			sampleCount;
} GpuRenderPass_t;

static bool GpuRenderPass_Create( GpuContext_t * context, GpuRenderPass_t * renderPass,
									const GpuSurfaceColorFormat_t colorFormat, const GpuSurfaceDepthFormat_t depthFormat,
									const GpuSampleCount_t sampleCount, const GpuRenderPassType_t type, const int flags )
{
	UNUSED_PARM( context );

	assert( type == GPU_RENDERPASS_TYPE_INLINE );

	renderPass->type = type;
	renderPass->flags = flags;
	renderPass->colorFormat = colorFormat;
	renderPass->depthFormat = depthFormat;
	renderPass->sampleCount = sampleCount;
	return true;
}

static void GpuRenderPass_Destroy( GpuContext_t * context, GpuRenderPass_t * renderPass )
{
	UNUSED_PARM( context );
	UNUSED_PARM( renderPass );
}

/*
================================================================================================================================

GPU framebuffer.

A framebuffer encapsulates either a swapchain or a buffered set of textures.
For optimal performance a framebuffer should only be created at load time, not at runtime.

GpuFramebuffer_t

static bool GpuFramebuffer_CreateFromSwapchain( GpuWindow_t * window, GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass );
static bool GpuFramebuffer_CreateFromTextures( GpuContext_t * context, GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass,
												const int width, const int height, const int numBuffers );
static bool GpuFramebuffer_CreateFromTextureArrays( GpuContext_t * context, GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass,
												const int width, const int height, const int numLayers, const int numBuffers, const bool multiview );
static void GpuFramebuffer_Destroy( GpuContext_t * context, GpuFramebuffer_t * framebuffer );

static int GpuFramebuffer_GetWidth( const GpuFramebuffer_t * framebuffer );
static int GpuFramebuffer_GetHeight( const GpuFramebuffer_t * framebuffer );
static ScreenRect_t GpuFramebuffer_GetRect( const GpuFramebuffer_t * framebuffer );
static int GpuFramebuffer_GetBufferCount( const GpuFramebuffer_t * framebuffer );
static GpuTexture_t * GpuFramebuffer_GetColorTexture( const GpuFramebuffer_t * framebuffer );

================================================================================================================================
*/

typedef struct
{
	GpuTexture_t *		colorTextures;
	GLuint				renderTexture;
	GLuint				depthBuffer;
	GLuint *			renderBuffers;
	GLuint *			resolveBuffers;
	bool				multiView;
	int					sampleCount;
	int					numFramebuffersPerTexture;
	int					numBuffers;
	int					currentBuffer;
} GpuFramebuffer_t;

typedef enum
{
	MSAA_OFF,
	MSAA_RESOLVE,
	MSAA_BLIT
} GpuMsaaMode_t;

static bool GpuFramebuffer_CreateFromSwapchain( GpuWindow_t * window, GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass )
{
	assert( window->sampleCount == renderPass->sampleCount );

	UNUSED_PARM( renderPass );

	memset( framebuffer, 0, sizeof( GpuFramebuffer_t ) );

	static const int NUM_BUFFERS = 1;

	framebuffer->colorTextures = (GpuTexture_t *) malloc( NUM_BUFFERS * sizeof( GpuTexture_t ) );
	framebuffer->renderTexture = 0;
	framebuffer->depthBuffer = 0;
	framebuffer->renderBuffers = (GLuint *) malloc( NUM_BUFFERS * sizeof( GLuint ) );
	framebuffer->resolveBuffers = framebuffer->renderBuffers;
	framebuffer->multiView = false;
	framebuffer->sampleCount = GPU_SAMPLE_COUNT_1;
	framebuffer->numFramebuffersPerTexture = 1;
	framebuffer->numBuffers = NUM_BUFFERS;
	framebuffer->currentBuffer = 0;

	// Create the color textures.
	for ( int bufferIndex = 0; bufferIndex < NUM_BUFFERS; bufferIndex++ )
	{
		assert( renderPass->colorFormat == window->colorFormat );
		assert( renderPass->depthFormat == window->depthFormat );

		GpuTexture_CreateFromSwapChain( &window->context, &framebuffer->colorTextures[bufferIndex], window, bufferIndex );

		assert( window->windowWidth == framebuffer->colorTextures[bufferIndex].width );
		assert( window->windowHeight == framebuffer->colorTextures[bufferIndex].height );

		framebuffer->renderBuffers[bufferIndex] = 0;
	}

	return true;
}

static bool GpuFramebuffer_CreateFromTextures( GpuContext_t * context, GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass,
												const int width, const int height, const int numBuffers )
{
	UNUSED_PARM( context );

	memset( framebuffer, 0, sizeof( GpuFramebuffer_t ) );

	framebuffer->colorTextures = (GpuTexture_t *) malloc( numBuffers * sizeof( GpuTexture_t ) );
	framebuffer->renderTexture = 0;
	framebuffer->depthBuffer = 0;
	framebuffer->renderBuffers = (GLuint *) malloc( numBuffers * sizeof( GLuint ) );
	framebuffer->resolveBuffers = framebuffer->renderBuffers;
	framebuffer->multiView = false;
	framebuffer->sampleCount = GPU_SAMPLE_COUNT_1;
	framebuffer->numFramebuffersPerTexture = 1;
	framebuffer->numBuffers = numBuffers;
	framebuffer->currentBuffer = 0;

	const GpuMsaaMode_t mode =	( ( renderPass->sampleCount > GPU_SAMPLE_COUNT_1 && glExtensions.multi_sampled_resolve ) ? MSAA_RESOLVE :
								( ( renderPass->sampleCount > GPU_SAMPLE_COUNT_1 ) ? MSAA_BLIT :
									MSAA_OFF ) );

	// Create the color textures.
	const GLenum colorFormat = GpuContext_InternalSurfaceColorFormat( renderPass->colorFormat );
	for ( int bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++ )
	{
		GpuTexture_Create2D( context, &framebuffer->colorTextures[bufferIndex], (GpuTextureFormat_t)colorFormat, GPU_SAMPLE_COUNT_1,
			width, height, 1, GPU_TEXTURE_USAGE_SAMPLED | GPU_TEXTURE_USAGE_COLOR_ATTACHMENT | GPU_TEXTURE_USAGE_STORAGE, NULL, 0 );
		GpuTexture_SetWrapMode( context, &framebuffer->colorTextures[bufferIndex], GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER );
	}

	// Create the depth buffer.
	if ( renderPass->depthFormat != GPU_SURFACE_DEPTH_FORMAT_NONE )
	{
		const GLenum depthFormat = GpuContext_InternalSurfaceDepthFormat( renderPass->depthFormat );

		GL( glGenRenderbuffers( 1, &framebuffer->depthBuffer ) );
		GL( glBindRenderbuffer( GL_RENDERBUFFER, framebuffer->depthBuffer ) );
		if ( mode == MSAA_RESOLVE )
		{
			GL( glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER, renderPass->sampleCount, depthFormat, width, height ) );
		}
		else if ( mode == MSAA_BLIT )
		{
			GL( glRenderbufferStorageMultisample( GL_RENDERBUFFER, renderPass->sampleCount, depthFormat, width, height ) );
		}
		else
		{
			GL( glRenderbufferStorage( GL_RENDERBUFFER, depthFormat, width, height ) );
		}
		GL( glBindRenderbuffer( GL_RENDERBUFFER, 0 ) );
	}

	// Create the render buffers.
	const int numRenderBuffers = ( mode == MSAA_BLIT ) ? 1 : numBuffers;
	for ( int bufferIndex = 0; bufferIndex < numRenderBuffers; bufferIndex++ )
	{
		GL( glGenFramebuffers( 1, &framebuffer->renderBuffers[bufferIndex] ) );
		GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, framebuffer->renderBuffers[bufferIndex] ) );
		if ( mode == MSAA_RESOLVE )
		{
			GL( glFramebufferTexture2DMultisampleEXT( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer->colorTextures[bufferIndex].texture, 0, renderPass->sampleCount ) );
		}
		else if ( mode == MSAA_BLIT )
		{
			GL( glRenderbufferStorageMultisample( GL_RENDERBUFFER, renderPass->sampleCount, colorFormat, width, height ) );
		}
		else
		{
			GL( glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer->colorTextures[bufferIndex].texture, 0 ) );
		}
		if ( renderPass->depthFormat != GPU_SURFACE_DEPTH_FORMAT_NONE )
		{
			GL( glFramebufferRenderbuffer( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebuffer->depthBuffer ) );
		}
		GL( glGetIntegerv( GL_SAMPLES, &framebuffer->sampleCount ) );
		GL( GLenum status = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );
		GL( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );
		GL( glClear( GL_COLOR_BUFFER_BIT ) );
		GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
		if ( status != GL_FRAMEBUFFER_COMPLETE )
		{
			Error( "Incomplete frame buffer object: %s", GlFramebufferStatusString( status ) );
			return false;
		}
	}

	// Create the resolve buffers.
	if ( mode == MSAA_BLIT )
	{
		framebuffer->resolveBuffers = (GLuint *) malloc( numBuffers * sizeof( GLuint ) );
		for ( int bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++ )
		{
			framebuffer->renderBuffers[bufferIndex] = framebuffer->renderBuffers[0];

			GL( glGenFramebuffers( 1, &framebuffer->resolveBuffers[bufferIndex] ) );
			GL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer->colorTextures[bufferIndex].texture, 0 ) );
			GL( GLenum status = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );
			GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
			if ( status != GL_FRAMEBUFFER_COMPLETE )
			{
				Error( "Incomplete frame buffer object: %s", GlFramebufferStatusString( status ) );
				return false;
			}
		}
	}

	return true;
}

static bool GpuFramebuffer_CreateFromTextureArrays( GpuContext_t * context, GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass,
												const int width, const int height, const int numLayers, const int numBuffers, const bool multiview )
{
	UNUSED_PARM( context );

	memset( framebuffer, 0, sizeof( GpuFramebuffer_t ) );

	framebuffer->colorTextures = (GpuTexture_t *) malloc( numBuffers * sizeof( GpuTexture_t ) );
	framebuffer->depthBuffer = 0;
	framebuffer->multiView = multiview;
	framebuffer->sampleCount = GPU_SAMPLE_COUNT_1;
	framebuffer->numFramebuffersPerTexture = ( multiview ? 1 : numLayers );
	framebuffer->renderBuffers = (GLuint *) malloc( numBuffers * framebuffer->numFramebuffersPerTexture * sizeof( GLuint ) );
	framebuffer->resolveBuffers = framebuffer->renderBuffers;
	framebuffer->numBuffers = numBuffers;
	framebuffer->currentBuffer = 0;

	const GpuMsaaMode_t mode =	( ( renderPass->sampleCount > GPU_SAMPLE_COUNT_1 && !multiview && glExtensions.multi_sampled_resolve ) ? MSAA_RESOLVE :
								( ( renderPass->sampleCount > GPU_SAMPLE_COUNT_1 && multiview && glExtensions.multi_view_multi_sampled_resolve ) ? MSAA_RESOLVE :
								( ( renderPass->sampleCount > GPU_SAMPLE_COUNT_1 && glExtensions.multi_sampled_storage ) ? MSAA_BLIT :
									MSAA_OFF ) ) );

	// Create the color textures.
	const GLenum colorFormat = GpuContext_InternalSurfaceColorFormat( renderPass->colorFormat );
	for ( int bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++ )
	{
		GpuTexture_Create2DArray( context, &framebuffer->colorTextures[bufferIndex], (GpuTextureFormat_t)colorFormat, GPU_SAMPLE_COUNT_1,
			width, height, numLayers, 1, GPU_TEXTURE_USAGE_SAMPLED | GPU_TEXTURE_USAGE_COLOR_ATTACHMENT | GPU_TEXTURE_USAGE_STORAGE, NULL, 0 );
		GpuTexture_SetWrapMode( context, &framebuffer->colorTextures[bufferIndex], GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER );
	}

	// Create the render texture.
	if ( mode == MSAA_BLIT )
	{
		GL( glGenTextures( 1, &framebuffer->renderTexture ) );
		GL( glBindTexture( GL_TEXTURE_2D_MULTISAMPLE_ARRAY, framebuffer->renderTexture ) );
		GL( glTexStorage3DMultisample( GL_TEXTURE_2D_MULTISAMPLE_ARRAY, renderPass->sampleCount, colorFormat, width, height, numLayers, GL_TRUE ) );
		GL( glBindTexture( GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0 ) );
	}

	// Create the depth buffer.
	if ( renderPass->depthFormat != GPU_SURFACE_DEPTH_FORMAT_NONE )
	{
		const GLenum depthFormat = GpuContext_InternalSurfaceDepthFormat( renderPass->depthFormat );
		const GLenum target = ( mode == MSAA_BLIT ) ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_ARRAY;

		GL( glGenTextures( 1, &framebuffer->depthBuffer ) );
		GL( glBindTexture( target, framebuffer->depthBuffer ) );
		if ( mode == MSAA_BLIT )
		{
			GL( glTexStorage3DMultisample( target, renderPass->sampleCount, depthFormat, width, height, numLayers, GL_TRUE ) );
		}
		else
		{
			GL( glTexStorage3D( target, 1, depthFormat, width, height, numLayers ) );
		}
		GL( glBindTexture( target, 0 ) );
	}

	// Create the render buffers.
	const int numRenderBuffers = ( mode == MSAA_BLIT ) ? 1 : numBuffers;
	for ( int bufferIndex = 0; bufferIndex < numRenderBuffers; bufferIndex++ )
	{
		for ( int layerIndex = 0; layerIndex < framebuffer->numFramebuffersPerTexture; layerIndex++ )
		{
			GL( glGenFramebuffers( 1, &framebuffer->renderBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] ) );
			GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, framebuffer->renderBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] ) );
			if ( multiview )
			{
				if ( mode == MSAA_RESOLVE )
				{
					GL( glFramebufferTextureMultisampleMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							framebuffer->colorTextures[bufferIndex].texture, 0 /* level */, renderPass->sampleCount /* samples */, 0 /* baseViewIndex */, numLayers /* numViews */ ) );
					if ( renderPass->depthFormat != GPU_SURFACE_DEPTH_FORMAT_NONE )
					{
						GL( glFramebufferTextureMultisampleMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
								framebuffer->depthBuffer, 0 /* level */, renderPass->sampleCount /* samples */, 0 /* baseViewIndex */, numLayers /* numViews */ ) );
					}
				}
				else if ( mode == MSAA_BLIT )
				{
					GL( glFramebufferTextureMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							framebuffer->renderTexture, 0 /* level */, 0 /* baseViewIndex */, numLayers /* numViews */ ) );
					if ( renderPass->depthFormat != GPU_SURFACE_DEPTH_FORMAT_NONE )
					{
						GL( glFramebufferTextureMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
								framebuffer->depthBuffer, 0 /* level */, 0 /* baseViewIndex */, numLayers /* numViews */ ) );
					}
				}
				else
				{
					GL( glFramebufferTextureMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							framebuffer->colorTextures[bufferIndex].texture, 0 /* level */, 0 /* baseViewIndex */, numLayers /* numViews */ ) );
					if ( renderPass->depthFormat != GPU_SURFACE_DEPTH_FORMAT_NONE )
					{
						GL( glFramebufferTextureMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
								framebuffer->depthBuffer, 0 /* level */, 0 /* baseViewIndex */, numLayers /* numViews */ ) );
					}
				}
			}
			else
			{
				if ( mode == MSAA_RESOLVE )
				{
					// Note: using glFramebufferTextureMultisampleMultiviewOVR with a single view because there is no glFramebufferTextureLayerMultisampleEXT
					GL( glFramebufferTextureMultisampleMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							framebuffer->colorTextures[bufferIndex].texture, 0 /* level */, renderPass->sampleCount /* samples */, layerIndex /* baseViewIndex */, 1 /* numViews */ ) );
					if ( renderPass->depthFormat != GPU_SURFACE_DEPTH_FORMAT_NONE )
					{
						GL( glFramebufferTextureMultisampleMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
								framebuffer->depthBuffer, 0 /* level */, renderPass->sampleCount /* samples */, layerIndex /* baseViewIndex */, 1 /* numViews */ ) );
					}
				}
				else if ( mode == MSAA_BLIT )
				{
					GL( glFramebufferTextureLayer( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							framebuffer->renderTexture, 0 /* level */, layerIndex /* layerIndex */ ) );
					if ( renderPass->depthFormat != GPU_SURFACE_DEPTH_FORMAT_NONE )
					{
						GL( glFramebufferTextureLayer( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
								framebuffer->depthBuffer, 0 /* level */, layerIndex /* layerIndex */ ) );
					}
				}
				else
				{
					GL( glFramebufferTextureLayer( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							framebuffer->colorTextures[bufferIndex].texture, 0 /* level */, layerIndex /* layerIndex */ ) );
					if ( renderPass->depthFormat != GPU_SURFACE_DEPTH_FORMAT_NONE )
					{
						GL( glFramebufferTextureLayer( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
								framebuffer->depthBuffer, 0 /* level */, layerIndex /* layerIndex */ ) );
					}
				}
			}
			GL( glGetIntegerv( GL_SAMPLES, &framebuffer->sampleCount ) );
			GL( GLenum status = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );
			GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
			if ( status != GL_FRAMEBUFFER_COMPLETE )
			{
				Error( "Incomplete frame buffer object: %s", GlFramebufferStatusString( status ) );
				return false;
			}
		}
	}

	// Create the resolve buffers.
	if ( mode == MSAA_BLIT )
	{
		framebuffer->resolveBuffers = (GLuint *) malloc( numBuffers * framebuffer->numFramebuffersPerTexture * sizeof( GLuint ) );
		for ( int bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++ )
		{
			for ( int layerIndex = 0; layerIndex < framebuffer->numFramebuffersPerTexture; layerIndex++ )
			{
				framebuffer->renderBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] = framebuffer->renderBuffers[layerIndex];

				GL( glGenFramebuffers( 1, &framebuffer->resolveBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] ) );
				GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, framebuffer->resolveBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] ) );
				GL( glFramebufferTextureLayer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, framebuffer->colorTextures[bufferIndex].texture, 0 /* level */, layerIndex /* layerIndex */ ) );
				GL( GLenum status = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );
				GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
				if ( status != GL_FRAMEBUFFER_COMPLETE )
				{
					Error( "Incomplete frame buffer object: %s", GlFramebufferStatusString( status ) );
					return false;
				}
			}
		}
	}
	return true;
}

static void GpuFramebuffer_Destroy( GpuContext_t * context, GpuFramebuffer_t * framebuffer )
{
	for ( int bufferIndex = 0; bufferIndex < framebuffer->numBuffers; bufferIndex++ )
	{
		if ( framebuffer->resolveBuffers != framebuffer->renderBuffers )
		{
			for ( int layerIndex = 0; layerIndex < framebuffer->numFramebuffersPerTexture; layerIndex++ )
			{
				if ( framebuffer->resolveBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] != 0 )
				{
					GL( glDeleteFramebuffers( 1, &framebuffer->resolveBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] ) );
				}
			}
		}
		if ( bufferIndex == 0 || framebuffer->renderBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + 0] != framebuffer->renderBuffers[0] )
		{
			for ( int layerIndex = 0; layerIndex < framebuffer->numFramebuffersPerTexture; layerIndex++ )
			{
				if ( framebuffer->renderBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] != 0 )
				{
					GL( glDeleteFramebuffers( 1, &framebuffer->renderBuffers[bufferIndex * framebuffer->numFramebuffersPerTexture + layerIndex] ) );
				}
			}
		}
	}
	if ( framebuffer->depthBuffer != 0 )
	{
		if ( framebuffer->colorTextures[0].layerCount > 0 )
		{
			GL( glDeleteTextures( 1, &framebuffer->depthBuffer ) );
		}
		else
		{
			GL( glDeleteRenderbuffers( 1, &framebuffer->depthBuffer ) );
		}
	}
	if ( framebuffer->renderTexture != 0 )
	{
		if ( framebuffer->colorTextures[0].layerCount > 0 )
		{
			GL( glDeleteTextures( 1, &framebuffer->renderTexture ) );
		}
		else
		{
			GL( glDeleteRenderbuffers( 1, &framebuffer->renderTexture ) );
		}
	}
	for ( int bufferIndex = 0; bufferIndex < framebuffer->numBuffers; bufferIndex++ )
	{
		if ( framebuffer->colorTextures[bufferIndex].texture != 0 )
		{
			GpuTexture_Destroy( context, &framebuffer->colorTextures[bufferIndex] );
		}
	}
	if ( framebuffer->resolveBuffers != framebuffer->renderBuffers )
	{
		free( framebuffer->resolveBuffers );
	}
	free( framebuffer->renderBuffers );
	free( framebuffer->colorTextures );

	memset( framebuffer, 0, sizeof( GpuFramebuffer_t ) );
}

static int GpuFramebuffer_GetWidth( const GpuFramebuffer_t * framebuffer )
{
	return framebuffer->colorTextures[framebuffer->currentBuffer].width;
}

static int GpuFramebuffer_GetHeight( const GpuFramebuffer_t * framebuffer )
{
	return framebuffer->colorTextures[framebuffer->currentBuffer].height;
}

static ScreenRect_t GpuFramebuffer_GetRect( const GpuFramebuffer_t * framebuffer )
{
	ScreenRect_t rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = framebuffer->colorTextures[framebuffer->currentBuffer].width;
	rect.height = framebuffer->colorTextures[framebuffer->currentBuffer].height;
	return rect;
}

static int GpuFramebuffer_GetBufferCount( const GpuFramebuffer_t * framebuffer )
{
	return framebuffer->numBuffers;
}

static GpuTexture_t * GpuFramebuffer_GetColorTexture( const GpuFramebuffer_t * framebuffer )
{
	assert( framebuffer->colorTextures != NULL );
	return &framebuffer->colorTextures[framebuffer->currentBuffer];
}

/*
================================================================================================================================

GPU program parms and layout.

GpuProgramStage_t
GpuProgramParmType_t
GpuProgramParmAccess_t
GpuProgramParm_t
GpuProgramParmLayout_t

static void GpuProgramParmLayout_Create( GpuContext_t * context, GpuProgramParmLayout_t * layout,
										const GpuProgramParm_t * parms, const int numParms,
										const GLuint program );
static void GpuProgramParmLayout_Destroy( GpuContext_t * context, GpuProgramParmLayout_t * layout );

================================================================================================================================
*/

#define MAX_PROGRAM_PARMS	16

typedef enum
{
	GPU_PROGRAM_STAGE_VERTEX,
	GPU_PROGRAM_STAGE_FRAGMENT,
	GPU_PROGRAM_STAGE_COMPUTE,
	GPU_PROGRAM_STAGE_MAX
} GpuProgramStage_t;

typedef enum
{
	GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,					// texture plus sampler bound together (GLSL: sampler)
	GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,					// not sampled, direct read-write storage (GLSL: image)
	GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM,					// read-only uniform buffer (GLSL: uniform)
	GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE,					// read-write storage buffer (GLSL: buffer)
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,				// int
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,		// int[2]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3,		// int[3]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4,		// int[4]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT,				// float
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,		// float[2]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3,		// float[3]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4,		// float[4]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X2,	// float[2][2]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X3,	// float[2][3]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X4,	// float[2][4]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X2,	// float[3][2]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X3,	// float[3][3]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	// float[3][4]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X2,	// float[4][2]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X3,	// float[4][3]
	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	// float[4][4]
	GPU_PROGRAM_PARM_TYPE_MAX
} GpuProgramParmType_t;

typedef enum
{
	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,
	GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,
	GPU_PROGRAM_PARM_ACCESS_READ_WRITE
} GpuProgramParmAccess_t;

typedef struct
{
	GpuProgramStage_t			stage;		// vertex, fragment or compute
	GpuProgramParmType_t		type;		// texture, buffer or push constant
	GpuProgramParmAccess_t		access;		// read and/or write
	int							index;		// index into GpuProgramParmState_t::parms
	const char * 				name;		// GLSL name
	int							binding;	// texture/buffer/uniform binding
} GpuProgramParm_t;

typedef struct
{
	int							numParms;
	const GpuProgramParm_t *	parms;
	int							offsetForIndex[MAX_PROGRAM_PARMS];	// push constant offsets into GpuProgramParmState_t::data based on GpuProgramParm_t::index
	GLint						parmLocations[MAX_PROGRAM_PARMS];	// OpenGL locations
	GLint						parmBindings[MAX_PROGRAM_PARMS];
	GLint						numSampledTextureBindings;
	GLint						numStorageTextureBindings;
	GLint						numUniformBufferBindings;
	GLint						numStorageBufferBindings;
} GpuProgramParmLayout_t;

static int GpuProgramParm_GetPushConstantSize( const GpuProgramParmType_t type )
{
	static const int parmSize[] =
	{
		(unsigned int)0,
		(unsigned int)0,
		(unsigned int)0,
		(unsigned int)0,
		(unsigned int)sizeof( int ),
		(unsigned int)sizeof( int[2] ),
		(unsigned int)sizeof( int[3] ),
		(unsigned int)sizeof( int[4] ),
		(unsigned int)sizeof( float ),
		(unsigned int)sizeof( float[2] ),
		(unsigned int)sizeof( float[3] ),
		(unsigned int)sizeof( float[4] ),
		(unsigned int)sizeof( float[2][2] ),
		(unsigned int)sizeof( float[2][3] ),
		(unsigned int)sizeof( float[2][4] ),
		(unsigned int)sizeof( float[3][2] ),
		(unsigned int)sizeof( float[3][3] ),
		(unsigned int)sizeof( float[3][4] ),
		(unsigned int)sizeof( float[4][2] ),
		(unsigned int)sizeof( float[4][3] ),
		(unsigned int)sizeof( float[4][4] )
	};
	assert( ARRAY_SIZE( parmSize ) == GPU_PROGRAM_PARM_TYPE_MAX );
	return parmSize[type];
}

static void GpuProgramParmLayout_Create( GpuContext_t * context, GpuProgramParmLayout_t * layout,
										const GpuProgramParm_t * parms, const int numParms,
										const GLuint program )
{
	UNUSED_PARM( context );
	assert( numParms <= MAX_PROGRAM_PARMS );

	memset( layout, 0, sizeof( GpuProgramParmLayout_t ) );

	layout->numParms = numParms;
	layout->parms = parms;

	int offset = 0;
	memset( layout->offsetForIndex, -1, sizeof( layout->offsetForIndex ) );

	// Get the texture/buffer/uniform locations and set bindings.
	for ( int i = 0; i < numParms; i++ )
	{
		if ( parms[i].type == GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED )
		{
			layout->parmLocations[i] = glGetUniformLocation( program, parms[i].name );
			assert( layout->parmLocations[i] != -1 );
			if ( layout->parmLocations[i] != -1 )
			{
				// set "texture image unit" binding
				layout->parmBindings[i] = layout->numSampledTextureBindings++;
				GL( glProgramUniform1i( program, layout->parmLocations[i], layout->parmBindings[i] ) );
			}
		}
		else if ( parms[i].type == GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE )
		{
			layout->parmLocations[i] = glGetUniformLocation( program, parms[i].name );
			assert( layout->parmLocations[i] != -1 );
			if ( layout->parmLocations[i] != -1 )
			{
				// set "image unit" binding
				layout->parmBindings[i] = layout->numStorageTextureBindings++;
#if !defined( OS_ANDROID )
				// OpenGL ES does not support changing the location after linking, so rely on the layout( binding ) being correct.
				GL( glProgramUniform1i( program, layout->parmLocations[i], layout->parmBindings[i] ) );
#endif
			}
		}
		else if ( parms[i].type == GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM )
		{
			layout->parmLocations[i] = glGetUniformBlockIndex( program, parms[i].name );
			assert( layout->parmLocations[i] != -1 );
			if ( layout->parmLocations[i] != -1 )
			{
				// set "uniform block" binding
				layout->parmBindings[i] = layout->numUniformBufferBindings++;
				GL( glUniformBlockBinding( program, layout->parmLocations[i], layout->parmBindings[i] ) );
			}
		}
		else if ( parms[i].type == GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE )
		{
			layout->parmLocations[i] = glGetProgramResourceIndex( program, GL_SHADER_STORAGE_BLOCK, parms[i].name );
			assert( layout->parmLocations[i] != -1 );
			if ( layout->parmLocations[i] != -1 )
			{
				// set "shader storage block" binding
				layout->parmBindings[i] = layout->numStorageBufferBindings++;
#if !defined( OS_ANDROID )
				// OpenGL ES does not support glShaderStorageBlockBinding, so rely on the layout( binding ) being correct.
				GL( glShaderStorageBlockBinding( program, layout->parmLocations[i], layout->parmBindings[i] ) );
#endif
			}
		}
		else
		{
			layout->parmLocations[i] = glGetUniformLocation( program, parms[i].name );
			assert( layout->parmLocations[i] != -1 );
			layout->parmBindings[i] = i;

			layout->offsetForIndex[parms[i].index] = offset;
			offset += GpuProgramParm_GetPushConstantSize( parms[i].type );
		}
	}

	assert( layout->numSampledTextureBindings <= glGetInteger( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS ) );
#if OPENGL_COMPUTE_ENABLED == 1
	assert( layout->numStorageTextureBindings <= glGetInteger( GL_MAX_IMAGE_UNITS ) );
	assert( layout->numUniformBufferBindings <= glGetInteger( GL_MAX_UNIFORM_BUFFER_BINDINGS ) );
	assert( layout->numStorageBufferBindings <= glGetInteger( GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS ) );
#endif
}

static void GpuProgramParmLayout_Destroy( GpuContext_t * context, GpuProgramParmLayout_t * layout )
{
	UNUSED_PARM( context );
	UNUSED_PARM( layout );
}

/*
================================================================================================================================

GPU graphics program.

A graphics program encapsulates a vertex and fragment program that are used to render geometry.
For optimal performance a graphics program should only be created at load time, not at runtime.

GpuGraphicsProgram_t

static bool GpuGraphicsProgram_Create( GpuContext_t * context, GpuGraphicsProgram_t * program,
										const void * vertexSourceData, const size_t vertexSourceSize,
										const void * fragmentSourceData, const size_t fragmentSourceSize,
										const GpuProgramParm_t * parms, const int numParms,
										const GpuVertexAttribute_t * vertexLayout, const int vertexAttribsFlags );
static void GpuGraphicsProgram_Destroy( GpuContext_t * context, GpuGraphicsProgram_t * program );

================================================================================================================================
*/

typedef struct
{
	GLuint					vertexShader;
	GLuint					fragmentShader;
	GLuint					program;
	GpuProgramParmLayout_t	parmLayout;
	int						vertexAttribsFlags;
	GLuint					hash;
} GpuGraphicsProgram_t;

static bool GpuGraphicsProgram_Create( GpuContext_t * context, GpuGraphicsProgram_t * program,
										const void * vertexSourceData, const size_t vertexSourceSize,
										const void * fragmentSourceData, const size_t fragmentSourceSize,
										const GpuProgramParm_t * parms, const int numParms,
										const GpuVertexAttribute_t * vertexLayout, const int vertexAttribsFlags )
{
	UNUSED_PARM( vertexSourceSize );
	UNUSED_PARM( fragmentSourceSize );

	program->vertexAttribsFlags = vertexAttribsFlags;

	GLint r;
	GL( program->vertexShader = glCreateShader( GL_VERTEX_SHADER ) );
	GL( glShaderSource( program->vertexShader, 1, (const char **)&vertexSourceData, 0 ) );
	GL( glCompileShader( program->vertexShader ) );
	GL( glGetShaderiv( program->vertexShader, GL_COMPILE_STATUS, &r ) );
	if ( r == GL_FALSE )
	{
		GLchar msg[4096];
		GLsizei length;
		GL( glGetShaderInfoLog( program->vertexShader, sizeof( msg ), &length, msg ) );
		Error( "%s\nlength=%d\n%s\n", (const char *)vertexSourceData, length, msg );
		return false;
	}

	GL( program->fragmentShader = glCreateShader( GL_FRAGMENT_SHADER ) );
	GL( glShaderSource( program->fragmentShader, 1, (const char **)&fragmentSourceData, 0 ) );
	GL( glCompileShader( program->fragmentShader ) );
	GL( glGetShaderiv( program->fragmentShader, GL_COMPILE_STATUS, &r ) );
	if ( r == GL_FALSE )
	{
		GLchar msg[4096];
		GLsizei length;
		GL( glGetShaderInfoLog( program->fragmentShader, sizeof( msg ), &length, msg ) );
		Error( "%s\nlength=%d\n%s\n", (const char *)fragmentSourceData, length, msg );
		return false;
	}

	GL( program->program = glCreateProgram() );
	GL( glAttachShader( program->program, program->vertexShader ) );
	GL( glAttachShader( program->program, program->fragmentShader ) );

	// Bind the vertex attribute locations before linking.
	GLuint location = 0;
	for ( int i = 0; vertexLayout[i].attributeFlag != 0; i++ )
	{
		if ( ( vertexLayout[i].attributeFlag & vertexAttribsFlags ) != 0 )
		{
			GL( glBindAttribLocation( program->program, location, vertexLayout[i].name ) );
			location += vertexLayout[i].locationCount;
		}
	}

	GL( glLinkProgram( program->program ) );
	GL( glGetProgramiv( program->program, GL_LINK_STATUS, &r ) );
	if ( r == GL_FALSE )
	{
		GLchar msg[4096];
		GL( glGetProgramInfoLog( program->program, sizeof( msg ), 0, msg ) );
		Error( "Linking program failed: %s\n", msg );
		return false;
	}

	// Verify the attributes.
	for ( int i = 0; vertexLayout[i].attributeFlag != 0; i++ )
	{
		if ( ( vertexLayout[i].attributeFlag & vertexAttribsFlags ) != 0 )
		{
			assert( glGetAttribLocation( program->program, vertexLayout[i].name ) != -1 );
		}
	}

	GpuProgramParmLayout_Create( context, &program->parmLayout, parms, numParms, program->program );

	// Calculate a hash of the vertex and fragment program source.
	unsigned int hash = 5381;
	for ( int i = 0; ((const char *)vertexSourceData)[i] != '\0'; i++ )
	{
		hash = ( ( hash << 5 ) - hash ) + ((const char *)vertexSourceData)[i];
	}
	for ( int i = 0; ((const char *)fragmentSourceData)[i] != '\0'; i++ )
	{
		hash = ( ( hash << 5 ) - hash ) + ((const char *)fragmentSourceData)[i];
	}
	program->hash = hash;

	return true;
}

static void GpuGraphicsProgram_Destroy( GpuContext_t * context, GpuGraphicsProgram_t * program )
{
	GpuProgramParmLayout_Destroy( context, &program->parmLayout );
	if ( program->program != 0 )
	{
		GL( glDeleteProgram( program->program ) );
		program->program = 0;
	}
	if ( program->vertexShader != 0 )
	{
		GL( glDeleteShader( program->vertexShader ) );
		program->vertexShader = 0;
	}
	if ( program->fragmentShader != 0 )
	{
		GL( glDeleteShader( program->fragmentShader ) );
		program->fragmentShader = 0;
	}
}

/*
================================================================================================================================

GPU compute program.

For optimal performance a compute program should only be created at load time, not at runtime.

GpuComputeProgram_t

static bool GpuComputeProgram_Create( GpuContext_t * context, GpuComputeProgram_t * program,
									const void * computeSourceData, const size_t computeSourceSize,
									const GpuProgramParm_t * parms, const int numParms );
static void GpuComputeProgram_Destroy( GpuContext_t * context, GpuComputeProgram_t * program );

================================================================================================================================
*/

typedef struct
{
	GLuint					computeShader;
	GLuint					program;
	GpuProgramParmLayout_t	parmLayout;
	GLuint					hash;
} GpuComputeProgram_t;

static bool GpuComputeProgram_Create( GpuContext_t * context, GpuComputeProgram_t * program,
									const void * computeSourceData, const size_t computeSourceSize,
									const GpuProgramParm_t * parms, const int numParms )
{
	UNUSED_PARM( context );
	UNUSED_PARM( computeSourceSize );

	GLint r;
	GL( program->computeShader = glCreateShader( GL_COMPUTE_SHADER ) );
	GL( glShaderSource( program->computeShader, 1, (const char **)&computeSourceData, 0 ) );
	GL( glCompileShader( program->computeShader ) );
	GL( glGetShaderiv( program->computeShader, GL_COMPILE_STATUS, &r ) );
	if ( r == GL_FALSE )
	{
		GLchar msg[4096];
		GL( glGetShaderInfoLog( program->computeShader, sizeof( msg ), 0, msg ) );
		Error( "%s\n%s\n", (const char *)computeSourceData, msg );
		return false;
	}

	GL( program->program = glCreateProgram() );
	GL( glAttachShader( program->program, program->computeShader ) );

	GL( glLinkProgram( program->program ) );
	GL( glGetProgramiv( program->program, GL_LINK_STATUS, &r ) );
	if ( r == GL_FALSE )
	{
		GLchar msg[4096];
		GL( glGetProgramInfoLog( program->program, sizeof( msg ), 0, msg ) );
		Error( "Linking program failed: %s\n", msg );
		return false;
	}

	GpuProgramParmLayout_Create( context, &program->parmLayout, parms, numParms, program->program );

	// Calculate a hash of the shader source.
	unsigned int hash = 5381;
	for ( int i = 0; ((const char *)computeSourceData)[i] != '\0'; i++ )
	{
		hash = ( ( hash << 5 ) - hash ) + ((const char *)computeSourceData)[i];
	}
	program->hash = hash;

	return true;
}

static void GpuComputeProgram_Destroy( GpuContext_t * context, GpuComputeProgram_t * program )
{
	GpuProgramParmLayout_Destroy( context, &program->parmLayout );

	if ( program->program != 0 )
	{
		GL( glDeleteProgram( program->program ) );
		program->program = 0;
	}
	if ( program->computeShader != 0 )
	{
		GL( glDeleteShader( program->computeShader ) );
		program->computeShader = 0;
	}
}

/*
================================================================================================================================

GPU graphics pipeline.

A graphics pipeline encapsulates the geometry, program and ROP state that is used to render.
For optimal performance a graphics pipeline should only be created at load time, not at runtime.
Due to the use of a Vertex Array Object (VAO), a graphics pipeline must be created using the same
context that is used to render with the graphics pipeline. The VAO is created here, when both the
geometry and the program are known, to avoid binding vertex attributes that are not used by the
vertex shader, and to avoid binding to a discontinuous set of vertex attribute locations.

GpuFrontFace_t
GpuCullMode_t
GpuCompareOp_t
GpuBlendOp_t
GpuBlendFactor_t
GpuRasterOperations_t
GpuGraphicsPipelineParms_t
GpuGraphicsPipeline_t

static bool GpuGraphicsPipeline_Create( GpuContext_t * context, GpuGraphicsPipeline_t * pipeline, const GpuGraphicsPipelineParms_t * parms );
static void GpuGraphicsPipeline_Destroy( GpuContext_t * context, GpuGraphicsPipeline_t * pipeline );

================================================================================================================================
*/

typedef enum
{
	GPU_FRONT_FACE_COUNTER_CLOCKWISE			= GL_CCW,
    GPU_FRONT_FACE_CLOCKWISE					= GL_CW
} GpuFrontFace_t;

typedef enum
{
	GPU_CULL_MODE_NONE							= GL_NONE,
	GPU_CULL_MODE_FRONT							= GL_FRONT,
	GPU_CULL_MODE_BACK							= GL_BACK
} GpuCullMode_t;

typedef enum
{
	GPU_COMPARE_OP_NEVER						= GL_NEVER,
	GPU_COMPARE_OP_LESS							= GL_LESS,
	GPU_COMPARE_OP_EQUAL						= GL_EQUAL,
	GPU_COMPARE_OP_LESS_OR_EQUAL				= GL_LEQUAL,
	GPU_COMPARE_OP_GREATER						= GL_GREATER,
	GPU_COMPARE_OP_NOT_EQUAL					= GL_NOTEQUAL,
	GPU_COMPARE_OP_GREATER_OR_EQUAL				= GL_GEQUAL,
	GPU_COMPARE_OP_ALWAYS						= GL_ALWAYS
} GpuCompareOp_t;

typedef enum
{
	GPU_BLEND_OP_ADD							= GL_FUNC_ADD,
	GPU_BLEND_OP_SUBTRACT						= GL_FUNC_SUBTRACT,
	GPU_BLEND_OP_REVERSE_SUBTRACT				= GL_FUNC_REVERSE_SUBTRACT,
	GPU_BLEND_OP_MIN							= GL_MIN,
	GPU_BLEND_OP_MAX							= GL_MAX
} GpuBlendOp_t;

typedef enum
{
	GPU_BLEND_FACTOR_ZERO						= GL_ZERO,
	GPU_BLEND_FACTOR_ONE						= GL_ONE,
	GPU_BLEND_FACTOR_SRC_COLOR					= GL_SRC_COLOR,
	GPU_BLEND_FACTOR_ONE_MINUS_SRC_COLOR		= GL_ONE_MINUS_SRC_COLOR,
	GPU_BLEND_FACTOR_DST_COLOR					= GL_DST_COLOR,
	GPU_BLEND_FACTOR_ONE_MINUS_DST_COLOR		= GL_ONE_MINUS_DST_COLOR,
	GPU_BLEND_FACTOR_SRC_ALPHA					= GL_SRC_ALPHA,
	GPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA		= GL_ONE_MINUS_SRC_ALPHA,
	GPU_BLEND_FACTOR_DST_ALPHA					= GL_DST_ALPHA,
	GPU_BLEND_FACTOR_ONE_MINUS_DST_ALPHA		= GL_ONE_MINUS_DST_ALPHA,
	GPU_BLEND_FACTOR_CONSTANT_COLOR				= GL_CONSTANT_COLOR,
	GPU_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR	= GL_ONE_MINUS_CONSTANT_COLOR,
	GPU_BLEND_FACTOR_CONSTANT_ALPHA				= GL_CONSTANT_ALPHA,
	GPU_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA	= GL_ONE_MINUS_CONSTANT_ALPHA,
	GPU_BLEND_FACTOR_SRC_ALPHA_SATURAT			= GL_SRC_ALPHA_SATURATE
} GpuBlendFactor_t;

typedef struct
{
	bool							blendEnable;
	bool							redWriteEnable;
	bool							blueWriteEnable;
	bool							greenWriteEnable;
	bool							alphaWriteEnable;
	bool							depthTestEnable;
	bool							depthWriteEnable;
	GpuFrontFace_t					frontFace;
	GpuCullMode_t					cullMode;
	GpuCompareOp_t					depthCompare;
	Vector4f_t						blendColor;
	GpuBlendOp_t					blendOpColor;
	GpuBlendFactor_t				blendSrcColor;
	GpuBlendFactor_t				blendDstColor;
	GpuBlendOp_t					blendOpAlpha;
	GpuBlendFactor_t				blendSrcAlpha;
	GpuBlendFactor_t				blendDstAlpha;
} GpuRasterOperations_t;

typedef struct
{
	GpuRasterOperations_t			rop;
	const GpuRenderPass_t *			renderPass;
	const GpuGraphicsProgram_t *	program;
	const GpuGeometry_t *			geometry;
} GpuGraphicsPipelineParms_t;

typedef struct
{
	GpuRasterOperations_t			rop;
	const GpuGraphicsProgram_t *	program;
	const GpuGeometry_t *			geometry;
	GLuint	 						vertexArrayObject;
} GpuGraphicsPipeline_t;

static void GpuGraphicsPipelineParms_Init( GpuGraphicsPipelineParms_t * parms )
{
	parms->rop.blendEnable = false;
	parms->rop.redWriteEnable = true;
	parms->rop.blueWriteEnable = true;
	parms->rop.greenWriteEnable = true;
	parms->rop.alphaWriteEnable = false;
	parms->rop.depthTestEnable = true;
	parms->rop.depthWriteEnable = true;
	parms->rop.frontFace = GPU_FRONT_FACE_COUNTER_CLOCKWISE;
	parms->rop.cullMode = GPU_CULL_MODE_BACK;
	parms->rop.depthCompare = GPU_COMPARE_OP_LESS_OR_EQUAL;
	parms->rop.blendColor.x = 0.0f;
	parms->rop.blendColor.y = 0.0f;
	parms->rop.blendColor.z = 0.0f;
	parms->rop.blendColor.w = 0.0f;
	parms->rop.blendOpColor = GPU_BLEND_OP_ADD;
	parms->rop.blendSrcColor = GPU_BLEND_FACTOR_ONE;
	parms->rop.blendDstColor = GPU_BLEND_FACTOR_ZERO;
	parms->rop.blendOpAlpha = GPU_BLEND_OP_ADD;
	parms->rop.blendSrcAlpha = GPU_BLEND_FACTOR_ONE;
	parms->rop.blendDstAlpha = GPU_BLEND_FACTOR_ZERO;
	parms->renderPass = NULL;
	parms->program = NULL;
	parms->geometry = NULL;
}

static void InitVertexAttributes( const bool instance,
								const GpuVertexAttribute_t * vertexLayout, const int numAttribs,
								const int storedAttribsFlags, const int usedAttribsFlags,
								GLuint * attribLocationCount )
{
	size_t offset = 0;
	for ( int i = 0; vertexLayout[i].attributeFlag != 0; i++ )
	{
		const GpuVertexAttribute_t * v = &vertexLayout[i];
		if ( ( v->attributeFlag & storedAttribsFlags ) != 0 )
		{
			if ( ( v->attributeFlag & usedAttribsFlags ) != 0 )
			{
				const size_t attribLocationSize = v->attributeSize / v->locationCount;
				const size_t attribStride = v->attributeSize;
				for ( int location = 0; location < v->locationCount; location++ )
				{
					GL( glEnableVertexAttribArray( *attribLocationCount + location ) );
					GL( glVertexAttribPointer( *attribLocationCount + location, v->componentCount, v->componentType, GL_FALSE,
												(GLsizei)attribStride, (void *)( offset + location * attribLocationSize ) ) );
					GL( glVertexAttribDivisor( *attribLocationCount + location, instance ? 1 : 0 ) );
				}
				*attribLocationCount += v->locationCount;
			}
			offset += numAttribs * v->attributeSize;
		}
	}
}

static bool GpuGraphicsPipeline_Create( GpuContext_t * context, GpuGraphicsPipeline_t * pipeline, const GpuGraphicsPipelineParms_t * parms )
{
	UNUSED_PARM( context );

	// Make sure the geometry provides all the attributes needed by the program.
	assert( ( ( parms->geometry->vertexAttribsFlags | parms->geometry->instanceAttribsFlags ) & parms->program->vertexAttribsFlags ) == parms->program->vertexAttribsFlags );

	memset( pipeline, 0, sizeof( GpuGraphicsPipeline_t ) );

	pipeline->rop = parms->rop;
	pipeline->program = parms->program;
	pipeline->geometry = parms->geometry;

	assert( pipeline->vertexArrayObject == 0 );

	GL( glGenVertexArrays( 1, &pipeline->vertexArrayObject ) );
	GL( glBindVertexArray( pipeline->vertexArrayObject ) );

	GLuint attribLocationCount = 0;

	GL( glBindBuffer( parms->geometry->vertexBuffer.target, parms->geometry->vertexBuffer.buffer ) );
	InitVertexAttributes( false, parms->geometry->layout,
							parms->geometry->vertexCount, parms->geometry->vertexAttribsFlags,
							parms->program->vertexAttribsFlags, &attribLocationCount );

	if ( parms->geometry->instanceBuffer.buffer != 0 )
	{
		GL( glBindBuffer( parms->geometry->instanceBuffer.target, parms->geometry->instanceBuffer.buffer ) );
		InitVertexAttributes( true, parms->geometry->layout,
								parms->geometry->instanceCount, parms->geometry->instanceAttribsFlags,
								parms->program->vertexAttribsFlags, &attribLocationCount );
	}

	GL( glBindBuffer( parms->geometry->indexBuffer.target, parms->geometry->indexBuffer.buffer ) );

	GL( glBindVertexArray( 0 ) );

	return true;
}

static void GpuGraphicsPipeline_Destroy( GpuContext_t * context, GpuGraphicsPipeline_t * pipeline )
{
	UNUSED_PARM( context );

	if ( pipeline->vertexArrayObject != 0 )
	{
		GL( glDeleteVertexArrays( 1, &pipeline->vertexArrayObject ) );
		pipeline->vertexArrayObject = 0;
	}
}

/*
================================================================================================================================

GPU compute pipeline.

A compute pipeline encapsulates a compute program.
For optimal performance a compute pipeline should only be created at load time, not at runtime.

GpuComputePipeline_t

static bool GpuComputePipeline_Create( GpuContext_t * context, GpuComputePipeline_t * pipeline, const GpuComputeProgram_t * program );
static void GpuComputePipeline_Destroy( GpuContext_t * context, GpuComputePipeline_t * pipeline );

================================================================================================================================
*/

typedef struct
{
	const GpuComputeProgram_t *	program;
} GpuComputePipeline_t;

static bool GpuComputePipeline_Create( GpuContext_t * context, GpuComputePipeline_t * pipeline, const GpuComputeProgram_t * program )
{
	UNUSED_PARM( context );

	pipeline->program = program;

	return true;
}

static void GpuComputePipeline_Destroy( GpuContext_t * context, GpuComputePipeline_t * pipeline )
{
	UNUSED_PARM( context );
	UNUSED_PARM( pipeline );
}

/*
================================================================================================================================

GPU fence.

A fence is used to notify completion of a command buffer.
For optimal performance a fence should only be created at load time, not at runtime.

GpuFence_t

static void GpuFence_Create( GpuContext_t * context, GpuFence_t * fence );
static void GpuFence_Destroy( GpuContext_t * context, GpuFence_t * fence );
static void GpuFence_Submit( GpuContext_t * context, GpuFence_t * fence );
static void GpuFence_IsSignalled( GpuContext_t * context, GpuFence_t * fence );

================================================================================================================================
*/

typedef struct
{
#if USE_SYNC_OBJECT == 0
	GLsync		sync;
#elif USE_SYNC_OBJECT == 1
	EGLDisplay	display;
	EGLSyncKHR	sync;
#elif USE_SYNC_OBJECT == 2
	GLuint		computeShader;
	GLuint		computeProgram;
	GLuint		storageBuffer;
	GLuint *	mappedBuffer;
#else
	#error "invalid USE_SYNC_OBJECT setting"
#endif
} GpuFence_t;

static void GpuFence_Create( GpuContext_t * context, GpuFence_t * fence )
{
	UNUSED_PARM( context );

#if USE_SYNC_OBJECT == 0
	fence->sync = 0;
#elif USE_SYNC_OBJECT == 1
	fence->display = 0;
	fence->sync = EGL_NO_SYNC_KHR;
#elif USE_SYNC_OBJECT == 2
	static const char syncComputeProgramGLSL[] =
		"#version " GLSL_PROGRAM_VERSION "\n"
		"\n"
		"layout( local_size_x = 1, local_size_y = 1 ) in;\n"
		"\n"
		"layout( std430, binding = 0 ) buffer syncBuffer { int sync[]; };\n"
		"\n"
		"void main()\n"
		"{\n"
		"	sync[0] = 0;\n"
		"}\n";
	const char * source = syncComputeProgramGLSL;
	// Create a compute program to write to a storage buffer.
	GLint r;
	GL( fence->computeShader = glCreateShader( GL_COMPUTE_SHADER ) );
	GL( glShaderSource( fence->computeShader, 1, (const char **)&source, 0 ) );
	GL( glCompileShader( fence->computeShader ) );
	GL( glGetShaderiv( fence->computeShader, GL_COMPILE_STATUS, &r ) );
	assert( r != GL_FALSE );
	GL( fence->computeProgram = glCreateProgram() );
	GL( glAttachShader( fence->computeProgram, fence->computeShader ) );
	GL( glLinkProgram( fence->computeProgram ) );
	GL( glGetProgramiv( fence->computeProgram, GL_LINK_STATUS, &r ) );
	assert( r != GL_FALSE );
	// Create the persistently mapped storage buffer.
	GL( glGenBuffers( 1, &fence->storageBuffer ) );
	GL( glBindBuffer( GL_SHADER_STORAGE_BUFFER, fence->storageBuffer ) );
	GL( glBufferStorage( GL_SHADER_STORAGE_BUFFER, sizeof( GLuint ), NULL, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT ) );
	GL( fence->mappedBuffer = (GLuint *)glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, sizeof( GLuint ), GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT ) );
	GL( glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 ) );
#else
	#error "invalid USE_SYNC_OBJECT setting"
#endif
}

static void GpuFence_Destroy( GpuContext_t * context, GpuFence_t * fence )
{
	UNUSED_PARM( context );

#if USE_SYNC_OBJECT == 0
	if ( fence->sync != 0 )
	{
		GL( glDeleteSync( fence->sync ) );
		fence->sync = 0;
	}
#elif USE_SYNC_OBJECT == 1
	if ( fence->sync != EGL_NO_SYNC_KHR )
	{
		EGL( eglDestroySyncKHR( fence->display, fence->sync ) );
		fence->display = 0;
		fence->sync = EGL_NO_SYNC_KHR;
	}
#elif USE_SYNC_OBJECT == 2
	GL( glBindBuffer( GL_SHADER_STORAGE_BUFFER, fence->storageBuffer ) );
	GL( glUnmapBuffer( GL_SHADER_STORAGE_BUFFER ) );
	GL( glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 ) );

	GL( glDeleteBuffers( 1, &fence->storageBuffer ) );
	GL( glDeleteProgram( fence->computeProgram ) );
	GL( glDeleteShader( fence->computeShader ) );
#else
	#error "invalid USE_SYNC_OBJECT setting"
#endif
}

static void GpuFence_Submit( GpuContext_t * context, GpuFence_t * fence )
{
	UNUSED_PARM( context );

#if USE_SYNC_OBJECT == 0
	// Destroy any old sync object.
	if ( fence->sync != 0 )
	{
		GL( glDeleteSync( fence->sync ) );
		fence->sync = 0;
	}
	// Create and insert a new sync object.
	GL( fence->sync = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 ) );
	// Force flushing the commands.
	// Note that some drivers will already flush when calling glFenceSync.
	GL( glClientWaitSync( fence->sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0 ) );
#elif USE_SYNC_OBJECT == 1
	// Destroy any old sync object.
	if ( fence->sync != EGL_NO_SYNC_KHR )
	{
		EGL( eglDestroySyncKHR( fence->display, fence->sync ) );
		fence->display = 0;
		fence->sync = EGL_NO_SYNC_KHR;
	}
	// Create and insert a new sync object.
	fence->display = eglGetCurrentDisplay();
	fence->sync = eglCreateSyncKHR( fence->display, EGL_SYNC_FENCE_KHR, NULL );
	if ( fence->sync == EGL_NO_SYNC_KHR )
	{
		Error( "eglCreateSyncKHR() : EGL_NO_SYNC_KHR" );
	}
	// Force flushing the commands.
	// Note that some drivers will already flush when calling eglCreateSyncKHR.
	EGL( eglClientWaitSyncKHR( fence->display, fence->sync, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, 0 ) );
#elif USE_SYNC_OBJECT == 2
	// Initialize the storage buffer to 1 on the client side.
	fence->mappedBuffer[0] = 1;
	// Use a compute shader to clear the persistently mapped storage buffer.
	// This relies on a compute shader only being executed after all other work has completed.
	GL( glUseProgram( fence->computeProgram ) );
	GL( glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 0, fence->storageBuffer ) );
	GL( glDispatchCompute( 1, 1, 1 ) );
	GL( glUseProgram( 0 ) );
	// Flush the commands.
	// Note that some drivers may ignore a flush which could result in the storage buffer never being updated.
	GL( glFlush() );
#else
	#error "invalid USE_SYNC_OBJECT setting"
#endif
}

static bool GpuFence_IsSignalled( GpuContext_t * context, GpuFence_t * fence )
{
	UNUSED_PARM( context );

	if ( fence == NULL )
	{
		return false;
	}
#if USE_SYNC_OBJECT == 0
	if ( glIsSync( fence->sync ) )
	{
		GL( const GLenum result = glClientWaitSync( fence->sync, 0, 0 ) );
		if ( result == GL_WAIT_FAILED )
		{
			Error( "glClientWaitSync() : GL_WAIT_FAILED" );
		}
		if ( result != GL_TIMEOUT_EXPIRED )
		{
			return true;
		}
	}
#elif USE_SYNC_OBJECT == 1
	if ( fence->sync != EGL_NO_SYNC_KHR )
	{
		const EGLint result = eglClientWaitSyncKHR( fence->display, fence->sync, 0, 0 );
		if ( result == EGL_FALSE )
		{
			Error( "eglClientWaitSyncKHR() : EGL_FALSE" );
		}
		if ( result != EGL_TIMEOUT_EXPIRED_KHR )
		{
			return true;
		}
	}
#elif USE_SYNC_OBJECT == 2
	if ( fence->mappedBuffer[0] == 0 )
	{
		return true;
	}
#else
	#error "invalid USE_SYNC_OBJECT setting"
#endif
	return false;
}

/*
================================================================================================================================

GPU timer.

A timer is used to measure the amount of time it takes to complete GPU commands.
For optimal performance a timer should only be created at load time, not at runtime.
To avoid synchronization, GpuTimer_GetMilliseconds() reports the time from GPU_TIMER_FRAMES_DELAYED frames ago.
Timer queries are allowed to overlap and can be nested.
Timer queries that are issued inside a render pass may not produce accurate times on tiling GPUs.

GpuTimer_t

static void GpuTimer_Create( GpuContext_t * context, GpuTimer_t * timer );
static void GpuTimer_Destroy( GpuContext_t * context, GpuTimer_t * timer );
static float GpuTimer_GetMilliseconds( GpuTimer_t * timer );

================================================================================================================================
*/

#define GPU_TIMER_FRAMES_DELAYED	2

typedef struct
{
	GLuint	beginQueries[GPU_TIMER_FRAMES_DELAYED];
	GLuint	endQueries[GPU_TIMER_FRAMES_DELAYED];
	int		queryIndex;
	float	gpuTime;
} GpuTimer_t;

static void GpuTimer_Create( GpuContext_t * context, GpuTimer_t * timer )
{
	UNUSED_PARM( context );

	if ( glExtensions.timer_query )
	{
		GL( glGenQueries( GPU_TIMER_FRAMES_DELAYED, timer->beginQueries ) );
		GL( glGenQueries( GPU_TIMER_FRAMES_DELAYED, timer->endQueries ) );
		timer->queryIndex = 0;
		timer->gpuTime = 0;
	}
}

static void GpuTimer_Destroy( GpuContext_t * context, GpuTimer_t * timer )
{
	UNUSED_PARM( context );

	if ( glExtensions.timer_query )
	{
		GL( glDeleteQueries( GPU_TIMER_FRAMES_DELAYED, timer->beginQueries ) );
		GL( glDeleteQueries( GPU_TIMER_FRAMES_DELAYED, timer->endQueries ) );
	}
}

static float GpuTimer_GetMilliseconds( GpuTimer_t * timer )
{
	if ( glExtensions.timer_query )
	{
		return timer->gpuTime;
	}
	else
	{
		return 0.0f;
	}
}

/*
================================================================================================================================

GPU program parm state.

GpuProgramParmState_t

================================================================================================================================
*/

#define SAVE_PUSH_CONSTANT_STATE 	1

typedef struct
{
	const void *	parms[MAX_PROGRAM_PARMS];
#if SAVE_PUSH_CONSTANT_STATE == 1
	unsigned char	data[MAX_PROGRAM_PARMS * sizeof( float[4] )];
#endif
} GpuProgramParmState_t;

static void GpuProgramParmState_SetParm( GpuProgramParmState_t * parmState, const GpuProgramParmLayout_t * parmLayout,
											const int index, const GpuProgramParmType_t parmType, const void * pointer )
{
	assert( index >= 0 && index < MAX_PROGRAM_PARMS );
	if ( pointer != NULL )
	{
		bool found = false;
		for ( int i = 0; i < parmLayout->numParms; i++ )
		{
			if ( parmLayout->parms[i].index == index )
			{
				assert( parmLayout->parms[i].type == parmType );
				found = true;
				break;
			}
		}
		// Currently parms can be set even if they are not used by the program.
		//assert( found );
		UNUSED_PARM( found );
	}

	parmState->parms[index] = pointer;

#if SAVE_PUSH_CONSTANT_STATE == 1
	const int pushConstantSize = GpuProgramParm_GetPushConstantSize( parmType );
	if ( pushConstantSize > 0 )
	{
		assert( parmLayout->offsetForIndex[index] >= 0 );
		assert( parmLayout->offsetForIndex[index] + pushConstantSize <= MAX_PROGRAM_PARMS * sizeof( float[4] ) );
		memcpy( &parmState->data[parmLayout->offsetForIndex[index]], pointer, pushConstantSize );
	}
#endif
}

static const void * GpuProgramParmState_NewPushConstantData( const GpuProgramParmLayout_t * newLayout, const int newParmIndex, const GpuProgramParmState_t * newParmState,
															const GpuProgramParmLayout_t * oldLayout, const int oldParmIndex, const GpuProgramParmState_t * oldParmState,
															const bool force )
{
#if SAVE_PUSH_CONSTANT_STATE == 1
	const GpuProgramParm_t * newParm = &newLayout->parms[newParmIndex];
	const unsigned char * newData = &newParmState->data[newLayout->offsetForIndex[newParm->index]];
	if ( force || oldLayout == NULL || oldParmIndex >= oldLayout->numParms )
	{
		return newData;
	}
	const GpuProgramParm_t * oldParm = &oldLayout->parms[oldParmIndex];
	const unsigned char * oldData = &oldParmState->data[oldLayout->offsetForIndex[oldParm->index]];
	if ( newParm->type != oldParm->type || newLayout->parmBindings[newParmIndex] != oldLayout->parmBindings[oldParmIndex] )
	{
		return newData;
	}
	const int pushConstantSize = GpuProgramParm_GetPushConstantSize( newParm->type );
	if ( memcmp( newData, oldData, pushConstantSize ) != 0 )
	{
		return newData;
	}
	return NULL;
#else
	if ( force || oldLayout == NULL || oldParmIndex >= oldLayout->numParms ||
			newLayout->parmBindings[newParmIndex] != oldLayout->parmBindings[oldParmIndex] ||
				newLayout->parms[newParmIndex].type != oldLayout->parms[oldParmIndex].type ||
					newParmState->parms[newLayout->parms[newParmIndex].index] != oldParmState->parms[oldLayout->parms[oldParmIndex].index] )
	{
		return newParmState->parms[newLayout->parms[newParmIndex].index];
	}
	return NULL;
#endif
}

/*
================================================================================================================================

GPU graphics commands.

A graphics command encapsulates all OpenGL state associated with a single draw call.
The pointers passed in as parameters are expected to point to unique objects that persist
at least past the submission of the command buffer into which the graphics command is
submitted. Because pointers are maintained as state, DO NOT use pointers to local
variables that will go out of scope before the command buffer is submitted.

GpuGraphicsCommand_t

static void GpuGraphicsCommand_Init( GpuGraphicsCommand_t * command );
static void GpuGraphicsCommand_SetPipeline( GpuGraphicsCommand_t * command, const GpuGraphicsPipeline_t * pipeline );
static void GpuGraphicsCommand_SetVertexBuffer( GpuGraphicsCommand_t * command, const GpuBuffer_t * vertexBuffer );
static void GpuGraphicsCommand_SetInstanceBuffer( GpuGraphicsCommand_t * command, const GpuBuffer_t * instanceBuffer );
static void GpuGraphicsCommand_SetParmTextureSampled( GpuGraphicsCommand_t * command, const int index, const GpuTexture_t * texture );
static void GpuGraphicsCommand_SetParmTextureStorage( GpuGraphicsCommand_t * command, const int index, const GpuTexture_t * texture );
static void GpuGraphicsCommand_SetParmBufferUniform( GpuGraphicsCommand_t * command, const int index, const GpuBuffer_t * buffer );
static void GpuGraphicsCommand_SetParmBufferStorage( GpuGraphicsCommand_t * command, const int index, const GpuBuffer_t * buffer );
static void GpuGraphicsCommand_SetParmInt( GpuGraphicsCommand_t * command, const int index, const int * value );
static void GpuGraphicsCommand_SetParmIntVector2( GpuGraphicsCommand_t * command, const int index, const Vector2i_t * value );
static void GpuGraphicsCommand_SetParmIntVector3( GpuGraphicsCommand_t * command, const int index, const Vector3i_t * value );
static void GpuGraphicsCommand_SetParmIntVector4( GpuGraphicsCommand_t * command, const int index, const Vector4i_t * value );
static void GpuGraphicsCommand_SetParmFloat( GpuGraphicsCommand_t * command, const int index, const float * value );
static void GpuGraphicsCommand_SetParmFloatVector2( GpuGraphicsCommand_t * command, const int index, const Vector2f_t * value );
static void GpuGraphicsCommand_SetParmFloatVector3( GpuGraphicsCommand_t * command, const int index, const Vector3f_t * value );
static void GpuGraphicsCommand_SetParmFloatVector4( GpuGraphicsCommand_t * command, const int index, const Vector3f_t * value );
static void GpuGraphicsCommand_SetParmFloatMatrix2x2( GpuGraphicsCommand_t * command, const int index, const Matrix2x2f_t * value );
static void GpuGraphicsCommand_SetParmFloatMatrix2x3( GpuGraphicsCommand_t * command, const int index, const Matrix2x3f_t * value );
static void GpuGraphicsCommand_SetParmFloatMatrix2x4( GpuGraphicsCommand_t * command, const int index, const Matrix2x4f_t * value );
static void GpuGraphicsCommand_SetParmFloatMatrix3x2( GpuGraphicsCommand_t * command, const int index, const Matrix3x2f_t * value );
static void GpuGraphicsCommand_SetParmFloatMatrix3x3( GpuGraphicsCommand_t * command, const int index, const Matrix3x3f_t * value );
static void GpuGraphicsCommand_SetParmFloatMatrix3x4( GpuGraphicsCommand_t * command, const int index, const Matrix3x4f_t * value );
static void GpuGraphicsCommand_SetParmFloatMatrix4x2( GpuGraphicsCommand_t * command, const int index, const Matrix4x2f_t * value );
static void GpuGraphicsCommand_SetParmFloatMatrix4x3( GpuGraphicsCommand_t * command, const int index, const Matrix4x3f_t * value );
static void GpuGraphicsCommand_SetParmFloatMatrix4x4( GpuGraphicsCommand_t * command, const int index, const Matrix4x4f_t * value );
static void GpuGraphicsCommand_SetNumInstances( GpuGraphicsCommand_t * command, const int numInstances );

================================================================================================================================
*/

typedef struct
{
	const GpuGraphicsPipeline_t *	pipeline;
	const GpuBuffer_t *				vertexBuffer;		// vertex buffer returned by GpuCommandBuffer_MapVertexAttributes
	const GpuBuffer_t *				instanceBuffer;		// instance buffer returned by GpuCommandBuffer_MapInstanceAttributes
	GpuProgramParmState_t			parmState;
	int								numInstances;
} GpuGraphicsCommand_t;

static void GpuGraphicsCommand_Init( GpuGraphicsCommand_t * command )
{
	command->pipeline = NULL;
	command->vertexBuffer = NULL;
	command->instanceBuffer = NULL;
	memset( (void *)&command->parmState, 0, sizeof( command->parmState ) );
	command->numInstances = 1;
}

static void GpuGraphicsCommand_SetPipeline( GpuGraphicsCommand_t * command, const GpuGraphicsPipeline_t * pipeline )
{
	command->pipeline = pipeline;
}

static void GpuGraphicsCommand_SetVertexBuffer( GpuGraphicsCommand_t * command, const GpuBuffer_t * vertexBuffer )
{
	command->vertexBuffer = vertexBuffer;
}

static void GpuGraphicsCommand_SetInstanceBuffer( GpuGraphicsCommand_t * command, const GpuBuffer_t * instanceBuffer )
{
	command->instanceBuffer = instanceBuffer;
}

static void GpuGraphicsCommand_SetParmTextureSampled( GpuGraphicsCommand_t * command, const int index, const GpuTexture_t * texture )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED, texture );
}

static void GpuGraphicsCommand_SetParmTextureStorage( GpuGraphicsCommand_t * command, const int index, const GpuTexture_t * texture )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE, texture );
}

static void GpuGraphicsCommand_SetParmBufferUniform( GpuGraphicsCommand_t * command, const int index, const GpuBuffer_t * buffer )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM, buffer );
}

static void GpuGraphicsCommand_SetParmBufferStorage( GpuGraphicsCommand_t * command, const int index, const GpuBuffer_t * buffer )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE, buffer );
}

static void GpuGraphicsCommand_SetParmInt( GpuGraphicsCommand_t * command, const int index, const int * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT, value );
}

static void GpuGraphicsCommand_SetParmIntVector2( GpuGraphicsCommand_t * command, const int index, const Vector2i_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2, value );
}

static void GpuGraphicsCommand_SetParmIntVector3( GpuGraphicsCommand_t * command, const int index, const Vector3i_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3, value );
}

static void GpuGraphicsCommand_SetParmIntVector4( GpuGraphicsCommand_t * command, const int index, const Vector4i_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4, value );
}

static void GpuGraphicsCommand_SetParmFloat( GpuGraphicsCommand_t * command, const int index, const float * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT, value );
}

static void GpuGraphicsCommand_SetParmFloatVector2( GpuGraphicsCommand_t * command, const int index, const Vector2f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2, value );
}

static void GpuGraphicsCommand_SetParmFloatVector3( GpuGraphicsCommand_t * command, const int index, const Vector3f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3, value );
}

static void GpuGraphicsCommand_SetParmFloatVector4( GpuGraphicsCommand_t * command, const int index, const Vector4f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4, value );
}

static void GpuGraphicsCommand_SetParmFloatMatrix2x2( GpuGraphicsCommand_t * command, const int index, const Matrix2x2f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X2, value );
}

static void GpuGraphicsCommand_SetParmFloatMatrix2x3( GpuGraphicsCommand_t * command, const int index, const Matrix2x3f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X3, value );
}

static void GpuGraphicsCommand_SetParmFloatMatrix2x4( GpuGraphicsCommand_t * command, const int index, const Matrix2x4f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X4, value );
}

static void GpuGraphicsCommand_SetParmFloatMatrix3x2( GpuGraphicsCommand_t * command, const int index, const Matrix3x2f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X2, value );
}

static void GpuGraphicsCommand_SetParmFloatMatrix3x3( GpuGraphicsCommand_t * command, const int index, const Matrix3x3f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X3, value );
}

static void GpuGraphicsCommand_SetParmFloatMatrix3x4( GpuGraphicsCommand_t * command, const int index, const Matrix3x4f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4, value );
}

static void GpuGraphicsCommand_SetParmFloatMatrix4x2( GpuGraphicsCommand_t * command, const int index, const Matrix4x2f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X2, value );
}

static void GpuGraphicsCommand_SetParmFloatMatrix4x3( GpuGraphicsCommand_t * command, const int index, const Matrix4x3f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X3, value );
}

static void GpuGraphicsCommand_SetParmFloatMatrix4x4( GpuGraphicsCommand_t * command, const int index, const Matrix4x4f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4, value );
}

static void GpuGraphicsCommand_SetNumInstances( GpuGraphicsCommand_t * command, const int numInstances )
{
	command->numInstances = numInstances;
}

/*
================================================================================================================================

GPU compute commands.

A compute command encapsulates all OpenGL state associated with a single dispatch.
The pointers passed in as parameters are expected to point to unique objects that persist
at least past the submission of the command buffer into which the compute command is
submitted. Because various pointer are maintained as state, DO NOT use pointers to local
variables that will go out of scope before the command buffer is submitted.

GpuComputeCommand_t

static void GpuComputeCommand_Init( GpuComputeCommand_t * command );
static void GpuComputeCommand_SetPipeline( GpuComputeCommand_t * command, const GpuComputePipeline_t * pipeline );
static void GpuComputeCommand_SetParmTextureSampled( GpuComputeCommand_t * command, const int index, const GpuTexture_t * texture );
static void GpuComputeCommand_SetParmTextureStorage( GpuComputeCommand_t * command, const int index, const GpuTexture_t * texture );
static void GpuComputeCommand_SetParmBufferUniform( GpuComputeCommand_t * command, const int index, const GpuBuffer_t * buffer );
static void GpuComputeCommand_SetParmBufferStorage( GpuComputeCommand_t * command, const int index, const GpuBuffer_t * buffer );
static void GpuComputeCommand_SetParmInt( GpuComputeCommand_t * command, const int index, const int * value );
static void GpuComputeCommand_SetParmIntVector2( GpuComputeCommand_t * command, const int index, const Vector2i_t * value );
static void GpuComputeCommand_SetParmIntVector3( GpuComputeCommand_t * command, const int index, const Vector3i_t * value );
static void GpuComputeCommand_SetParmIntVector4( GpuComputeCommand_t * command, const int index, const Vector4i_t * value );
static void GpuComputeCommand_SetParmFloat( GpuComputeCommand_t * command, const int index, const float * value );
static void GpuComputeCommand_SetParmFloatVector2( GpuComputeCommand_t * command, const int index, const Vector2f_t * value );
static void GpuComputeCommand_SetParmFloatVector3( GpuComputeCommand_t * command, const int index, const Vector3f_t * value );
static void GpuComputeCommand_SetParmFloatVector4( GpuComputeCommand_t * command, const int index, const Vector3f_t * value );
static void GpuComputeCommand_SetParmFloatMatrix2x2( GpuComputeCommand_t * command, const int index, const Matrix2x2f_t * value );
static void GpuComputeCommand_SetParmFloatMatrix2x3( GpuComputeCommand_t * command, const int index, const Matrix2x3f_t * value );
static void GpuComputeCommand_SetParmFloatMatrix2x4( GpuComputeCommand_t * command, const int index, const Matrix2x4f_t * value );
static void GpuComputeCommand_SetParmFloatMatrix3x2( GpuComputeCommand_t * command, const int index, const Matrix3x2f_t * value );
static void GpuComputeCommand_SetParmFloatMatrix3x3( GpuComputeCommand_t * command, const int index, const Matrix3x3f_t * value );
static void GpuComputeCommand_SetParmFloatMatrix3x4( GpuComputeCommand_t * command, const int index, const Matrix3x4f_t * value );
static void GpuComputeCommand_SetParmFloatMatrix4x2( GpuComputeCommand_t * command, const int index, const Matrix4x2f_t * value );
static void GpuComputeCommand_SetParmFloatMatrix4x3( GpuComputeCommand_t * command, const int index, const Matrix4x3f_t * value );
static void GpuComputeCommand_SetParmFloatMatrix4x4( GpuComputeCommand_t * command, const int index, const Matrix4x4f_t * value );
static void GpuComputeCommand_SetDimensions( GpuComputeCommand_t * command, const int x, const int y, const int z );

================================================================================================================================
*/

typedef struct
{
	const GpuComputePipeline_t *	pipeline;
	GpuProgramParmState_t			parmState;
	int								x;
	int								y;
	int								z;
} GpuComputeCommand_t;

static void GpuComputeCommand_Init( GpuComputeCommand_t * command )
{
	command->pipeline = NULL;
	memset( (void *)&command->parmState, 0, sizeof( command->parmState ) );
	command->x = 1;
	command->y = 1;
	command->z = 1;
}

static void GpuComputeCommand_SetPipeline( GpuComputeCommand_t * command, const GpuComputePipeline_t * pipeline )
{
	command->pipeline = pipeline;
}

static void GpuComputeCommand_SetParmTextureSampled( GpuComputeCommand_t * command, const int index, const GpuTexture_t * texture )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED, texture );
}

static void GpuComputeCommand_SetParmTextureStorage( GpuComputeCommand_t * command, const int index, const GpuTexture_t * texture )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE, texture );
}

static void GpuComputeCommand_SetParmBufferUniform( GpuComputeCommand_t * command, const int index, const GpuBuffer_t * buffer )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM, buffer );
}

static void GpuComputeCommand_SetParmBufferStorage( GpuComputeCommand_t * command, const int index, const GpuBuffer_t * buffer )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE, buffer );
}

static void GpuComputeCommand_SetParmInt( GpuComputeCommand_t * command, const int index, const int * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT, value );
}

static void GpuComputeCommand_SetParmIntVector2( GpuComputeCommand_t * command, const int index, const Vector2i_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2, value );
}

static void GpuComputeCommand_SetParmIntVector3( GpuComputeCommand_t * command, const int index, const Vector3i_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3, value );
}

static void GpuComputeCommand_SetParmIntVector4( GpuComputeCommand_t * command, const int index, const Vector4i_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4, value );
}

static void GpuComputeCommand_SetParmFloat( GpuComputeCommand_t * command, const int index, const float * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT, value );
}

static void GpuComputeCommand_SetParmFloatVector2( GpuComputeCommand_t * command, const int index, const Vector2f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2, value );
}

static void GpuComputeCommand_SetParmFloatVector3( GpuComputeCommand_t * command, const int index, const Vector3f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3, value );
}

static void GpuComputeCommand_SetParmFloatVector4( GpuComputeCommand_t * command, const int index, const Vector4f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4, value );
}

static void GpuComputeCommand_SetParmFloatMatrix2x2( GpuComputeCommand_t * command, const int index, const Matrix2x2f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X2, value );
}

static void GpuComputeCommand_SetParmFloatMatrix2x3( GpuComputeCommand_t * command, const int index, const Matrix2x3f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X3, value );
}

static void GpuComputeCommand_SetParmFloatMatrix2x4( GpuComputeCommand_t * command, const int index, const Matrix2x4f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X4, value );
}

static void GpuComputeCommand_SetParmFloatMatrix3x2( GpuComputeCommand_t * command, const int index, const Matrix3x2f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X2, value );
}

static void GpuComputeCommand_SetParmFloatMatrix3x3( GpuComputeCommand_t * command, const int index, const Matrix3x3f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X3, value );
}

static void GpuComputeCommand_SetParmFloatMatrix3x4( GpuComputeCommand_t * command, const int index, const Matrix3x4f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4, value );
}

static void GpuComputeCommand_SetParmFloatMatrix4x2( GpuComputeCommand_t * command, const int index, const Matrix4x2f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X2, value );
}

static void GpuComputeCommand_SetParmFloatMatrix4x3( GpuComputeCommand_t * command, const int index, const Matrix4x3f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X3, value );
}

static void GpuComputeCommand_SetParmFloatMatrix4x4( GpuComputeCommand_t * command, const int index, const Matrix4x4f_t * value )
{
	GpuProgramParmState_SetParm( &command->parmState, &command->pipeline->program->parmLayout, index, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4, value );
}

static void GpuComputeCommand_SetDimensions( GpuComputeCommand_t * command, const int x, const int y, const int z )
{
	command->x = x;
	command->y = y;
	command->z = z;
}

/*
================================================================================================================================

GPU command buffer.

A command buffer is used to record graphics and compute commands.
For optimal performance a command buffer should only be created at load time, not at runtime.
When a command is submitted, the state of the command is compared with the currently saved state,
and only the state that has changed translates into OpenGL function calls.

GpuCommandBuffer_t
GpuCommandBufferType_t
GpuBufferUnmapType_t

static void GpuCommandBuffer_Create( GpuContext_t * context, GpuCommandBuffer_t * commandBuffer, const GpuCommandBufferType_t type, const int numBuffers );
static void GpuCommandBuffer_Destroy( GpuContext_t * context, GpuCommandBuffer_t * commandBuffer );

static void GpuCommandBuffer_BeginPrimary( GpuCommandBuffer_t * commandBuffer );
static void GpuCommandBuffer_EndPrimary( GpuCommandBuffer_t * commandBuffer );
static GpuFence_t * GpuCommandBuffer_SubmitPrimary( GpuCommandBuffer_t * commandBuffer );

static void GpuCommandBuffer_ChangeTextureUsage( GpuCommandBuffer_t * commandBuffer, GpuTexture_t * texture, const GpuTextureUsage_t usage );

static void GpuCommandBuffer_BeginFramebuffer( GpuCommandBuffer_t * commandBuffer, GpuFramebuffer_t * framebuffer, const int arrayLayer, const GpuTextureUsage_t usage );
static void GpuCommandBuffer_EndFramebuffer( GpuCommandBuffer_t * commandBuffer, GpuFramebuffer_t * framebuffer, const int arrayLayer, const GpuTextureUsage_t usage );

static void GpuCommandBuffer_BeginTimer( GpuCommandBuffer_t * commandBuffer, GpuTimer_t * timer );
static void GpuCommandBuffer_EndTimer( GpuCommandBuffer_t * commandBuffer, GpuTimer_t * timer );

static void GpuCommandBuffer_BeginRenderPass( GpuCommandBuffer_t * commandBuffer, GpuRenderPass_t * renderPass, GpuFramebuffer_t * framebuffer, const ScreenRect_t * rect );
static void GpuCommandBuffer_EndRenderPass( GpuCommandBuffer_t * commandBuffer, GpuRenderPass_t * renderPass );

static void GpuCommandBuffer_SetViewport( GpuCommandBuffer_t * commandBuffer, const ScreenRect_t * rect );
static void GpuCommandBuffer_SetScissor( GpuCommandBuffer_t * commandBuffer, const ScreenRect_t * rect );

static void GpuCommandBuffer_SubmitGraphicsCommand( GpuCommandBuffer_t * commandBuffer, const GpuGraphicsCommand_t * command );
static void GpuCommandBuffer_SubmitComputeCommand( GpuCommandBuffer_t * commandBuffer, const GpuComputeCommand_t * command );

static GpuBuffer_t * GpuCommandBuffer_MapBuffer( GpuCommandBuffer_t * commandBuffer, GpuBuffer_t * buffer, void ** data );
static void GpuCommandBuffer_UnmapBuffer( GpuCommandBuffer_t * commandBuffer, GpuBuffer_t * buffer, GpuBuffer_t * mappedBuffer, const GpuBufferUnmapType_t type );

static GpuBuffer_t * GpuCommandBuffer_MapVertexAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuVertexAttributeArraysBase_t * attribs );
static void GpuCommandBuffer_UnmapVertexAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuBuffer_t * mappedVertexBuffer, const GpuBufferUnmapType_t type );

static GpuBuffer_t * GpuCommandBuffer_MapInstanceAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuVertexAttributeArraysBase_t * attribs );
static void GpuCommandBuffer_UnmapInstanceAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuBuffer_t * mappedInstanceBuffer, const GpuBufferUnmapType_t type );

================================================================================================================================
*/

typedef enum
{
	GPU_BUFFER_UNMAP_TYPE_USE_ALLOCATED,			// use the newly allocated (host visible) buffer
	GPU_BUFFER_UNMAP_TYPE_COPY_BACK					// copy back to the original buffer
} GpuBufferUnmapType_t;

typedef enum
{
	GPU_COMMAND_BUFFER_TYPE_PRIMARY,
	GPU_COMMAND_BUFFER_TYPE_SECONDARY,
	GPU_COMMAND_BUFFER_TYPE_SECONDARY_CONTINUE_RENDER_PASS
} GpuCommandBufferType_t;

typedef struct
{
	GpuCommandBufferType_t	type;
	int						numBuffers;
	int						currentBuffer;
	GpuFence_t *			fences;
	GpuContext_t *			context;
	GpuGraphicsCommand_t	currentGraphicsState;
	GpuComputeCommand_t		currentComputeState;
	GpuFramebuffer_t *		currentFramebuffer;
	GpuRenderPass_t *		currentRenderPass;
	GpuTextureUsage_t		currentTextureUsage;
} GpuCommandBuffer_t;

static void GpuCommandBuffer_Create( GpuContext_t * context, GpuCommandBuffer_t * commandBuffer, const GpuCommandBufferType_t type, const int numBuffers )
{
	assert( type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );

	memset( commandBuffer, 0, sizeof( GpuCommandBuffer_t ) );

	commandBuffer->type = type;
	commandBuffer->numBuffers = numBuffers;
	commandBuffer->currentBuffer = 0;
	commandBuffer->context = context;

	commandBuffer->fences = (GpuFence_t *) malloc( numBuffers * sizeof( GpuFence_t ) );

	for ( int i = 0; i < numBuffers; i++ )
	{
		GpuFence_Create( context, &commandBuffer->fences[i] );
	}
}

static void GpuCommandBuffer_Destroy( GpuContext_t * context, GpuCommandBuffer_t * commandBuffer )
{
	assert( context == commandBuffer->context );

	for ( int i = 0; i < commandBuffer->numBuffers; i++ )
	{
		GpuFence_Destroy( context, &commandBuffer->fences[i] );
	}

	free( commandBuffer->fences );

	memset( commandBuffer, 0, sizeof( GpuCommandBuffer_t ) );
}

void GpuCommandBuffer_ChangeRopState( const GpuRasterOperations_t * cmdRop, const GpuRasterOperations_t * stateRop )
{
	// Set front face.
	if ( stateRop == NULL ||
			cmdRop->frontFace != stateRop->frontFace )
	{
		GL( glFrontFace( cmdRop->frontFace ) );
	}
	// Set face culling.
	if ( stateRop == NULL ||
			cmdRop->cullMode != stateRop->cullMode )
	{
		if ( cmdRop->cullMode != GPU_CULL_MODE_NONE )
		{
			GL( glEnable( GL_CULL_FACE ) );
			GL( glCullFace( cmdRop->cullMode ) );
		}
		else
		{
			GL( glDisable( GL_CULL_FACE ) );
		}
	}
	// Enable / disable depth testing.
	if ( stateRop == NULL ||
			cmdRop->depthTestEnable != stateRop->depthTestEnable )
	{
		if ( cmdRop->depthTestEnable ) { GL( glEnable( GL_DEPTH_TEST ) ); } else { GL( glDisable( GL_DEPTH_TEST ) ); }
	}
	// The depth test function is only used when depth testing is enabled.
	if ( stateRop == NULL ||
			cmdRop->depthCompare != stateRop->depthCompare )
	{
		GL( glDepthFunc( cmdRop->depthCompare ) );
	}
	// Depth is only written when depth testing is enabled.
	// Set the depth function to GL_ALWAYS to write depth without actually testing depth.
	if ( stateRop == NULL ||
			cmdRop->depthWriteEnable != stateRop->depthWriteEnable )
	{
		if ( cmdRop->depthWriteEnable ) { GL( glDepthMask( GL_TRUE ) ); } else { GL( glDepthMask( GL_FALSE ) ); }
	}
	// Enable / disable blending.
	if ( stateRop == NULL ||
			cmdRop->blendEnable != stateRop->blendEnable )
	{
		if ( cmdRop->blendEnable ) { GL( glEnable( GL_BLEND ) ); } else { GL( glDisable( GL_BLEND ) ); }
	}
	// Enable / disable writing alpha.
	if ( stateRop == NULL ||
			cmdRop->redWriteEnable != stateRop->redWriteEnable ||
			cmdRop->blueWriteEnable != stateRop->blueWriteEnable ||
			cmdRop->greenWriteEnable != stateRop->greenWriteEnable ||
			cmdRop->alphaWriteEnable != stateRop->alphaWriteEnable )
	{
		GL( glColorMask(	cmdRop->redWriteEnable ? GL_TRUE : GL_FALSE,
							cmdRop->blueWriteEnable ? GL_TRUE : GL_FALSE,
							cmdRop->greenWriteEnable ? GL_TRUE : GL_FALSE,
							cmdRop->alphaWriteEnable ? GL_TRUE : GL_FALSE ) );
	}
	// The blend equation is only used when blending is enabled.
	if ( stateRop == NULL ||
			cmdRop->blendOpColor != stateRop->blendOpColor ||
				cmdRop->blendOpAlpha != stateRop->blendOpAlpha )
	{
		GL( glBlendEquationSeparate( cmdRop->blendOpColor, cmdRop->blendOpAlpha ) );
	}
	// The blend function is only used when blending is enabled.
	if ( stateRop == NULL ||
			cmdRop->blendSrcColor != stateRop->blendSrcColor ||
				cmdRop->blendDstColor != stateRop->blendDstColor ||
					cmdRop->blendSrcAlpha != stateRop->blendSrcAlpha ||
						cmdRop->blendDstAlpha != stateRop->blendDstAlpha )
	{
		GL( glBlendFuncSeparate( cmdRop->blendSrcColor, cmdRop->blendDstColor, cmdRop->blendSrcAlpha, cmdRop->blendDstAlpha ) );
	}
	// The blend color is only used when blending is enabled.
	if ( stateRop == NULL ||
			cmdRop->blendColor.x != stateRop->blendColor.x ||
			cmdRop->blendColor.y != stateRop->blendColor.y ||
			cmdRop->blendColor.z != stateRop->blendColor.z ||
			cmdRop->blendColor.w != stateRop->blendColor.w )
	{
		GL( glBlendColor( cmdRop->blendColor.x, cmdRop->blendColor.y, cmdRop->blendColor.z, cmdRop->blendColor.w ) );
	}
}

static void GpuCommandBuffer_BeginPrimary( GpuCommandBuffer_t * commandBuffer )
{
	assert( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( commandBuffer->currentRenderPass == NULL );

	commandBuffer->currentBuffer = ( commandBuffer->currentBuffer + 1 ) % commandBuffer->numBuffers;

	GpuGraphicsCommand_Init( &commandBuffer->currentGraphicsState );
	GpuComputeCommand_Init( &commandBuffer->currentComputeState );
	commandBuffer->currentTextureUsage = GPU_TEXTURE_USAGE_UNDEFINED;

	GpuGraphicsPipelineParms_t parms;
	GpuGraphicsPipelineParms_Init( &parms );
	GpuCommandBuffer_ChangeRopState( &parms.rop, NULL );

	GL( glUseProgram( 0 ) );
	GL( glBindVertexArray( 0 ) );
}

static void GpuCommandBuffer_EndPrimary( GpuCommandBuffer_t * commandBuffer )
{
	assert( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( commandBuffer->currentRenderPass == NULL );

	UNUSED_PARM( commandBuffer );
}

static GpuFence_t * GpuCommandBuffer_SubmitPrimary( GpuCommandBuffer_t * commandBuffer )
{
	assert( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( commandBuffer->currentRenderPass == NULL );

	GpuFence_t * fence = &commandBuffer->fences[commandBuffer->currentBuffer];
	GpuFence_Submit( commandBuffer->context, fence );

	return fence;
}

static void GpuCommandBuffer_ChangeTextureUsage( GpuCommandBuffer_t * commandBuffer, GpuTexture_t * texture, const GpuTextureUsage_t usage )
{
	assert( ( texture->usageFlags & usage ) != 0 );

	texture->usage = usage;

	if ( usage == commandBuffer->currentTextureUsage )
	{
		return;
	}

	const GLbitfield barriers =	( ( usage == GPU_TEXTURE_USAGE_TRANSFER_SRC ) ?		GL_TEXTURE_UPDATE_BARRIER_BIT :
								( ( usage == GPU_TEXTURE_USAGE_TRANSFER_DST ) ?		GL_TEXTURE_UPDATE_BARRIER_BIT :
								( ( usage == GPU_TEXTURE_USAGE_SAMPLED ) ?			GL_TEXTURE_FETCH_BARRIER_BIT :
								( ( usage == GPU_TEXTURE_USAGE_STORAGE ) ?			GL_SHADER_IMAGE_ACCESS_BARRIER_BIT :
								( ( usage == GPU_TEXTURE_USAGE_COLOR_ATTACHMENT ) ?	GL_FRAMEBUFFER_BARRIER_BIT :
								( ( usage == GPU_TEXTURE_USAGE_PRESENTATION ) ?		GL_ALL_BARRIER_BITS : GL_ALL_BARRIER_BITS ) ) ) ) ) );

	GL( glMemoryBarrier( barriers ) );

	commandBuffer->currentTextureUsage = usage;
}

static void GpuCommandBuffer_BeginFramebuffer( GpuCommandBuffer_t * commandBuffer, GpuFramebuffer_t * framebuffer, const int arrayLayer, const GpuTextureUsage_t usage )
{
	assert( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == NULL );
	assert( commandBuffer->currentRenderPass == NULL );
	assert( arrayLayer >= 0 && arrayLayer < framebuffer->numFramebuffersPerTexture );

	// Only advance when rendering to the first layer.
	if ( arrayLayer == 0 )
	{
		framebuffer->currentBuffer = ( framebuffer->currentBuffer + 1 ) % framebuffer->numBuffers;
	}

	GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, framebuffer->renderBuffers[framebuffer->currentBuffer * framebuffer->numFramebuffersPerTexture + arrayLayer] ) );

	if ( framebuffer->colorTextures != NULL )
	{
		framebuffer->colorTextures[framebuffer->currentBuffer].usage = usage;
	}
	commandBuffer->currentFramebuffer = framebuffer;
}

static void GpuCommandBuffer_EndFramebuffer( GpuCommandBuffer_t * commandBuffer, GpuFramebuffer_t * framebuffer, const int arrayLayer, const GpuTextureUsage_t usage )
{
	assert( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentFramebuffer == framebuffer );
	assert( commandBuffer->currentRenderPass == NULL );
	assert( arrayLayer >= 0 && arrayLayer < framebuffer->numFramebuffersPerTexture );

	UNUSED_PARM( framebuffer );
	UNUSED_PARM( arrayLayer );

	// If clamp to border is not available.
	if ( !glExtensions.texture_clamp_to_border )
	{
		// If rendering to a texture.
		if ( framebuffer->renderBuffers[framebuffer->currentBuffer * framebuffer->numFramebuffersPerTexture + arrayLayer] != 0 )
		{
			// Explicitly clear the border texels to black if the texture has clamp-to-border set.
			const GpuTexture_t * texture = &framebuffer->colorTextures[framebuffer->currentBuffer];
			if ( texture->wrapMode == GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER )
			{
				// Clear to fully opaque black.
				GL( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );
				// bottom
				GL( glScissor( 0, 0, texture->width, 1 ) );
				GL( glClear( GL_COLOR_BUFFER_BIT ) );
				// top
				GL( glScissor( 0, texture->height - 1, texture->width, 1 ) );
				GL( glClear( GL_COLOR_BUFFER_BIT ) );
				// left
				GL( glScissor( 0, 0, 1, texture->height ) );
				GL( glClear( GL_COLOR_BUFFER_BIT ) );
				// right
				GL( glScissor( texture->width - 1, 0, 1, texture->height ) );
				GL( glClear( GL_COLOR_BUFFER_BIT ) );
			}
		}
	}

#if defined( OS_ANDROID )
	// If this framebuffer has a depth buffer.
	if ( framebuffer->depthBuffer != 0 )
	{
		// Discard the depth buffer, so a tiler won't need to write it back out to memory.
		const GLenum depthAttachment[1] = { GL_DEPTH_ATTACHMENT };
		GL( glInvalidateFramebuffer( GL_DRAW_FRAMEBUFFER, 1, depthAttachment ) );
	}
#endif

	if ( framebuffer->resolveBuffers != framebuffer->renderBuffers )
	{
		const ScreenRect_t rect = GpuFramebuffer_GetRect( framebuffer );
		glBindFramebuffer( GL_READ_FRAMEBUFFER, framebuffer->renderBuffers[framebuffer->currentBuffer * framebuffer->numFramebuffersPerTexture + arrayLayer] );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, framebuffer->resolveBuffers[framebuffer->currentBuffer * framebuffer->numFramebuffersPerTexture + arrayLayer] );
		glBlitFramebuffer(	rect.x, rect.y, rect.width, rect.height,
							rect.x, rect.y, rect.width, rect.height,
							GL_COLOR_BUFFER_BIT, GL_NEAREST );
		glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
	}

	GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );

	if ( framebuffer->colorTextures != NULL )
	{
		framebuffer->colorTextures[framebuffer->currentBuffer].usage = usage;
	}
	commandBuffer->currentFramebuffer = NULL;
}

static void GpuCommandBuffer_BeginTimer( GpuCommandBuffer_t * commandBuffer, GpuTimer_t * timer )
{
	UNUSED_PARM( commandBuffer );

	if ( glExtensions.timer_query )
	{
		if ( timer->queryIndex >= GPU_TIMER_FRAMES_DELAYED )
		{
			GLuint64 beginGpuTime = 0;
			GL( glGetQueryObjectui64v( timer->beginQueries[timer->queryIndex % GPU_TIMER_FRAMES_DELAYED], GL_QUERY_RESULT, &beginGpuTime ) );
			GLuint64 endGpuTime = 0;
			GL( glGetQueryObjectui64v( timer->endQueries[timer->queryIndex % GPU_TIMER_FRAMES_DELAYED], GL_QUERY_RESULT, &endGpuTime ) );

			timer->gpuTime = ( endGpuTime - beginGpuTime ) / ( 1000.0f * 1000.0f );
		}

		GL( glQueryCounter( timer->beginQueries[timer->queryIndex % GPU_TIMER_FRAMES_DELAYED], GL_TIMESTAMP ) );
	}
}

static void GpuCommandBuffer_EndTimer( GpuCommandBuffer_t * commandBuffer, GpuTimer_t * timer )
{
	UNUSED_PARM( commandBuffer );

	if ( glExtensions.timer_query )
	{
		GL( glQueryCounter( timer->endQueries[timer->queryIndex % GPU_TIMER_FRAMES_DELAYED], GL_TIMESTAMP ) );
		timer->queryIndex++;
	}
}

static void GpuCommandBuffer_BeginRenderPass( GpuCommandBuffer_t * commandBuffer, GpuRenderPass_t * renderPass, GpuFramebuffer_t * framebuffer, const ScreenRect_t * rect )
{
	assert( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentRenderPass == NULL );
	assert( commandBuffer->currentFramebuffer == framebuffer );

	UNUSED_PARM( framebuffer );

	if ( ( renderPass->flags & ( GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER | GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER ) ) != 0 )
	{
		GL( glEnable( GL_SCISSOR_TEST ) );
		GL( glScissor( rect->x, rect->y, rect->width, rect->height ) );
		GL( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );
		GL( glClear(	( ( ( renderPass->flags & GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER ) != 0 ) ? GL_COLOR_BUFFER_BIT : 0 ) |
						( ( ( renderPass->flags & GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER ) != 0 ) ? GL_DEPTH_BUFFER_BIT : 0 ) ) );
	}

	commandBuffer->currentRenderPass = renderPass;
}

static void GpuCommandBuffer_EndRenderPass( GpuCommandBuffer_t * commandBuffer, GpuRenderPass_t * renderPass )
{
	assert( commandBuffer->type == GPU_COMMAND_BUFFER_TYPE_PRIMARY );
	assert( commandBuffer->currentRenderPass == renderPass );

	UNUSED_PARM( renderPass );

	commandBuffer->currentRenderPass = NULL;
}

static void GpuCommandBuffer_SetViewport( GpuCommandBuffer_t * commandBuffer, const ScreenRect_t * rect )
{
	UNUSED_PARM( commandBuffer );

	GL( glViewport( rect->x, rect->y, rect->width, rect->height ) );
}

static void GpuCommandBuffer_SetScissor( GpuCommandBuffer_t * commandBuffer, const ScreenRect_t * rect )
{
	UNUSED_PARM( commandBuffer );

	GL( glEnable( GL_SCISSOR_TEST ) );
	GL( glScissor( rect->x, rect->y, rect->width, rect->height ) );
}

static void GpuCommandBuffer_UpdateProgramParms( const GpuProgramParmLayout_t * newLayout,
												const GpuProgramParmLayout_t * oldLayout,
												const GpuProgramParmState_t * newParmState,
												const GpuProgramParmState_t * oldParmState,
												const bool force )
{
	const GpuTexture_t * oldSampledTextures[MAX_PROGRAM_PARMS] = { 0 };
	const GpuTexture_t * oldStorageTextures[MAX_PROGRAM_PARMS] = { 0 };
	const GpuBuffer_t * oldUniformBuffers[MAX_PROGRAM_PARMS] = { 0 };
	const GpuBuffer_t * oldStorageBuffers[MAX_PROGRAM_PARMS] = { 0 };
	int oldPushConstantParms[MAX_PROGRAM_PARMS] = { 0 };

	if ( oldLayout != NULL )
	{
		// Unbind from the bind points that will no longer be used, and gather
		// the objects that are bound at the bind points that will be used.
		for ( int i = 0; i < oldLayout->numParms; i++ )
		{
			const int index = oldLayout->parms[i].index;
			const int binding = oldLayout->parmBindings[i];

			if ( oldLayout->parms[i].type == GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED )
			{
				if ( binding >= newLayout->numSampledTextureBindings )
				{
					const GpuTexture_t * stateTexture = (const GpuTexture_t *)oldParmState->parms[index];
					GL( glActiveTexture( GL_TEXTURE0 + binding ) );
					GL( glBindTexture( stateTexture->target, 0 ) );
				}
				else
				{
					oldSampledTextures[binding] = (const GpuTexture_t *)oldParmState->parms[index];
				}
			}
			else if ( oldLayout->parms[i].type == GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE )
			{
				if ( binding >= newLayout->numStorageTextureBindings )
				{
					GL( glBindImageTexture( binding, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8 ) );
				}
				else
				{
					oldStorageTextures[binding] = (const GpuTexture_t *)oldParmState->parms[index];
				}
			}
			else if ( oldLayout->parms[i].type == GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM )
			{
				if ( binding >= newLayout->numUniformBufferBindings )
				{
					GL( glBindBufferBase( GL_UNIFORM_BUFFER, binding, 0 ) );
				}
				else
				{
					oldUniformBuffers[binding] = (const GpuBuffer_t *)oldParmState->parms[index];
				}
			}
			else if ( oldLayout->parms[i].type == GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE )
			{
				if ( binding >= newLayout->numStorageBufferBindings )
				{
					GL( glBindBufferBase( GL_SHADER_STORAGE_BUFFER, binding, 0 ) );
				}
				else
				{
					oldStorageBuffers[binding] = (const GpuBuffer_t *)oldParmState->parms[index];
				}
			}
			else
			{
				oldPushConstantParms[binding] = i;
			}
		}
	}

	/*
		Update the bind points:

		"texture image units"
		"image units"
		"uniform buffers"
		"shader storage buffers"
		"uniforms"

		Note that each bind point uses its own range of binding indices where each starts at zero.

		Note that even though multiple targets can be bound to the same texture image unit,
		the OpenGL spec disallows rendering from multiple targets using a single texture image unit.
	*/
	for ( int i = 0; i < newLayout->numParms; i++ )
	{
		const int index = newLayout->parms[i].index;
		const int binding = newLayout->parmBindings[i];

		assert( newParmState->parms[index] != NULL );
		if ( newLayout->parms[i].type == GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED )
		{
			const GpuTexture_t * texture = (const GpuTexture_t *)newParmState->parms[index];
			assert( texture->usage == GPU_TEXTURE_USAGE_SAMPLED );
			if ( force || texture != oldSampledTextures[binding] )
			{
				GL( glActiveTexture( GL_TEXTURE0 + binding ) );
				GL( glBindTexture( texture->target, texture->texture ) );
			}
		}
		else if ( newLayout->parms[i].type == GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE )
		{
			const GpuTexture_t * texture = (const GpuTexture_t *)newParmState->parms[index];
			assert( texture->usage == GPU_TEXTURE_USAGE_STORAGE );
			if ( force || texture != oldStorageTextures[binding] )
			{
				const GLenum access =	( ( newLayout->parms[i].access == GPU_PROGRAM_PARM_ACCESS_READ_ONLY ) ?		GL_READ_ONLY :
										( ( newLayout->parms[i].access == GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY ) ?	GL_WRITE_ONLY :
										( ( newLayout->parms[i].access == GPU_PROGRAM_PARM_ACCESS_READ_WRITE ) ?	GL_READ_WRITE :
																													0 ) ) );
				GL( glBindImageTexture( binding, texture->texture, 0, GL_FALSE, 0, access, texture->format ) );
			}
		}
		else if ( newLayout->parms[i].type == GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM )
		{
			const GpuBuffer_t * buffer = (const GpuBuffer_t *)newParmState->parms[index];
			assert( buffer->target == GL_UNIFORM_BUFFER );
			if ( force || buffer != oldUniformBuffers[binding] )
			{
				GL( glBindBufferBase( GL_UNIFORM_BUFFER, binding, buffer->buffer ) );
			}
		}
		else if ( newLayout->parms[i].type == GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE )
		{
			const GpuBuffer_t * buffer = (const GpuBuffer_t *)newParmState->parms[index];
			assert( buffer->target == GL_SHADER_STORAGE_BUFFER );
			if ( force || buffer != oldStorageBuffers[binding] )
			{
				GL( glBindBufferBase( GL_SHADER_STORAGE_BUFFER, binding, buffer->buffer ) );
			}
		}
		else
		{
			const void * newData = GpuProgramParmState_NewPushConstantData( newLayout, i, newParmState, oldLayout, oldPushConstantParms[binding], oldParmState, force );
			if ( newData != NULL )
			{
				const GLint location = newLayout->parmLocations[i];
				switch ( newLayout->parms[i].type )
				{
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT:				GL( glUniform1iv( location, 1, (const GLint *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2:		GL( glUniform2iv( location, 1, (const GLint *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3:		GL( glUniform3iv( location, 1, (const GLint *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4:		GL( glUniform4iv( location, 1, (const GLint *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT:				GL( glUniform1fv( location, 1, (const GLfloat *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2:		GL( glUniform2fv( location, 1, (const GLfloat *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3:		GL( glUniform3fv( location, 1, (const GLfloat *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4:		GL( glUniform4fv( location, 1, (const GLfloat *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X2:	GL( glUniformMatrix2fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X3:	GL( glUniformMatrix2x3fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X4:	GL( glUniformMatrix2x4fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X2:	GL( glUniformMatrix3x2fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X3:	GL( glUniformMatrix3fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4:	GL( glUniformMatrix3x4fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X2:	GL( glUniformMatrix4x2fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X3:	GL( glUniformMatrix4x3fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4:	GL( glUniformMatrix4fv( location, 1, GL_FALSE, (const GLfloat *)newData ) ); break;
					default: assert( false ); break;
				}
			}
		}
	}
}

static void GpuCommandBuffer_SubmitGraphicsCommand( GpuCommandBuffer_t * commandBuffer, const GpuGraphicsCommand_t * command )
{
	assert( commandBuffer->currentRenderPass != NULL );

	const GpuGraphicsCommand_t * state = &commandBuffer->currentGraphicsState;

	GpuCommandBuffer_ChangeRopState( &command->pipeline->rop, ( state->pipeline != NULL ) ? &state->pipeline->rop : NULL );

	// Compare programs based on a vertex/fragment source code hash value to minimize state changes when
	// the same source code is compiled to programs in different locations.
	const bool differentProgram = ( state->pipeline == NULL || command->pipeline->program->hash != state->pipeline->program->hash );

	if ( differentProgram )
	{
		GL( glUseProgram( command->pipeline->program->program ) );
	}

	GpuCommandBuffer_UpdateProgramParms( &command->pipeline->program->parmLayout,
										( state->pipeline != NULL ) ? &state->pipeline->program->parmLayout : NULL,
										&command->parmState, &state->parmState, differentProgram );

	if ( command->pipeline != state->pipeline )
	{
		GL( glBindVertexArray( command->pipeline->vertexArrayObject ) );
	}

	const GLenum indexType = ( sizeof( GpuTriangleIndex_t ) == sizeof( GLuint ) ) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
	if ( command->numInstances > 1 )
	{
		GL( glDrawElementsInstanced( GL_TRIANGLES, command->pipeline->geometry->indexCount, indexType, NULL, command->numInstances ) );
	}
	else
	{
		GL( glDrawElements( GL_TRIANGLES, command->pipeline->geometry->indexCount, indexType, NULL ) );
	}

	commandBuffer->currentGraphicsState = *command;
	commandBuffer->currentTextureUsage = GPU_TEXTURE_USAGE_UNDEFINED;
}

static void GpuCommandBuffer_SubmitComputeCommand( GpuCommandBuffer_t * commandBuffer, const GpuComputeCommand_t * command )
{
	assert( commandBuffer->currentRenderPass == NULL );

	const GpuComputeCommand_t * state = &commandBuffer->currentComputeState;

	// Compare programs based on a kernel source code hash value to minimize state changes when
	// the same source code is compiled to programs in different locations.
	const bool differentProgram = ( state->pipeline == NULL || command->pipeline->program->hash != state->pipeline->program->hash );

	if ( differentProgram )
	{
		GL( glUseProgram( command->pipeline->program->program ) );
	}

	GpuCommandBuffer_UpdateProgramParms( &command->pipeline->program->parmLayout,
										( state->pipeline != NULL ) ? &state->pipeline->program->parmLayout : NULL,
										&command->parmState, &state->parmState, differentProgram );

	GL( glDispatchCompute( command->x, command->y, command->z ) );

	commandBuffer->currentComputeState = *command;
	commandBuffer->currentTextureUsage = GPU_TEXTURE_USAGE_UNDEFINED;
}

static GpuBuffer_t * GpuCommandBuffer_MapBuffer( GpuCommandBuffer_t * commandBuffer, GpuBuffer_t * buffer, void ** data )
{
	UNUSED_PARM( commandBuffer );

	GL( glBindBuffer( buffer->target, buffer->buffer ) );
	GL( *data = glMapBufferRange( buffer->target, 0, buffer->size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT ) );
	GL( glBindBuffer( buffer->target, 0 ) );

	return buffer;
}

static void GpuCommandBuffer_UnmapBuffer( GpuCommandBuffer_t * commandBuffer, GpuBuffer_t * buffer, GpuBuffer_t * mappedBuffer, const GpuBufferUnmapType_t type )
{
	UNUSED_PARM( commandBuffer );
	UNUSED_PARM( buffer );

	assert( buffer == mappedBuffer );

	GL( glBindBuffer( mappedBuffer->target, mappedBuffer->buffer ) );
	GL( glUnmapBuffer( mappedBuffer->target ) );
	GL( glBindBuffer( mappedBuffer->target, 0 ) );

	if ( type == GPU_BUFFER_UNMAP_TYPE_COPY_BACK )
	{
		// Can only copy outside a render pass.
		assert( commandBuffer->currentRenderPass == NULL );
	}
}

static GpuBuffer_t * GpuCommandBuffer_MapVertexAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuVertexAttributeArraysBase_t * attribs )
{
	void * data = NULL;
	GpuBuffer_t * buffer = GpuCommandBuffer_MapBuffer( commandBuffer, &geometry->vertexBuffer, &data );

	attribs->layout = geometry->layout;
	GpuVertexAttributeArrays_Map( attribs, data, buffer->size, geometry->vertexCount, geometry->vertexAttribsFlags );

	return buffer;
}

static void GpuCommandBuffer_UnmapVertexAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuBuffer_t * mappedVertexBuffer, const GpuBufferUnmapType_t type )
{
	GpuCommandBuffer_UnmapBuffer( commandBuffer, &geometry->vertexBuffer, mappedVertexBuffer, type );
}

static GpuBuffer_t * GpuCommandBuffer_MapInstanceAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuVertexAttributeArraysBase_t * attribs )
{
	void * data = NULL;
	GpuBuffer_t * buffer = GpuCommandBuffer_MapBuffer( commandBuffer, &geometry->instanceBuffer, &data );

	attribs->layout = geometry->layout;
	GpuVertexAttributeArrays_Map( attribs, data, buffer->size, geometry->instanceCount, geometry->instanceAttribsFlags );

	return buffer;
}

static void GpuCommandBuffer_UnmapInstanceAttributes( GpuCommandBuffer_t * commandBuffer, GpuGeometry_t * geometry, GpuBuffer_t * mappedInstanceBuffer, const GpuBufferUnmapType_t type )
{
	GpuCommandBuffer_UnmapBuffer( commandBuffer, &geometry->instanceBuffer, mappedInstanceBuffer, type );
}

static void GpuCommandBuffer_Blit( GpuCommandBuffer_t * commandBuffer, GpuFramebuffer_t * srcFramebuffer, GpuFramebuffer_t * dstFramebuffer )
{
	UNUSED_PARM( commandBuffer );

	GpuTexture_t * srcTexture = &srcFramebuffer->colorTextures[srcFramebuffer->currentBuffer];
	GpuTexture_t * dstTexture = &dstFramebuffer->colorTextures[dstFramebuffer->currentBuffer];
	assert( srcTexture->width == dstTexture->width && srcTexture->height == dstTexture->height );
	UNUSED_PARM( dstTexture );

	GL( glBindFramebuffer( GL_READ_FRAMEBUFFER, srcFramebuffer->renderBuffers[srcFramebuffer->currentBuffer] ) );
	GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, dstFramebuffer->renderBuffers[dstFramebuffer->currentBuffer] ) );
	GL( glBlitFramebuffer( 0, 0, srcTexture->width, srcTexture->height,
							0, 0, srcTexture->width, srcTexture->height, GL_COLOR_BUFFER_BIT, GL_NEAREST ) );
	GL( glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 ) );
	GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
}

/*
================================================================================================================================

Bar graph.

Real-time bar graph where new bars scroll in on the right and old bars scroll out on the left.
Optionally supports stacking of bars. A bar value is in the range [0, 1] where 1 is a full height bar.
The bar graph position x,y,width,height is specified in clip coordinates in the range [-1, 1].

BarGraph_t

static void BarGraph_Create( GpuContext_t * context, BarGraph_t * barGraph, GpuRenderPass_t * renderPass,
								const float x, const float y, const float width, const float height,
								const int numBars, const int numStacked, const Vector4f_t * backgroundColor );
static void BarGraph_Destroy( GpuContext_t * context, BarGraph_t * barGraph );
static void BarGraph_AddBar( BarGraph_t * barGraph, const int stackedBar, const float value, const Vector4f_t * color, const bool advance );

static void BarGraph_UpdateGraphics( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph );
static void BarGraph_RenderGraphics( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph );

static void BarGraph_UpdateCompute( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph );
static void BarGraph_RenderCompute( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph, GpuFramebuffer_t * framebuffer );

================================================================================================================================
*/

typedef struct
{
	ClipRect_t			clipRect;
	int					numBars;
	int					numStacked;
	int					barIndex;
	float *				barValues;
	Vector4f_t *		barColors;
	Vector4f_t			backgroundColor;
	struct
	{
		GpuGeometry_t			quad;
		GpuGraphicsProgram_t	program;
		GpuGraphicsPipeline_t	pipeline;
		int						numInstances;
	} graphics;
#if OPENGL_COMPUTE_ENABLED == 1
	struct
	{
		GpuBuffer_t				barValueBuffer;
		GpuBuffer_t				barColorBuffer;
		Vector2i_t				barGraphOffset;
		GpuComputeProgram_t		program;
		GpuComputePipeline_t	pipeline;
	} compute;
#endif
} BarGraph_t;

static const GpuProgramParm_t barGraphGraphicsProgramParms[] =
{
	{ 0 }
};

static const char barGraphVertexProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"in vec3 vertexPosition;\n"
	"in mat4 vertexTransform;\n"
	"out vec4 fragmentColor;\n"
	"out gl_PerVertex { vec4 gl_Position; };\n"
	"vec3 multiply4x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[1].x * v.y + m[2].x * v.z + m[3].x,\n"
	"		m[0].y * v.x + m[1].y * v.y + m[2].y * v.z + m[3].y,\n"
	"		m[0].z * v.x + m[1].z * v.y + m[2].z * v.z + m[3].z );\n"
	"}\n"
	"void main( void )\n"
	"{\n"
	"	gl_Position.xyz = multiply4x3( vertexTransform, vertexPosition );\n"
	"	gl_Position.w = 1.0;\n"
	"	fragmentColor.r = vertexTransform[0][3];\n"
	"	fragmentColor.g = vertexTransform[1][3];\n"
	"	fragmentColor.b = vertexTransform[2][3];\n"
	"	fragmentColor.a = vertexTransform[3][3];\n"
	"}\n";

static const char barGraphFragmentProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"in lowp vec4 fragmentColor;\n"
	"out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	outColor = fragmentColor;\n"
	"}\n";

#if OPENGL_COMPUTE_ENABLED == 1

enum
{
	COMPUTE_PROGRAM_TEXTURE_BAR_GRAPH_DEST,
	COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_VALUES,
	COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_COLORS,
	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_BARS,
	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_STACKED,
	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_INDEX,
	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_GRAPH_OFFSET,
	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BACK_GROUND_COLOR
};

static const GpuProgramParm_t barGraphComputeProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,				GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,	COMPUTE_PROGRAM_TEXTURE_BAR_GRAPH_DEST,					"dest",				0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_VALUES,			"barValueBuffer",	0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_BUFFER_STORAGE,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_COLORS,			"barColorBuffer",	1 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BACK_GROUND_COLOR,	"backgroundColor",	0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_GRAPH_OFFSET,		"barGraphOffset",	1 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_BARS,				"numBars",			2 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_STACKED,			"numStacked",		3 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_INDEX,			"barIndex",			4 }
};

#define BARGRAPH_LOCAL_SIZE_X	8
#define BARGRAPH_LOCAL_SIZE_Y	8

static const char barGraphComputeProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"\n"
	"layout( local_size_x = " STRINGIFY( BARGRAPH_LOCAL_SIZE_X ) ", local_size_y = " STRINGIFY( BARGRAPH_LOCAL_SIZE_Y ) " ) in;\n"
	"\n"
	"layout( rgba8, binding = 0 ) uniform writeonly " ES_HIGHP " image2D dest;\n"
	"layout( std430, binding = 0 ) buffer barValueBuffer { float barValues[]; };\n"
	"layout( std430, binding = 1 ) buffer barColorBuffer { vec4 barColors[]; };\n"
	"uniform lowp vec4 backgroundColor;\n"
	"uniform ivec2 barGraphOffset;\n"
	"uniform int numBars;\n"
	"uniform int numStacked;\n"
 	"uniform int barIndex;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	ivec2 barGraph = ivec2( gl_GlobalInvocationID.xy );\n"
	"	ivec2 barGraphSize = ivec2( gl_NumWorkGroups.xy * gl_WorkGroupSize.xy );\n"
	"\n"
	"	int index = barGraph.x * numBars / barGraphSize.x;\n"
	"	int barOffset = ( ( barIndex + index ) % numBars ) * numStacked;\n"
	"	float barColorScale = ( ( index & 1 ) != 0 ) ? 0.75f : 1.0f;\n"
	"\n"
	"	vec4 rgba = backgroundColor;\n"
	"	float localY = float( barGraph.y );\n"
	"	float stackedBarValue = 0.0f;\n"
	"	for ( int i = 0; i < numStacked; i++ )\n"
	"	{\n"
	"		stackedBarValue += barValues[barOffset + i];\n"
	"		if ( localY < stackedBarValue * float( barGraphSize.y ) )\n"
	"		{\n"
	"			rgba = barColors[barOffset + i] * barColorScale;\n"
	"			break;\n"
	"		}\n"
	"	}\n"
	"\n"
	"	imageStore( dest, barGraphOffset + barGraph, rgba );\n"
	"}\n";

#endif

static void BarGraph_Create( GpuContext_t * context, BarGraph_t * barGraph, GpuRenderPass_t * renderPass,
								const float x, const float y, const float width, const float height,
								const int numBars, const int numStacked, const Vector4f_t * backgroundColor )
{
	barGraph->clipRect.x = x;
	barGraph->clipRect.y = y;
	barGraph->clipRect.width = width;
	barGraph->clipRect.height = height;
	barGraph->numBars = numBars;
	barGraph->numStacked = numStacked;
	barGraph->barIndex = 0;
	barGraph->barValues = (float *) AllocAlignedMemory( numBars * numStacked * sizeof( barGraph->barValues[0] ), sizeof( void * ) );
	barGraph->barColors = (Vector4f_t *) AllocAlignedMemory( numBars * numStacked * sizeof( barGraph->barColors[0] ), sizeof( Vector4f_t ) );

	for ( int i = 0; i < numBars * numStacked; i++ )
	{
		barGraph->barValues[i] = 0.0f;
		barGraph->barColors[i] = colorGreen;
	}

	barGraph->backgroundColor = *backgroundColor;

	// graphics
	{
		GpuGeometry_CreateQuad( context, &barGraph->graphics.quad, 1.0f, 0.5f );
		GpuGeometry_AddInstanceAttributes( context, &barGraph->graphics.quad, numBars * numStacked + 1, VERTEX_ATTRIBUTE_FLAG_TRANSFORM );

		GpuGraphicsProgram_Create( context, &barGraph->graphics.program,
									PROGRAM( barGraphVertexProgram ), sizeof( PROGRAM( barGraphVertexProgram ) ),
									PROGRAM( barGraphFragmentProgram ), sizeof( PROGRAM( barGraphFragmentProgram ) ),
									barGraphGraphicsProgramParms, 0,
									barGraph->graphics.quad.layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_TRANSFORM );

		GpuGraphicsPipelineParms_t pipelineParms;
		GpuGraphicsPipelineParms_Init( &pipelineParms );

		pipelineParms.rop.depthTestEnable = false;
		pipelineParms.rop.depthWriteEnable = false;
		pipelineParms.renderPass = renderPass;
		pipelineParms.program = &barGraph->graphics.program;
		pipelineParms.geometry = &barGraph->graphics.quad;

		GpuGraphicsPipeline_Create( context, &barGraph->graphics.pipeline, &pipelineParms );

		barGraph->graphics.numInstances = 0;
	}

#if OPENGL_COMPUTE_ENABLED == 1
	// compute
	{
		GpuBuffer_Create( context, &barGraph->compute.barValueBuffer, GPU_BUFFER_TYPE_STORAGE,
							barGraph->numBars * barGraph->numStacked * sizeof( barGraph->barValues[0] ), NULL, false );
		GpuBuffer_Create( context, &barGraph->compute.barColorBuffer, GPU_BUFFER_TYPE_STORAGE,
							barGraph->numBars * barGraph->numStacked * sizeof( barGraph->barColors[0] ), NULL, false );

		GpuComputeProgram_Create( context, &barGraph->compute.program,
									PROGRAM( barGraphComputeProgram ), sizeof( PROGRAM( barGraphComputeProgram ) ),
									barGraphComputeProgramParms, ARRAY_SIZE( barGraphComputeProgramParms ) );

		GpuComputePipeline_Create( context, &barGraph->compute.pipeline, &barGraph->compute.program );
	}
#endif
}

static void BarGraph_Destroy( GpuContext_t * context, BarGraph_t * barGraph )
{
	FreeAlignedMemory( barGraph->barValues );
	FreeAlignedMemory( barGraph->barColors );

	// graphics
	{
		GpuGraphicsPipeline_Destroy( context, &barGraph->graphics.pipeline );
		GpuGraphicsProgram_Destroy( context, &barGraph->graphics.program );
		GpuGeometry_Destroy( context, &barGraph->graphics.quad );
	}

#if OPENGL_COMPUTE_ENABLED == 1
	// compute
	{
		GpuComputePipeline_Destroy( context, &barGraph->compute.pipeline );
		GpuComputeProgram_Destroy( context, &barGraph->compute.program );
		GpuBuffer_Destroy( context, &barGraph->compute.barValueBuffer );
		GpuBuffer_Destroy( context, &barGraph->compute.barColorBuffer );
	}
#endif
}

static void BarGraph_AddBar( BarGraph_t * barGraph, const int stackedBar, const float value, const Vector4f_t * color, const bool advance )
{
	assert( stackedBar >= 0 && stackedBar < barGraph->numStacked );
	barGraph->barValues[barGraph->barIndex * barGraph->numStacked + stackedBar] = value;
	barGraph->barColors[barGraph->barIndex * barGraph->numStacked + stackedBar] = *color;
	if ( advance )
	{
		barGraph->barIndex = ( barGraph->barIndex + 1 ) % barGraph->numBars;
	}
}

static void BarGraph_UpdateGraphics( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph )
{
	GpuVertexAttributeArrays_t attribs;
	GpuBuffer_t * instanceBuffer = GpuCommandBuffer_MapInstanceAttributes( commandBuffer, &barGraph->graphics.quad, &attribs.base );

#if defined( GRAPHICS_API_VULKAN )
	const float flipY = -1.0f;
#else
	const float flipY = 1.0f;
#endif

	int numInstances = 0;
	Matrix4x4f_t * backgroundMatrix = &attribs.transform[numInstances++];

	// Write in order to write-combined memory.
	backgroundMatrix->m[0][0] = barGraph->clipRect.width;
	backgroundMatrix->m[0][1] = 0.0f;
	backgroundMatrix->m[0][2] = 0.0f;
	backgroundMatrix->m[0][3] = barGraph->backgroundColor.x;

	backgroundMatrix->m[1][0] = 0.0f;
	backgroundMatrix->m[1][1] = barGraph->clipRect.height * flipY;
	backgroundMatrix->m[1][2] = 0.0f;
	backgroundMatrix->m[1][3] = barGraph->backgroundColor.y;

	backgroundMatrix->m[2][0] = 0.0f;
	backgroundMatrix->m[2][1] = 0.0f;
	backgroundMatrix->m[2][2] = 0.0f;
	backgroundMatrix->m[2][3] = barGraph->backgroundColor.z;

	backgroundMatrix->m[3][0] = barGraph->clipRect.x;
	backgroundMatrix->m[3][1] = barGraph->clipRect.y * flipY;
	backgroundMatrix->m[3][2] = 0.0f;
	backgroundMatrix->m[3][3] = barGraph->backgroundColor.w;

	const float barWidth = barGraph->clipRect.width / barGraph->numBars;

	for ( int i = 0; i < barGraph->numBars; i++ )
	{
		const int barIndex = ( ( barGraph->barIndex + i ) % barGraph->numBars ) * barGraph->numStacked;
		const float barColorScale = ( i & 1 ) ? 0.75f : 1.0f;

		float stackedBarValue = 0.0f;
		for ( int j = 0; j < barGraph->numStacked; j++ )
		{
			float value = barGraph->barValues[barIndex + j];
			if ( stackedBarValue + value > 1.0f )
			{
				value = 1.0f - stackedBarValue;
			}
			if ( value <= 0.0f )
			{
				continue;
			}

			Matrix4x4f_t * barMatrix = &attribs.transform[numInstances++];

			// Write in order to write-combined memory.
			barMatrix->m[0][0] = barWidth;
			barMatrix->m[0][1] = 0.0f;
			barMatrix->m[0][2] = 0.0f;
			barMatrix->m[0][3] = barGraph->barColors[barIndex + j].x * barColorScale;

			barMatrix->m[1][0] = 0.0f;
			barMatrix->m[1][1] = value * barGraph->clipRect.height * flipY;
			barMatrix->m[1][2] = 0.0f;
			barMatrix->m[1][3] = barGraph->barColors[barIndex + j].y * barColorScale;

			barMatrix->m[2][0] = 0.0f;
			barMatrix->m[2][1] = 0.0f;
			barMatrix->m[2][2] = 1.0f;
			barMatrix->m[2][3] = barGraph->barColors[barIndex + j].z * barColorScale;

			barMatrix->m[3][0] = barGraph->clipRect.x + i * barWidth;
			barMatrix->m[3][1] = ( barGraph->clipRect.y + stackedBarValue * barGraph->clipRect.height ) * flipY;
			barMatrix->m[3][2] = 0.0f;
			barMatrix->m[3][3] = barGraph->barColors[barIndex + j].w;

			stackedBarValue += value;
		}
	}

	GpuCommandBuffer_UnmapInstanceAttributes( commandBuffer, &barGraph->graphics.quad, instanceBuffer, GPU_BUFFER_UNMAP_TYPE_COPY_BACK );

	assert( numInstances <= barGraph->numBars * barGraph->numStacked + 1 );
	barGraph->graphics.numInstances = numInstances;
}

static void BarGraph_RenderGraphics( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph )
{
	GpuGraphicsCommand_t command;
	GpuGraphicsCommand_Init( &command );
	GpuGraphicsCommand_SetPipeline( &command, &barGraph->graphics.pipeline );
	GpuGraphicsCommand_SetNumInstances( &command, barGraph->graphics.numInstances );

	GpuCommandBuffer_SubmitGraphicsCommand( commandBuffer, &command );
}

static void BarGraph_UpdateCompute( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph )
{
#if OPENGL_COMPUTE_ENABLED == 1
	void * barValues = NULL;
	GpuBuffer_t * mappedBarValueBuffer = GpuCommandBuffer_MapBuffer( commandBuffer, &barGraph->compute.barValueBuffer, &barValues );
	memcpy( barValues, barGraph->barValues, barGraph->numBars * barGraph->numStacked * sizeof( barGraph->barValues[0] ) );
	GpuCommandBuffer_UnmapBuffer( commandBuffer, &barGraph->compute.barValueBuffer, mappedBarValueBuffer, GPU_BUFFER_UNMAP_TYPE_COPY_BACK );

	void * barColors = NULL;
	GpuBuffer_t * mappedBarColorBuffer = GpuCommandBuffer_MapBuffer( commandBuffer, &barGraph->compute.barColorBuffer, &barColors );
	memcpy( barColors, barGraph->barColors, barGraph->numBars * barGraph->numStacked * sizeof( barGraph->barColors[0] ) );
	GpuCommandBuffer_UnmapBuffer( commandBuffer, &barGraph->compute.barColorBuffer, mappedBarColorBuffer, GPU_BUFFER_UNMAP_TYPE_COPY_BACK );
#else
	UNUSED_PARM( commandBuffer );
	UNUSED_PARM( barGraph );
#endif
}

static void BarGraph_RenderCompute( GpuCommandBuffer_t * commandBuffer, BarGraph_t * barGraph, GpuFramebuffer_t * framebuffer )
{
#if OPENGL_COMPUTE_ENABLED == 1
	const int screenWidth = GpuFramebuffer_GetWidth( framebuffer );
	const int screenHeight = GpuFramebuffer_GetHeight( framebuffer );
	ScreenRect_t screenRect = ClipRect_ToScreenRect( &barGraph->clipRect, screenWidth, screenHeight );
	barGraph->compute.barGraphOffset.x = screenRect.x;
#if defined( GRAPHICS_API_VULKAN )
	barGraph->compute.barGraphOffset.y = screenHeight - 1 - screenRect.y;
#else
	barGraph->compute.barGraphOffset.y = screenRect.y;
#endif

	screenRect.width = ROUNDUP( screenRect.width, 8 );
	screenRect.height = ROUNDUP( screenRect.height, 8 );

	assert( screenRect.width % BARGRAPH_LOCAL_SIZE_X == 0 );
	assert( screenRect.height % BARGRAPH_LOCAL_SIZE_Y == 0 );

	GpuComputeCommand_t command;
	GpuComputeCommand_Init( &command );
	GpuComputeCommand_SetPipeline( &command, &barGraph->compute.pipeline );
	GpuComputeCommand_SetParmTextureStorage( &command, COMPUTE_PROGRAM_TEXTURE_BAR_GRAPH_DEST, GpuFramebuffer_GetColorTexture( framebuffer ) );
	GpuComputeCommand_SetParmBufferStorage( &command, COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_VALUES, &barGraph->compute.barValueBuffer );
	GpuComputeCommand_SetParmBufferStorage( &command, COMPUTE_PROGRAM_BUFFER_BAR_GRAPH_BAR_COLORS, &barGraph->compute.barColorBuffer );
	GpuComputeCommand_SetParmFloatVector4( &command, COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BACK_GROUND_COLOR, &barGraph->backgroundColor );
	GpuComputeCommand_SetParmIntVector2( &command, COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_GRAPH_OFFSET, &barGraph->compute.barGraphOffset );
	GpuComputeCommand_SetParmInt( &command, COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_BARS, &barGraph->numBars );
	GpuComputeCommand_SetParmInt( &command, COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_NUM_STACKED, &barGraph->numStacked );
	GpuComputeCommand_SetParmInt( &command, COMPUTE_PROGRAM_UNIFORM_BAR_GRAPH_BAR_INDEX, &barGraph->barIndex );
	GpuComputeCommand_SetDimensions( &command, screenRect.width / BARGRAPH_LOCAL_SIZE_X, screenRect.height / BARGRAPH_LOCAL_SIZE_Y, 1 );

	GpuCommandBuffer_SubmitComputeCommand( commandBuffer, &command );
#else
	UNUSED_PARM( commandBuffer );
	UNUSED_PARM( barGraph );
	UNUSED_PARM( framebuffer );
#endif
}

/*
================================================================================================================================

Time warp bar graphs.

TimeWarpBarGraphs_t

static void TimeWarpBarGraphs_Create( GpuContext_t * context, TimeWarpBarGraphs_t * bargraphs, GpuFramebuffer_t * framebuffer );
static void TimeWarpBarGraphs_Destroy( GpuContext_t * context, TimeWarpBarGraphs_t * bargraphs );

static void TimeWarpBarGraphs_UpdateGraphics( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs );
static void TimeWarpBarGraphs_RenderGraphics( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs );

static void TimeWarpBarGraphs_UpdateCompute( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs );
static void TimeWarpBarGraphs_RenderCompute( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs, GpuFramebuffer_t * framebuffer );

static float TimeWarpBarGraphs_GetGpuMillisecondsGraphics( TimeWarpBarGraphs_t * bargraphs );
static float TimeWarpBarGraphs_GetGpuMillisecondsCompute( TimeWarpBarGraphs_t * bargraphs );

================================================================================================================================
*/

#define BARGRAPH_VIRTUAL_PIXELS_WIDE		1920
#define BARGRAPH_VIRTUAL_PIXELS_HIGH		1080

#if defined( OS_ANDROID )
#define BARGRAPH_INSET						64
#else
#define BARGRAPH_INSET						16
#endif

static const ScreenRect_t eyeTextureFrameRateBarGraphRect			= { BARGRAPH_INSET + 0 * 264, BARGRAPH_INSET, 256, 128 };
static const ScreenRect_t timeWarpFrameRateBarGraphRect				= { BARGRAPH_INSET + 1 * 264, BARGRAPH_INSET, 256, 128 };
static const ScreenRect_t frameCpuTimeBarGraphRect					= { BARGRAPH_INSET + 2 * 264, BARGRAPH_INSET, 256, 128 };
static const ScreenRect_t frameGpuTimeBarGraphRect					= { BARGRAPH_INSET + 3 * 264, BARGRAPH_INSET, 256, 128 };

static const ScreenRect_t multiViewBarGraphRect						= { 2 * BARGRAPH_VIRTUAL_PIXELS_WIDE / 3 + 0 * 40, BARGRAPH_INSET, 32, 32 };
static const ScreenRect_t correctChromaticAberrationBarGraphRect	= { 2 * BARGRAPH_VIRTUAL_PIXELS_WIDE / 3 + 1 * 40, BARGRAPH_INSET, 32, 32 };
static const ScreenRect_t timeWarpImplementationBarGraphRect		= { 2 * BARGRAPH_VIRTUAL_PIXELS_WIDE / 3 + 2 * 40, BARGRAPH_INSET, 32, 32 };

static const ScreenRect_t displayResolutionLevelBarGraphRect		= { BARGRAPH_VIRTUAL_PIXELS_WIDE - 7 * 40 - BARGRAPH_INSET, BARGRAPH_INSET, 32, 128 };
static const ScreenRect_t eyeImageResolutionLevelBarGraphRect		= { BARGRAPH_VIRTUAL_PIXELS_WIDE - 6 * 40 - BARGRAPH_INSET, BARGRAPH_INSET, 32, 128 };
static const ScreenRect_t eyeImageSamplesLevelBarGraphRect			= { BARGRAPH_VIRTUAL_PIXELS_WIDE - 5 * 40 - BARGRAPH_INSET, BARGRAPH_INSET, 32, 128 };

static const ScreenRect_t sceneDrawCallLevelBarGraphRect			= { BARGRAPH_VIRTUAL_PIXELS_WIDE - 3 * 40 - BARGRAPH_INSET, BARGRAPH_INSET, 32, 128 };
static const ScreenRect_t sceneTriangleLevelBarGraphRect			= { BARGRAPH_VIRTUAL_PIXELS_WIDE - 2 * 40 - BARGRAPH_INSET, BARGRAPH_INSET, 32, 128 };
static const ScreenRect_t sceneFragmentLevelBarGraphRect			= { BARGRAPH_VIRTUAL_PIXELS_WIDE - 1 * 40 - BARGRAPH_INSET, BARGRAPH_INSET, 32, 128 };

typedef enum
{
	BAR_GRAPH_HIDDEN,
	BAR_GRAPH_VISIBLE,
	BAR_GRAPH_PAUSED
} BarGraphState_t;

typedef struct
{
	BarGraphState_t	barGraphState;

	BarGraph_t		eyeTexturesFrameRateGraph;
	BarGraph_t		timeWarpFrameRateGraph;
	BarGraph_t		frameCpuTimeBarGraph;
	BarGraph_t		frameGpuTimeBarGraph;

	BarGraph_t		multiViewBarGraph;
	BarGraph_t		correctChromaticAberrationBarGraph;
	BarGraph_t		timeWarpImplementationBarGraph;

	BarGraph_t		displayResolutionLevelBarGraph;
	BarGraph_t		eyeImageResolutionLevelBarGraph;
	BarGraph_t		eyeImageSamplesLevelBarGraph;

	BarGraph_t		sceneDrawCallLevelBarGraph;
	BarGraph_t		sceneTriangleLevelBarGraph;
	BarGraph_t		sceneFragmentLevelBarGraph;

	GpuTimer_t		barGraphTimer;
} TimeWarpBarGraphs_t;

enum
{
	PROFILE_TIME_EYE_TEXTURES,
	PROFILE_TIME_TIME_WARP,
	PROFILE_TIME_BAR_GRAPHS,
	PROFILE_TIME_BLIT,
	PROFILE_TIME_OVERFLOW,
	PROFILE_TIME_MAX
};

static const Vector4f_t * profileTimeBarColors[] =
{
	&colorPurple,
	&colorGreen,
	&colorYellow,
	&colorBlue,
	&colorRed
};

static void BarGraph_CreateVirtualRect( GpuContext_t * context, BarGraph_t * barGraph, GpuRenderPass_t * renderPass,
						const ScreenRect_t * virtualRect, const int numBars, const int numStacked, const Vector4f_t * backgroundColor )
{
	const ClipRect_t clipRect = ScreenRect_ToClipRect( virtualRect, BARGRAPH_VIRTUAL_PIXELS_WIDE, BARGRAPH_VIRTUAL_PIXELS_HIGH );
	BarGraph_Create( context, barGraph, renderPass, clipRect.x, clipRect.y, clipRect.width, clipRect.height, numBars, numStacked, backgroundColor );
}

static void TimeWarpBarGraphs_Create( GpuContext_t * context, TimeWarpBarGraphs_t * bargraphs, GpuRenderPass_t * renderPass )
{
	bargraphs->barGraphState = BAR_GRAPH_VISIBLE;

	BarGraph_CreateVirtualRect( context, &bargraphs->eyeTexturesFrameRateGraph, renderPass, &eyeTextureFrameRateBarGraphRect, 64, 1, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->timeWarpFrameRateGraph, renderPass, &timeWarpFrameRateBarGraphRect, 64, 1, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->frameCpuTimeBarGraph, renderPass, &frameCpuTimeBarGraphRect, 64, PROFILE_TIME_MAX, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->frameGpuTimeBarGraph, renderPass, &frameGpuTimeBarGraphRect, 64, PROFILE_TIME_MAX, &colorDarkGrey );

	BarGraph_CreateVirtualRect( context, &bargraphs->multiViewBarGraph, renderPass, &multiViewBarGraphRect, 1, 1, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->correctChromaticAberrationBarGraph, renderPass, &correctChromaticAberrationBarGraphRect, 1, 1, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->timeWarpImplementationBarGraph, renderPass, &timeWarpImplementationBarGraphRect, 1, 1, &colorDarkGrey );

	BarGraph_CreateVirtualRect( context, &bargraphs->displayResolutionLevelBarGraph, renderPass, &displayResolutionLevelBarGraphRect, 1, 4, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->eyeImageResolutionLevelBarGraph, renderPass, &eyeImageResolutionLevelBarGraphRect, 1, 4, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->eyeImageSamplesLevelBarGraph, renderPass, &eyeImageSamplesLevelBarGraphRect, 1, 4, &colorDarkGrey );

	BarGraph_CreateVirtualRect( context, &bargraphs->sceneDrawCallLevelBarGraph, renderPass, &sceneDrawCallLevelBarGraphRect, 1, 4, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->sceneTriangleLevelBarGraph, renderPass, &sceneTriangleLevelBarGraphRect, 1, 4, &colorDarkGrey );
	BarGraph_CreateVirtualRect( context, &bargraphs->sceneFragmentLevelBarGraph, renderPass, &sceneFragmentLevelBarGraphRect, 1, 4, &colorDarkGrey );

	BarGraph_AddBar( &bargraphs->displayResolutionLevelBarGraph, 0, 0.25f, &colorBlue, false );
	BarGraph_AddBar( &bargraphs->eyeImageResolutionLevelBarGraph, 0, 0.25f, &colorBlue, false );
	BarGraph_AddBar( &bargraphs->eyeImageSamplesLevelBarGraph, 0, 0.25f, &colorBlue, false );

	BarGraph_AddBar( &bargraphs->sceneDrawCallLevelBarGraph, 0, 0.25f, &colorBlue, false );
	BarGraph_AddBar( &bargraphs->sceneTriangleLevelBarGraph, 0, 0.25f, &colorBlue, false );
	BarGraph_AddBar( &bargraphs->sceneFragmentLevelBarGraph, 0, 0.25f, &colorBlue, false );

	GpuTimer_Create( context, &bargraphs->barGraphTimer );
}

static void TimeWarpBarGraphs_Destroy( GpuContext_t * context, TimeWarpBarGraphs_t * bargraphs )
{
	BarGraph_Destroy( context, &bargraphs->eyeTexturesFrameRateGraph );
	BarGraph_Destroy( context, &bargraphs->timeWarpFrameRateGraph );
	BarGraph_Destroy( context, &bargraphs->frameCpuTimeBarGraph );
	BarGraph_Destroy( context, &bargraphs->frameGpuTimeBarGraph );

	BarGraph_Destroy( context, &bargraphs->multiViewBarGraph );
	BarGraph_Destroy( context, &bargraphs->correctChromaticAberrationBarGraph );
	BarGraph_Destroy( context, &bargraphs->timeWarpImplementationBarGraph );

	BarGraph_Destroy( context, &bargraphs->displayResolutionLevelBarGraph );
	BarGraph_Destroy( context, &bargraphs->eyeImageResolutionLevelBarGraph );
	BarGraph_Destroy( context, &bargraphs->eyeImageSamplesLevelBarGraph );

	BarGraph_Destroy( context, &bargraphs->sceneDrawCallLevelBarGraph );
	BarGraph_Destroy( context, &bargraphs->sceneTriangleLevelBarGraph );
	BarGraph_Destroy( context, &bargraphs->sceneFragmentLevelBarGraph );

	GpuTimer_Destroy( context, &bargraphs->barGraphTimer );
}

static void TimeWarpBarGraphs_UpdateGraphics( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->eyeTexturesFrameRateGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->timeWarpFrameRateGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->frameCpuTimeBarGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->frameGpuTimeBarGraph );

		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->multiViewBarGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->correctChromaticAberrationBarGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->timeWarpImplementationBarGraph );

		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->displayResolutionLevelBarGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->eyeImageResolutionLevelBarGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->eyeImageSamplesLevelBarGraph );

		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->sceneDrawCallLevelBarGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->sceneTriangleLevelBarGraph );
		BarGraph_UpdateGraphics( commandBuffer, &bargraphs->sceneFragmentLevelBarGraph );
	}
}

static void TimeWarpBarGraphs_RenderGraphics( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		GpuCommandBuffer_BeginTimer( commandBuffer, &bargraphs->barGraphTimer );

		BarGraph_RenderGraphics( commandBuffer, &bargraphs->eyeTexturesFrameRateGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->timeWarpFrameRateGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->frameCpuTimeBarGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->frameGpuTimeBarGraph );

		BarGraph_RenderGraphics( commandBuffer, &bargraphs->multiViewBarGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->correctChromaticAberrationBarGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->timeWarpImplementationBarGraph );

		BarGraph_RenderGraphics( commandBuffer, &bargraphs->displayResolutionLevelBarGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->eyeImageResolutionLevelBarGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->eyeImageSamplesLevelBarGraph );

		BarGraph_RenderGraphics( commandBuffer, &bargraphs->sceneDrawCallLevelBarGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->sceneTriangleLevelBarGraph );
		BarGraph_RenderGraphics( commandBuffer, &bargraphs->sceneFragmentLevelBarGraph );

		GpuCommandBuffer_EndTimer( commandBuffer, &bargraphs->barGraphTimer );
	}
}

static void TimeWarpBarGraphs_UpdateCompute( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->eyeTexturesFrameRateGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->timeWarpFrameRateGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->frameCpuTimeBarGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->frameGpuTimeBarGraph );

		BarGraph_UpdateCompute( commandBuffer, &bargraphs->multiViewBarGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->correctChromaticAberrationBarGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->timeWarpImplementationBarGraph );

		BarGraph_UpdateCompute( commandBuffer, &bargraphs->displayResolutionLevelBarGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->eyeImageResolutionLevelBarGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->eyeImageSamplesLevelBarGraph );

		BarGraph_UpdateCompute( commandBuffer, &bargraphs->sceneDrawCallLevelBarGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->sceneTriangleLevelBarGraph );
		BarGraph_UpdateCompute( commandBuffer, &bargraphs->sceneFragmentLevelBarGraph );
	}
}

static void TimeWarpBarGraphs_RenderCompute( GpuCommandBuffer_t * commandBuffer, TimeWarpBarGraphs_t * bargraphs, GpuFramebuffer_t * framebuffer )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		GpuCommandBuffer_BeginTimer( commandBuffer, &bargraphs->barGraphTimer );

		BarGraph_RenderCompute( commandBuffer, &bargraphs->eyeTexturesFrameRateGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->timeWarpFrameRateGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->frameCpuTimeBarGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->frameGpuTimeBarGraph, framebuffer );

		BarGraph_RenderCompute( commandBuffer, &bargraphs->multiViewBarGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->correctChromaticAberrationBarGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->timeWarpImplementationBarGraph, framebuffer );

		BarGraph_RenderCompute( commandBuffer, &bargraphs->displayResolutionLevelBarGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->eyeImageResolutionLevelBarGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->eyeImageSamplesLevelBarGraph, framebuffer );

		BarGraph_RenderCompute( commandBuffer, &bargraphs->sceneDrawCallLevelBarGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->sceneTriangleLevelBarGraph, framebuffer );
		BarGraph_RenderCompute( commandBuffer, &bargraphs->sceneFragmentLevelBarGraph, framebuffer );

		GpuCommandBuffer_EndTimer( commandBuffer, &bargraphs->barGraphTimer );
	}
}

static float TimeWarpBarGraphs_GetGpuMillisecondsGraphics( TimeWarpBarGraphs_t * bargraphs )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		return GpuTimer_GetMilliseconds( &bargraphs->barGraphTimer );
	}
	return 0.0f;
}

static float TimeWarpBarGraphs_GetGpuMillisecondsCompute( TimeWarpBarGraphs_t * bargraphs )
{
	if ( bargraphs->barGraphState != BAR_GRAPH_HIDDEN )
	{
		return GpuTimer_GetMilliseconds( &bargraphs->barGraphTimer );
	}
	return 0.0f;
}

/*
================================================================================================================================

HMD

HmdInfo_t

================================================================================================================================
*/

#define NUM_EYES				2
#define NUM_COLOR_CHANNELS		3

typedef struct
{
	int		displayPixelsWide;
	int		displayPixelsHigh;
	int		tilePixelsWide;
	int		tilePixelsHigh;
	int		eyeTilesWide;
	int		eyeTilesHigh;
	int		visiblePixelsWide;
	int		visiblePixelsHigh;
	float	visibleMetersWide;
	float	visibleMetersHigh;
	float	lensSeparationInMeters;
	float	metersPerTanAngleAtCenter;
	int		numKnots;
	float	K[11];
	float	chromaticAberration[4];
} HmdInfo_t;

typedef struct
{
	float	interpupillaryDistance;
} BodyInfo_t;

static const HmdInfo_t * GetDefaultHmdInfo( const int displayPixelsWide, const int displayPixelsHigh )
{
	static HmdInfo_t hmdInfo;
	hmdInfo.displayPixelsWide = displayPixelsWide;
	hmdInfo.displayPixelsHigh = displayPixelsHigh;
	hmdInfo.tilePixelsWide = 32;
	hmdInfo.tilePixelsHigh = 32;
	hmdInfo.eyeTilesWide = displayPixelsWide / hmdInfo.tilePixelsWide / NUM_EYES;
	hmdInfo.eyeTilesHigh = displayPixelsHigh / hmdInfo.tilePixelsHigh;
	hmdInfo.visiblePixelsWide = hmdInfo.eyeTilesWide * hmdInfo.tilePixelsWide * NUM_EYES;
	hmdInfo.visiblePixelsHigh = hmdInfo.eyeTilesHigh * hmdInfo.tilePixelsHigh;
	hmdInfo.visibleMetersWide = 0.11047f * ( hmdInfo.eyeTilesWide * hmdInfo.tilePixelsWide * NUM_EYES ) / displayPixelsWide;
	hmdInfo.visibleMetersHigh = 0.06214f * ( hmdInfo.eyeTilesHigh * hmdInfo.tilePixelsHigh ) / displayPixelsHigh;
	hmdInfo.lensSeparationInMeters = hmdInfo.visibleMetersWide / NUM_EYES;
	hmdInfo.metersPerTanAngleAtCenter = 0.037f;
	hmdInfo.numKnots = 11;
	hmdInfo.K[0] = 1.0f;
	hmdInfo.K[1] = 1.021f;
	hmdInfo.K[2] = 1.051f;
	hmdInfo.K[3] = 1.086f;
	hmdInfo.K[4] = 1.128f;
	hmdInfo.K[5] = 1.177f;
	hmdInfo.K[6] = 1.232f;
	hmdInfo.K[7] = 1.295f;
	hmdInfo.K[8] = 1.368f;
	hmdInfo.K[9] = 1.452f;
	hmdInfo.K[10] = 1.560f;
	hmdInfo.chromaticAberration[0] = -0.006f;
	hmdInfo.chromaticAberration[1] =  0.0f;
	hmdInfo.chromaticAberration[2] =  0.014f;
	hmdInfo.chromaticAberration[3] =  0.0f;
	return &hmdInfo;
}

static const BodyInfo_t * GetDefaultBodyInfo()
{
	static BodyInfo_t bodyInfo;
	bodyInfo.interpupillaryDistance	= 0.0640f;	// average interpupillary distance
	return &bodyInfo;
}

static bool hmd_headRotationDisabled = false;

static void GetHmdViewMatrixForTime( Matrix4x4f_t * viewMatrix, const Microseconds_t time )
{
	if ( hmd_headRotationDisabled )
	{
		Matrix4x4f_CreateIdentity( viewMatrix );
		return;
	}

	const float offset = time * ( MATH_PI / 1000.0f / 1000.0f );
	const float degrees = 10.0f;
	const float degreesX = sinf( offset ) * degrees;
	const float degreesY = cosf( offset ) * degrees;

	Matrix4x4f_CreateRotation( viewMatrix, degreesX, degreesY, 0.0f );
}

static void CalculateTimeWarpTransform( Matrix4x4f_t * transform, const Matrix4x4f_t * renderProjectionMatrix,
										const Matrix4x4f_t * renderViewMatrix, const Matrix4x4f_t * newViewMatrix )
{
	// Convert the projection matrix from [-1, 1] space to [0, 1] space.
	const Matrix4x4f_t texCoordProjection =
	{ {
		{ 0.5f * renderProjectionMatrix->m[0][0],        0.0f,                                           0.0f,  0.0f },
		{ 0.0f,                                          0.5f * renderProjectionMatrix->m[1][1],         0.0f,  0.0f },
		{ 0.5f * renderProjectionMatrix->m[2][0] - 0.5f, 0.5f * renderProjectionMatrix->m[2][1] - 0.5f, -1.0f,  0.0f },
		{ 0.0f,                                          0.0f,                                           0.0f,  1.0f }
	} };

	// Calculate the delta between the view matrix used for rendering and
	// a more recent or predicted view matrix based on new sensor input.
	Matrix4x4f_t inverseRenderViewMatrix;
	Matrix4x4f_InvertHomogeneous( &inverseRenderViewMatrix, renderViewMatrix );

	Matrix4x4f_t deltaViewMatrix;
	Matrix4x4f_Multiply( &deltaViewMatrix, &inverseRenderViewMatrix, newViewMatrix );

	Matrix4x4f_t inverseDeltaViewMatrix;
	Matrix4x4f_InvertHomogeneous( &inverseDeltaViewMatrix, &deltaViewMatrix );

	// Make the delta rotation only.
	inverseDeltaViewMatrix.m[3][0] = 0.0f;
	inverseDeltaViewMatrix.m[3][1] = 0.0f;
	inverseDeltaViewMatrix.m[3][2] = 0.0f;

	// Accumulate the transforms.
	Matrix4x4f_Multiply( transform, &texCoordProjection, &inverseDeltaViewMatrix );
}

/*
================================================================================================================================

Distortion meshes.

MeshCoord_t

================================================================================================================================
*/

typedef struct
{
	float x;
	float y;
} MeshCoord_t;

static float MaxFloat( float x, float y ) { return ( x > y ) ? x : y; }
static float MinFloat( float x, float y ) { return ( x < y ) ? x : y; }

// A Catmull-Rom spline through the values K[0], K[1], K[2] ... K[numKnots-1] evenly spaced from 0.0 to 1.0
static float EvaluateCatmullRomSpline( const float value, float const * K, const int numKnots )
{
	const float scaledValue = (float)( numKnots - 1 ) * value;
	const float scaledValueFloor = MaxFloat( 0.0f, MinFloat( (float)( numKnots - 1 ), floorf( scaledValue ) ) );
	const float t = scaledValue - scaledValueFloor;
	const int k = (int)scaledValueFloor;

	float p0 = 0.0f;
	float p1 = 0.0f;
	float m0 = 0.0f;
	float m1 = 0.0f;

	if ( k == 0 )
	{
		p0 = K[0];
		m0 = K[1] - K[0];
		p1 = K[1];
		m1 = 0.5f * ( K[2] - K[0] );
	}
	else if ( k < numKnots - 2 )
	{
		p0 = K[k];
		m0 = 0.5f * ( K[k+1] - K[k-1] );
		p1 = K[k+1];
		m1 = 0.5f * ( K[k+2] - K[k] );
	}
	else if ( k == numKnots - 2 )
	{
		p0 = K[k];
		m0 = 0.5f * ( K[k+1] - K[k-1] );
		p1 = K[k+1];
		m1 = K[k+1] - K[k];
	}
	else if ( k == numKnots - 1 )
	{
		p0 = K[k];
		m0 = K[k] - K[k-1];
		p1 = p0 + m0;
		m1 = m0;
	}

	const float omt = 1.0f - t;
	const float res = ( p0 * ( 1.0f + 2.0f *   t ) + m0 *   t ) * omt * omt
					+ ( p1 * ( 1.0f + 2.0f * omt ) - m1 * omt ) *   t *   t;
	return res;
}

static void BuildDistortionMeshes( MeshCoord_t * meshCoords[NUM_EYES][NUM_COLOR_CHANNELS], const HmdInfo_t * hmdInfo )
{
	const float horizontalShiftMeters = ( hmdInfo->lensSeparationInMeters / 2 ) - ( hmdInfo->visibleMetersWide / 4 );
	const float horizontalShiftView = horizontalShiftMeters / ( hmdInfo->visibleMetersWide / 2 );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		for ( int y = 0; y <= hmdInfo->eyeTilesHigh; y++ )
		{
			const float yf = 1.0f - (float)y / (float)hmdInfo->eyeTilesHigh;

			for ( int x = 0; x <= hmdInfo->eyeTilesWide; x++ )
			{
				const float xf = (float)x / (float)hmdInfo->eyeTilesWide;

				const float in[2] = { ( eye ? -horizontalShiftView : horizontalShiftView ) + xf, yf };
				const float ndcToPixels[2] = { hmdInfo->visiblePixelsWide * 0.25f, hmdInfo->visiblePixelsHigh * 0.5f };
				const float pixelsToMeters[2] = { hmdInfo->visibleMetersWide / hmdInfo->visiblePixelsWide, hmdInfo->visibleMetersHigh / hmdInfo->visiblePixelsHigh };

				float theta[2];
				for ( int i = 0; i < 2; i++ )
				{
					const float unit = in[i];
					const float ndc = 2.0f * unit - 1.0f;
					const float pixels = ndc * ndcToPixels[i];
					const float meters = pixels * pixelsToMeters[i];
					const float tanAngle = meters / hmdInfo->metersPerTanAngleAtCenter;
					theta[i] = tanAngle;
				}

				const float rsq = theta[0] * theta[0] + theta[1] * theta[1];
				const float scale = EvaluateCatmullRomSpline( rsq, hmdInfo->K, hmdInfo->numKnots );
				const float chromaScale[NUM_COLOR_CHANNELS] =
				{
					scale * ( 1.0f + hmdInfo->chromaticAberration[0] + rsq * hmdInfo->chromaticAberration[1] ),
					scale,
					scale * ( 1.0f + hmdInfo->chromaticAberration[2] + rsq * hmdInfo->chromaticAberration[3] )
				};

				const int vertNum = y * ( hmdInfo->eyeTilesWide + 1 ) + x;
				for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
				{
					meshCoords[eye][channel][vertNum].x = chromaScale[channel] * theta[0];
					meshCoords[eye][channel][vertNum].y = chromaScale[channel] * theta[1];
				}
			}
		}
	}
}

/*
================================================================================================================================

Time warp graphics rendering.

TimeWarpGraphics_t

static void TimeWarpGraphics_Create( GpuContext_t * context, TimeWarpGraphics_t * graphics,
									const HmdInfo_t * hmdInfo, GpuRenderPass_t * renderPass );
static void TimeWarpGraphics_Destroy( GpuContext_t * context, TimeWarpGraphics_t * graphics );
static void TimeWarpGraphics_Render( GpuCommandBuffer_t * commandBuffer, TimeWarpGraphics_t * graphics,
									GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass,
									const Microseconds_t refreshStartTime, const Microseconds_t refreshEndTime,
									const Matrix4x4f_t * projectionMatrix, const Matrix4x4f_t * viewMatrix,
									GpuTexture_t * const eyeTexture[NUM_EYES], const int eyeArrayLayer[NUM_EYES],
									const bool correctChromaticAberration, TimeWarpBarGraphs_t * bargraphs,
									float cpuTimes[PROFILE_TIME_MAX], float gpuTimes[PROFILE_TIME_MAX] );

================================================================================================================================
*/

typedef struct
{
	HmdInfo_t				hmdInfo;
	GpuGeometry_t			distortionMesh[NUM_EYES];
	GpuGraphicsProgram_t	timeWarpSpatialProgram;
	GpuGraphicsProgram_t	timeWarpChromaticProgram;
	GpuGraphicsPipeline_t	timeWarpSpatialPipeline[NUM_EYES];
	GpuGraphicsPipeline_t	timeWarpChromaticPipeline[NUM_EYES];
	GpuTimer_t				timeWarpGpuTime;
} TimeWarpGraphics_t;

enum
{
	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,
	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM,
	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_ARRAY_LAYER,
	GRAPHICS_PROGRAM_TEXTURE_TIMEWARP_SOURCE
};

static const GpuProgramParm_t timeWarpSpatialGraphicsProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_VERTEX,		GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,	"TimeWarpStartTransform",	0 },
	{ GPU_PROGRAM_STAGE_VERTEX,		GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM,	"TimeWarpEndTransform",		1 },
	{ GPU_PROGRAM_STAGE_FRAGMENT,	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_ARRAY_LAYER,		"ArrayLayer",				2 },
	{ GPU_PROGRAM_STAGE_FRAGMENT,	GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_TEXTURE_TIMEWARP_SOURCE,			"Texture",					0 }
};

static const char timeWarpSpatialVertexProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform highp mat3x4 TimeWarpStartTransform;\n"
	"uniform highp mat3x4 TimeWarpEndTransform;\n"
	"in highp vec3 vertexPosition;\n"
	"in highp vec2 vertexUv1;\n"
	"out mediump vec2 fragmentUv1;\n"
	"out gl_PerVertex { vec4 gl_Position; };\n"
	"void main( void )\n"
	"{\n"
	"	gl_Position = vec4( vertexPosition, 1.0 );\n"
	"\n"
	"	float displayFraction = vertexPosition.x * 0.5 + 0.5;\n"	// landscape left-to-right
	"\n"
	"	vec3 startUv1 = vec4( vertexUv1, -1, 1 ) * TimeWarpStartTransform;\n"
	"	vec3 endUv1 = vec4( vertexUv1, -1, 1 ) * TimeWarpEndTransform;\n"
	"	vec3 curUv1 = mix( startUv1, endUv1, displayFraction );\n"
	"	fragmentUv1 = curUv1.xy * ( 1.0 / max( curUv1.z, 0.00001 ) );\n"
	"}\n";

static const char timeWarpSpatialFragmentProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform int ArrayLayer;\n"
	"uniform highp sampler2DArray Texture;\n"
	"in mediump vec2 fragmentUv1;\n"
	"out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	outColor = texture( Texture, vec3( fragmentUv1, ArrayLayer ) );\n"
	"}\n";

static const GpuProgramParm_t timeWarpChromaticGraphicsProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_VERTEX,		GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,	"TimeWarpStartTransform",	0 },
	{ GPU_PROGRAM_STAGE_VERTEX,		GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM,	"TimeWarpEndTransform",		1 },
	{ GPU_PROGRAM_STAGE_FRAGMENT,	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_ARRAY_LAYER,		"ArrayLayer",				2 },
	{ GPU_PROGRAM_STAGE_FRAGMENT,	GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	GRAPHICS_PROGRAM_TEXTURE_TIMEWARP_SOURCE,			"Texture",					0 }
};

static const char timeWarpChromaticVertexProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform highp mat3x4 TimeWarpStartTransform;\n"
	"uniform highp mat3x4 TimeWarpEndTransform;\n"
	"in highp vec3 vertexPosition;\n"
	"in highp vec2 vertexUv0;\n"
	"in highp vec2 vertexUv1;\n"
	"in highp vec2 vertexUv2;\n"
	"out mediump vec2 fragmentUv0;\n"
	"out mediump vec2 fragmentUv1;\n"
	"out mediump vec2 fragmentUv2;\n"
	"out gl_PerVertex { vec4 gl_Position; };\n"
	"void main( void )\n"
	"{\n"
	"	gl_Position = vec4( vertexPosition, 1.0 );\n"
	"\n"
	"	float displayFraction = vertexPosition.x * 0.5 + 0.5;\n"	// landscape left-to-right
	"\n"
	"	vec3 startUv0 = vec4( vertexUv0, -1, 1 ) * TimeWarpStartTransform;\n"
	"	vec3 startUv1 = vec4( vertexUv1, -1, 1 ) * TimeWarpStartTransform;\n"
	"	vec3 startUv2 = vec4( vertexUv2, -1, 1 ) * TimeWarpStartTransform;\n"
	"\n"
	"	vec3 endUv0 = vec4( vertexUv0, -1, 1 ) * TimeWarpEndTransform;\n"
	"	vec3 endUv1 = vec4( vertexUv1, -1, 1 ) * TimeWarpEndTransform;\n"
	"	vec3 endUv2 = vec4( vertexUv2, -1, 1 ) * TimeWarpEndTransform;\n"
	"\n"
	"	vec3 curUv0 = mix( startUv0, endUv0, displayFraction );\n"
	"	vec3 curUv1 = mix( startUv1, endUv1, displayFraction );\n"
	"	vec3 curUv2 = mix( startUv2, endUv2, displayFraction );\n"
	"\n"
	"	fragmentUv0 = curUv0.xy * ( 1.0 / max( curUv0.z, 0.00001 ) );\n"
	"	fragmentUv1 = curUv1.xy * ( 1.0 / max( curUv1.z, 0.00001 ) );\n"
	"	fragmentUv2 = curUv2.xy * ( 1.0 / max( curUv2.z, 0.00001 ) );\n"
	"}\n";

static const char timeWarpChromaticFragmentProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform int ArrayLayer;\n"
	"uniform highp sampler2DArray Texture;\n"
	"in mediump vec2 fragmentUv0;\n"
	"in mediump vec2 fragmentUv1;\n"
	"in mediump vec2 fragmentUv2;\n"
	"out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	outColor.r = texture( Texture, vec3( fragmentUv0, ArrayLayer ) ).r;\n"
	"	outColor.g = texture( Texture, vec3( fragmentUv1, ArrayLayer ) ).g;\n"
	"	outColor.b = texture( Texture, vec3( fragmentUv2, ArrayLayer ) ).b;\n"
	"	outColor.a = 1.0;\n"
	"}\n";

static void TimeWarpGraphics_Create( GpuContext_t * context, TimeWarpGraphics_t * graphics,
									const HmdInfo_t * hmdInfo, GpuRenderPass_t * renderPass )
{
	memset( graphics, 0, sizeof( TimeWarpGraphics_t ) );

	graphics->hmdInfo = *hmdInfo;

	const int vertexCount = ( hmdInfo->eyeTilesHigh + 1 ) * ( hmdInfo->eyeTilesWide + 1 );
	const int indexCount = hmdInfo->eyeTilesHigh * hmdInfo->eyeTilesWide * 6;

	GpuTriangleIndex_t * indices = (GpuTriangleIndex_t *) malloc( indexCount * sizeof( indices[0] ) );
	for ( int y = 0; y < hmdInfo->eyeTilesHigh; y++ )
	{
		for ( int x = 0; x < hmdInfo->eyeTilesWide; x++ )
		{
			const int offset = ( y * hmdInfo->eyeTilesWide + x ) * 6;

			indices[offset + 0] = (GpuTriangleIndex_t)( ( y + 0 ) * ( hmdInfo->eyeTilesWide + 1 ) + ( x + 0 ) );
			indices[offset + 1] = (GpuTriangleIndex_t)( ( y + 1 ) * ( hmdInfo->eyeTilesWide + 1 ) + ( x + 0 ) );
			indices[offset + 2] = (GpuTriangleIndex_t)( ( y + 0 ) * ( hmdInfo->eyeTilesWide + 1 ) + ( x + 1 ) );

			indices[offset + 3] = (GpuTriangleIndex_t)( ( y + 0 ) * ( hmdInfo->eyeTilesWide + 1 ) + ( x + 1 ) );
			indices[offset + 4] = (GpuTriangleIndex_t)( ( y + 1 ) * ( hmdInfo->eyeTilesWide + 1 ) + ( x + 0 ) );
			indices[offset + 5] = (GpuTriangleIndex_t)( ( y + 1 ) * ( hmdInfo->eyeTilesWide + 1 ) + ( x + 1 ) );
		}
	}

	GpuVertexAttributeArrays_t vertexAttribs;
	GpuVertexAttributeArrays_Alloc( &vertexAttribs.base,
									DefaultVertexAttributeLayout, vertexCount,
									VERTEX_ATTRIBUTE_FLAG_POSITION |
									VERTEX_ATTRIBUTE_FLAG_UV0 |
									VERTEX_ATTRIBUTE_FLAG_UV1 |
									VERTEX_ATTRIBUTE_FLAG_UV2 );

	const int numMeshCoords = ( hmdInfo->eyeTilesWide + 1 ) * ( hmdInfo->eyeTilesHigh + 1 );
	MeshCoord_t * meshCoordsBasePtr = (MeshCoord_t *) malloc( NUM_EYES * NUM_COLOR_CHANNELS * numMeshCoords * sizeof( MeshCoord_t ) );
	MeshCoord_t * meshCoords[NUM_EYES][NUM_COLOR_CHANNELS] =
	{
		{ meshCoordsBasePtr + 0 * numMeshCoords, meshCoordsBasePtr + 1 * numMeshCoords, meshCoordsBasePtr + 2 * numMeshCoords },
		{ meshCoordsBasePtr + 3 * numMeshCoords, meshCoordsBasePtr + 4 * numMeshCoords, meshCoordsBasePtr + 5 * numMeshCoords }
	};
	BuildDistortionMeshes( meshCoords, hmdInfo );

#if defined( GRAPHICS_API_VULKAN )
	const float flipY = -1.0f;
#else
	const float flipY = 1.0f;
#endif

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		for ( int y = 0; y <= hmdInfo->eyeTilesHigh; y++ )
		{
			for ( int x = 0; x <= hmdInfo->eyeTilesWide; x++ )
			{
				const int index = y * ( hmdInfo->eyeTilesWide + 1 ) + x;
				vertexAttribs.position[index].x = ( -1.0f + eye + ( (float)x / hmdInfo->eyeTilesWide ) );
				vertexAttribs.position[index].y = ( -1.0f + 2.0f * ( ( hmdInfo->eyeTilesHigh - (float)y ) / hmdInfo->eyeTilesHigh ) *
													( (float)( hmdInfo->eyeTilesHigh * hmdInfo->tilePixelsHigh ) / hmdInfo->displayPixelsHigh ) ) * flipY;
				vertexAttribs.position[index].z = 0.0f;
				vertexAttribs.uv0[index].x = meshCoords[eye][0][index].x;
				vertexAttribs.uv0[index].y = meshCoords[eye][0][index].y;
				vertexAttribs.uv1[index].x = meshCoords[eye][1][index].x;
				vertexAttribs.uv1[index].y = meshCoords[eye][1][index].y;
				vertexAttribs.uv2[index].x = meshCoords[eye][2][index].x;
				vertexAttribs.uv2[index].y = meshCoords[eye][2][index].y;
			}
		}

		GpuGeometry_Create( context, &graphics->distortionMesh[eye], &vertexAttribs.base, vertexCount, indices, indexCount );
	}

	free( meshCoordsBasePtr );
	GpuVertexAttributeArrays_Free( &vertexAttribs.base );
	free( indices );

	GpuGraphicsProgram_Create( context, &graphics->timeWarpSpatialProgram,
								PROGRAM( timeWarpSpatialVertexProgram ), sizeof( PROGRAM( timeWarpSpatialVertexProgram ) ),
								PROGRAM( timeWarpSpatialFragmentProgram ), sizeof( PROGRAM( timeWarpSpatialFragmentProgram ) ),
								timeWarpSpatialGraphicsProgramParms, ARRAY_SIZE( timeWarpSpatialGraphicsProgramParms ),
								graphics->distortionMesh[0].layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_UV1 );
	GpuGraphicsProgram_Create( context, &graphics->timeWarpChromaticProgram,
								PROGRAM( timeWarpChromaticVertexProgram ), sizeof( PROGRAM( timeWarpChromaticVertexProgram ) ),
								PROGRAM( timeWarpChromaticFragmentProgram ), sizeof( PROGRAM( timeWarpChromaticFragmentProgram ) ),
								timeWarpChromaticGraphicsProgramParms, ARRAY_SIZE( timeWarpChromaticGraphicsProgramParms ),
								graphics->distortionMesh[0].layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_UV0 |
								VERTEX_ATTRIBUTE_FLAG_UV1 | VERTEX_ATTRIBUTE_FLAG_UV2 );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		GpuGraphicsPipelineParms_t pipelineParms;
		GpuGraphicsPipelineParms_Init( &pipelineParms );

		pipelineParms.rop.depthTestEnable = false;
		pipelineParms.rop.depthWriteEnable = false;
		pipelineParms.renderPass = renderPass;
		pipelineParms.program = &graphics->timeWarpSpatialProgram;
		pipelineParms.geometry = &graphics->distortionMesh[eye];

		GpuGraphicsPipeline_Create( context, &graphics->timeWarpSpatialPipeline[eye], &pipelineParms );

		pipelineParms.program = &graphics->timeWarpChromaticProgram;
		pipelineParms.geometry = &graphics->distortionMesh[eye];

		GpuGraphicsPipeline_Create( context, &graphics->timeWarpChromaticPipeline[eye], &pipelineParms );
	}

	GpuTimer_Create( context, &graphics->timeWarpGpuTime );
}

static void TimeWarpGraphics_Destroy( GpuContext_t * context, TimeWarpGraphics_t * graphics )
{
	GpuTimer_Destroy( context, &graphics->timeWarpGpuTime );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		GpuGraphicsPipeline_Destroy( context, &graphics->timeWarpSpatialPipeline[eye] );
		GpuGraphicsPipeline_Destroy( context, &graphics->timeWarpChromaticPipeline[eye] );
	}

	GpuGraphicsProgram_Destroy( context, &graphics->timeWarpSpatialProgram );
	GpuGraphicsProgram_Destroy( context, &graphics->timeWarpChromaticProgram );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		GpuGeometry_Destroy( context, &graphics->distortionMesh[eye] );
	}
}

static void TimeWarpGraphics_Render( GpuCommandBuffer_t * commandBuffer, TimeWarpGraphics_t * graphics,
									GpuFramebuffer_t * framebuffer, GpuRenderPass_t * renderPass,
									const Microseconds_t refreshStartTime, const Microseconds_t refreshEndTime,
									const Matrix4x4f_t * projectionMatrix, const Matrix4x4f_t * viewMatrix,
									GpuTexture_t * const eyeTexture[NUM_EYES], const int eyeArrayLayer[NUM_EYES],
									const bool correctChromaticAberration, TimeWarpBarGraphs_t * bargraphs,
									float cpuTimes[PROFILE_TIME_MAX], float gpuTimes[PROFILE_TIME_MAX] )
{
	const Microseconds_t t0 = GetTimeMicroseconds();

	Matrix4x4f_t displayRefreshStartViewMatrix;
	Matrix4x4f_t displayRefreshEndViewMatrix;
	GetHmdViewMatrixForTime( &displayRefreshStartViewMatrix, refreshStartTime );
	GetHmdViewMatrixForTime( &displayRefreshEndViewMatrix, refreshEndTime );

	Matrix4x4f_t timeWarpStartTransform;
	Matrix4x4f_t timeWarpEndTransform;
	CalculateTimeWarpTransform( &timeWarpStartTransform, projectionMatrix, viewMatrix, &displayRefreshStartViewMatrix );
	CalculateTimeWarpTransform( &timeWarpEndTransform, projectionMatrix, viewMatrix, &displayRefreshEndViewMatrix );

	Matrix3x4f_t timeWarpStartTransform3x4;
	Matrix3x4f_t timeWarpEndTransform3x4;
	Matrix3x4f_CreateFromMatrix4x4f( &timeWarpStartTransform3x4, &timeWarpStartTransform );
	Matrix3x4f_CreateFromMatrix4x4f( &timeWarpEndTransform3x4, &timeWarpEndTransform );

	const ScreenRect_t screenRect = GpuFramebuffer_GetRect( framebuffer );

	GpuCommandBuffer_BeginPrimary( commandBuffer );
	GpuCommandBuffer_BeginFramebuffer( commandBuffer, framebuffer, 0, GPU_TEXTURE_USAGE_COLOR_ATTACHMENT );

	TimeWarpBarGraphs_UpdateGraphics( commandBuffer, bargraphs );

	GpuCommandBuffer_BeginTimer( commandBuffer, &graphics->timeWarpGpuTime );
	GpuCommandBuffer_BeginRenderPass( commandBuffer, renderPass, framebuffer, &screenRect );

	GpuCommandBuffer_SetViewport( commandBuffer, &screenRect );
	GpuCommandBuffer_SetScissor( commandBuffer, &screenRect );

	for ( int eye = 0; eye < NUM_EYES; eye ++ )
	{
		GpuGraphicsCommand_t command;
		GpuGraphicsCommand_Init( &command );
		GpuGraphicsCommand_SetPipeline( &command, correctChromaticAberration ? &graphics->timeWarpChromaticPipeline[eye] : &graphics->timeWarpSpatialPipeline[eye] );
		GpuGraphicsCommand_SetParmFloatMatrix3x4( &command, GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM, &timeWarpStartTransform3x4 );
		GpuGraphicsCommand_SetParmFloatMatrix3x4( &command, GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM, &timeWarpEndTransform3x4 );
		GpuGraphicsCommand_SetParmInt( &command, GRAPHICS_PROGRAM_UNIFORM_TIMEWARP_ARRAY_LAYER, &eyeArrayLayer[eye] );
		GpuGraphicsCommand_SetParmTextureSampled( &command, GRAPHICS_PROGRAM_TEXTURE_TIMEWARP_SOURCE, eyeTexture[eye] );

		GpuCommandBuffer_SubmitGraphicsCommand( commandBuffer, &command );
	}

	const Microseconds_t t1 = GetTimeMicroseconds();

	TimeWarpBarGraphs_RenderGraphics( commandBuffer, bargraphs );

	GpuCommandBuffer_EndRenderPass( commandBuffer, renderPass );
	GpuCommandBuffer_EndTimer( commandBuffer, &graphics->timeWarpGpuTime );

	GpuCommandBuffer_EndFramebuffer( commandBuffer, framebuffer, 0, GPU_TEXTURE_USAGE_PRESENTATION );
	GpuCommandBuffer_EndPrimary( commandBuffer );

	GpuCommandBuffer_SubmitPrimary( commandBuffer );

	const Microseconds_t t2 = GetTimeMicroseconds();

	cpuTimes[PROFILE_TIME_TIME_WARP] = ( t1 - t0 ) * ( 1.0f / 1000.0f );
	cpuTimes[PROFILE_TIME_BAR_GRAPHS] = ( t2 - t1 ) * ( 1.0f / 1000.0f );
	cpuTimes[PROFILE_TIME_BLIT] = 0.0f;

	const float barGraphGpuTime = TimeWarpBarGraphs_GetGpuMillisecondsGraphics( bargraphs );

	gpuTimes[PROFILE_TIME_TIME_WARP] = GpuTimer_GetMilliseconds( &graphics->timeWarpGpuTime ) - barGraphGpuTime;
	gpuTimes[PROFILE_TIME_BAR_GRAPHS] = barGraphGpuTime;
	gpuTimes[PROFILE_TIME_BLIT] = 0.0f;

#if GL_FINISH_SYNC == 1
	GL( glFinish() );
#endif
}

/*
================================================================================================================================

Time warp compute rendering.

TimeWarpCompute_t

static void TimeWarpCompute_Create( GpuContext_t * context, TimeWarpCompute_t * compute, const HmdInfo_t * hmdInfo,
									GpuRenderPass_t * renderPass, GpuWindow_t * window );
static void TimeWarpCompute_Destroy( GpuContext_t * context, TimeWarpCompute_t * compute );
static void TimeWarpCompute_Render( GpuCommandBuffer_t * commandBuffer, TimeWarpCompute_t * compute,
									GpuFramebuffer_t * framebuffer,
									const Microseconds_t refreshStartTime, const Microseconds_t refreshEndTime,
									const Matrix4x4f_t * projectionMatrix, const Matrix4x4f_t * viewMatrix,
									GpuTexture_t * const eyeTexture[NUM_EYES], const int eyeArrayLayer[NUM_EYES],
									const bool correctChromaticAberration, TimeWarpBarGraphs_t * bargraphs,
									float cpuTimes[PROFILE_TIME_MAX], float gpuTimes[PROFILE_TIME_MAX] );

================================================================================================================================
*/

#if OPENGL_COMPUTE_ENABLED == 1

typedef struct
{
	HmdInfo_t				hmdInfo;
	GpuTexture_t			distortionImage[NUM_EYES][NUM_COLOR_CHANNELS];
	GpuTexture_t			timeWarpImage[NUM_EYES][NUM_COLOR_CHANNELS];
	GpuComputeProgram_t		timeWarpTransformProgram;
	GpuComputeProgram_t		timeWarpSpatialProgram;
	GpuComputeProgram_t		timeWarpChromaticProgram;
	GpuComputePipeline_t	timeWarpTransformPipeline;
	GpuComputePipeline_t	timeWarpSpatialPipeline;
	GpuComputePipeline_t	timeWarpChromaticPipeline;
	GpuTimer_t				timeWarpGpuTime;
	GpuFramebuffer_t		framebuffer;
} TimeWarpCompute_t;

enum
{
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_DST,
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_SRC,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_DIMENSIONS,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM
};

static const GpuProgramParm_t timeWarpTransformComputeProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,					GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_DST,		"dst",						0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_SRC,		"src",						1 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,		GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_DIMENSIONS,		"dimensions",				0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE,				"eye",						1 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM,	"timeWarpStartTransform",	2 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM,		"timeWarpEndTransform",		3 }
};

#define TRANSFORM_LOCAL_SIZE_X		8
#define TRANSFORM_LOCAL_SIZE_Y		8

static const char timeWarpTransformComputeProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"\n"
	"layout( local_size_x = " STRINGIFY( TRANSFORM_LOCAL_SIZE_X ) ", local_size_y = " STRINGIFY( TRANSFORM_LOCAL_SIZE_Y ) " ) in;\n"
	"\n"
	"layout( rgba16f, binding = 0 ) uniform writeonly " ES_HIGHP " image2D dst;\n"
	"layout( rgba32f, binding = 1 ) uniform readonly " ES_HIGHP " image2D src;\n"
	"uniform highp mat3x4 timeWarpStartTransform;\n"
 	"uniform highp mat3x4 timeWarpEndTransform;\n"
	"uniform ivec2 dimensions;\n"
	"uniform int eye;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	ivec2 mesh = ivec2( gl_GlobalInvocationID.xy );\n"
	"	if ( mesh.x >= dimensions.x || mesh.y >= dimensions.y )\n"
	"	{\n"
	"		return;\n"
	"	}\n"
	"	int eyeTilesWide = int( gl_NumWorkGroups.x * gl_WorkGroupSize.x ) - 1;\n"
	"	int eyeTilesHigh = int( gl_NumWorkGroups.y * gl_WorkGroupSize.y ) - 1;\n"
	"\n"
	"	vec2 coords = imageLoad( src, mesh ).xy;\n"
	"\n"
	"	float displayFraction = float( eye * eyeTilesWide + mesh.x ) / ( float( eyeTilesWide ) * 2.0f );\n"		// landscape left-to-right
	"	vec3 start = vec4( coords, -1.0f, 1.0f ) * timeWarpStartTransform;\n"
	"	vec3 end = vec4( coords, -1.0f, 1.0f ) * timeWarpEndTransform;\n"
	"	vec3 cur = start + displayFraction * ( end - start );\n"
	"	float rcpZ = 1.0f / cur.z;\n"
	"\n"
	"	imageStore( dst, mesh, vec4( cur.xy * rcpZ, 0.0f, 0.0f ) );\n"
	"}\n";

enum
{
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_DEST,
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_EYE_IMAGE,
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_R,
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_G,
	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_B,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_SCALE,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_BIAS,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_LAYER,
	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE_PIXEL_OFFSET
};

static const GpuProgramParm_t timeWarpSpatialComputeProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,				GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_DEST,				"dest",				0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_EYE_IMAGE,			"eyeImage",			0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_G,		"warpImageG",		1 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_SCALE,		"imageScale",		0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_BIAS,		"imageBias",		1 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE_PIXEL_OFFSET,	"eyePixelOffset",	3 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_LAYER,		"imageLayer",		2 }
};

#define SPATIAL_LOCAL_SIZE_X		8
#define SPATIAL_LOCAL_SIZE_Y		8

static const char timeWarpSpatialComputeProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"\n"
	"layout( local_size_x = " STRINGIFY( SPATIAL_LOCAL_SIZE_X ) ", local_size_y = " STRINGIFY( SPATIAL_LOCAL_SIZE_Y ) " ) in;\n"
	"\n"
	"// imageScale = {	eyeTilesWide / ( eyeTilesWide + 1 ) / eyePixelsWide,\n"
	"//					eyeTilesHigh / ( eyeTilesHigh + 1 ) / eyePixelsHigh };\n"
	"// imageBias  = {	0.5f / ( eyeTilesWide + 1 ),\n"
	"//					0.5f / ( eyeTilesHigh + 1 ) };\n"
	"layout( rgba8, binding = 0 ) uniform writeonly " ES_HIGHP " image2D dest;\n"
	"uniform highp sampler2DArray eyeImage;\n"
	"uniform highp sampler2D warpImageG;\n"
	"uniform highp vec2 imageScale;\n"
	"uniform highp vec2 imageBias;\n"
	"uniform ivec2 eyePixelOffset;\n"
	"uniform int imageLayer;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	vec2 tile = ( vec2( gl_GlobalInvocationID.xy ) + vec2( 0.5f ) ) * imageScale + imageBias;\n"
	"\n"
	"	vec2 eyeCoords = texture( warpImageG, tile ).xy;\n"
	"\n"
	"	vec4 rgba = texture( eyeImage, vec3( eyeCoords, imageLayer ) );\n"
	"\n"
	"	imageStore( dest, ivec2( int( gl_GlobalInvocationID.x ) + eyePixelOffset.x, eyePixelOffset.y - 1 - int( gl_GlobalInvocationID.y ) ), rgba );\n"
	"}\n";

static const GpuProgramParm_t timeWarpChromaticComputeProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_STORAGE,				GPU_PROGRAM_PARM_ACCESS_WRITE_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_DEST,				"dest",				0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_EYE_IMAGE,			"eyeImage",			0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_R,		"warpImageR",		1 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_G,		"warpImageG",		2 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,				GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_B,		"warpImageB",		3 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_SCALE,		"imageScale",		0 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_BIAS,		"imageBias",		1 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE_PIXEL_OFFSET,	"eyePixelOffset",	3 },
	{ GPU_PROGRAM_STAGE_COMPUTE, GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT,			GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_LAYER,		"imageLayer",		2 }
};

#define CHROMATIC_LOCAL_SIZE_X		8
#define CHROMATIC_LOCAL_SIZE_Y		8

static const char timeWarpChromaticComputeProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"\n"
	"layout( local_size_x = " STRINGIFY( CHROMATIC_LOCAL_SIZE_X ) ", local_size_y = " STRINGIFY( CHROMATIC_LOCAL_SIZE_Y ) " ) in;\n"
	"\n"
	"// imageScale = {	eyeTilesWide / ( eyeTilesWide + 1 ) / eyePixelsWide,\n"
	"//					eyeTilesHigh / ( eyeTilesHigh + 1 ) / eyePixelsHigh };\n"
	"// imageBias  = {	0.5f / ( eyeTilesWide + 1 ),\n"
	"//					0.5f / ( eyeTilesHigh + 1 ) };\n"
	"layout( rgba8, binding = 0 ) uniform writeonly " ES_HIGHP " image2D dest;\n"
	"uniform highp sampler2DArray eyeImage;\n"
	"uniform highp sampler2D warpImageR;\n"
	"uniform highp sampler2D warpImageG;\n"
	"uniform highp sampler2D warpImageB;\n"
	"uniform highp vec2 imageScale;\n"
	"uniform highp vec2 imageBias;\n"
	"uniform ivec2 eyePixelOffset;\n"
	"uniform int imageLayer;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	vec2 tile = ( vec2( gl_GlobalInvocationID.xy ) + vec2( 0.5f ) ) * imageScale + imageBias;\n"
	"\n"
	"	vec2 eyeCoordsR = texture( warpImageR, tile ).xy;\n"
	"	vec2 eyeCoordsG = texture( warpImageG, tile ).xy;\n"
	"	vec2 eyeCoordsB = texture( warpImageB, tile ).xy;\n"
	"\n"
	"	vec4 rgba;\n"
	"	rgba.x = texture( eyeImage, vec3( eyeCoordsR, imageLayer ) ).x;\n"
	"	rgba.y = texture( eyeImage, vec3( eyeCoordsG, imageLayer ) ).y;\n"
	"	rgba.z = texture( eyeImage, vec3( eyeCoordsB, imageLayer ) ).z;\n"
	"	rgba.w = 1.0f;\n"
	"\n"
	"	imageStore( dest, ivec2( int( gl_GlobalInvocationID.x ) + eyePixelOffset.x, eyePixelOffset.y - 1 - int( gl_GlobalInvocationID.y ) ), rgba );\n"
	"}\n";

static void TimeWarpCompute_Create( GpuContext_t * context, TimeWarpCompute_t * compute,
									const HmdInfo_t * hmdInfo, GpuRenderPass_t * renderPass, GpuWindow_t * window )
{
	memset( compute, 0, sizeof( TimeWarpCompute_t ) );

	compute->hmdInfo = *hmdInfo;

	const int numMeshCoords = ( hmdInfo->eyeTilesHigh + 1 ) * ( hmdInfo->eyeTilesWide + 1 );
	MeshCoord_t * meshCoordsBasePtr = (MeshCoord_t *) malloc( NUM_EYES * NUM_COLOR_CHANNELS * numMeshCoords * sizeof( MeshCoord_t ) );
	MeshCoord_t * meshCoords[NUM_EYES][NUM_COLOR_CHANNELS] =
	{
		{ meshCoordsBasePtr + 0 * numMeshCoords, meshCoordsBasePtr + 1 * numMeshCoords, meshCoordsBasePtr + 2 * numMeshCoords },
		{ meshCoordsBasePtr + 3 * numMeshCoords, meshCoordsBasePtr + 4 * numMeshCoords, meshCoordsBasePtr + 5 * numMeshCoords }
	};
	BuildDistortionMeshes( meshCoords, hmdInfo );

	float * rgbaFloat = (float *) malloc( numMeshCoords * 4 * sizeof( float ) );
	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
		{
			for ( int i = 0; i < numMeshCoords; i++ )
			{
				rgbaFloat[i * 4 + 0] = meshCoords[eye][channel][i].x;
				rgbaFloat[i * 4 + 1] = meshCoords[eye][channel][i].y;
				rgbaFloat[i * 4 + 2] = 0.0f;
				rgbaFloat[i * 4 + 3] = 0.0f;
			}
			const size_t rgbaSize = numMeshCoords * 4 * sizeof( float );
			GpuTexture_Create2D( context, &compute->distortionImage[eye][channel],
								GPU_TEXTURE_FORMAT_R32G32B32A32_SFLOAT, GPU_SAMPLE_COUNT_1,
								hmdInfo->eyeTilesWide + 1, hmdInfo->eyeTilesHigh + 1, 1, GPU_TEXTURE_USAGE_STORAGE, rgbaFloat, rgbaSize );
			GpuTexture_Create2D( context, &compute->timeWarpImage[eye][channel],
								GPU_TEXTURE_FORMAT_R16G16B16A16_SFLOAT, GPU_SAMPLE_COUNT_1,
								hmdInfo->eyeTilesWide + 1, hmdInfo->eyeTilesHigh + 1, 1, GPU_TEXTURE_USAGE_STORAGE | GPU_TEXTURE_USAGE_SAMPLED, NULL, 0 );
		}
	}
	free( rgbaFloat );

	free( meshCoordsBasePtr );

	GpuComputeProgram_Create( context, &compute->timeWarpTransformProgram,
								PROGRAM( timeWarpTransformComputeProgram ), sizeof( PROGRAM( timeWarpTransformComputeProgram ) ),
								timeWarpTransformComputeProgramParms, ARRAY_SIZE( timeWarpTransformComputeProgramParms ) );
	GpuComputeProgram_Create( context, &compute->timeWarpSpatialProgram,
								PROGRAM( timeWarpSpatialComputeProgram ), sizeof( PROGRAM( timeWarpSpatialComputeProgram ) ),
								timeWarpSpatialComputeProgramParms, ARRAY_SIZE( timeWarpSpatialComputeProgramParms ) );
	GpuComputeProgram_Create( context, &compute->timeWarpChromaticProgram,
								PROGRAM( timeWarpChromaticComputeProgram ), sizeof( PROGRAM( timeWarpChromaticComputeProgram ) ),
								timeWarpChromaticComputeProgramParms, ARRAY_SIZE( timeWarpChromaticComputeProgramParms ) );

	GpuComputePipeline_Create( context, &compute->timeWarpTransformPipeline, &compute->timeWarpTransformProgram );
	GpuComputePipeline_Create( context, &compute->timeWarpSpatialPipeline, &compute->timeWarpSpatialProgram );
	GpuComputePipeline_Create( context, &compute->timeWarpChromaticPipeline, &compute->timeWarpChromaticProgram );

	GpuTimer_Create( context, &compute->timeWarpGpuTime );

	GpuFramebuffer_CreateFromTextures( context, &compute->framebuffer, renderPass, window->windowWidth, window->windowHeight, 1 );
}

static void TimeWarpCompute_Destroy( GpuContext_t * context, TimeWarpCompute_t * compute )
{
	GpuFramebuffer_Destroy( context, &compute->framebuffer );

	GpuTimer_Destroy( context, &compute->timeWarpGpuTime );

	GpuComputePipeline_Destroy( context, &compute->timeWarpTransformPipeline );
	GpuComputePipeline_Destroy( context, &compute->timeWarpSpatialPipeline );
	GpuComputePipeline_Destroy( context, &compute->timeWarpChromaticPipeline );

	GpuComputeProgram_Destroy( context, &compute->timeWarpTransformProgram );
	GpuComputeProgram_Destroy( context, &compute->timeWarpSpatialProgram );
	GpuComputeProgram_Destroy( context, &compute->timeWarpChromaticProgram );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
		{
			GpuTexture_Destroy( context, &compute->distortionImage[eye][channel] );
			GpuTexture_Destroy( context, &compute->timeWarpImage[eye][channel] );
		}
	}

	memset( compute, 0, sizeof( TimeWarpCompute_t ) );
}

static void TimeWarpCompute_Render( GpuCommandBuffer_t * commandBuffer, TimeWarpCompute_t * compute,
									GpuFramebuffer_t * framebuffer,
									const Microseconds_t refreshStartTime, const Microseconds_t refreshEndTime,
									const Matrix4x4f_t * projectionMatrix, const Matrix4x4f_t * viewMatrix,
									GpuTexture_t * const eyeTexture[NUM_EYES], const int eyeArrayLayer[NUM_EYES],
									const bool correctChromaticAberration, TimeWarpBarGraphs_t * bargraphs,
									float cpuTimes[PROFILE_TIME_MAX], float gpuTimes[PROFILE_TIME_MAX] )
{
	const Microseconds_t t0 = GetTimeMicroseconds();

	Matrix4x4f_t displayRefreshStartViewMatrix;
	Matrix4x4f_t displayRefreshEndViewMatrix;
	GetHmdViewMatrixForTime( &displayRefreshStartViewMatrix, refreshStartTime );
	GetHmdViewMatrixForTime( &displayRefreshEndViewMatrix, refreshEndTime );

	Matrix4x4f_t timeWarpStartTransform;
	Matrix4x4f_t timeWarpEndTransform;
	CalculateTimeWarpTransform( &timeWarpStartTransform, projectionMatrix, viewMatrix, &displayRefreshStartViewMatrix );
	CalculateTimeWarpTransform( &timeWarpEndTransform, projectionMatrix, viewMatrix, &displayRefreshEndViewMatrix );

	Matrix3x4f_t timeWarpStartTransform3x4;
	Matrix3x4f_t timeWarpEndTransform3x4;
	Matrix3x4f_CreateFromMatrix4x4f( &timeWarpStartTransform3x4, &timeWarpStartTransform );
	Matrix3x4f_CreateFromMatrix4x4f( &timeWarpEndTransform3x4, &timeWarpEndTransform );

	GpuCommandBuffer_BeginPrimary( commandBuffer );
	GpuCommandBuffer_BeginFramebuffer( commandBuffer, &compute->framebuffer, 0, GPU_TEXTURE_USAGE_STORAGE );

	GpuCommandBuffer_BeginTimer( commandBuffer, &compute->timeWarpGpuTime );

	for ( int eye = 0; eye < NUM_EYES; eye ++ )
	{
		for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
		{
			GpuCommandBuffer_ChangeTextureUsage( commandBuffer, &compute->timeWarpImage[eye][channel], GPU_TEXTURE_USAGE_STORAGE );
			GpuCommandBuffer_ChangeTextureUsage( commandBuffer, &compute->distortionImage[eye][channel], GPU_TEXTURE_USAGE_STORAGE );
		}
	}

	const Vector2i_t dimensions = { compute->hmdInfo.eyeTilesWide + 1, compute->hmdInfo.eyeTilesHigh + 1 };
	const int eyeIndex[NUM_EYES] = { 0, 1 };

	for ( int eye = 0; eye < NUM_EYES; eye ++ )
	{
		for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
		{
			GpuComputeCommand_t command;
			GpuComputeCommand_Init( &command );
			GpuComputeCommand_SetPipeline( &command, &compute->timeWarpTransformPipeline );
			GpuComputeCommand_SetParmTextureStorage( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_DST, &compute->timeWarpImage[eye][channel] );
			GpuComputeCommand_SetParmTextureStorage( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_TRANSFORM_SRC, &compute->distortionImage[eye][channel] );
			GpuComputeCommand_SetParmFloatMatrix3x4( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_START_TRANSFORM, &timeWarpStartTransform3x4 );
			GpuComputeCommand_SetParmFloatMatrix3x4( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_END_TRANSFORM, &timeWarpEndTransform3x4 );
			GpuComputeCommand_SetParmIntVector2( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_DIMENSIONS, &dimensions );
			GpuComputeCommand_SetParmInt( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE, &eyeIndex[eye] );
			GpuComputeCommand_SetDimensions( &command, ( dimensions.x + TRANSFORM_LOCAL_SIZE_X - 1 ) / TRANSFORM_LOCAL_SIZE_X, 
														( dimensions.y + TRANSFORM_LOCAL_SIZE_Y - 1 ) / TRANSFORM_LOCAL_SIZE_Y, 1 );

			GpuCommandBuffer_SubmitComputeCommand( commandBuffer, &command );
		}
	}

	for ( int eye = 0; eye < NUM_EYES; eye ++ )
	{
		for ( int channel = 0; channel < NUM_COLOR_CHANNELS; channel++ )
		{
			GpuCommandBuffer_ChangeTextureUsage( commandBuffer, &compute->timeWarpImage[eye][channel], GPU_TEXTURE_USAGE_SAMPLED );
		}
	}
	GpuCommandBuffer_ChangeTextureUsage( commandBuffer, GpuFramebuffer_GetColorTexture( &compute->framebuffer ), GPU_TEXTURE_USAGE_STORAGE );

	const int screenWidth = GpuFramebuffer_GetWidth( &compute->framebuffer );
	const int screenHeight = GpuFramebuffer_GetHeight( &compute->framebuffer );
	const int eyePixelsWide = screenWidth / NUM_EYES;
	const int eyePixelsHigh = screenHeight * compute->hmdInfo.eyeTilesHigh * compute->hmdInfo.tilePixelsHigh / compute->hmdInfo.displayPixelsHigh;
	const Vector2f_t imageScale =
	{
		(float)compute->hmdInfo.eyeTilesWide / ( compute->hmdInfo.eyeTilesWide + 1 ) / eyePixelsWide,
		(float)compute->hmdInfo.eyeTilesHigh / ( compute->hmdInfo.eyeTilesHigh + 1 ) / eyePixelsHigh
	};
	const Vector2f_t imageBias =
	{
		0.5f / ( compute->hmdInfo.eyeTilesWide + 1 ),
		0.5f / ( compute->hmdInfo.eyeTilesHigh + 1 )
	};
	const Vector2i_t eyePixelOffset[NUM_EYES] =
	{
#if defined( GRAPHICS_API_VULKAN )
		{ 0 * eyePixelsWide, screenHeight - eyePixelsHigh },
		{ 1 * eyePixelsWide, screenHeight - eyePixelsHigh }
#else
		{ 0 * eyePixelsWide, eyePixelsHigh },
		{ 1 * eyePixelsWide, eyePixelsHigh }
#endif
	};

	for ( int eye = 0; eye < NUM_EYES; eye ++ )
	{
		assert( screenWidth % ( correctChromaticAberration ? CHROMATIC_LOCAL_SIZE_X : SPATIAL_LOCAL_SIZE_X ) == 0 );
		assert( screenHeight % ( correctChromaticAberration ? CHROMATIC_LOCAL_SIZE_Y : SPATIAL_LOCAL_SIZE_Y ) == 0 );

		GpuComputeCommand_t command;
		GpuComputeCommand_Init( &command );
		GpuComputeCommand_SetPipeline( &command, correctChromaticAberration ? &compute->timeWarpChromaticPipeline : &compute->timeWarpSpatialPipeline );
		GpuComputeCommand_SetParmTextureStorage( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_DEST, GpuFramebuffer_GetColorTexture( &compute->framebuffer ) );
		GpuComputeCommand_SetParmTextureSampled( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_EYE_IMAGE, eyeTexture[eye] );
		GpuComputeCommand_SetParmTextureSampled( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_R, &compute->timeWarpImage[eye][0] );
		GpuComputeCommand_SetParmTextureSampled( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_G, &compute->timeWarpImage[eye][1] );
		GpuComputeCommand_SetParmTextureSampled( &command, COMPUTE_PROGRAM_TEXTURE_TIMEWARP_WARP_IMAGE_B, &compute->timeWarpImage[eye][2] );
		GpuComputeCommand_SetParmFloatVector2( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_SCALE, &imageScale );
		GpuComputeCommand_SetParmFloatVector2( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_BIAS, &imageBias );
		GpuComputeCommand_SetParmIntVector2( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_EYE_PIXEL_OFFSET, &eyePixelOffset[eye] );
		GpuComputeCommand_SetParmInt( &command, COMPUTE_PROGRAM_UNIFORM_TIMEWARP_IMAGE_LAYER, &eyeArrayLayer[eye] );
		GpuComputeCommand_SetDimensions( &command, screenWidth / ( correctChromaticAberration ? CHROMATIC_LOCAL_SIZE_X : SPATIAL_LOCAL_SIZE_X ) / 2,
													screenHeight / ( correctChromaticAberration ? CHROMATIC_LOCAL_SIZE_Y : SPATIAL_LOCAL_SIZE_Y ), 1 );

		GpuCommandBuffer_SubmitComputeCommand( commandBuffer, &command );
	}

	const Microseconds_t t1 = GetTimeMicroseconds();

	TimeWarpBarGraphs_UpdateCompute( commandBuffer, bargraphs );
	TimeWarpBarGraphs_RenderCompute( commandBuffer, bargraphs, &compute->framebuffer );

	const Microseconds_t t2 = GetTimeMicroseconds();

	GpuCommandBuffer_Blit( commandBuffer, &compute->framebuffer, framebuffer );

	GpuCommandBuffer_EndTimer( commandBuffer, &compute->timeWarpGpuTime );

	GpuCommandBuffer_EndFramebuffer( commandBuffer, &compute->framebuffer, 0, GPU_TEXTURE_USAGE_PRESENTATION );
	GpuCommandBuffer_EndPrimary( commandBuffer );

	GpuCommandBuffer_SubmitPrimary( commandBuffer );

	const Microseconds_t t3 = GetTimeMicroseconds();

	cpuTimes[PROFILE_TIME_TIME_WARP] = ( t1 - t0 ) * ( 1.0f / 1000.0f );
	cpuTimes[PROFILE_TIME_BAR_GRAPHS] = ( t2 - t1 ) * ( 1.0f / 1000.0f );
	cpuTimes[PROFILE_TIME_BLIT] = ( t3 - t2 ) * ( 1.0f / 1000.0f );

	const float barGraphGpuTime = TimeWarpBarGraphs_GetGpuMillisecondsCompute( bargraphs );

	gpuTimes[PROFILE_TIME_TIME_WARP] = GpuTimer_GetMilliseconds( &compute->timeWarpGpuTime ) - barGraphGpuTime;
	gpuTimes[PROFILE_TIME_BAR_GRAPHS] = barGraphGpuTime;
	gpuTimes[PROFILE_TIME_BLIT] = 0.0f;

#if GL_FINISH_SYNC == 1
	GL( glFinish() );
#endif
}

#else

typedef struct
{
	int		empty;
} TimeWarpCompute_t;

static void TimeWarpCompute_Create( GpuContext_t * context, TimeWarpCompute_t * compute,
									const HmdInfo_t * hmdInfo, GpuRenderPass_t * renderPass, GpuWindow_t * window )
{
	UNUSED_PARM( context );
	UNUSED_PARM( compute );
	UNUSED_PARM( hmdInfo );
	UNUSED_PARM( renderPass );
	UNUSED_PARM( window );
}

static void TimeWarpCompute_Destroy( GpuContext_t * context, TimeWarpCompute_t * compute )
{
	UNUSED_PARM( context );
	UNUSED_PARM( compute );
}

static void TimeWarpCompute_Render( GpuCommandBuffer_t * commandBuffer, TimeWarpCompute_t * compute,
									GpuFramebuffer_t * framebuffer,
									const Microseconds_t refreshStartTime, const Microseconds_t refreshEndTime,
									const Matrix4x4f_t * projectionMatrix, const Matrix4x4f_t * viewMatrix,
									GpuTexture_t * const eyeTexture[NUM_EYES], const int eyeArrayLayer[NUM_EYES],
									const bool correctChromaticAberration, TimeWarpBarGraphs_t * bargraphs,
									float cpuTimes[PROFILE_TIME_MAX], float gpuTimes[PROFILE_TIME_MAX] )
{
	UNUSED_PARM( commandBuffer );
	UNUSED_PARM( compute );
	UNUSED_PARM( framebuffer );
	UNUSED_PARM( refreshStartTime );
	UNUSED_PARM( refreshEndTime );
	UNUSED_PARM( viewMatrix );
	UNUSED_PARM( eyeTexture );
	UNUSED_PARM( eyeArrayLayer );
	UNUSED_PARM( correctChromaticAberration );
	UNUSED_PARM( bargraphs );
	UNUSED_PARM( cpuTimes );
	UNUSED_PARM( gpuTimes );
}

#endif

/*
================================================================================================================================

Time warp rendering.

TimeWarp_t

static void TimeWarp_Create( TimeWarp_t * timeWarp, GpuWindow_t * window );
static void TimeWarp_Destroy( TimeWarp_t * timeWarp, GpuWindow_t * window );

static void TimeWarp_SetBarGraphState( TimeWarp_t * timeWarp, const BarGraphState_t state );
static void TimeWarp_CycleBarGraphState( TimeWarp_t * timeWarp );
static void TimeWarp_SetImplementation( TimeWarp_t * timeWarp, const TimeWarpImplementation_t implementation );
static void TimeWarp_CycleImplementation( TimeWarp_t * timeWarp );
static void TimeWarp_SetChromaticAberrationCorrection( TimeWarp_t * timeWarp, const bool set );
static void TimeWarp_ToggleChromaticAberrationCorrection( TimeWarp_t * timeWarp );
static void TimeWarp_SetMultiView( TimeWarp_t * timeWarp, const bool enabled );

static void TimeWarp_SetDisplayResolutionLevel( TimeWarp_t * timeWarp, const int level );
static void TimeWarp_SetEyeImageResolutionLevel( TimeWarp_t * timeWarp, const int level );
static void TimeWarp_SetEyeImageSamplesLevel( TimeWarp_t * timeWarp, const int level );

static void TimeWarp_SetDrawCallLevel( TimeWarp_t * timeWarp, const int level );
static void TimeWarp_SetTriangleLevel( TimeWarp_t * timeWarp, const int level );
static void TimeWarp_SetFragmentLevel( TimeWarp_t * timeWarp, const int level );

static Microseconds_t TimeWarp_GetPredictedDisplayTime( TimeWarp_t * timeWarp, const int frameIndex );
static void TimeWarp_SubmitFrame( TimeWarp_t * timeWarp, const int frameIndex, const Microseconds_t displayTime,
									const Matrix4x4f_t * viewMatrix, const Matrix4x4_t * projectionMatrix,
									GpuTexture_t * eyeTexture[NUM_EYES], GpuFence_t * eyeCompletionFence[NUM_EYES],
									int eyeArrayLayer[NUM_EYES], float eyeTexturesCpuTime, float eyeTexturesGpuTime );
static void TimeWarp_Render( TimeWarp_t * timeWarp );

================================================================================================================================
*/

#define AVERAGE_FRAME_RATE_FRAMES		20

typedef enum
{
	TIMEWARP_IMPLEMENTATION_GRAPHICS,
	TIMEWARP_IMPLEMENTATION_COMPUTE,
	TIMEWARP_IMPLEMENTATION_MAX
} TimeWarpImplementation_t;

typedef struct
{
	int							index;
	int							frameIndex;
	Microseconds_t				displayTime;
	Matrix4x4f_t				viewMatrix;
	Matrix4x4f_t				projectionMatrix;
	GpuTexture_t *				texture[NUM_EYES];
	GpuFence_t *				completionFence[NUM_EYES];
	int							arrayLayer[NUM_EYES];
	float						cpuTime;
	float						gpuTime;
} EyeTextures_t;

typedef struct
{
	long long					frameIndex;
	Microseconds_t				vsyncTime;
	Microseconds_t				frameTime;
} FrameTiming_t;

typedef struct
{
	GpuWindow_t *				window;
	GpuTexture_t				defaultTexture;
	Microseconds_t				displayTime;
	Matrix4x4f_t				viewMatrix;
	Matrix4x4f_t				projectionMatrix;
	GpuTexture_t *				eyeTexture[NUM_EYES];
	int							eyeArrayLayer[NUM_EYES];

	Mutex_t						newEyeTexturesMutex;
	Signal_t					newEyeTexturesConsumed;
	EyeTextures_t				newEyeTextures;
	int							eyeTexturesPresentIndex;
	int							eyeTexturesConsumedIndex;

	FrameTiming_t				frameTiming;
	Mutex_t						frameTimingMutex;
	Signal_t					vsyncSignal;

	float						refreshRate;
	Microseconds_t				frameCpuTime[AVERAGE_FRAME_RATE_FRAMES];
	int							eyeTexturesFrames[AVERAGE_FRAME_RATE_FRAMES];
	int							timeWarpFrames;
	float						cpuTimes[PROFILE_TIME_MAX];
	float						gpuTimes[PROFILE_TIME_MAX];

	GpuRenderPass_t				renderPass;
	GpuFramebuffer_t			framebuffer;
	GpuCommandBuffer_t			commandBuffer;
	bool						correctChromaticAberration;
	TimeWarpImplementation_t	implementation;
	TimeWarpGraphics_t			graphics;
	TimeWarpCompute_t			compute;
	TimeWarpBarGraphs_t			bargraphs;
} TimeWarp_t;

static void TimeWarp_Create( TimeWarp_t * timeWarp, GpuWindow_t * window )
{
	timeWarp->window = window;

	GpuTexture_CreateDefault( &window->context, &timeWarp->defaultTexture, GPU_TEXTURE_DEFAULT_CIRCLES, 1024, 1024, 0, 2, 1, false, true );
	GpuTexture_SetWrapMode( &window->context, &timeWarp->defaultTexture, GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER );

	Mutex_Create( &timeWarp->newEyeTexturesMutex );
	Signal_Create( &timeWarp->newEyeTexturesConsumed, true );
	Signal_Raise( &timeWarp->newEyeTexturesConsumed );

	timeWarp->newEyeTextures.index = 0;
	timeWarp->newEyeTextures.displayTime = 0;
	Matrix4x4f_CreateIdentity( &timeWarp->newEyeTextures.viewMatrix );
	Matrix4x4f_CreateProjectionFov( &timeWarp->newEyeTextures.projectionMatrix, 80.0f, 80.0f, 0.0f, 0.0f, 0.1f, 0.0f );
	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		timeWarp->newEyeTextures.texture[eye] = &timeWarp->defaultTexture;
		timeWarp->newEyeTextures.completionFence[eye] = NULL;
		timeWarp->newEyeTextures.arrayLayer[eye] = eye;
	}
	timeWarp->newEyeTextures.cpuTime = 0.0f;
	timeWarp->newEyeTextures.gpuTime = 0.0f;

	timeWarp->displayTime = 0;
	timeWarp->viewMatrix = timeWarp->newEyeTextures.viewMatrix;
	timeWarp->projectionMatrix = timeWarp->newEyeTextures.projectionMatrix;
	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		timeWarp->eyeTexture[eye] = timeWarp->newEyeTextures.texture[eye];
		timeWarp->eyeArrayLayer[eye] = timeWarp->newEyeTextures.arrayLayer[eye];
	}

	timeWarp->eyeTexturesPresentIndex = 1;
	timeWarp->eyeTexturesConsumedIndex = 0;

	timeWarp->frameTiming.frameIndex = 0;
	timeWarp->frameTiming.vsyncTime = 0;
	timeWarp->frameTiming.frameTime = 0;
	Mutex_Create( &timeWarp->frameTimingMutex );
	Signal_Create( &timeWarp->vsyncSignal, false );

	timeWarp->refreshRate = window->windowRefreshRate;
	for ( int i = 0; i < AVERAGE_FRAME_RATE_FRAMES; i++ )
	{
		timeWarp->frameCpuTime[i] = 0;
		timeWarp->eyeTexturesFrames[i] = 0;
	}
	timeWarp->timeWarpFrames = 0;

	GpuRenderPass_Create( &window->context, &timeWarp->renderPass, window->colorFormat, window->depthFormat,
							GPU_SAMPLE_COUNT_1, GPU_RENDERPASS_TYPE_INLINE,
							GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER );
	GpuFramebuffer_CreateFromSwapchain( window, &timeWarp->framebuffer, &timeWarp->renderPass );
	GpuCommandBuffer_Create( &window->context, &timeWarp->commandBuffer, GPU_COMMAND_BUFFER_TYPE_PRIMARY, GpuFramebuffer_GetBufferCount( &timeWarp->framebuffer ) );

	timeWarp->correctChromaticAberration = false;
	timeWarp->implementation = TIMEWARP_IMPLEMENTATION_GRAPHICS;

	const HmdInfo_t * hmdInfo = GetDefaultHmdInfo( window->windowWidth, window->windowHeight );

	TimeWarpGraphics_Create( &window->context, &timeWarp->graphics, hmdInfo, &timeWarp->renderPass );
	TimeWarpCompute_Create( &window->context, &timeWarp->compute, hmdInfo, &timeWarp->renderPass, window );
	TimeWarpBarGraphs_Create( &window->context, &timeWarp->bargraphs, &timeWarp->renderPass );

	memset( timeWarp->cpuTimes, 0, sizeof( timeWarp->cpuTimes ) );
	memset( timeWarp->gpuTimes, 0, sizeof( timeWarp->gpuTimes ) );
}

static void TimeWarp_Destroy( TimeWarp_t * timeWarp, GpuWindow_t * window )
{
	GpuContext_WaitIdle( &window->context );

	TimeWarpGraphics_Destroy( &window->context, &timeWarp->graphics );
	TimeWarpCompute_Destroy( &window->context, &timeWarp->compute );
	TimeWarpBarGraphs_Destroy( &window->context, &timeWarp->bargraphs );

	GpuCommandBuffer_Destroy( &window->context, &timeWarp->commandBuffer );
	GpuFramebuffer_Destroy( &window->context, &timeWarp->framebuffer );
	GpuRenderPass_Destroy( &window->context, &timeWarp->renderPass );

	Signal_Destroy( &timeWarp->newEyeTexturesConsumed );
	Mutex_Destroy( &timeWarp->newEyeTexturesMutex );
	Mutex_Destroy( &timeWarp->frameTimingMutex );
	Signal_Destroy( &timeWarp->vsyncSignal );

	GpuTexture_Destroy( &window->context, &timeWarp->defaultTexture );
}

static void TimeWarp_SetBarGraphState( TimeWarp_t * timeWarp, const BarGraphState_t state )
{
	timeWarp->bargraphs.barGraphState = state;
}

static void TimeWarp_CycleBarGraphState( TimeWarp_t * timeWarp )
{
	timeWarp->bargraphs.barGraphState = (BarGraphState_t)( ( timeWarp->bargraphs.barGraphState + 1 ) % 3 );
}

static void TimeWarp_SetImplementation( TimeWarp_t * timeWarp, const TimeWarpImplementation_t implementation )
{
	timeWarp->implementation = implementation;
	const float delta = ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_GRAPHICS ) ? 0.0f : 1.0f;
	BarGraph_AddBar( &timeWarp->bargraphs.timeWarpImplementationBarGraph, 0, delta, &colorRed, false );
}

static void TimeWarp_CycleImplementation( TimeWarp_t * timeWarp )
{
	timeWarp->implementation = (TimeWarpImplementation_t)( ( timeWarp->implementation + 1 ) % TIMEWARP_IMPLEMENTATION_MAX );
#if OPENGL_COMPUTE_ENABLED == 0
	if ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_COMPUTE )
	{
		timeWarp->implementation = (TimeWarpImplementation_t)( ( timeWarp->implementation + 1 ) % TIMEWARP_IMPLEMENTATION_MAX );
	}
#endif
	const float delta = ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_GRAPHICS ) ? 0.0f : 1.0f;
	BarGraph_AddBar( &timeWarp->bargraphs.timeWarpImplementationBarGraph, 0, delta, &colorRed, false );
}

static void TimeWarp_SetChromaticAberrationCorrection( TimeWarp_t * timeWarp, const bool set )
{
	timeWarp->correctChromaticAberration = set;
	BarGraph_AddBar( &timeWarp->bargraphs.correctChromaticAberrationBarGraph, 0, timeWarp->correctChromaticAberration ? 1.0f : 0.0f, &colorRed, false );
}

static void TimeWarp_ToggleChromaticAberrationCorrection( TimeWarp_t * timeWarp )
{
	timeWarp->correctChromaticAberration = !timeWarp->correctChromaticAberration;
	BarGraph_AddBar( &timeWarp->bargraphs.correctChromaticAberrationBarGraph, 0, timeWarp->correctChromaticAberration ? 1.0f : 0.0f, &colorRed, false );
}

static void TimeWarp_SetMultiView( TimeWarp_t * timeWarp, const bool enabled )
{
	BarGraph_AddBar( &timeWarp->bargraphs.multiViewBarGraph, 0, enabled ? 1.0f : 0.0f, &colorRed, false );
}

static void TimeWarp_SetDisplayResolutionLevel( TimeWarp_t * timeWarp, const int level )
{
	const Vector4f_t * levelColor[4] = { &colorBlue, &colorGreen, &colorYellow, &colorRed };
	for ( int i = 0; i < 4; i++ )
	{
		BarGraph_AddBar( &timeWarp->bargraphs.displayResolutionLevelBarGraph, i, ( i <= level ) ? 0.25f : 0.0f, levelColor[i], false );
	}
}

static void TimeWarp_SetEyeImageResolutionLevel( TimeWarp_t * timeWarp, const int level )
{
	const Vector4f_t * levelColor[4] = { &colorBlue, &colorGreen, &colorYellow, &colorRed };
	for ( int i = 0; i < 4; i++ )
	{
		BarGraph_AddBar( &timeWarp->bargraphs.eyeImageResolutionLevelBarGraph, i, ( i <= level ) ? 0.25f : 0.0f, levelColor[i], false );
	}
}

static void TimeWarp_SetEyeImageSamplesLevel( TimeWarp_t * timeWarp, const int level )
{
	const Vector4f_t * levelColor[4] = { &colorBlue, &colorGreen, &colorYellow, &colorRed };
	for ( int i = 0; i < 4; i++ )
	{
		BarGraph_AddBar( &timeWarp->bargraphs.eyeImageSamplesLevelBarGraph, i, ( i <= level ) ? 0.25f : 0.0f, levelColor[i], false );
	}
}

static void TimeWarp_SetDrawCallLevel( TimeWarp_t * timeWarp, const int level )
{
	const Vector4f_t * levelColor[4] = { &colorBlue, &colorGreen, &colorYellow, &colorRed };
	for ( int i = 0; i < 4; i++ )
	{
		BarGraph_AddBar( &timeWarp->bargraphs.sceneDrawCallLevelBarGraph, i, ( i <= level ) ? 0.25f : 0.0f, levelColor[i], false );
	}
}

static void TimeWarp_SetTriangleLevel( TimeWarp_t * timeWarp, const int level )
{
	const Vector4f_t * levelColor[4] = { &colorBlue, &colorGreen, &colorYellow, &colorRed };
	for ( int i = 0; i < 4; i++ )
	{
		BarGraph_AddBar( &timeWarp->bargraphs.sceneTriangleLevelBarGraph, i, ( i <= level ) ? 0.25f : 0.0f, levelColor[i], false );
	}
}

static void TimeWarp_SetFragmentLevel( TimeWarp_t * timeWarp, const int level )
{
	const Vector4f_t * levelColor[4] = { &colorBlue, &colorGreen, &colorYellow, &colorRed };
	for ( int i = 0; i < 4; i++ )
	{
		BarGraph_AddBar( &timeWarp->bargraphs.sceneFragmentLevelBarGraph, i, ( i <= level ) ? 0.25f : 0.0f, levelColor[i], false );
	}
}

static Microseconds_t TimeWarp_GetPredictedDisplayTime( TimeWarp_t * timeWarp, const int frameIndex )
{
	Mutex_Lock( &timeWarp->frameTimingMutex, true );
	const FrameTiming_t frameTiming = timeWarp->frameTiming;
	Mutex_Unlock( &timeWarp->frameTimingMutex );

	// The time warp thread is currently released by SwapBuffers shortly after a V-Sync.
	// Where possible, the time warp thread then waits until a short time before the next V-Sync
	// giving it just enough time to warp the latest eye textures onto the display. The time warp
	// thread then tries to pick up the latest eye textures that will be displayed the next V-Sync.
	// The time warp then warps the new eye textures onto the display for the next V-Sync. After
	// this V-Sync the main thread is released and can start working on new eye textures that
	// are will be displayed effectively 2 display refresh cycles in the future.

	return frameTiming.vsyncTime + ( frameIndex - frameTiming.frameIndex ) * frameTiming.frameTime;
}

static void TimeWarp_SubmitFrame( TimeWarp_t * timeWarp, const int frameIndex, const Microseconds_t displayTime,
									const Matrix4x4f_t * viewMatrix, const Matrix4x4f_t * projectionMatrix,
									GpuTexture_t * eyeTexture[NUM_EYES], GpuFence_t * eyeCompletionFence[NUM_EYES],
									int eyeArrayLayer[NUM_EYES], float eyeTexturesCpuTime, float eyeTexturesGpuTime )
{
	EyeTextures_t newEyeTextures;
	newEyeTextures.index = timeWarp->eyeTexturesPresentIndex++;
	newEyeTextures.frameIndex = frameIndex;
	newEyeTextures.displayTime = displayTime;
	newEyeTextures.viewMatrix = *viewMatrix;
	newEyeTextures.projectionMatrix = *projectionMatrix;
	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		newEyeTextures.texture[eye] = eyeTexture[eye];
		newEyeTextures.completionFence[eye] = eyeCompletionFence[eye];
		newEyeTextures.arrayLayer[eye] = eyeArrayLayer[eye];
	}
	newEyeTextures.cpuTime = eyeTexturesCpuTime;
	newEyeTextures.gpuTime = eyeTexturesGpuTime;

	// Wait for the previous eye textures to be consumed before overwriting them.
	Signal_Wait( &timeWarp->newEyeTexturesConsumed, -1 );

	Mutex_Lock( &timeWarp->newEyeTexturesMutex, true );
	timeWarp->newEyeTextures = newEyeTextures;
	Mutex_Unlock( &timeWarp->newEyeTexturesMutex );

	// Wait for at least one V-Sync to pass to avoid piling up frames of latency.
	Signal_Wait( &timeWarp->vsyncSignal, -1 );

	FrameTiming_t newFrameTiming;
	newFrameTiming.frameIndex = frameIndex;
	newFrameTiming.vsyncTime = GpuWindow_GetNextSwapTimeMicroseconds( timeWarp->window );
	newFrameTiming.frameTime = GpuWindow_GetFrameTimeMicroseconds( timeWarp->window );

	Mutex_Lock( &timeWarp->frameTimingMutex, true );
	timeWarp->frameTiming = newFrameTiming;
	Mutex_Unlock( &timeWarp->frameTimingMutex );
}

static void TimeWarp_Render( TimeWarp_t * timeWarp )
{
	const Microseconds_t nextSwapTime = GpuWindow_GetNextSwapTimeMicroseconds( timeWarp->window );
	const Microseconds_t frameTime = GpuWindow_GetFrameTimeMicroseconds( timeWarp->window );

	// Wait until close to the next V-Sync but still far enough away to allow the time warp to complete rendering.
	GpuWindow_DelayBeforeSwap( timeWarp->window, frameTime / 2 );

	timeWarp->eyeTexturesFrames[timeWarp->timeWarpFrames % AVERAGE_FRAME_RATE_FRAMES] = 0;

	// Try to pick up the latest eye textures but never block the time warp thread.
	// It is better to display an old set of eye textures than to miss the next V-Sync
	// in case another thread is suspended while holding on to the mutex.
	if ( Mutex_Lock( &timeWarp->newEyeTexturesMutex, false ) )
	{
		EyeTextures_t newEyeTextures = timeWarp->newEyeTextures;
		Mutex_Unlock( &timeWarp->newEyeTexturesMutex );

		// If this is a new set of eye textures.
		if ( newEyeTextures.index > timeWarp->eyeTexturesConsumedIndex &&
				// Never display the eye textures before they are meant to be displayed.
				newEyeTextures.displayTime < nextSwapTime + frameTime / 2 &&
					// Make sure both eye textures have completed rendering.
					GpuFence_IsSignalled( &timeWarp->window->context, newEyeTextures.completionFence[0] ) &&
						GpuFence_IsSignalled( &timeWarp->window->context, newEyeTextures.completionFence[1] ) )
		{
			assert( newEyeTextures.index == timeWarp->eyeTexturesConsumedIndex + 1 );
			timeWarp->eyeTexturesConsumedIndex = newEyeTextures.index;
			timeWarp->displayTime = newEyeTextures.displayTime;
			timeWarp->projectionMatrix = newEyeTextures.projectionMatrix;
			timeWarp->viewMatrix = newEyeTextures.viewMatrix;
			for ( int eye = 0; eye < NUM_EYES; eye++ )
			{
				timeWarp->eyeTexture[eye] = newEyeTextures.texture[eye];
				timeWarp->eyeArrayLayer[eye] = newEyeTextures.arrayLayer[eye];
			}
			timeWarp->cpuTimes[PROFILE_TIME_EYE_TEXTURES] = newEyeTextures.cpuTime;
			timeWarp->gpuTimes[PROFILE_TIME_EYE_TEXTURES] = newEyeTextures.gpuTime;
			timeWarp->eyeTexturesFrames[timeWarp->timeWarpFrames % AVERAGE_FRAME_RATE_FRAMES] = 1;
			Signal_Clear( &timeWarp->vsyncSignal );
			Signal_Raise( &timeWarp->newEyeTexturesConsumed );
		}
	}

	// Calculate the eye texture and time warp frame rates.
	float timeWarpFrameRate = timeWarp->refreshRate;
	float eyeTexturesFrameRate = timeWarp->refreshRate;
	{
		Microseconds_t lastTime = timeWarp->frameCpuTime[timeWarp->timeWarpFrames % AVERAGE_FRAME_RATE_FRAMES];
		Microseconds_t time = nextSwapTime;
		timeWarp->frameCpuTime[timeWarp->timeWarpFrames % AVERAGE_FRAME_RATE_FRAMES] = time;
		timeWarp->timeWarpFrames++;
		if ( timeWarp->timeWarpFrames > AVERAGE_FRAME_RATE_FRAMES )
		{
			int timeWarpFrames = AVERAGE_FRAME_RATE_FRAMES;
			int eyeTexturesFrames = 0;
			for ( int i = 0; i < AVERAGE_FRAME_RATE_FRAMES; i++ )
			{
				eyeTexturesFrames += timeWarp->eyeTexturesFrames[i];
			}

			timeWarpFrameRate = timeWarpFrames * 1000.0f * 1000.0f / ( time - lastTime );
			eyeTexturesFrameRate = eyeTexturesFrames * 1000.0f * 1000.0f / ( time - lastTime );
		}
	}

	// Update the bar graphs if not paused.
	if ( timeWarp->bargraphs.barGraphState == BAR_GRAPH_VISIBLE )
	{
		const Vector4f_t * eyeTexturesFrameRateColor = ( eyeTexturesFrameRate > timeWarp->refreshRate - 0.5f ) ? &colorPurple : &colorRed;
		const Vector4f_t * timeWarpFrameRateColor = ( timeWarpFrameRate > timeWarp->refreshRate - 0.5f ) ? &colorGreen : &colorRed;

		BarGraph_AddBar( &timeWarp->bargraphs.eyeTexturesFrameRateGraph, 0, eyeTexturesFrameRate / timeWarp->refreshRate, eyeTexturesFrameRateColor, true );
		BarGraph_AddBar( &timeWarp->bargraphs.timeWarpFrameRateGraph, 0, timeWarpFrameRate / timeWarp->refreshRate, timeWarpFrameRateColor, true );

		for ( int i = 0; i < 2; i++ )
		{
			const float * times = ( i == 0 ) ? timeWarp->cpuTimes : timeWarp->gpuTimes;
			float barHeights[PROFILE_TIME_MAX];
			float totalBarHeight = 0.0f;
			for ( int p = 0; p < PROFILE_TIME_MAX; p++ )
			{
				barHeights[p] = times[p] * timeWarp->refreshRate * ( 1.0f / 1000.0f );
				totalBarHeight += barHeights[p];
			}

			const float limit = 0.9f;
			if ( totalBarHeight > limit )
			{
				totalBarHeight = 0.0f;
				for ( int p = 0; p < PROFILE_TIME_MAX; p++ )
				{
					barHeights[p] = ( totalBarHeight + barHeights[p] > limit ) ? ( limit - totalBarHeight ) : barHeights[p];
					totalBarHeight += barHeights[p];
				}
				barHeights[PROFILE_TIME_OVERFLOW] = 1.0f - limit;
			}

			BarGraph_t * barGraph = ( i == 0 ) ? &timeWarp->bargraphs.frameCpuTimeBarGraph : &timeWarp->bargraphs.frameGpuTimeBarGraph;
			for ( int p = 0; p < PROFILE_TIME_MAX; p++ )
			{
				BarGraph_AddBar( barGraph, p, barHeights[p], profileTimeBarColors[p], ( p == PROFILE_TIME_MAX - 1 ) );
			}
		}
	}

	FrameLog_BeginFrame();

	//assert( timeWarp->displayTime == nextSwapTime );
	const Microseconds_t refreshStartTime = nextSwapTime;
	const Microseconds_t refreshEndTime = refreshStartTime /* + display refresh time for an incremental display refresh */;

	if ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_GRAPHICS )
	{
		TimeWarpGraphics_Render( &timeWarp->commandBuffer, &timeWarp->graphics, &timeWarp->framebuffer, &timeWarp->renderPass,
								refreshStartTime, refreshEndTime,
								&timeWarp->projectionMatrix, &timeWarp->viewMatrix,
								timeWarp->eyeTexture, timeWarp->eyeArrayLayer, timeWarp->correctChromaticAberration,
								&timeWarp->bargraphs, timeWarp->cpuTimes, timeWarp->gpuTimes );
	}
	else if ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_COMPUTE )
	{
		TimeWarpCompute_Render( &timeWarp->commandBuffer, &timeWarp->compute, &timeWarp->framebuffer,
								refreshStartTime, refreshEndTime,
								&timeWarp->projectionMatrix, &timeWarp->viewMatrix,
								timeWarp->eyeTexture, timeWarp->eyeArrayLayer, timeWarp->correctChromaticAberration,
								&timeWarp->bargraphs, timeWarp->cpuTimes, timeWarp->gpuTimes );
	}

	const int gpuTimeFramesDelayed = ( timeWarp->implementation == TIMEWARP_IMPLEMENTATION_GRAPHICS ) ? GPU_TIMER_FRAMES_DELAYED : 0;

	FrameLog_EndFrame(	timeWarp->cpuTimes[PROFILE_TIME_TIME_WARP] +
						timeWarp->cpuTimes[PROFILE_TIME_BAR_GRAPHS] +
						timeWarp->cpuTimes[PROFILE_TIME_BLIT],
						timeWarp->gpuTimes[PROFILE_TIME_TIME_WARP] +
						timeWarp->gpuTimes[PROFILE_TIME_BAR_GRAPHS] +
						timeWarp->gpuTimes[PROFILE_TIME_BLIT], gpuTimeFramesDelayed );

	GpuWindow_SwapBuffers( timeWarp->window );

	Signal_Raise( &timeWarp->vsyncSignal );
}

/*
================================================================================================================================

ViewState_t

static void ViewState_Init( ViewState_t * viewState, const float interpupillaryDistance );
static void ViewState_HandleInput( ViewState_t * viewState, GpuWindowInput_t * input, const Microseconds_t time );
static void ViewState_HandleHmd( ViewState_t * viewState, const Microseconds_t time );

================================================================================================================================
*/

typedef struct ViewState_t
{
	float						interpupillaryDistance;
	Vector4f_t					viewport;
	Vector3f_t					viewTranslationalVelocity;
	Vector3f_t					viewRotationalVelocity;
	Vector3f_t					viewTranslation;
	Vector3f_t					viewRotation;
	Matrix4x4f_t				hmdViewMatrix;						// HMD view matrix.
	Matrix4x4f_t				centerViewMatrix;					// Center view matrix.
	Matrix4x4f_t				viewMatrix[NUM_EYES];				// Per eye view matrix.
	Matrix4x4f_t				projectionMatrix[NUM_EYES];			// Per eye projection matrix.
	Matrix4x4f_t				viewInverseMatrix[NUM_EYES];		// Per eye inverse view matrix.
	Matrix4x4f_t				projectionInverseMatrix[NUM_EYES];	// Per eye inverse projection matrix.
	Matrix4x4f_t				combinedViewProjectionMatrix;		// Combined matrix containing all views for culling.
} ViewState_t;

static void ViewState_DerivedData( ViewState_t * viewState )
{
	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		Matrix4x4f_Invert( &viewState->viewInverseMatrix[eye], &viewState->viewMatrix[eye] );
		Matrix4x4f_Invert( &viewState->projectionInverseMatrix[eye], &viewState->projectionMatrix[eye] );
	}

	// Derive a combined view and projection matrix that encapsulates both views.
	Matrix4x4f_t combinedProjectionMatrix;
	combinedProjectionMatrix = viewState->projectionMatrix[0];
	combinedProjectionMatrix.m[0][0] = viewState->projectionMatrix[0].m[0][0] / ( fabsf( viewState->projectionMatrix[0].m[2][0] ) + 1.0f );
	combinedProjectionMatrix.m[2][0] = 0.0f;
 
	Matrix4x4f_t moveBackMatrix;
	Matrix4x4f_CreateTranslation( &moveBackMatrix, 0.0f, 0.0f, -0.5f * viewState->interpupillaryDistance * combinedProjectionMatrix.m[0][0] );

	Matrix4x4f_t combinedViewMatrix;
	Matrix4x4f_Multiply( &combinedViewMatrix, &moveBackMatrix, &viewState->centerViewMatrix );

	Matrix4x4f_Multiply( &viewState->combinedViewProjectionMatrix, &combinedProjectionMatrix, &combinedViewMatrix );
}

static void ViewState_Init( ViewState_t * viewState, const float interpupillaryDistance )
{
	viewState->interpupillaryDistance = interpupillaryDistance;
	viewState->viewport.x = 0.0f;
	viewState->viewport.y = 0.0f;
	viewState->viewport.z = 1.0f;
	viewState->viewport.w = 1.0f;
	viewState->viewTranslationalVelocity.x = 0.0f;
	viewState->viewTranslationalVelocity.y = 0.0f;
	viewState->viewTranslationalVelocity.z = 0.0f;
	viewState->viewRotationalVelocity.x = 0.0f;
	viewState->viewRotationalVelocity.y = 0.0f;
	viewState->viewRotationalVelocity.z = 0.0f;
	viewState->viewTranslation.x = 0.0f;
	viewState->viewTranslation.y = 1.5f;
	viewState->viewTranslation.z = 0.25f;
	viewState->viewRotation.x = 0.0f;
	viewState->viewRotation.y = 0.0f;
	viewState->viewRotation.z = 0.0f;

	Matrix4x4f_CreateIdentity( &viewState->hmdViewMatrix );
	Matrix4x4f_CreateIdentity( &viewState->centerViewMatrix );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		Matrix4x4f_CreateIdentity( &viewState->viewMatrix[eye] );
		Matrix4x4f_CreateProjectionFov( &viewState->projectionMatrix[eye], 90.0f, 60.0f, 0.0f, 0.0f, 0.01f, 0.0f );

		Matrix4x4f_Invert( &viewState->viewInverseMatrix[eye], &viewState->viewMatrix[eye] );
		Matrix4x4f_Invert( &viewState->projectionInverseMatrix[eye], &viewState->projectionMatrix[eye] );
	}

	ViewState_DerivedData( viewState );
}

static void ViewState_HandleInput( ViewState_t * viewState, GpuWindowInput_t * input, const Microseconds_t time )
{
	static const float TRANSLATION_UNITS_PER_TAP		= 0.005f;
	static const float TRANSLATION_UNITS_DECAY			= 0.0025f;
	static const float ROTATION_DEGREES_PER_TAP			= 0.25f;
	static const float ROTATION_DEGREES_DECAY			= 0.125f;
	static const Vector3f_t minTranslationalVelocity	= { -0.05f, -0.05f, -0.05f};
	static const Vector3f_t maxTranslationalVelocity	= { 0.05f, 0.05f, 0.05f };
	static const Vector3f_t minRotationalVelocity		= { -2.0f, -2.0f, -2.0f };
	static const Vector3f_t maxRotationalVelocity		= { 2.0f, 2.0f, 2.0f };

	GetHmdViewMatrixForTime( &viewState->hmdViewMatrix, time );

	Vector3f_t translationDelta = { 0.0f, 0.0f, 0.0f };
	Vector3f_t rotationDelta = { 0.0f, 0.0f, 0.0f };

	// NOTE: don't consume but only 'check' the keyboard state in case the input is maintained on another thread.
	if ( GpuWindowInput_CheckKeyboardKey( input, KEY_SHIFT_LEFT ) )
	{
		if ( GpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_UP ) )			{ rotationDelta.x -= ROTATION_DEGREES_PER_TAP; }
		else if ( GpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_DOWN ) )	{ rotationDelta.x += ROTATION_DEGREES_PER_TAP; }
		else if ( GpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_LEFT ) )	{ rotationDelta.y += ROTATION_DEGREES_PER_TAP; }
		else if ( GpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_RIGHT ) )	{ rotationDelta.y -= ROTATION_DEGREES_PER_TAP; }
	}
	else if ( GpuWindowInput_CheckKeyboardKey( input, KEY_CTRL_LEFT ) )
	{
		if ( GpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_UP ) )			{ translationDelta.y += TRANSLATION_UNITS_PER_TAP; }
		else if ( GpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_DOWN ) )	{ translationDelta.y -= TRANSLATION_UNITS_PER_TAP; }
		else if ( GpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_LEFT ) )	{ translationDelta.x -= TRANSLATION_UNITS_PER_TAP; }
		else if ( GpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_RIGHT ) )	{ translationDelta.x += TRANSLATION_UNITS_PER_TAP; }
	}
	else
	{
		if ( GpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_UP ) )			{ translationDelta.z -= TRANSLATION_UNITS_PER_TAP; }
		else if ( GpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_DOWN ) )	{ translationDelta.z += TRANSLATION_UNITS_PER_TAP; }
		else if ( GpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_LEFT ) )	{ rotationDelta.y += ROTATION_DEGREES_PER_TAP; }
		else if ( GpuWindowInput_CheckKeyboardKey( input, KEY_CURSOR_RIGHT ) )	{ rotationDelta.y -= ROTATION_DEGREES_PER_TAP; }
	}

	Vector3f_Decay( &viewState->viewTranslationalVelocity, &viewState->viewTranslationalVelocity, TRANSLATION_UNITS_DECAY );
	Vector3f_Decay( &viewState->viewRotationalVelocity, &viewState->viewRotationalVelocity, ROTATION_DEGREES_DECAY );

	Vector3f_Add( &viewState->viewTranslationalVelocity, &viewState->viewTranslationalVelocity, &translationDelta );
	Vector3f_Add( &viewState->viewRotationalVelocity, &viewState->viewRotationalVelocity, &rotationDelta );

	Vector3f_Max( &viewState->viewTranslationalVelocity, &viewState->viewTranslationalVelocity, &minTranslationalVelocity );
	Vector3f_Min( &viewState->viewTranslationalVelocity, &viewState->viewTranslationalVelocity, &maxTranslationalVelocity );

	Vector3f_Max( &viewState->viewRotationalVelocity, &viewState->viewRotationalVelocity, &minRotationalVelocity );
	Vector3f_Min( &viewState->viewRotationalVelocity, &viewState->viewRotationalVelocity, &maxRotationalVelocity );

	Vector3f_Add( &viewState->viewRotation, &viewState->viewRotation, &viewState->viewRotationalVelocity );

	Matrix4x4f_t yawRotation;
	Matrix4x4f_CreateRotation( &yawRotation, 0.0f, viewState->viewRotation.y, 0.0f );

	Vector3f_t rotatedTranslationalVelocity;
	Matrix4x4f_TransformVector3f( &rotatedTranslationalVelocity, &yawRotation, &viewState->viewTranslationalVelocity );

	Vector3f_Add( &viewState->viewTranslation, &viewState->viewTranslation, &rotatedTranslationalVelocity );

	Matrix4x4f_t viewRotation;
	Matrix4x4f_CreateRotation( &viewRotation, viewState->viewRotation.x, viewState->viewRotation.y, viewState->viewRotation.z );

	Matrix4x4f_t viewRotationTranspose;
	Matrix4x4f_Transpose( &viewRotationTranspose, &viewRotation );

	Matrix4x4f_t viewTranslation;
	Matrix4x4f_CreateTranslation( &viewTranslation, -viewState->viewTranslation.x, -viewState->viewTranslation.y, -viewState->viewTranslation.z );

	Matrix4x4f_t inputViewMatrix;
	Matrix4x4f_Multiply( &inputViewMatrix, &viewRotationTranspose, &viewTranslation );

	Matrix4x4f_Multiply( &viewState->centerViewMatrix, &viewState->hmdViewMatrix, &inputViewMatrix );

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		Matrix4x4f_t eyeOffsetMatrix;
		Matrix4x4f_CreateTranslation( &eyeOffsetMatrix, ( eye ? -0.5f : 0.5f ) * viewState->interpupillaryDistance, 0.0f, 0.0f );

		Matrix4x4f_Multiply( &viewState->viewMatrix[eye], &eyeOffsetMatrix, &viewState->centerViewMatrix );
		Matrix4x4f_CreateProjectionFov( &viewState->projectionMatrix[eye], 90.0f, 60.0f, 0.0f, 0.0f, 0.01f, 0.0f );
	}

	ViewState_DerivedData( viewState );
}

static void ViewState_HandleHmd( ViewState_t * viewState, const Microseconds_t time )
{
	GetHmdViewMatrixForTime( &viewState->hmdViewMatrix, time );

	viewState->centerViewMatrix = viewState->hmdViewMatrix;

	for ( int eye = 0; eye < NUM_EYES; eye++ )
	{
		Matrix4x4f_t eyeOffsetMatrix;
		Matrix4x4f_CreateTranslation( &eyeOffsetMatrix, ( eye ? -0.5f : 0.5f ) * viewState->interpupillaryDistance, 0.0f, 0.0f );

		Matrix4x4f_Multiply( &viewState->viewMatrix[eye], &eyeOffsetMatrix, &viewState->centerViewMatrix );
		Matrix4x4f_CreateProjectionFov( &viewState->projectionMatrix[eye], 90.0f, 72.0f, 0.0f, 0.0f, 0.01f, 0.0f );
	}

	ViewState_DerivedData( viewState );
}

/*
================================================================================================================================

Scene settings.

SceneSettings_t

static void SceneSettings_ToggleSimulationPaused( SceneSettings_t * settings );
static void SceneSettings_ToggleMultiView( SceneSettings_t * settings );
static void SceneSettings_SetSimulationPaused( SceneSettings_t * settings, const bool set );
static void SceneSettings_SetMultiView( SceneSettings_t * settings, const bool set );
static bool SceneSettings_GetSimulationPaused( SceneSettings_t * settings );
static bool SceneSettings_GetMultiView( SceneSettings_t * settings );

static void SceneSettings_CycleDisplayResolutionLevel( SceneSettings_t * settings );
static void SceneSettings_CycleEyeImageResolutionLevel( SceneSettings_t * settings );
static void SceneSettings_CycleEyeImageSamplesLevel( SceneSettings_t * settings );

static void SceneSettings_CycleDrawCallLevel( SceneSettings_t * settings );
static void SceneSettings_CycleTriangleLevel( SceneSettings_t * settings );
static void SceneSettings_CycleFragmentLevel( SceneSettings_t * settings );

static void SceneSettings_SetDisplayResolutionLevel( SceneSettings_t * settings, const int level );
static void SceneSettings_SetEyeImageResolutionLevel( SceneSettings_t * settings, const int level );
static void SceneSettings_SetEyeImageSamplesLevel( SceneSettings_t * settings, const int level );

static void SceneSettings_SetDrawCallLevel( SceneSettings_t * settings, const int level );
static void SceneSettings_SetTriangleLevel( SceneSettings_t * settings, const int level );
static void SceneSettings_SetFragmentLevel( SceneSettings_t * settings, const int level );

static int SceneSettings_GetDisplayResolutionLevel( const SceneSettings_t * settings );
static int SceneSettings_GetEyeImageResolutionLevel( const SceneSettings_t * settings );
static int SceneSettings_GetEyeImageSamplesLevel( const SceneSettings_t * settings );

static int SceneSettings_GetDrawCallLevel( const SceneSettings_t * settings );
static int SceneSettings_GetTriangleLevel( const SceneSettings_t * settings );
static int SceneSettings_GetFragmentLevel( const SceneSettings_t * settings );

================================================================================================================================
*/

#define MAX_DISPLAY_RESOLUTION_LEVELS		4
#define MAX_EYE_IMAGE_RESOLUTION_LEVELS		4
#define MAX_EYE_IMAGE_SAMPLES_LEVELS		4

#define MAX_SCENE_DRAWCALL_LEVELS			4
#define MAX_SCENE_TRIANGLE_LEVELS			4
#define MAX_SCENE_FRAGMENT_LEVELS			4

static const int displayResolutionTable[] =
{
	1920, 1080,
	2560, 1440,
	3840, 2160,
	7680, 4320
};

typedef struct
{
	bool	simulationPaused;
	bool	useMultiView;
	int		displayResolutionLevel;
	int		eyeImageResolutionLevel;
	int		eyeImageSamplesLevel;
	int		drawCallLevel;
	int		triangleLevel;
	int		fragmentLevel;
	int		maxDisplayResolutionLevels;
	int		maxEyeImageResolutionLevels;
	int		maxEyeImageSamplesLevels;
} SceneSettings_t;

static void SceneSettings_Init( GpuContext_t * context, SceneSettings_t * settings )
{
	const int maxEyeImageSamplesLevels = IntegerLog2( glGetInteger( GL_MAX_SAMPLES ) + 1 );

	settings->simulationPaused = false;
	settings->useMultiView = false;
	settings->displayResolutionLevel = 0;
	settings->eyeImageResolutionLevel = 0;
	settings->eyeImageSamplesLevel = 0;
	settings->drawCallLevel = 0;
	settings->triangleLevel = 0;
	settings->fragmentLevel = 0;
	settings->maxDisplayResolutionLevels =	( !GpuWindow_SupportedResolution( displayResolutionTable[1 * 2 + 0], displayResolutionTable[1 * 2 + 1] ) ? 1 :
											( !GpuWindow_SupportedResolution( displayResolutionTable[2 * 2 + 0], displayResolutionTable[2 * 2 + 1] ) ? 2 :
											( !GpuWindow_SupportedResolution( displayResolutionTable[3 * 2 + 0], displayResolutionTable[3 * 2 + 1] ) ? 3 : 4 ) ) );
	settings->maxEyeImageResolutionLevels = MAX_EYE_IMAGE_RESOLUTION_LEVELS;
	settings->maxEyeImageSamplesLevels = ( maxEyeImageSamplesLevels < MAX_EYE_IMAGE_SAMPLES_LEVELS ) ? maxEyeImageSamplesLevels : MAX_EYE_IMAGE_SAMPLES_LEVELS;

	UNUSED_PARM( context );
}

static void CycleLevel( int * x, const int max ) { (*x) = ( (*x) + 1 ) % max; }

static void SceneSettings_ToggleSimulationPaused( SceneSettings_t * settings ) { settings->simulationPaused = !settings->simulationPaused; }
static void SceneSettings_ToggleMultiView( SceneSettings_t * settings ) { settings->useMultiView = !settings->useMultiView; }

static void SceneSettings_SetSimulationPaused( SceneSettings_t * settings, const bool set ) { settings->simulationPaused = set; }
static void SceneSettings_SetMultiView( SceneSettings_t * settings, const bool set ) { settings->useMultiView = set; }

static bool SceneSettings_GetSimulationPaused( SceneSettings_t * settings ) { return settings->simulationPaused; }
static bool SceneSettings_GetMultiView( SceneSettings_t * settings ) { return settings->useMultiView; }

static void SceneSettings_CycleDisplayResolutionLevel( SceneSettings_t * settings ) { CycleLevel( &settings->displayResolutionLevel, settings->maxDisplayResolutionLevels ); }
static void SceneSettings_CycleEyeImageResolutionLevel( SceneSettings_t * settings ) { CycleLevel( &settings->eyeImageResolutionLevel, settings->maxEyeImageResolutionLevels ); }
static void SceneSettings_CycleEyeImageSamplesLevel( SceneSettings_t * settings ) { CycleLevel( &settings->eyeImageSamplesLevel, settings->maxEyeImageSamplesLevels ); }

static void SceneSettings_CycleDrawCallLevel( SceneSettings_t * settings ) { CycleLevel( &settings->drawCallLevel, MAX_SCENE_DRAWCALL_LEVELS ); }
static void SceneSettings_CycleTriangleLevel( SceneSettings_t * settings ) { CycleLevel( &settings->triangleLevel, MAX_SCENE_TRIANGLE_LEVELS ); }
static void SceneSettings_CycleFragmentLevel( SceneSettings_t * settings ) { CycleLevel( &settings->fragmentLevel, MAX_SCENE_FRAGMENT_LEVELS ); }

static void SceneSettings_SetDisplayResolutionLevel( SceneSettings_t * settings, const int level ) { settings->displayResolutionLevel = ( level < settings->maxDisplayResolutionLevels ) ? level : settings->maxDisplayResolutionLevels; }
static void SceneSettings_SetEyeImageResolutionLevel( SceneSettings_t * settings, const int level ) { settings->eyeImageResolutionLevel = ( level < settings->maxEyeImageResolutionLevels ) ? level : settings->maxEyeImageResolutionLevels; }
static void SceneSettings_SetEyeImageSamplesLevel( SceneSettings_t * settings, const int level ) { settings->eyeImageSamplesLevel = ( level < settings->maxEyeImageSamplesLevels ) ? level : settings->maxEyeImageSamplesLevels; }

static void SceneSettings_SetDrawCallLevel( SceneSettings_t * settings, const int level ) { settings->drawCallLevel = level; }
static void SceneSettings_SetTriangleLevel( SceneSettings_t * settings, const int level ) { settings->triangleLevel = level; }
static void SceneSettings_SetFragmentLevel( SceneSettings_t * settings, const int level ) { settings->fragmentLevel = level; }

static int SceneSettings_GetDisplayResolutionLevel( const SceneSettings_t * settings ) { return settings->eyeImageResolutionLevel; }
static int SceneSettings_GetEyeImageResolutionLevel( const SceneSettings_t * settings ) { return settings->eyeImageResolutionLevel; }
static int SceneSettings_GetEyeImageSamplesLevel( const SceneSettings_t * settings ) { return settings->eyeImageSamplesLevel; }

static int SceneSettings_GetDrawCallLevel( const SceneSettings_t * settings ) { return settings->drawCallLevel; }
static int SceneSettings_GetTriangleLevel( const SceneSettings_t * settings ) { return settings->triangleLevel; }
static int SceneSettings_GetFragmentLevel( const SceneSettings_t * settings ) { return settings->fragmentLevel; }

/*
================================================================================================================================

Performance scene rendering.

PerfScene_t

static void PerfScene_Create( GpuContext_t * context, PerfScene_t * scene, SceneSettings_t * settings, GpuRenderPass_t * renderPass );
static void PerfScene_Destroy( GpuContext_t * context, PerfScene_t * scene );
static void PerfScene_Simulate( PerfScene_t * scene, ViewState_t * viewState, const Microseconds_t time );
static void PerfScene_UpdateBuffers( GpuCommandBuffer_t * commandBuffer, PerfScene_t * scene, ViewState_t * viewState, const int eye );
static void PerfScene_Render( GpuCommandBuffer_t * commandBuffer, PerfScene_t * scene );

================================================================================================================================
*/

typedef struct
{
	// assets
	GpuGeometry_t			geometry[MAX_SCENE_TRIANGLE_LEVELS];
	GpuGraphicsProgram_t	program[MAX_SCENE_FRAGMENT_LEVELS];
	GpuGraphicsPipeline_t	pipelines[MAX_SCENE_TRIANGLE_LEVELS][MAX_SCENE_FRAGMENT_LEVELS];
	GpuBuffer_t				sceneMatrices;
	GpuTexture_t			diffuseTexture;
	GpuTexture_t			specularTexture;
	GpuTexture_t			normalTexture;
	SceneSettings_t			settings;
	SceneSettings_t *		newSettings;
	// simulation state
	float					bigRotationX;
	float					bigRotationY;
	float					smallRotationX;
	float					smallRotationY;
	Matrix4x4f_t *			modelMatrix;
} PerfScene_t;

enum
{
	PROGRAM_UNIFORM_MODEL_MATRIX,
	PROGRAM_UNIFORM_SCENE_MATRICES,
	PROGRAM_TEXTURE_0,
	PROGRAM_TEXTURE_1,
	PROGRAM_TEXTURE_2
};

static GpuProgramParm_t flatShadedProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_VERTEX,	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_UNIFORM_MODEL_MATRIX,		"ModelMatrix",		0 },
	{ GPU_PROGRAM_STAGE_VERTEX,	GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_UNIFORM_SCENE_MATRICES,		"SceneMatrices",	0 }
};

static const char flatShadedVertexProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform mat4 ModelMatrix;\n"
	"uniform SceneMatrices\n"
	"{\n"
	"	mat4 ViewMatrix;\n"
	"	mat4 ProjectionMatrix;\n"
	"} ub;\n"
	"in vec3 vertexPosition;\n"
	"in vec3 vertexNormal;\n"
	"out vec3 fragmentEyeDir;\n"
	"out vec3 fragmentNormal;\n"
	"out gl_PerVertex { vec4 gl_Position; };\n"
	"vec3 multiply3x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[1].x * v.y + m[2].x * v.z,\n"
	"		m[0].y * v.x + m[1].y * v.y + m[2].y * v.z,\n"
	"		m[0].z * v.x + m[1].z * v.y + m[2].z * v.z );\n"
	"}\n"
	"vec3 transposeMultiply3x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[0].y * v.y + m[0].z * v.z,\n"
	"		m[1].x * v.x + m[1].y * v.y + m[1].z * v.z,\n"
	"		m[2].x * v.x + m[2].y * v.y + m[2].z * v.z );\n"
	"}\n"
	"void main( void )\n"
	"{\n"
	"	vec4 vertexWorldPos = ModelMatrix * vec4( vertexPosition, 1.0 );\n"
	"	vec3 eyeWorldPos = transposeMultiply3x3( ub.ViewMatrix, -vec3( ub.ViewMatrix[3] ) );\n"
	"	gl_Position = ub.ProjectionMatrix * ( ub.ViewMatrix * vertexWorldPos );\n"
	"	fragmentEyeDir = eyeWorldPos - vec3( vertexWorldPos );\n"
	"	fragmentNormal = multiply3x3( ModelMatrix, vertexNormal );\n"
	"}\n";

static const char flatShadedMultiViewVertexProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"#define NUM_VIEWS 2\n"
	"#define VIEW_ID gl_ViewID_OVR\n"
	"#extension GL_OVR_multiview2 : require\n"
	"layout( num_views = NUM_VIEWS ) in;\n"
	"\n"
	"uniform mat4 ModelMatrix;\n"
	"uniform SceneMatrices\n"
	"{\n"
	"	mat4 ViewMatrix[NUM_VIEWS];\n"
	"	mat4 ProjectionMatrix[NUM_VIEWS];\n"
	"} ub;\n"
	"in vec3 vertexPosition;\n"
	"in vec3 vertexNormal;\n"
	"out vec3 fragmentEyeDir;\n"
	"out vec3 fragmentNormal;\n"
	"vec3 multiply3x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[1].x * v.y + m[2].x * v.z,\n"
	"		m[0].y * v.x + m[1].y * v.y + m[2].y * v.z,\n"
	"		m[0].z * v.x + m[1].z * v.y + m[2].z * v.z );\n"
	"}\n"
	"vec3 transposeMultiply3x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[0].y * v.y + m[0].z * v.z,\n"
	"		m[1].x * v.x + m[1].y * v.y + m[1].z * v.z,\n"
	"		m[2].x * v.x + m[2].y * v.y + m[2].z * v.z );\n"
	"}\n"
	"void main( void )\n"
	"{\n"
	"	vec4 vertexWorldPos = ModelMatrix * vec4( vertexPosition, 1.0 );\n"
	"	vec3 eyeWorldPos = transposeMultiply3x3( ub.ViewMatrix[VIEW_ID], -vec3( ub.ViewMatrix[VIEW_ID][3] ) );\n"
	"	gl_Position = ub.ProjectionMatrix[VIEW_ID] * ( ub.ViewMatrix[VIEW_ID] * vertexWorldPos );\n"
	"	fragmentEyeDir = eyeWorldPos - vec3( vertexWorldPos );\n"
	"	fragmentNormal = multiply3x3( ModelMatrix, vertexNormal );\n"
	"}\n";

static const char flatShadedFragmentProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"in lowp vec3 fragmentEyeDir;\n"
	"in lowp vec3 fragmentNormal;\n"
	"out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	lowp vec3 diffuseMap = vec3( 0.2, 0.2, 1.0 );\n"
	"	lowp vec3 specularMap = vec3( 0.5, 0.5, 0.5 );\n"
	"	lowp float specularPower = 10.0;\n"
	"	lowp vec3 eyeDir = normalize( fragmentEyeDir );\n"
	"	lowp vec3 normal = normalize( fragmentNormal );\n"
	"\n"
	"	lowp vec3 lightDir = normalize( vec3( -1.0, 1.0, 1.0 ) );\n"
	"	lowp vec3 lightReflection = normalize( 2.0 * dot( lightDir, normal ) * normal - lightDir );\n"
	"	lowp vec3 lightDiffuse = diffuseMap * ( max( dot( normal, lightDir ), 0.0 ) * 0.5 + 0.5 );\n"
	"	lowp vec3 lightSpecular = specularMap * pow( max( dot( lightReflection, eyeDir ), 0.0 ), specularPower );\n"
	"\n"
	"	outColor.xyz = lightDiffuse + lightSpecular;\n"
	"	outColor.w = 1.0;\n"
	"}\n";

static GpuProgramParm_t normalMappedProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_VERTEX,		GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_UNIFORM_MODEL_MATRIX,		"ModelMatrix",		0 },
	{ GPU_PROGRAM_STAGE_VERTEX,		GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_UNIFORM_SCENE_MATRICES,		"SceneMatrices",	0 },
	{ GPU_PROGRAM_STAGE_FRAGMENT,	GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_TEXTURE_0,					"Texture0",			0 },
	{ GPU_PROGRAM_STAGE_FRAGMENT,	GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_TEXTURE_1,					"Texture1",			1 },
	{ GPU_PROGRAM_STAGE_FRAGMENT,	GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED,					GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	PROGRAM_TEXTURE_2,					"Texture2",			2 }
};

static const char normalMappedVertexProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform mat4 ModelMatrix;\n"
	"uniform SceneMatrices\n"
	"{\n"
	"	mat4 ViewMatrix;\n"
	"	mat4 ProjectionMatrix;\n"
	"} ub;\n"
	"in vec3 vertexPosition;\n"
	"in vec3 vertexNormal;\n"
	"in vec3 vertexTangent;\n"
	"in vec3 vertexBinormal;\n"
	"in vec2 vertexUv0;\n"
	"out vec3 fragmentEyeDir;\n"
	"out vec3 fragmentNormal;\n"
	"out vec3 fragmentTangent;\n"
	"out vec3 fragmentBinormal;\n"
	"out vec2 fragmentUv0;\n"
	"out gl_PerVertex { vec4 gl_Position; };\n"
	"vec3 multiply3x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[1].x * v.y + m[2].x * v.z,\n"
	"		m[0].y * v.x + m[1].y * v.y + m[2].y * v.z,\n"
	"		m[0].z * v.x + m[1].z * v.y + m[2].z * v.z );\n"
	"}\n"
	"vec3 transposeMultiply3x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[0].y * v.y + m[0].z * v.z,\n"
	"		m[1].x * v.x + m[1].y * v.y + m[1].z * v.z,\n"
	"		m[2].x * v.x + m[2].y * v.y + m[2].z * v.z );\n"
	"}\n"
	"void main( void )\n"
	"{\n"
	"	vec4 vertexWorldPos = ModelMatrix * vec4( vertexPosition, 1.0 );\n"
	"	vec3 eyeWorldPos = transposeMultiply3x3( ub.ViewMatrix, -vec3( ub.ViewMatrix[3] ) );\n"
	"	gl_Position = ub.ProjectionMatrix * ( ub.ViewMatrix * vertexWorldPos );\n"
	"	fragmentEyeDir = eyeWorldPos - vec3( vertexWorldPos );\n"
	"	fragmentNormal = multiply3x3( ModelMatrix, vertexNormal );\n"
	"	fragmentTangent = multiply3x3( ModelMatrix, vertexTangent );\n"
	"	fragmentBinormal = multiply3x3( ModelMatrix, vertexBinormal );\n"
	"	fragmentUv0 = vertexUv0;\n"
	"}\n";

static const char normalMappedMultiViewVertexProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"#define NUM_VIEWS 2\n"
	"#define VIEW_ID gl_ViewID_OVR\n"
	"#extension GL_OVR_multiview2 : require\n"
	"layout( num_views = NUM_VIEWS ) in;\n"
	"\n"
	"uniform mat4 ModelMatrix;\n"
	"uniform SceneMatrices\n"
	"{\n"
	"	mat4 ViewMatrix[NUM_VIEWS];\n"
	"	mat4 ProjectionMatrix[NUM_VIEWS];\n"
	"} ub;\n"
	"in vec3 vertexPosition;\n"
	"in vec3 vertexNormal;\n"
	"in vec3 vertexTangent;\n"
	"in vec3 vertexBinormal;\n"
	"in vec2 vertexUv0;\n"
	"out vec3 fragmentEyeDir;\n"
	"out vec3 fragmentNormal;\n"
	"out vec3 fragmentTangent;\n"
	"out vec3 fragmentBinormal;\n"
	"out vec2 fragmentUv0;\n"
	"vec3 multiply3x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[1].x * v.y + m[2].x * v.z,\n"
	"		m[0].y * v.x + m[1].y * v.y + m[2].y * v.z,\n"
	"		m[0].z * v.x + m[1].z * v.y + m[2].z * v.z );\n"
	"}\n"
	"vec3 transposeMultiply3x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[0].y * v.y + m[0].z * v.z,\n"
	"		m[1].x * v.x + m[1].y * v.y + m[1].z * v.z,\n"
	"		m[2].x * v.x + m[2].y * v.y + m[2].z * v.z );\n"
	"}\n"
	"void main( void )\n"
	"{\n"
	"	vec4 vertexWorldPos = ModelMatrix * vec4( vertexPosition, 1.0 );\n"
	"	vec3 eyeWorldPos = transposeMultiply3x3( ub.ViewMatrix[VIEW_ID], -vec3( ub.ViewMatrix[VIEW_ID][3] ) );\n"
	"	gl_Position = ub.ProjectionMatrix[VIEW_ID] * ( ub.ViewMatrix[VIEW_ID] * vertexWorldPos );\n"
	"	fragmentEyeDir = eyeWorldPos - vec3( vertexWorldPos );\n"
	"	fragmentNormal = multiply3x3( ModelMatrix, vertexNormal );\n"
	"	fragmentTangent = multiply3x3( ModelMatrix, vertexTangent );\n"
	"	fragmentBinormal = multiply3x3( ModelMatrix, vertexBinormal );\n"
	"	fragmentUv0 = vertexUv0;\n"
	"}\n";

static const char normalMapped100LightsFragmentProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform sampler2D Texture0;\n"
	"uniform sampler2D Texture1;\n"
	"uniform sampler2D Texture2;\n"
	"in lowp vec3 fragmentEyeDir;\n"
	"in lowp vec3 fragmentNormal;\n"
	"in lowp vec3 fragmentTangent;\n"
	"in lowp vec3 fragmentBinormal;\n"
	"in lowp vec2 fragmentUv0;\n"
	"out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	lowp vec3 diffuseMap = texture( Texture0, fragmentUv0 ).xyz;\n"
	"	lowp vec3 specularMap = texture( Texture1, fragmentUv0 ).xyz * 2.0;\n"
	"	lowp vec3 normalMap = texture( Texture2, fragmentUv0 ).xyz * 2.0 - 1.0;\n"
	"	lowp float specularPower = 10.0;\n"
	"	lowp vec3 eyeDir = normalize( fragmentEyeDir );\n"
	"	lowp vec3 normal = normalize( normalMap.x * fragmentTangent + normalMap.y * fragmentBinormal + normalMap.z * fragmentNormal );\n"
	"\n"
	"	lowp vec3 color = vec3( 0 );\n"
	"	for ( int i = 0; i < 100; i++ )\n"
	"	{\n"
	"		lowp vec3 lightDir = normalize( vec3( -1.0, 1.0, 1.0 ) );\n"
	"		lowp vec3 lightReflection = normalize( 2.0 * dot( lightDir, normal ) * normal - lightDir );\n"
	"		lowp vec3 lightDiffuse = diffuseMap * ( max( dot( normal, lightDir ), 0.0 ) * 0.5 + 0.5 );\n"
	"		lowp vec3 lightSpecular = specularMap * pow( max( dot( lightReflection, eyeDir ), 0.0 ), specularPower );\n"
	"		color += ( lightDiffuse + lightSpecular ) * ( 1.0 / 100.0 );\n"
	"	}\n"
	"\n"
	"	outColor.xyz = color;\n"
	"	outColor.w = 1.0;\n"
	"}\n";

static const char normalMapped1000LightsFragmentProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform sampler2D Texture0;\n"
	"uniform sampler2D Texture1;\n"
	"uniform sampler2D Texture2;\n"
	"in lowp vec3 fragmentEyeDir;\n"
	"in lowp vec3 fragmentNormal;\n"
	"in lowp vec3 fragmentTangent;\n"
	"in lowp vec3 fragmentBinormal;\n"
	"in lowp vec2 fragmentUv0;\n"
	"out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	lowp vec3 diffuseMap = texture( Texture0, fragmentUv0 ).xyz;\n"
	"	lowp vec3 specularMap = texture( Texture1, fragmentUv0 ).xyz * 2.0;\n"
	"	lowp vec3 normalMap = texture( Texture2, fragmentUv0 ).xyz * 2.0 - 1.0;\n"
	"	lowp float specularPower = 10.0;\n"
	"	lowp vec3 eyeDir = normalize( fragmentEyeDir );\n"
	"	lowp vec3 normal = normalize( normalMap.x * fragmentTangent + normalMap.y * fragmentBinormal + normalMap.z * fragmentNormal );\n"
	"\n"
	"	lowp vec3 color = vec3( 0 );\n"
	"	for ( int i = 0; i < 1000; i++ )\n"
	"	{\n"
	"		lowp vec3 lightDir = normalize( vec3( -1.0, 1.0, 1.0 ) );\n"
	"		lowp vec3 lightReflection = normalize( 2.0 * dot( lightDir, normal ) * normal - lightDir );\n"
	"		lowp vec3 lightDiffuse = diffuseMap * ( max( dot( normal, lightDir ), 0.0 ) * 0.5 + 0.5 );\n"
	"		lowp vec3 lightSpecular = specularMap * pow( max( dot( lightReflection, eyeDir ), 0.0 ), specularPower );\n"
	"		color += ( lightDiffuse + lightSpecular ) * ( 1.0 / 1000.0 );\n"
	"	}\n"
	"\n"
	"	outColor.xyz = color;\n"
	"	outColor.w = 1.0;\n"
	"}\n";

static const char normalMapped2000LightsFragmentProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform sampler2D Texture0;\n"
	"uniform sampler2D Texture1;\n"
	"uniform sampler2D Texture2;\n"
	"in lowp vec3 fragmentEyeDir;\n"
	"in lowp vec3 fragmentNormal;\n"
	"in lowp vec3 fragmentTangent;\n"
	"in lowp vec3 fragmentBinormal;\n"
	"in lowp vec2 fragmentUv0;\n"
	"out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	lowp vec3 diffuseMap = texture( Texture0, fragmentUv0 ).xyz;\n"
	"	lowp vec3 specularMap = texture( Texture1, fragmentUv0 ).xyz * 2.0;\n"
	"	lowp vec3 normalMap = texture( Texture2, fragmentUv0 ).xyz * 2.0 - 1.0;\n"
	"	lowp float specularPower = 10.0;\n"
	"	lowp vec3 eyeDir = normalize( fragmentEyeDir );\n"
	"	lowp vec3 normal = normalize( normalMap.x * fragmentTangent + normalMap.y * fragmentBinormal + normalMap.z * fragmentNormal );\n"
	"\n"
	"	lowp vec3 color = vec3( 0 );\n"
	"	for ( int i = 0; i < 2000; i++ )\n"
	"	{\n"
	"		lowp vec3 lightDir = normalize( vec3( -1.0, 1.0, 1.0 ) );\n"
	"		lowp vec3 lightReflection = normalize( 2.0 * dot( lightDir, normal ) * normal - lightDir );\n"
	"		lowp vec3 lightDiffuse = diffuseMap * ( max( dot( normal, lightDir ), 0.0 ) * 0.5 + 0.5 );\n"
	"		lowp vec3 lightSpecular = specularMap * pow( max( dot( lightReflection, eyeDir ), 0.0 ), specularPower );\n"
	"		color += ( lightDiffuse + lightSpecular ) * ( 1.0 / 2000.0 );\n"
	"	}\n"
	"\n"
	"	outColor.xyz = color;\n"
	"	outColor.w = 1.0;\n"
	"}\n";

static void PerfScene_Create( GpuContext_t * context, PerfScene_t * scene, SceneSettings_t * settings, GpuRenderPass_t * renderPass )
{
	memset( scene, 0, sizeof( PerfScene_t ) );

	GpuGeometry_CreateCube( context, &scene->geometry[0], 0.0f, 0.5f );			// 12 triangles
	GpuGeometry_CreateTorus( context, &scene->geometry[1], 8, 0.0f, 1.0f );		// 128 triangles
	GpuGeometry_CreateTorus( context, &scene->geometry[2], 16, 0.0f, 1.0f );	// 512 triangles
	GpuGeometry_CreateTorus( context, &scene->geometry[3], 32, 0.0f, 1.0f );	// 2048 triangles

	GpuGraphicsProgram_Create( context, &scene->program[0],
								settings->useMultiView ? PROGRAM( flatShadedMultiViewVertexProgram ) : PROGRAM( flatShadedVertexProgram ),
								settings->useMultiView ? sizeof( PROGRAM( flatShadedMultiViewVertexProgram ) ) : sizeof( PROGRAM( flatShadedVertexProgram ) ),
								PROGRAM( flatShadedFragmentProgram ),
								sizeof( PROGRAM( flatShadedFragmentProgram ) ),
								flatShadedProgramParms, ARRAY_SIZE( flatShadedProgramParms ),
								scene->geometry[0].layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_NORMAL );
	GpuGraphicsProgram_Create( context, &scene->program[1],
								settings->useMultiView ? PROGRAM( normalMappedMultiViewVertexProgram ) : PROGRAM( normalMappedVertexProgram ),
								settings->useMultiView ? sizeof( PROGRAM( normalMappedMultiViewVertexProgram ) ) : sizeof( PROGRAM( normalMappedVertexProgram ) ),
								PROGRAM( normalMapped100LightsFragmentProgram ),
								sizeof( PROGRAM( normalMapped100LightsFragmentProgram ) ),
								normalMappedProgramParms, ARRAY_SIZE( normalMappedProgramParms ),
								scene->geometry[0].layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_NORMAL |
								VERTEX_ATTRIBUTE_FLAG_TANGENT | VERTEX_ATTRIBUTE_FLAG_BINORMAL |
								VERTEX_ATTRIBUTE_FLAG_UV0 );
	GpuGraphicsProgram_Create( context, &scene->program[2],
								settings->useMultiView ? PROGRAM( normalMappedMultiViewVertexProgram ) : PROGRAM( normalMappedVertexProgram ),
								settings->useMultiView ? sizeof( PROGRAM( normalMappedMultiViewVertexProgram ) ) : sizeof( PROGRAM( normalMappedVertexProgram ) ),
								PROGRAM( normalMapped1000LightsFragmentProgram ),
								sizeof( PROGRAM( normalMapped1000LightsFragmentProgram ) ),
								normalMappedProgramParms, ARRAY_SIZE( normalMappedProgramParms ),
								scene->geometry[0].layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_NORMAL |
								VERTEX_ATTRIBUTE_FLAG_TANGENT | VERTEX_ATTRIBUTE_FLAG_BINORMAL |
								VERTEX_ATTRIBUTE_FLAG_UV0 );
	GpuGraphicsProgram_Create( context, &scene->program[3],
								settings->useMultiView ? PROGRAM( normalMappedMultiViewVertexProgram ) : PROGRAM( normalMappedVertexProgram ),
								settings->useMultiView ? sizeof( PROGRAM( normalMappedMultiViewVertexProgram ) ) : sizeof( PROGRAM( normalMappedVertexProgram ) ),
								PROGRAM( normalMapped2000LightsFragmentProgram ),
								sizeof( PROGRAM( normalMapped2000LightsFragmentProgram ) ),
								normalMappedProgramParms, ARRAY_SIZE( normalMappedProgramParms ),
								scene->geometry[0].layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_NORMAL |
								VERTEX_ATTRIBUTE_FLAG_TANGENT | VERTEX_ATTRIBUTE_FLAG_BINORMAL |
								VERTEX_ATTRIBUTE_FLAG_UV0 );

	for ( int i = 0; i < MAX_SCENE_TRIANGLE_LEVELS; i++ )
	{
		for ( int j = 0; j < MAX_SCENE_FRAGMENT_LEVELS; j++ )
		{
			GpuGraphicsPipelineParms_t pipelineParms;
			GpuGraphicsPipelineParms_Init( &pipelineParms );

			pipelineParms.renderPass = renderPass;
			pipelineParms.program = &scene->program[j];
			pipelineParms.geometry = &scene->geometry[i];

			GpuGraphicsPipeline_Create( context, &scene->pipelines[i][j], &pipelineParms );
		}
	}

	GpuBuffer_Create( context, &scene->sceneMatrices, GPU_BUFFER_TYPE_UNIFORM, ( settings->useMultiView ? 4 : 2 ) * sizeof( Matrix4x4f_t ), NULL, false );

	GpuTexture_CreateDefault( context, &scene->diffuseTexture, GPU_TEXTURE_DEFAULT_CHECKERBOARD, 256, 256, 0, 0, 1, true, false );
	GpuTexture_CreateDefault( context, &scene->specularTexture, GPU_TEXTURE_DEFAULT_CHECKERBOARD, 256, 256, 0, 0, 1, true, false );
	GpuTexture_CreateDefault( context, &scene->normalTexture, GPU_TEXTURE_DEFAULT_PYRAMIDS, 256, 256, 0, 0, 1, true, false );

	scene->settings = *settings;
	scene->newSettings = settings;

	const int maxDimension = 2 * ( 1 << ( MAX_SCENE_DRAWCALL_LEVELS - 1 ) );

	scene->bigRotationX = 0.0f;
	scene->bigRotationY = 0.0f;
	scene->smallRotationX = 0.0f;
	scene->smallRotationY = 0.0f;

	scene->modelMatrix = (Matrix4x4f_t *) AllocAlignedMemory( maxDimension * maxDimension * maxDimension * sizeof( Matrix4x4f_t ), sizeof( Matrix4x4f_t ) );
}

static void PerfScene_Destroy( GpuContext_t * context, PerfScene_t * scene )
{
	GpuContext_WaitIdle( context );

	for ( int i = 0; i < MAX_SCENE_TRIANGLE_LEVELS; i++ )
	{
		for ( int j = 0; j < MAX_SCENE_FRAGMENT_LEVELS; j++ )
		{
			GpuGraphicsPipeline_Destroy( context, &scene->pipelines[i][j] );
		}
	}

	for ( int i = 0; i < MAX_SCENE_TRIANGLE_LEVELS; i++ )
	{
		GpuGeometry_Destroy( context, &scene->geometry[i] );
	}

	for ( int i = 0; i < MAX_SCENE_FRAGMENT_LEVELS; i++ )
	{
		GpuGraphicsProgram_Destroy( context, &scene->program[i] );
	}

	GpuBuffer_Destroy( context, &scene->sceneMatrices );

	GpuTexture_Destroy( context, &scene->diffuseTexture );
	GpuTexture_Destroy( context, &scene->specularTexture );
	GpuTexture_Destroy( context, &scene->normalTexture );

	FreeAlignedMemory( scene->modelMatrix );
	scene->modelMatrix = NULL;
}

static void PerfScene_Simulate( PerfScene_t * scene, ViewState_t * viewState, const Microseconds_t time )
{
	// Must recreate the scene if multi-view is enabled/disabled.
	assert( scene->settings.useMultiView == scene->newSettings->useMultiView );
	scene->settings = *scene->newSettings;

	ViewState_HandleHmd( viewState, time );

	if ( !scene->settings.simulationPaused )
	{
		const float offset = time * ( MATH_PI / 1000.0f / 1000.0f );
		scene->bigRotationX = 20.0f * offset;
		scene->bigRotationY = 10.0f * offset;
		scene->smallRotationX = -60.0f * offset;
		scene->smallRotationY = -40.0f * offset;
	}
}

static void PerfScene_UpdateBuffers( GpuCommandBuffer_t * commandBuffer, PerfScene_t * scene, ViewState_t * viewState, const int eye )
{
	void * sceneMatrices = NULL;
	GpuBuffer_t * sceneMatricesBuffer = GpuCommandBuffer_MapBuffer( commandBuffer, &scene->sceneMatrices, &sceneMatrices );
	const int numMatrices = scene->settings.useMultiView ? 2 : 1;
	memcpy( (char *)sceneMatrices + 0 * numMatrices * sizeof( Matrix4x4f_t ), &viewState->viewMatrix[eye], numMatrices * sizeof( Matrix4x4f_t ) );
	memcpy( (char *)sceneMatrices + 1 * numMatrices * sizeof( Matrix4x4f_t ), &viewState->projectionMatrix[eye], numMatrices * sizeof( Matrix4x4f_t ) );
	GpuCommandBuffer_UnmapBuffer( commandBuffer, &scene->sceneMatrices, sceneMatricesBuffer, GPU_BUFFER_UNMAP_TYPE_COPY_BACK );
}

static void PerfScene_Render( GpuCommandBuffer_t * commandBuffer, PerfScene_t * scene )
{
	const int dimension = 2 * ( 1 << scene->settings.drawCallLevel );
	const float cubeOffset = ( dimension - 1.0f ) * 0.5f;
	const float cubeScale = 2.0f;

	Matrix4x4f_t bigRotationMatrix;
	Matrix4x4f_CreateRotation( &bigRotationMatrix, scene->bigRotationX, scene->bigRotationY, 0.0f );

	Matrix4x4f_t bigTranslationMatrix;
	Matrix4x4f_CreateTranslation( &bigTranslationMatrix, 0.0f, 0.0f, - 2.5f * dimension );

	Matrix4x4f_t bigTransformMatrix;
	Matrix4x4f_Multiply( &bigTransformMatrix, &bigTranslationMatrix, &bigRotationMatrix );

	Matrix4x4f_t smallRotationMatrix;
	Matrix4x4f_CreateRotation( &smallRotationMatrix, scene->smallRotationX, scene->smallRotationY, 0.0f );

	GpuGraphicsCommand_t command;
	GpuGraphicsCommand_Init( &command );
	GpuGraphicsCommand_SetPipeline( &command, &scene->pipelines[scene->settings.triangleLevel][scene->settings.fragmentLevel] );
	GpuGraphicsCommand_SetParmBufferUniform( &command, PROGRAM_UNIFORM_SCENE_MATRICES, &scene->sceneMatrices );
	GpuGraphicsCommand_SetParmTextureSampled( &command, PROGRAM_TEXTURE_0, ( scene->settings.fragmentLevel >= 1 ) ? &scene->diffuseTexture : NULL );
	GpuGraphicsCommand_SetParmTextureSampled( &command, PROGRAM_TEXTURE_1, ( scene->settings.fragmentLevel >= 1 ) ? &scene->specularTexture : NULL );
	GpuGraphicsCommand_SetParmTextureSampled( &command, PROGRAM_TEXTURE_2, ( scene->settings.fragmentLevel >= 1 ) ? &scene->normalTexture : NULL );

	for ( int x = 0; x < dimension; x++ )
	{
		for ( int y = 0; y < dimension; y++ )
		{
			for ( int z = 0; z < dimension; z++ )
			{
				Matrix4x4f_t smallTranslationMatrix;
				Matrix4x4f_CreateTranslation( &smallTranslationMatrix, cubeScale * ( x - cubeOffset ), cubeScale * ( y - cubeOffset ), cubeScale * ( z - cubeOffset ) );

				Matrix4x4f_t smallTransformMatrix;
				Matrix4x4f_Multiply( &smallTransformMatrix, &smallTranslationMatrix, &smallRotationMatrix );

				Matrix4x4f_t * modelMatrix = &scene->modelMatrix[( x * dimension + y ) * dimension + z];
				Matrix4x4f_Multiply( modelMatrix, &bigTransformMatrix, &smallTransformMatrix );

				GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, PROGRAM_UNIFORM_MODEL_MATRIX, modelMatrix );

				GpuCommandBuffer_SubmitGraphicsCommand( commandBuffer, &command );
			}
		}
	}
}

#if USE_GLTF == 1

/*
================================================================================================================================

glTF scene rendering.

GltfScene_t

static bool GltfScene_CreateFromFile( GpuContext_t * context, GltfScene_t * scene, const char * fileName, GpuRenderPass_t * renderPass );
static void GltfScene_Destroy( GpuContext_t * context, GltfScene_t * scene );
static void GltfScene_Simulate( GltfScene_t * scene, ViewState_t * viewState, GpuWindowInput_t * input, const Microseconds_t time );
static void GltfScene_UpdateBuffers( GpuCommandBuffer_t * commandBuffer, const GltfScene_t * scene, const ViewState_t * viewState, const int eye );
static void GltfScene_Render( GpuCommandBuffer_t * commandBuffer, const GltfScene_t * scene, const ViewState_t * viewState, const int eye );

================================================================================================================================
*/

#include <utils/json.h>
#include <utils/base64.h>

static GpuProgramParm_t unitCubeFlatShadeProgramParms[] =
{
	{ GPU_PROGRAM_STAGE_VERTEX,	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	0,	"ModelMatrix",		0 },
	{ GPU_PROGRAM_STAGE_VERTEX,	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	1,	"ViewMatrix",		0 },
	{ GPU_PROGRAM_STAGE_VERTEX,	GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4,	GPU_PROGRAM_PARM_ACCESS_READ_ONLY,	2,	"ProjectionMatrix",	0 }
};

static const char unitCubeFlatShadeVertexProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"uniform mat4 ModelMatrix;\n"
	"uniform mat4 ViewMatrix;\n"
	"uniform mat4 ProjectionMatrix;\n"
	"in vec3 vertexPosition;\n"
	"in vec3 vertexNormal;\n"
	"out vec3 fragmentEyeDir;\n"
	"out vec3 fragmentNormal;\n"
	"out gl_PerVertex { vec4 gl_Position; };\n"
	"vec3 multiply3x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[1].x * v.y + m[2].x * v.z,\n"
	"		m[0].y * v.x + m[1].y * v.y + m[2].y * v.z,\n"
	"		m[0].z * v.x + m[1].z * v.y + m[2].z * v.z );\n"
	"}\n"
	"vec3 transposeMultiply3x3( mat4 m, vec3 v )\n"
	"{\n"
	"	return vec3(\n"
	"		m[0].x * v.x + m[0].y * v.y + m[0].z * v.z,\n"
	"		m[1].x * v.x + m[1].y * v.y + m[1].z * v.z,\n"
	"		m[2].x * v.x + m[2].y * v.y + m[2].z * v.z );\n"
	"}\n"
	"void main( void )\n"
	"{\n"
	"	vec4 vertexWorldPos = ModelMatrix * vec4( vertexPosition, 1.0 );\n"
	"	vec3 eyeWorldPos = transposeMultiply3x3( ViewMatrix, -vec3( ViewMatrix[3] ) );\n"
	"	gl_Position = ProjectionMatrix * ( ViewMatrix * vertexWorldPos );\n"
	"	fragmentEyeDir = eyeWorldPos - vec3( vertexWorldPos );\n"
	"	fragmentNormal = multiply3x3( ModelMatrix, vertexNormal );\n"
	"}\n";

static const char unitCubeFlatShadeFragmentProgramGLSL[] =
	"#version " GLSL_PROGRAM_VERSION "\n"
	GLSL_EXTENSIONS
	"in lowp vec3 fragmentEyeDir;\n"
	"in lowp vec3 fragmentNormal;\n"
	"out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	lowp vec3 diffuseMap = vec3( 0.2, 0.2, 1.0 );\n"
	"	lowp vec3 specularMap = vec3( 0.5, 0.5, 0.5 );\n"
	"	lowp float specularPower = 10.0;\n"
	"	lowp vec3 eyeDir = normalize( fragmentEyeDir );\n"
	"	lowp vec3 normal = normalize( fragmentNormal );\n"
	"\n"
	"	lowp vec3 lightDir = normalize( vec3( -1.0, 1.0, 1.0 ) );\n"
	"	lowp vec3 lightReflection = normalize( 2.0 * dot( lightDir, normal ) * normal - lightDir );\n"
	"	lowp vec3 lightDiffuse = diffuseMap * ( max( dot( normal, lightDir ), 0.0 ) * 0.5 + 0.5 );\n"
	"	lowp vec3 lightSpecular = specularMap * pow( max( dot( lightReflection, eyeDir ), 0.0 ), specularPower );\n"
	"\n"
	"	outColor.xyz = lightDiffuse + lightSpecular;\n"
	"	outColor.w = 1.0;\n"
	"}\n";

typedef struct GltfBuffer_t
{
	char *						name;
	char *						type;
	size_t						byteLength;
	unsigned char *				bufferData;
} GltfBuffer_t;

typedef struct GltfBufferView_t
{
	char *						name;
	const GltfBuffer_t *		buffer;
	size_t						byteOffset;
	size_t						byteLength;
	int							target;
} GltfBufferView_t;

typedef struct GltfAccessor_t
{
	char *						name;
	char *						type;
	const GltfBufferView_t *	bufferView;
	size_t						byteOffset;
	size_t						byteStride;
	int							componentType;
	int							count;
	int							intMin[16];
	int							intMax[16];
	float						floatMin[16];
	float						floatMax[16];
} GltfAccessor_t;

typedef struct GltfImage_t
{
	char *						name;
	char *						uri;
} GltfImage_t;

typedef struct GltfSampler_t
{
	char *						name;
	int							magFilter;	// default GL_LINEAR
	int							minFilter;	// default GL_NEAREST_MIPMAP_LINEAR
	int							wrapS;		// default GL_REPEAT
	int							wrapT;		// default GL_REPEAT
} GltfSampler_t;

typedef struct GltfTexture_t
{
	char *						name;
	const GltfImage_t *			image;
	const GltfSampler_t *		sampler;
	GpuTexture_t				texture;
} GltfTexture_t;

typedef struct GltfShader_t
{
	char *						name;
	char *						uriGlslOpenGL;
	char *						uriGlslVulkan;
	char *						uriSpirvOpenGL;
	char *						uriSpirvVulkan;
	int							type;
} GltfShader_t;

typedef struct GltfProgram_t
{
	char *						name;
	unsigned char *				vertexSource;
	unsigned char *				fragmentSource;
	int							vertexSourceSize;
	int							fragmentSourceSize;
} GltfProgram_t;

typedef enum
{
	GLTF_UNIFORM_SEMANTIC_NONE,
	GLTF_UNIFORM_SEMANTIC_DEFAULT_VALUE,
	GLTF_UNIFORM_SEMANTIC_LOCAL,
	GLTF_UNIFORM_SEMANTIC_VIEW,
	GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE,
	GLTF_UNIFORM_SEMANTIC_PROJECTION,
	GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE,
	GLTF_UNIFORM_SEMANTIC_MODEL,
	GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE,
	GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE_TRANSPOSE,
	GLTF_UNIFORM_SEMANTIC_MODEL_VIEW,
	GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE,
	GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE_TRANSPOSE,
	GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION,
	GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION_INVERSE,
	GLTF_UNIFORM_SEMANTIC_VIEWPORT,
	GLTF_UNIFORM_SEMANTIC_JOINTMATRIX
} GltfUniformSemantic_t;

static struct
{
	const char *			name;
	GltfUniformSemantic_t	semantic;
}
gltfUniformSemanticNames[] =
{
	{ "",								GLTF_UNIFORM_SEMANTIC_NONE },
	{ "",								GLTF_UNIFORM_SEMANTIC_DEFAULT_VALUE },
	{ "LOCAL",							GLTF_UNIFORM_SEMANTIC_LOCAL },
	{ "VIEW",							GLTF_UNIFORM_SEMANTIC_VIEW },
	{ "VIEWINVERSE",					GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE },
	{ "PROJECTION",						GLTF_UNIFORM_SEMANTIC_PROJECTION },
	{ "PROJECTIONINVERSE",				GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE },
	{ "MODEL",							GLTF_UNIFORM_SEMANTIC_MODEL },
	{ "MODELINVERSE",					GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE },
	{ "MODELINVERSETRANSPOSE",			GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE_TRANSPOSE },
	{ "MODELVIEW",						GLTF_UNIFORM_SEMANTIC_MODEL_VIEW },
	{ "MODELVIEWINVERSE",				GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE },
	{ "MODELVIEWINVERSETRANSPOSE",		GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE_TRANSPOSE },
	{ "MODELVIEWPROJECTION",			GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION },
	{ "MODELVIEWPROJECTIONINVERSE",		GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION_INVERSE },
	{ "VIEWPORT",						GLTF_UNIFORM_SEMANTIC_VIEWPORT },
	{ "JOINTMATRIX",					GLTF_UNIFORM_SEMANTIC_JOINTMATRIX },
	{ NULL,								0 }
};

typedef struct GltfUniformValue_t
{
	GltfTexture_t *				texture;
	int							intValue[16];
	float						floatValue[16];
} GltfUniformValue_t;

typedef struct GltfUniform_t
{
	char *						name;
	GltfUniformSemantic_t		semantic;
	GpuProgramParmType_t		type;
	int							index;
	GltfUniformValue_t			defaultValue;
} GltfUniform_t;

typedef struct GltfTechnique_t
{
	char *						name;
	GpuGraphicsProgram_t		program;
	GpuProgramParm_t *			parms;
	GltfUniform_t *				uniforms;
	int							uniformCount;
	GpuRasterOperations_t		rop;
} GltfTechnique_t;

typedef struct GltfMaterialValue_t
{
	GltfUniform_t *				uniform;
	GltfUniformValue_t			value;
} GltfMaterialValue_t;

typedef struct GltfMaterial_t
{
	char *						name;
	const GltfTechnique_t *		technique;
	GltfMaterialValue_t *		values;
	int							valueCount;
} GltfMaterial_t;

typedef struct GltfSurface_t
{
	const GltfMaterial_t *		material;		// material used to render this surface
	GpuGeometry_t				geometry;		// surface geometry
	GpuGraphicsPipeline_t		pipeline;		// rendering pipeline for this surface
	Vector3f_t					mins;			// minimums of the surface geometry excluding animations
	Vector3f_t					maxs;			// maximums of the surface geometry excluding animations
} GltfSurface_t;

typedef struct GltfModel_t
{
	char *						name;
	GltfSurface_t *				surfaces;
	int							surfaceCount;
} GltfModel_t;

typedef struct GltfAnimationChannel_t
{
	char *						nodeName;
	struct GltfNode_t *			node;
	Quatf_t *					rotation;
	Vector3f_t *				translation;
	Vector3f_t *				scale;
} GltfAnimationChannel_t;

typedef struct GltfAnimation_t
{
	char *						name;
	float *						sampleTimes;
	int							sampleCount;
	GltfAnimationChannel_t *	channels;
	int							channelCount;
} GltfAnimation_t;

typedef struct GltfJoint_t
{
	char *						name;
	struct GltfNode_t *			node;
} GltfJoint_t;

typedef struct GltfSkin_t
{
	char *						name;
	struct GltfNode_t *			parent;
	Matrix4x4f_t				bindShapeMatrix;
	Matrix4x4f_t *				inverseBindMatrices;
	Vector3f_t *				jointGeometryMins;		// joint local space minimums of the geometry influenced by each joint
	Vector3f_t *				jointGeometryMaxs;		// joint local space maximums of the geometry influenced by each joint
	GltfJoint_t *				joints;					// joints of this skin
	int							jointCount;				// number of joints
	GpuBuffer_t					jointBuffer;			// buffer with joint matrices
	Vector3f_t					mins;					// minimums of the complete skin geometry (modified at run-time)
	Vector3f_t					maxs;					// maximums of the complete skin geometry (modified at run-time)
	bool						culled;					// true if the skin is culled (modified at run-time)
} GltfSkin_t;

typedef enum
{
	GLTF_CAMERA_TYPE_PERSPECTIVE,
	GLTF_CAMERA_TYPE_ORTHOGRAPHIC
} GltfCameraType_t;

typedef struct GltfCamera_t
{
	char *						name;
	GltfCameraType_t			type;
	struct
	{
		float					aspectRatio;
		float					fovDegreesX;
		float					fovDegreesY;
		float					nearZ;
		float					farZ;
	}							perspective;
	struct
	{
		float					magX;
		float					magY;
		float					nearZ;
		float					farZ;
	}							orthographic;
} GltfCamera_t;

typedef struct GltfNode_t
{
	char *						name;
	char *						jointName;
	Quatf_t						rotation;			// (modified at run-time)
	Vector3f_t					translation;		// (modified at run-time)
	Vector3f_t					scale;				// (modified at run-time)
	Matrix4x4f_t				localTransform;		// (modified at run-time)
	Matrix4x4f_t				globalTransform;	// (modified at run-time)
	int							subTreeNodeCount;	// this node plus the number of direct or indirect decendants
	struct GltfNode_t **		children;
	char **						childNames;
	int							childCount;
	struct GltfNode_t *			parent;
	struct GltfCamera_t *		camera;
	struct GltfSkin_t *			skin;
	struct GltfModel_t **		models;
	int							modelCount;
} GltfNode_t;

typedef struct GltfSubTree_t
{
	GltfNode_t *				nodes;				// points into GltfScene_t::nodes
	int							nodeCount;
} GltfSubTree_t;

typedef struct GltfSubScene_t
{
	char *						name;
	GltfSubTree_t *				subTrees;
	int							subTreeCount;
} GltfSubScene_t;

typedef struct GltfScene_t
{
	GltfBuffer_t *				buffers;
	int *						bufferNameHash;
	int							bufferCount;
	GltfBufferView_t *			bufferViews;
	int *						bufferViewNameHash;
	int							bufferViewCount;
	GltfAccessor_t *			accessors;
	int *						accessorNameHash;
	int							accessorCount;
	GltfImage_t *				images;
	int *						imageNameHash;
	int							imageCount;
	GltfSampler_t *				samplers;
	int *						samplerNameHash;
	int							samplerCount;
	GltfTexture_t *				textures;
	int *						textureNameHash;
	int							textureCount;
	GltfShader_t *				shaders;
	int *						shaderNameHash;
	int							shaderCount;
	GltfProgram_t *				programs;
	int *						programNameHash;
	int							programCount;
	GltfTechnique_t *			techniques;
	int *						techniqueNameHash;
	int							techniqueCount;
	GltfMaterial_t *			materials;
	int *						materialNameHash;
	int							materialCount;
	GltfSkin_t *				skins;
	int *						skinNameHash;
	int							skinCount;
	GltfModel_t *				models;
	int *						modelNameHash;
	int							modelCount;
	GltfAnimation_t *			animations;
	int *						animationNameHash;
	int							animationCount;
	GltfCamera_t *				cameras;
	int *						cameraNameHash;
	int							cameraCount;
	GltfNode_t *				nodes;
	int *						nodeNameHash;
	int *						nodeJointNameHash;
	int							nodeCount;
	GltfNode_t **				rootNodes;
	int							rootNodeCount;
	GltfSubScene_t *			subScenes;
	int *						subSceneNameHash;
	int							subSceneCount;
	GltfSubScene_t *			currentSubScene;

	GpuBuffer_t					defaultJointBuffer;
	GpuGeometry_t				unitCubeGeometry;
	GpuGraphicsProgram_t		unitCubeFlatShadeProgram;
	GpuGraphicsPipeline_t		unitCubePipeline;
} GltfScene_t;

#define HASH_TABLE_SIZE		256

static unsigned int StringHash( const char * string )
{
	unsigned int hash = 5381;
	for ( int i = 0; string[i] != '\0'; i++ )
	{
		hash = ( ( hash << 5 ) - hash ) + string[i];
	}
	return ( hash & ( HASH_TABLE_SIZE - 1 ) );
}

#define GLTF_HASH( type, typeCapitalized, name, nameCapitalized ) \
	static void Gltf_Create##typeCapitalized##nameCapitalized##Hash( GltfScene_t * scene ) \
	{ \
		scene->type##nameCapitalized##Hash = (int *) malloc( ( HASH_TABLE_SIZE + scene->type##Count ) * sizeof( scene->type##nameCapitalized##Hash[0] ) ); \
		memset( scene->type##nameCapitalized##Hash, -1, ( HASH_TABLE_SIZE + scene->type##Count ) * sizeof( scene->type##nameCapitalized##Hash[0] ) ); \
		for ( int i = 0; i < scene->type##Count; i++ ) \
		{ \
			const unsigned int hash = StringHash( scene->type##s[i].name ); \
			scene->type##nameCapitalized##Hash[HASH_TABLE_SIZE + i] = scene->type##nameCapitalized##Hash[hash]; \
			scene->type##nameCapitalized##Hash[hash] = i; \
		} \
	} \
	\
	static Gltf##typeCapitalized##_t * Gltf_Get##typeCapitalized##By##nameCapitalized( const GltfScene_t * scene, const char * name ) \
	{ \
		const unsigned int hash = StringHash( name ); \
		for ( int i = scene->type##nameCapitalized##Hash[hash]; i >= 0; i = scene->type##nameCapitalized##Hash[HASH_TABLE_SIZE + i] ) \
		{ \
			if ( strcmp( scene->type##s[i].name, name ) == 0 ) \
			{ \
				return &scene->type##s[i]; \
			} \
		} \
		return NULL; \
	}

GLTF_HASH( buffer,		Buffer,		name,		Name );
GLTF_HASH( bufferView,	BufferView,	name,		Name );
GLTF_HASH( accessor,	Accessor,	name,		Name );
GLTF_HASH( image,		Image,		name,		Name );
GLTF_HASH( sampler,		Sampler,	name,		Name );
GLTF_HASH( texture,		Texture,	name,		Name );
GLTF_HASH( shader,		Shader,		name,		Name );
GLTF_HASH( program,		Program,	name,		Name );
GLTF_HASH( technique,	Technique,	name,		Name );
GLTF_HASH( material,	Material,	name,		Name );
GLTF_HASH( skin,		Skin,		name,		Name );
GLTF_HASH( model,		Model,		name,		Name );
GLTF_HASH( animation,	Animation,	name,		Name );
GLTF_HASH( camera,		Camera,		name,		Name );
GLTF_HASH( node,		Node,		name,		Name );
GLTF_HASH( node,		Node,		jointName,	JointName );
GLTF_HASH( subScene,	SubScene,	name,		Name );

static GltfAccessor_t * Gltf_GetAccessorByNameAndType( const GltfScene_t * scene, const char * name, const char * type, const int componentType )
{
	GltfAccessor_t * accessor = Gltf_GetAccessorByName( scene, name );
	if ( accessor != NULL &&
			accessor->componentType == componentType &&
				strcmp( accessor->type, type ) == 0 )
	{
		return accessor;
	}
	return NULL;
}

static char * Gltf_strdup( const char * str )
{
	char * out = (char *)malloc( strlen( str ) + 1 );
	strcpy( out, str );
	return out;
}

static unsigned char * Gltf_ReadFile( const char * fileName, int * outSizeInBytes )
{
	if ( outSizeInBytes != NULL )
	{
		*outSizeInBytes = 0;
	}
	FILE * file = fopen( fileName, "rb" );
	if ( file == NULL )
	{
		return NULL;
	}
	fseek( file, 0L, SEEK_END );
	size_t bufferSize = ftell( file );
	fseek( file, 0L, SEEK_SET );
	unsigned char * buffer = (unsigned char *) malloc( bufferSize + 1 );
	if ( fread( buffer, 1, bufferSize, file ) != bufferSize )
	{
		fclose( file );
		free( buffer );
		return NULL;
	}
	buffer[bufferSize] = '\0';
	fclose( file );
	if ( outSizeInBytes != NULL )
	{
		*outSizeInBytes = (int)bufferSize;
	}
	return buffer;
}

static unsigned char * Gltf_ReadBase64( const char * base64, int * outSizeInBytes )
{
	const int base64SizeInBytes = (int)strlen( base64 );
	const int dataSizeInBytes = Base64_DecodeSizeInBytes( base64, base64SizeInBytes );
	unsigned char * buffer = (unsigned char *)malloc( dataSizeInBytes );
	Base64_Decode( buffer, base64, base64SizeInBytes );
	if ( outSizeInBytes != NULL )
	{
		*outSizeInBytes = dataSizeInBytes;
	}
	return buffer;
}

static unsigned char * Gltf_ReadUri( const char * uri, int * outSizeInBytes )
{
	if ( strncmp( uri, "data:", 5 ) == 0 )
	{
		// plain text
		if ( strncmp( uri, "data:text/plain,", 16 ) == 0 )
		{
			return (unsigned char *)Gltf_strdup( uri + 16 );
		}
		// base64 text "shader"
		else if ( strncmp( uri, "data:text/plain;base64,", 23 ) == 0 )
		{
			return Gltf_ReadBase64( uri + 23, outSizeInBytes );
		}
		// base64 binary "buffer"
		else if ( strncmp( uri, "data:application/octet-stream;base64,", 37 ) == 0 )
		{
			return Gltf_ReadBase64( uri + 37, outSizeInBytes );
		}
		// base64 KTX "image"
		else if ( strncmp( uri, "data:image/ktx;base64,", 22 ) == 0 )
		{
			return Gltf_ReadBase64( uri + 22, outSizeInBytes );
		}
	}
	return Gltf_ReadFile( uri, outSizeInBytes );
}

static void Gltf_ParseIntArray( int * elements, const int count, const Json_t * arrayNode )
{
	int i = 0;
	for ( ; i < Json_GetMemberCount( arrayNode ) && i < count; i++ )
	{
		elements[i] = Json_GetInt32( Json_GetMemberByIndex( arrayNode, i ), 0 );
	}
	for ( ; i < count; i++ )
	{
		elements[i] = 0;
	}
}

static void Gltf_ParseFloatArray( float * elements, const int count, const Json_t * arrayNode )
{
	int i = 0;
	for ( ; i < Json_GetMemberCount( arrayNode ) && i < count; i++ )
	{
		elements[i] = Json_GetFloat( Json_GetMemberByIndex( arrayNode, i ), 0.0f );
	}
	for ( ; i < count; i++ )
	{
		elements[i] = 0.0f;
	}
}

static void Gltf_ParseUniformValue( GltfUniformValue_t * value, const Json_t * json, const GpuProgramParmType_t type, const GltfScene_t * scene )
{
	switch ( type )
	{
		case GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED:					value->texture = Gltf_GetTextureByName( scene, Json_GetString( json, "" ) ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT:				value->intValue[0] = Json_GetInt32( json, 0 ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2:		Gltf_ParseIntArray( value->intValue, 16, json ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3:		Gltf_ParseIntArray( value->intValue, 16, json ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4:		Gltf_ParseIntArray( value->intValue, 16, json ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT:				value->floatValue[0] = Json_GetFloat( json, 0.0f ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2:		Gltf_ParseFloatArray( value->floatValue, 2, json ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3:		Gltf_ParseFloatArray( value->floatValue, 3, json ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4:		Gltf_ParseFloatArray( value->floatValue, 4, json ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X2:	Gltf_ParseFloatArray( value->floatValue, 2*2, json ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X3:	Gltf_ParseFloatArray( value->floatValue, 2*3, json ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X4:	Gltf_ParseFloatArray( value->floatValue, 2*4, json ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X2:	Gltf_ParseFloatArray( value->floatValue, 3*2, json ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X3:	Gltf_ParseFloatArray( value->floatValue, 3*3, json ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4:	Gltf_ParseFloatArray( value->floatValue, 3*4, json ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X2:	Gltf_ParseFloatArray( value->floatValue, 4*2, json ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X3:	Gltf_ParseFloatArray( value->floatValue, 4*3, json ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4:	Gltf_ParseFloatArray( value->floatValue, 4*4, json ); break;
		default: break;
	}
}

static GpuTextureFilter_t Gltf_GetTextureFilter( const int filter )
{
	switch ( filter )
	{
		case GL_NEAREST:					return GPU_TEXTURE_FILTER_NEAREST;
		case GL_LINEAR:						return GPU_TEXTURE_FILTER_LINEAR;
		case GL_NEAREST_MIPMAP_NEAREST:		return GPU_TEXTURE_FILTER_NEAREST;
		case GL_LINEAR_MIPMAP_NEAREST:		return GPU_TEXTURE_FILTER_NEAREST;
		case GL_NEAREST_MIPMAP_LINEAR:		return GPU_TEXTURE_FILTER_BILINEAR;
		case GL_LINEAR_MIPMAP_LINEAR:		return GPU_TEXTURE_FILTER_BILINEAR;
		default:							return GPU_TEXTURE_FILTER_BILINEAR;
	}
}

static GpuTextureWrapMode_t Gltf_GetTextureWrapMode( const int wrapMode )
{
	switch ( wrapMode )
	{
		case GL_REPEAT:				return GPU_TEXTURE_WRAP_MODE_REPEAT;
		case GL_CLAMP_TO_EDGE:		return GPU_TEXTURE_WRAP_MODE_CLAMP_TO_EDGE;
		case GL_CLAMP_TO_BORDER:	return GPU_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER;
		default:					return GPU_TEXTURE_WRAP_MODE_REPEAT;
	}
}

static GpuFrontFace_t Gltf_GetFrontFace( const int face )
{
	switch ( face )
	{
		case GL_CCW:	return GPU_FRONT_FACE_COUNTER_CLOCKWISE;
		case GL_CW:		return GPU_FRONT_FACE_CLOCKWISE;
		default:		return GPU_FRONT_FACE_COUNTER_CLOCKWISE;
	}
}

static GpuCullMode_t Gltf_GetCullMode( const int mode )
{
	switch ( mode )
	{
		case GL_NONE:	return GPU_CULL_MODE_NONE;
		case GL_FRONT:	return GPU_CULL_MODE_FRONT;
		case GL_BACK:	return GPU_CULL_MODE_BACK;
		default:		return GPU_CULL_MODE_BACK;
	}
}

static GpuCompareOp_t Gltf_GetCompareOp( const int op )
{
	switch ( op )
	{
		case GL_NEVER:		return GPU_COMPARE_OP_NEVER;
		case GL_LESS:		return GPU_COMPARE_OP_LESS;
		case GL_EQUAL:		return GPU_COMPARE_OP_EQUAL;
		case GL_LEQUAL:		return GPU_COMPARE_OP_LESS_OR_EQUAL;
		case GL_GREATER:	return GPU_COMPARE_OP_GREATER;
		case GL_NOTEQUAL:	return GPU_COMPARE_OP_NOT_EQUAL;
		case GL_GEQUAL:		return GPU_COMPARE_OP_GREATER_OR_EQUAL;
		case GL_ALWAYS:		return GPU_COMPARE_OP_ALWAYS;
		default:			return GPU_COMPARE_OP_LESS;
	}
}

static GpuBlendOp_t Gltf_GetBlendOp( const int op )
{
	switch( op )
	{
		case GL_FUNC_ADD:				return GPU_BLEND_OP_ADD;
		case GL_FUNC_SUBTRACT:			return GPU_BLEND_OP_SUBTRACT;
		case GL_FUNC_REVERSE_SUBTRACT:	return GPU_BLEND_OP_REVERSE_SUBTRACT;
		case GL_MIN:					return GPU_BLEND_OP_MIN;
		case GL_MAX:					return GPU_BLEND_OP_MAX;
		default:						return GPU_BLEND_OP_ADD;
	}
}

static GpuBlendFactor_t Gltf_GetBlendFactor( const int factor )
{
	switch ( factor )
	{
		case GL_ZERO:						return GPU_BLEND_FACTOR_ZERO;
		case GL_ONE:						return GPU_BLEND_FACTOR_ONE;
		case GL_SRC_COLOR:					return GPU_BLEND_FACTOR_SRC_COLOR;
		case GL_ONE_MINUS_SRC_COLOR:		return GPU_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case GL_DST_COLOR:					return GPU_BLEND_FACTOR_DST_COLOR;
		case GL_ONE_MINUS_DST_COLOR:		return GPU_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case GL_SRC_ALPHA:					return GPU_BLEND_FACTOR_SRC_ALPHA;
		case GL_ONE_MINUS_SRC_ALPHA:		return GPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case GL_DST_ALPHA:					return GPU_BLEND_FACTOR_DST_ALPHA;
		case GL_ONE_MINUS_DST_ALPHA:		return GPU_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case GL_CONSTANT_COLOR:				return GPU_BLEND_FACTOR_CONSTANT_COLOR;
		case GL_ONE_MINUS_CONSTANT_COLOR:	return GPU_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		case GL_CONSTANT_ALPHA:				return GPU_BLEND_FACTOR_CONSTANT_ALPHA;
		case GL_ONE_MINUS_CONSTANT_ALPHA:	return GPU_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
		case GL_SRC_ALPHA_SATURATE:			return GPU_BLEND_FACTOR_SRC_ALPHA_SATURAT;
		default:							return GPU_BLEND_FACTOR_ZERO;
	}
}

static void * Gltf_GetBufferData( const GltfAccessor_t * access )
{
	if ( access != NULL )
	{
		return access->bufferView->buffer->bufferData + access->bufferView->byteOffset + access->byteOffset;
	}
	return NULL;
}

// Sort the nodes such that parents come before their children and every sub-tree is a contiguous sequence of nodes.
// Note that the node graph must be acyclic and no node may be a direct or indirect descendant of more than one node.
static void Gltf_SortNodes( GltfNode_t * nodes, const int nodeCount )
{
	GltfNode_t * nodeStack = (GltfNode_t *) malloc( nodeCount * sizeof( GltfNode_t ) );
	int stackSize = 0;
	int stackOffset = 0;
	for ( int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++ )
	{
		bool foundParent = false;
		for ( int nodeSearchIndex = 0; nodeSearchIndex < nodeCount; nodeSearchIndex++ )
		{
			for ( int childIndex = 0; childIndex < nodes[nodeSearchIndex].childCount; childIndex++ )
			{
				if ( strcmp( nodes[nodeSearchIndex].childNames[childIndex], nodes[nodeIndex].name ) == 0 )
				{
					foundParent = true;
					break;
				}
			}
		}
		if ( !foundParent )
		{
			const int subTreeStartOffset = stackSize;
			nodeStack[stackSize++] = nodes[nodeIndex];
			while( stackOffset < stackSize )
			{
				const GltfNode_t * node = &nodeStack[stackOffset++];
				for ( int childIndex = 0; childIndex < node->childCount; childIndex++ )
				{
					for ( int nodeSearchIndex = 0; nodeSearchIndex < nodeCount; nodeSearchIndex++ )
					{
						if ( strcmp( node->childNames[childIndex], nodes[nodeSearchIndex].name ) == 0 )
						{
							assert( stackSize < nodeCount );
							nodeStack[stackSize++] = nodes[nodeSearchIndex];
							break;
						}
					}
				}
			}
			for ( int updateNodeIndex = subTreeStartOffset; updateNodeIndex < stackSize; updateNodeIndex++ )
			{
				nodeStack[updateNodeIndex].subTreeNodeCount = stackSize - updateNodeIndex;
			}
		}
	}
	assert( stackSize == nodeCount );
	memcpy( nodes, nodeStack, nodeCount );
	free( nodeStack );
}

static bool GltfScene_CreateFromFile( GpuContext_t * context, GltfScene_t * scene, const char * fileName, GpuRenderPass_t * renderPass )
{
	const Microseconds_t t0 = GetTimeMicroseconds();

	memset( scene, 0, sizeof( GltfScene_t ) );

	// Based on a GL_MAX_UNIFORM_BLOCK_SIZE of 16384 on the ARM Mali.
	const int MAX_JOINTS = 16384 / sizeof( Matrix4x4f_t );

	Json_t * rootNode = Json_Create();
	if ( Json_ReadFromFile( rootNode, fileName, NULL ) )
	{
		const Json_t * asset = Json_GetMemberByName( rootNode, "asset" );
		const char * version = Json_GetString( Json_GetMemberByName( asset, "version" ), "1.0" );
		if ( strcmp( version, "1.0" ) != 0 )
		{
			Json_Destroy( rootNode );
			return false;
		}

		//
		// glTF buffers
		//
		{
			const Microseconds_t startTime = GetTimeMicroseconds();

			const Json_t * buffers = Json_GetMemberByName( rootNode, "buffers" );
			scene->bufferCount = Json_GetMemberCount( buffers );
			scene->buffers = (GltfBuffer_t *) calloc( scene->bufferCount, sizeof( GltfBuffer_t ) );
			for ( int bufferIndex = 0; bufferIndex < scene->bufferCount; bufferIndex++ )
			{
				const Json_t * buffer = Json_GetMemberByIndex( buffers, bufferIndex );
				scene->buffers[bufferIndex].name = Gltf_strdup( Json_GetMemberName( buffer ) );
				scene->buffers[bufferIndex].byteLength = Json_GetUint64( Json_GetMemberByName( buffer, "byteLength" ), 0 );
				scene->buffers[bufferIndex].type = Gltf_strdup( Json_GetString( Json_GetMemberByName( buffer, "type" ), "" ) );
				scene->buffers[bufferIndex].bufferData = Gltf_ReadUri( Json_GetString( Json_GetMemberByName( buffer, "uri" ), "" ), NULL );
				assert( scene->buffers[bufferIndex].name[0] != '\0' );
				assert( scene->buffers[bufferIndex].byteLength != 0 );
				assert( scene->buffers[bufferIndex].bufferData != NULL );
			}
			Gltf_CreateBufferNameHash( scene );

			const Microseconds_t endTime = GetTimeMicroseconds();
			Print( "%1.3f seconds to load buffers\n", ( endTime - startTime ) * 1e-6f );
		}

		//
		// glTF bufferViews
		//
		{
			const Microseconds_t startTime = GetTimeMicroseconds();

			const Json_t * bufferViews = Json_GetMemberByName( rootNode, "bufferViews" );
			scene->bufferViewCount = Json_GetMemberCount( bufferViews );
			scene->bufferViews = (GltfBufferView_t *) calloc( scene->bufferViewCount, sizeof( GltfBufferView_t ) );
			for ( int bufferViewIndex = 0; bufferViewIndex < scene->bufferViewCount; bufferViewIndex++ )
			{
				const Json_t * view = Json_GetMemberByIndex( bufferViews, bufferViewIndex );
				scene->bufferViews[bufferViewIndex].name = Gltf_strdup( Json_GetMemberName( view ) );
				scene->bufferViews[bufferViewIndex].buffer = Gltf_GetBufferByName( scene, Json_GetString( Json_GetMemberByName( view, "buffer" ), "" ) );
				scene->bufferViews[bufferViewIndex].byteOffset = (size_t) Json_GetUint64( Json_GetMemberByName( view, "byteOffset" ), 0 );
				scene->bufferViews[bufferViewIndex].byteLength = (size_t) Json_GetUint64( Json_GetMemberByName( view, "byteLength" ), 0 );
				scene->bufferViews[bufferViewIndex].target = Json_GetUint16( Json_GetMemberByName( view, "target" ), 0 );
				assert( scene->bufferViews[bufferViewIndex].name[0] != '\0' );
				assert( scene->bufferViews[bufferViewIndex].buffer != NULL );
				assert( scene->bufferViews[bufferViewIndex].byteLength != 0 );
				assert( scene->bufferViews[bufferViewIndex].byteOffset + scene->bufferViews[bufferViewIndex].byteLength <= scene->bufferViews[bufferViewIndex].buffer->byteLength );
			}
			Gltf_CreateBufferViewNameHash( scene );

			const Microseconds_t endTime = GetTimeMicroseconds();
			Print( "%1.3f seconds to load buffer views\n", ( endTime - startTime ) * 1e-6f );
		}

		//
		// glTF accessors
		//
		{
			const Microseconds_t startTime = GetTimeMicroseconds();

			const Json_t * accessors = Json_GetMemberByName( rootNode, "accessors" );
			scene->accessorCount = Json_GetMemberCount( accessors );
			scene->accessors = (GltfAccessor_t *) calloc( scene->accessorCount, sizeof( GltfAccessor_t ) );
			for ( int accessorIndex = 0; accessorIndex < scene->accessorCount; accessorIndex++ )
			{
				const Json_t * access = Json_GetMemberByIndex( accessors, accessorIndex );
				scene->accessors[accessorIndex].name = Gltf_strdup( Json_GetMemberName( access ) );
				scene->accessors[accessorIndex].bufferView = Gltf_GetBufferViewByName( scene, Json_GetString( Json_GetMemberByName( access, "bufferView" ), "" ) );
				scene->accessors[accessorIndex].byteOffset = (size_t) Json_GetUint64( Json_GetMemberByName( access, "byteOffset" ), 0 );
				scene->accessors[accessorIndex].byteStride = (size_t) Json_GetUint64( Json_GetMemberByName( access, "byteStride" ), 0 );
				scene->accessors[accessorIndex].componentType = Json_GetUint16( Json_GetMemberByName( access, "componentType" ), 0 );
				scene->accessors[accessorIndex].count = Json_GetInt32( Json_GetMemberByName( access, "count" ), 0 );
				scene->accessors[accessorIndex].type =  Gltf_strdup( Json_GetString( Json_GetMemberByName( access, "type" ), "" ) );
				const Json_t * min = Json_GetMemberByName( access, "min" );
				const Json_t * max = Json_GetMemberByName( access, "max" );
				if ( min != NULL && max != NULL )
				{
					int componentCount = 0;
					if ( strcmp( scene->accessors[accessorIndex].type, "SCALAR" ) == 0 ) { componentCount = 1; }
					else if ( strcmp( scene->accessors[accessorIndex].type, "VEC2" ) == 0 ) { componentCount = 2; }
					else if ( strcmp( scene->accessors[accessorIndex].type, "VEC3" ) == 0 ) { componentCount = 3; }
					else if ( strcmp( scene->accessors[accessorIndex].type, "VEC4" ) == 0 ) { componentCount = 4; }
					else if ( strcmp( scene->accessors[accessorIndex].type, "MAT2" ) == 0 ) { componentCount = 4; }
					else if ( strcmp( scene->accessors[accessorIndex].type, "MAT3" ) == 0 ) { componentCount = 9; }
					else if ( strcmp( scene->accessors[accessorIndex].type, "MAT4" ) == 0 ) { componentCount = 16; }

					switch ( scene->accessors[accessorIndex].componentType )
					{
						case GL_BYTE:
						case GL_UNSIGNED_BYTE:
						case GL_SHORT:
						case GL_UNSIGNED_SHORT:
							Gltf_ParseIntArray( scene->accessors[accessorIndex].intMin, componentCount, min );
							Gltf_ParseIntArray( scene->accessors[accessorIndex].intMax, componentCount, max );
							break;
						case GL_FLOAT:
							Gltf_ParseFloatArray( scene->accessors[accessorIndex].floatMin, componentCount, min );
							Gltf_ParseFloatArray( scene->accessors[accessorIndex].floatMax, componentCount, max );
							break;
					}
				}
				assert( scene->accessors[accessorIndex].name[0] != '\0' );
				assert( scene->accessors[accessorIndex].bufferView != NULL );
				assert( scene->accessors[accessorIndex].byteStride != 0 );
				assert( scene->accessors[accessorIndex].componentType != 0 );
				assert( scene->accessors[accessorIndex].count != 0 );
				assert( scene->accessors[accessorIndex].type[0] != '\0' );
				assert( scene->accessors[accessorIndex].byteOffset + scene->accessors[accessorIndex].count * scene->accessors[accessorIndex].byteStride <= scene->accessors[accessorIndex].bufferView->byteLength );
			}
			Gltf_CreateAccessorNameHash( scene );

			const Microseconds_t endTime = GetTimeMicroseconds();
			Print( "%1.3f seconds to load accessors\n", ( endTime - startTime ) * 1e-6f );
		}

		//
		// glTF images
		//
		{
			const Microseconds_t startTime = GetTimeMicroseconds();

			const Json_t * images = Json_GetMemberByName( rootNode, "images" );
			scene->imageCount = Json_GetMemberCount( images );
			scene->images = (GltfImage_t *) calloc( scene->imageCount, sizeof( GltfImage_t ) );
			for ( int imageIndex = 0; imageIndex < scene->imageCount; imageIndex++ )
			{
				const Json_t * image = Json_GetMemberByIndex( images, imageIndex );
				scene->images[imageIndex].name = Gltf_strdup( Json_GetMemberName( image ) );
				scene->images[imageIndex].uri = Gltf_strdup( Json_GetString( Json_GetMemberByName( image, "uri" ), "" ) );
				assert( scene->images[imageIndex].name[0] != '\0' );
				assert( scene->images[imageIndex].uri[0] != '\0' );
			}
			Gltf_CreateImageNameHash( scene );

			const Microseconds_t endTime = GetTimeMicroseconds();
			Print( "%1.3f seconds to load images\n", ( endTime - startTime ) * 1e-6f );
		}

		//
		// glTF samplers
		//
		{
			const Microseconds_t startTime = GetTimeMicroseconds();

			const Json_t * samplers = Json_GetMemberByName( rootNode, "samplers" );
			scene->samplerCount = Json_GetMemberCount( samplers );
			scene->samplers = (GltfSampler_t *) calloc( scene->samplerCount, sizeof( GltfSampler_t ) );
			for ( int samplerIndex = 0; samplerIndex < scene->samplerCount; samplerIndex++ )
			{
				const Json_t * sampler = Json_GetMemberByIndex( samplers, samplerIndex );
				scene->samplers[samplerIndex].name = Gltf_strdup( Json_GetMemberName( sampler ) );
				scene->samplers[samplerIndex].magFilter = Json_GetUint16( Json_GetMemberByName( sampler, "magFilter" ), GL_LINEAR );
				scene->samplers[samplerIndex].minFilter = Json_GetUint16( Json_GetMemberByName( sampler, "minFilter" ), GL_NEAREST_MIPMAP_LINEAR );
				scene->samplers[samplerIndex].wrapS = Json_GetUint16( Json_GetMemberByName( sampler, "wrapS" ), GL_REPEAT );
				scene->samplers[samplerIndex].wrapT = Json_GetUint16( Json_GetMemberByName( sampler, "wrapT" ), GL_REPEAT );
				assert( scene->samplers[samplerIndex].name[0] != '\0' );
			}
			Gltf_CreateSamplerNameHash( scene );

			const Microseconds_t endTime = GetTimeMicroseconds();
			Print( "%1.3f seconds to load samplers\n", ( endTime - startTime ) * 1e-6f );
		}

		//
		// glTF textures
		//
		{
			const Microseconds_t startTime = GetTimeMicroseconds();

			const Json_t * textures = Json_GetMemberByName( rootNode, "textures" );
			scene->textureCount = Json_GetMemberCount( textures );
			scene->textures = (GltfTexture_t *) calloc( scene->textureCount, sizeof( GltfTexture_t ) );
			for ( int textureIndex = 0; textureIndex < scene->textureCount; textureIndex++ )
			{
				const Json_t * texture = Json_GetMemberByIndex( textures, textureIndex );
				scene->textures[textureIndex].name = Gltf_strdup( Json_GetMemberName( texture ) );
				scene->textures[textureIndex].image = Gltf_GetImageByName( scene, Json_GetString( Json_GetMemberByName( texture, "source" ), "" ) );
				scene->textures[textureIndex].sampler = Gltf_GetSamplerByName( scene, Json_GetString( Json_GetMemberByName( texture, "sampler" ), "" ) );

				assert( scene->textures[textureIndex].name[0] != '\0' );
				assert( scene->textures[textureIndex].image != NULL );
				//assert( scene->textures[textureIndex].sampler != NULL );

				// The "format", "internalFormat", "target" and "type" are automatically derived from the KTX file.
				int dataSizeInBytes = 0;
				unsigned char * data = Gltf_ReadUri( scene->textures[textureIndex].image->uri, &dataSizeInBytes );
				GpuTexture_CreateFromKTX( context, &scene->textures[textureIndex].texture, scene->textures[textureIndex].name, data, dataSizeInBytes );
				free( data );
			}
			Gltf_CreateTextureNameHash( scene );

			const Microseconds_t endTime = GetTimeMicroseconds();
			Print( "%1.3f seconds to load textures\n", ( endTime - startTime ) * 1e-6f );
		}

		//
		// glTF shaders
		//
		{
			const Microseconds_t startTime = GetTimeMicroseconds();

			const Json_t * shaders = Json_GetMemberByName( rootNode, "shaders" );
			scene->shaderCount = Json_GetMemberCount( shaders );
			scene->shaders = (GltfShader_t *) calloc( scene->shaderCount, sizeof( GltfShader_t ) );
			for ( int shaderIndex = 0; shaderIndex < scene->shaderCount; shaderIndex++ )
			{
				const Json_t * shader = Json_GetMemberByIndex( shaders, shaderIndex );
				scene->shaders[shaderIndex].name = Gltf_strdup( Json_GetMemberName( shader ) );
				scene->shaders[shaderIndex].uriGlslOpenGL = Gltf_strdup( Json_GetString( Json_GetMemberByName( shader, "uri" ), "" ) );
				scene->shaders[shaderIndex].uriGlslVulkan = Gltf_strdup( Json_GetString( Json_GetMemberByName( shader, "uriGlslVulkan" ), "" ) );
				scene->shaders[shaderIndex].uriSpirvOpenGL = Gltf_strdup( Json_GetString( Json_GetMemberByName( shader, "uriSpirvOpenGL" ), "" ) );
				scene->shaders[shaderIndex].uriSpirvVulkan = Gltf_strdup( Json_GetString( Json_GetMemberByName( shader, "uriSpirvVulkan" ), "" ) );
				scene->shaders[shaderIndex].type = Json_GetUint16( Json_GetMemberByName( shader, "type" ), 0 );
				assert( scene->shaders[shaderIndex].name[0] != '\0' );
				assert( scene->shaders[shaderIndex].uriGlslOpenGL[0] != '\0' );
				assert( scene->shaders[shaderIndex].uriGlslVulkan != '\0' );
				assert( scene->shaders[shaderIndex].uriSpirvOpenGL != '\0' );
				assert( scene->shaders[shaderIndex].uriSpirvVulkan != '\0' );
				assert( scene->shaders[shaderIndex].type != 0 );
			}
			Gltf_CreateShaderNameHash( scene );

			const Microseconds_t endTime = GetTimeMicroseconds();
			Print( "%1.3f seconds to load shaders\n", ( endTime - startTime ) * 1e-6f );
		}

		//
		// glTF programs
		//
		{
			const Microseconds_t startTime = GetTimeMicroseconds();

			const Json_t * programs = Json_GetMemberByName( rootNode, "programs" );
			scene->programCount = Json_GetMemberCount( programs );
			scene->programs = (GltfProgram_t *) calloc( scene->programCount, sizeof( GltfProgram_t ) );
			for ( int programIndex = 0; programIndex < scene->programCount; programIndex++ )
			{
				const Json_t * program = Json_GetMemberByIndex( programs, programIndex );
				const char * vertexShaderName = Json_GetString( Json_GetMemberByName( program, "vertexShader" ), "" );
				const char * fragmentShaderName = Json_GetString( Json_GetMemberByName( program, "fragmentShader" ), "" );
				const GltfShader_t * vertexShader = Gltf_GetShaderByName( scene, vertexShaderName );
				const GltfShader_t * fragmentShader = Gltf_GetShaderByName( scene, fragmentShaderName );

				assert( vertexShader != NULL );
				assert( fragmentShader != NULL );

				scene->programs[programIndex].name = Gltf_strdup( Json_GetMemberName( program ) );
				scene->programs[programIndex].vertexSource = Gltf_ReadUri( vertexShader->uriGlslOpenGL, &scene->programs[programIndex].vertexSourceSize );
				scene->programs[programIndex].fragmentSource = Gltf_ReadUri( fragmentShader->uriGlslOpenGL, &scene->programs[programIndex].fragmentSourceSize );
				assert( scene->programs[programIndex].name[0] != '\0' );
				assert( scene->programs[programIndex].vertexSource[0] != '\0' );
				assert( scene->programs[programIndex].fragmentSource[0] != '\0' );
			}
			Gltf_CreateProgramNameHash( scene );

			const Microseconds_t endTime = GetTimeMicroseconds();
			Print( "%1.3f seconds to load programs\n", ( endTime - startTime ) * 1e-6f );
		}

		//
		// glTF techniques
		//
		{
			const Microseconds_t startTime = GetTimeMicroseconds();

			const Json_t * techniques = Json_GetMemberByName( rootNode, "techniques" );
			scene->techniqueCount = Json_GetMemberCount( techniques );
			scene->techniques = (GltfTechnique_t *) calloc( scene->techniqueCount, sizeof( GltfTechnique_t ) );
			for ( int techniqueIndex = 0; techniqueIndex < scene->techniqueCount; techniqueIndex++ )
			{
				const Json_t * technique = Json_GetMemberByIndex( techniques, techniqueIndex );
				scene->techniques[techniqueIndex].name = Gltf_strdup( Json_GetMemberName( technique ) );
				const GltfProgram_t * program = Gltf_GetProgramByName( scene, Json_GetString( Json_GetMemberByName( technique, "program" ), "" ) );

				assert( scene->techniques[techniqueIndex].name[0] != '\0' );
				assert( program != NULL );

				int vertexAttribsFlags = 0;
				const Json_t * attributes = Json_GetMemberByName( technique, "attributes" );
				const int attributeCount = Json_GetMemberCount( attributes );
				for ( int j = 0; j < attributeCount; j++ )
				{
					const char * attrib = Json_GetString( Json_GetMemberByIndex( attributes, j ), "" );
					if ( strcmp( attrib, "POSITION" ) == 0 )		{ vertexAttribsFlags |= VERTEX_ATTRIBUTE_FLAG_POSITION; }
					else if ( strcmp( attrib, "NORMAL" ) == 0 )		{ vertexAttribsFlags |= VERTEX_ATTRIBUTE_FLAG_NORMAL; }
					else if ( strcmp( attrib, "TANGENT" ) == 0 )	{ vertexAttribsFlags |= VERTEX_ATTRIBUTE_FLAG_TANGENT; }
					else if ( strcmp( attrib, "BINORMAL" ) == 0 )	{ vertexAttribsFlags |= VERTEX_ATTRIBUTE_FLAG_BINORMAL; }
					else if ( strcmp( attrib, "COLOR" ) == 0 )		{ vertexAttribsFlags |= VERTEX_ATTRIBUTE_FLAG_COLOR; }
					else if ( strcmp( attrib, "TEXCOORD_0" ) == 0 )	{ vertexAttribsFlags |= VERTEX_ATTRIBUTE_FLAG_UV0; }
					else if ( strcmp( attrib, "TEXCOORD_1" ) == 0 )	{ vertexAttribsFlags |= VERTEX_ATTRIBUTE_FLAG_UV1; }
					else if ( strcmp( attrib, "TEXCOORD_2" ) == 0 )	{ vertexAttribsFlags |= VERTEX_ATTRIBUTE_FLAG_UV2; }
					else if ( strcmp( attrib, "JOINT" ) == 0 )		{ vertexAttribsFlags |= VERTEX_ATTRIBUTE_FLAG_JOINT_INDICES; }
					else if ( strcmp( attrib, "WEIGHT" ) == 0 )		{ vertexAttribsFlags |= VERTEX_ATTRIBUTE_FLAG_JOINT_WEIGHTS; }
				}

				// Must have at least positions.
				assert( ( vertexAttribsFlags & VERTEX_ATTRIBUTE_FLAG_POSITION ) != 0 );

				const Json_t * uniforms = Json_GetMemberByName( technique, "uniforms" );
				const Json_t * parameters = Json_GetMemberByName( technique, "parameters" );
				const int uniformCount = Json_GetMemberCount( uniforms );
				scene->techniques[techniqueIndex].parms = (GpuProgramParm_t *) calloc( uniformCount, sizeof( GpuProgramParm_t ) );
				scene->techniques[techniqueIndex].uniformCount = uniformCount;
				scene->techniques[techniqueIndex].uniforms = (GltfUniform_t *) calloc( uniformCount, sizeof( GltfUniform_t ) );
				memset( scene->techniques[techniqueIndex].uniforms, 0, uniformCount * sizeof( GltfUniform_t ) );
				for ( int j = 0; j < uniformCount; j++ )
				{
					const Json_t * uniform = Json_GetMemberByIndex( uniforms, j );
					const char * uniformName = Json_GetMemberName( uniform );
					const char * parmName = Json_GetString( uniform, "" );
					const Json_t * parameter = Json_GetMemberByName( parameters, parmName );
					const char * semantic = Json_GetString( Json_GetMemberByName( parameter, "semantic" ), "" );
					const int stage = Json_GetUint32( Json_GetMemberByName( parameter, "stage" ), 0 );
					const int type = Json_GetUint16( Json_GetMemberByName( parameter, "type" ), 0 );
					const int binding = Json_GetUint32( Json_GetMemberByName( parameter, "bindingOpenGL" ), 0 );

					GpuProgramParmType_t parmType = GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED;
					switch ( type )
					{
						case GL_SAMPLER_2D:		parmType = GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED; break;
						case GL_SAMPLER_3D:		parmType = GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED; break;
						case GL_SAMPLER_CUBE:	parmType = GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED; break;
						case GL_INT:			parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT; break;
						case GL_INT_VEC2:		parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2; break;
						case GL_INT_VEC3:		parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3; break;
						case GL_INT_VEC4:		parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4; break;
						case GL_FLOAT:			parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT; break;
						case GL_FLOAT_VEC2:		parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2; break;
						case GL_FLOAT_VEC3:		parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3; break;
						case GL_FLOAT_VEC4:		parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4; break;
						case GL_FLOAT_MAT2:		parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X2; break;
						case GL_FLOAT_MAT2x3:	parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X3; break;
						case GL_FLOAT_MAT2x4:	parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X4; break;
						case GL_FLOAT_MAT3x2:	parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X2; break;
						case GL_FLOAT_MAT3:		parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X3; break;
						case GL_FLOAT_MAT3x4:	parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4; break;
						case GL_FLOAT_MAT4x2:	parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X2; break;
						case GL_FLOAT_MAT4x3:	parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X3; break;
						case GL_FLOAT_MAT4:		parmType = GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4; break;
					}

					if ( strcmp( semantic, "JOINTMATRIX" ) == 0 )
					{
						parmType = GPU_PROGRAM_PARM_TYPE_BUFFER_UNIFORM;
					}

					scene->techniques[techniqueIndex].parms[j].stage = ( stage == GL_VERTEX_SHADER ) ? GPU_PROGRAM_STAGE_VERTEX : GPU_PROGRAM_STAGE_FRAGMENT;
					scene->techniques[techniqueIndex].parms[j].type = parmType;
					scene->techniques[techniqueIndex].parms[j].access = GPU_PROGRAM_PARM_ACCESS_READ_ONLY;	// assume all parms are read-only
					scene->techniques[techniqueIndex].parms[j].index = j;
					scene->techniques[techniqueIndex].parms[j].name = Gltf_strdup( uniformName );
					scene->techniques[techniqueIndex].parms[j].binding = binding;

					scene->techniques[techniqueIndex].uniforms[j].name = Gltf_strdup( parmName );
					scene->techniques[techniqueIndex].uniforms[j].semantic = GLTF_UNIFORM_SEMANTIC_NONE;		// default to the material setting the uniform
					scene->techniques[techniqueIndex].uniforms[j].type = parmType;
					scene->techniques[techniqueIndex].uniforms[j].index = j;
					for ( int s = 0; s < sizeof( gltfUniformSemanticNames ) / sizeof( gltfUniformSemanticNames[0] ); s++ )
					{
						if ( strcmp( gltfUniformSemanticNames[s].name, semantic ) == 0 )
						{
							scene->techniques[techniqueIndex].uniforms[j].semantic = gltfUniformSemanticNames[s].semantic;
							break;
						}
					}
					const Json_t * value = Json_GetMemberByName( parameter, "value" );
					if ( value != NULL )
					{
						GltfUniform_t * techniqueUniform = &scene->techniques[techniqueIndex].uniforms[j];
						techniqueUniform->semantic = GLTF_UNIFORM_SEMANTIC_DEFAULT_VALUE;
						Gltf_ParseUniformValue( &techniqueUniform->defaultValue, value, techniqueUniform->type, scene );
					}
				}

				scene->techniques[techniqueIndex].rop.blendEnable = false;
				scene->techniques[techniqueIndex].rop.redWriteEnable = true;
				scene->techniques[techniqueIndex].rop.blueWriteEnable = true;
				scene->techniques[techniqueIndex].rop.greenWriteEnable = true;
				scene->techniques[techniqueIndex].rop.alphaWriteEnable = false;
				scene->techniques[techniqueIndex].rop.depthTestEnable = false;
				scene->techniques[techniqueIndex].rop.depthWriteEnable = false;
				scene->techniques[techniqueIndex].rop.frontFace = GPU_FRONT_FACE_COUNTER_CLOCKWISE;
				scene->techniques[techniqueIndex].rop.cullMode = GPU_CULL_MODE_NONE;
				scene->techniques[techniqueIndex].rop.depthCompare = GPU_COMPARE_OP_LESS_OR_EQUAL;
				scene->techniques[techniqueIndex].rop.blendColor.x = 0.0f;
				scene->techniques[techniqueIndex].rop.blendColor.y = 0.0f;
				scene->techniques[techniqueIndex].rop.blendColor.z = 0.0f;
				scene->techniques[techniqueIndex].rop.blendColor.w = 0.0f;
				scene->techniques[techniqueIndex].rop.blendOpColor = GPU_BLEND_OP_ADD;
				scene->techniques[techniqueIndex].rop.blendSrcColor = GPU_BLEND_FACTOR_ONE;
				scene->techniques[techniqueIndex].rop.blendDstColor = GPU_BLEND_FACTOR_ZERO;
				scene->techniques[techniqueIndex].rop.blendOpAlpha = GPU_BLEND_OP_ADD;
				scene->techniques[techniqueIndex].rop.blendSrcAlpha = GPU_BLEND_FACTOR_ONE;
				scene->techniques[techniqueIndex].rop.blendDstAlpha = GPU_BLEND_FACTOR_ZERO;

				const Json_t * states = Json_GetMemberByName( technique, "states" );
				const Json_t * enable = Json_GetMemberByName( states, "enable" );
				const int enableCount = Json_GetMemberCount( enable );
				for ( int enableIndex = 0; enableIndex < enableCount; enableIndex++ )
				{
					const int enableState = Json_GetUint16( Json_GetMemberByIndex( enable, enableIndex ), 0 );
					switch ( enableState )
					{
						case GL_BLEND:
							scene->techniques[techniqueIndex].rop.blendEnable = true;
							scene->techniques[techniqueIndex].rop.blendOpColor = GPU_BLEND_OP_ADD;
							scene->techniques[techniqueIndex].rop.blendSrcColor = GPU_BLEND_FACTOR_SRC_ALPHA;
							scene->techniques[techniqueIndex].rop.blendDstColor = GPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
							break;
						case GL_DEPTH_TEST:
							scene->techniques[techniqueIndex].rop.depthTestEnable = true;
							break;
						case GL_DEPTH_WRITEMASK:
							scene->techniques[techniqueIndex].rop.depthWriteEnable = true;
							break;
						case GL_CULL_FACE:
							scene->techniques[techniqueIndex].rop.cullMode = GPU_CULL_MODE_BACK;
							break;
						case GL_POLYGON_OFFSET_FILL:
							assert( false );
							break;
						case GL_SAMPLE_ALPHA_TO_COVERAGE:
							assert( false );
							break;
						case GL_SCISSOR_TEST:
							assert( false );
							break;
					}
				}

				const Json_t * functions = Json_GetMemberByName( states, "functions" );
				const int functionCount = Json_GetMemberCount( functions );
				for ( int functionIndex = 0; functionIndex < functionCount; functionIndex++ )
				{
					const Json_t * func = Json_GetMemberByIndex( functions, functionIndex );
					const char * funcName = Json_GetMemberName( func );
					if ( strcmp( funcName, "blendColor" ) == 0 )
					{
						// [float:red, float:blue, float:green, float:alpha]
						scene->techniques[techniqueIndex].rop.blendColor.x = Json_GetFloat( Json_GetMemberByIndex( func, 0 ), 0.0f );
						scene->techniques[techniqueIndex].rop.blendColor.y = Json_GetFloat( Json_GetMemberByIndex( func, 1 ), 0.0f );
						scene->techniques[techniqueIndex].rop.blendColor.z = Json_GetFloat( Json_GetMemberByIndex( func, 2 ), 0.0f );
						scene->techniques[techniqueIndex].rop.blendColor.w = Json_GetFloat( Json_GetMemberByIndex( func, 3 ), 0.0f );
					}
					else if ( strcmp( funcName, "blendEquationSeparate" ) == 0 )
					{
						// [GLenum:GL_FUNC_* (rgb), GLenum:GL_FUNC_* (alpha)]
						scene->techniques[techniqueIndex].rop.blendOpColor = Gltf_GetBlendOp( Json_GetUint16( Json_GetMemberByIndex( func, 0 ), 0 ) );
						scene->techniques[techniqueIndex].rop.blendOpAlpha = Gltf_GetBlendOp( Json_GetUint16( Json_GetMemberByIndex( func, 1 ), 0 ) );
					}
					else if ( strcmp( funcName, "blendFuncSeparate" ) == 0 )
					{
						// [GLenum:GL_ONE (srcRGB), GLenum:GL_ZERO (dstRGB), GLenum:GL_ONE (srcAlpha), GLenum:GL_ZERO (dstAlpha)]
						scene->techniques[techniqueIndex].rop.blendSrcColor = Gltf_GetBlendFactor( Json_GetUint16( Json_GetMemberByIndex( func, 0 ), 0 ) );
						scene->techniques[techniqueIndex].rop.blendDstColor = Gltf_GetBlendFactor( Json_GetUint16( Json_GetMemberByIndex( func, 1 ), 0 ) );
						scene->techniques[techniqueIndex].rop.blendSrcAlpha = Gltf_GetBlendFactor( Json_GetUint16( Json_GetMemberByIndex( func, 2 ), 0 ) );
						scene->techniques[techniqueIndex].rop.blendDstAlpha = Gltf_GetBlendFactor( Json_GetUint16( Json_GetMemberByIndex( func, 3 ), 0 ) );
					}
					else if ( strcmp( funcName, "colorMask" ) == 0 )
					{
						// [bool:red, bool:green, bool:blue, bool:alpha]
						scene->techniques[techniqueIndex].rop.redWriteEnable = Json_GetBool( Json_GetMemberByIndex( func, 0 ), false );
						scene->techniques[techniqueIndex].rop.blueWriteEnable = Json_GetBool( Json_GetMemberByIndex( func, 1 ), false );
						scene->techniques[techniqueIndex].rop.greenWriteEnable = Json_GetBool( Json_GetMemberByIndex( func, 2 ), false );
						scene->techniques[techniqueIndex].rop.alphaWriteEnable = Json_GetBool( Json_GetMemberByIndex( func, 3 ), false );
					}
					else if ( strcmp( funcName, "cullFace" ) == 0 )
					{
						// [GLenum:GL_BACK,GL_FRONT]
						scene->techniques[techniqueIndex].rop.cullMode = Gltf_GetCullMode( Json_GetUint16( Json_GetMemberByIndex( func, 0 ), 0 ) );
					}
					else if ( strcmp( funcName, "depthFunc" ) == 0 )
					{
						// [GLenum:GL_LESS,GL_LEQUAL,GL_GREATER]
						scene->techniques[techniqueIndex].rop.depthCompare = Gltf_GetCompareOp( Json_GetUint16( Json_GetMemberByIndex( func, 0 ), 0 ) );
					}
					else if ( strcmp( funcName, "depthMask" ) == 0 )
					{
						// [bool:mask]
						scene->techniques[techniqueIndex].rop.depthWriteEnable = Json_GetBool( Json_GetMemberByIndex( func, 0 ), false );
					}
					else if ( strcmp( funcName, "frontFace" ) == 0 )
					{
						// [Glenum:GL_CCW,GL_CW]
						scene->techniques[techniqueIndex].rop.frontFace = Gltf_GetFrontFace( Json_GetUint16( Json_GetMemberByIndex( func, 0 ), 0 ) );
					}
					else if ( strcmp( funcName, "lineWidth" ) == 0 )
					{
						// [float:width]
						assert( false );
					}
					else if ( strcmp( funcName, "polygonOffset" ) == 0 )
					{
						// [float:factor, float:units]
						assert( false );
					}
					else if ( strcmp( funcName, "depthRange" ) == 0 )
					{
						// [float:znear, float:zfar]
						assert( false );
					}
					else if ( strcmp( funcName, "scissor" ) == 0 )
					{
						// [int:x, int:y, int:width, int:height]
						assert( false );
					}
				}

				GpuGraphicsProgram_Create( context, &scene->techniques[techniqueIndex].program,
											program->vertexSource, program->vertexSourceSize,
											program->fragmentSource, program->fragmentSourceSize,
											scene->techniques[techniqueIndex].parms, uniformCount, DefaultVertexAttributeLayout, vertexAttribsFlags );
			}
			Gltf_CreateTechniqueNameHash( scene );

			const Microseconds_t endTime = GetTimeMicroseconds();
			Print( "%1.3f seconds to load techniques\n", ( endTime - startTime ) * 1e-6f );
		}

		//
		// glTF materials
		//
		{
			const Microseconds_t startTime = GetTimeMicroseconds();

			const Json_t * materials = Json_GetMemberByName( rootNode, "materials" );
			scene->materialCount = Json_GetMemberCount( materials );
			scene->materials = (GltfMaterial_t *) calloc( scene->materialCount, sizeof( GltfMaterial_t ) );
			for ( int materialIndex = 0; materialIndex < scene->materialCount; materialIndex++ )
			{
				const Json_t * material = Json_GetMemberByIndex( materials, materialIndex );
				const GltfTechnique_t * technique = Gltf_GetTechniqueByName( scene, Json_GetString( Json_GetMemberByName( material, "technique" ), "" ) );
				scene->materials[materialIndex].name = Gltf_strdup( Json_GetMemberName( material ) );
				scene->materials[materialIndex].technique = technique;

				assert( scene->materials[materialIndex].name[0] != '\0' );
				assert( scene->materials[materialIndex].technique != NULL );

				const Json_t * values = Json_GetMemberByName( material, "values" );
				scene->materials[materialIndex].valueCount = Json_GetMemberCount( values );
				scene->materials[materialIndex].values = (GltfMaterialValue_t *) calloc( scene->materials[materialIndex].valueCount, sizeof( GltfMaterialValue_t ) );
				for ( int valueIndex = 0; valueIndex < scene->materials[materialIndex].valueCount; valueIndex++ )
				{
					const Json_t * value = Json_GetMemberByIndex( values, valueIndex );
					const char * valueName = Json_GetMemberName( value );
					GltfUniform_t * uniform = NULL;
					for ( int uniformIndex = 0; uniformIndex < technique->uniformCount; uniformIndex++ )
					{
						if ( strcmp( technique->uniforms[uniformIndex].name, valueName ) == 0 )
						{
							uniform = &technique->uniforms[uniformIndex];
							break;
						}
					}
					if ( uniform == NULL )
					{
						assert( false );
						continue;
					}
					assert( uniform->semantic == GLTF_UNIFORM_SEMANTIC_NONE || uniform->semantic == GLTF_UNIFORM_SEMANTIC_DEFAULT_VALUE );
					scene->materials[materialIndex].values[valueIndex].uniform = uniform;
					Gltf_ParseUniformValue( &scene->materials[materialIndex].values[valueIndex].value, value, uniform->type, scene );
				}
				// Make sure that the material sets any uniforms that do not have a special semantic or a default value.
				for ( int uniformIndex = 0; uniformIndex < technique->uniformCount; uniformIndex++ )
				{
					if ( technique->uniforms[uniformIndex].semantic == GLTF_UNIFORM_SEMANTIC_NONE )
					{
						bool found = false;
						for ( int valueIndex = 0; valueIndex < scene->materials[materialIndex].valueCount; valueIndex++ )
						{
							if ( scene->materials[materialIndex].values[valueIndex].uniform == &technique->uniforms[uniformIndex] )
							{
								found = true;
							}
						}
						assert( found );
						UNUSED_PARM( found );
					}
				}
			}
			Gltf_CreateMaterialNameHash( scene );

			const Microseconds_t endTime = GetTimeMicroseconds();
			Print( "%1.3f seconds to load materials\n", ( endTime - startTime ) * 1e-6f );
		}

		//
		// glTF meshes
		//
		{
			const Microseconds_t startTime = GetTimeMicroseconds();

			const Json_t * models = Json_GetMemberByName( rootNode, "meshes" );
			scene->modelCount = Json_GetMemberCount( models );
			scene->models = (GltfModel_t *) calloc( scene->modelCount, sizeof( GltfModel_t ) );
			for ( int meshIndex = 0; meshIndex < scene->modelCount; meshIndex++ )
			{
				const Json_t * model = Json_GetMemberByIndex( models, meshIndex );
				scene->models[meshIndex].name = Gltf_strdup( Json_GetMemberName( model ) );

				assert( scene->models[meshIndex].name[0] != '\0' );

				const Json_t * primitives = Json_GetMemberByName( model, "primitives" );
				scene->models[meshIndex].surfaceCount = Json_GetMemberCount( primitives );
				scene->models[meshIndex].surfaces = (GltfSurface_t *) calloc( scene->models[meshIndex].surfaceCount, sizeof( GltfSurface_t ) );
				for ( int surfaceIndex = 0; surfaceIndex < scene->models[meshIndex].surfaceCount; surfaceIndex++ )
				{
					GltfSurface_t * surface = &scene->models[meshIndex].surfaces[surfaceIndex];

					const Json_t * primitive = Json_GetMemberByIndex( primitives, surfaceIndex );
					const Json_t * attributes = Json_GetMemberByName( primitive, "attributes" );

					const char * positionAccessorName		= Json_GetString( Json_GetMemberByName( attributes, "POSITION" ), "" );
					const char * normalAccessorName			= Json_GetString( Json_GetMemberByName( attributes, "NORMAL" ), "" );
					const char * tangentAccessorName		= Json_GetString( Json_GetMemberByName( attributes, "TANGENT" ), "" );
					const char * binormalAccessorName		= Json_GetString( Json_GetMemberByName( attributes, "BINORMAL" ), "" );
					const char * colorAccessorName			= Json_GetString( Json_GetMemberByName( attributes, "COLOR" ), "" );
					const char * uv0AccessorName			= Json_GetString( Json_GetMemberByName( attributes, "TEXCOORD_0" ), "" );
					const char * uv1AccessorName			= Json_GetString( Json_GetMemberByName( attributes, "TEXCOORD_1" ), "" );
					const char * uv2AccessorName			= Json_GetString( Json_GetMemberByName( attributes, "TEXCOORD_2" ), "" );
					const char * jointIndicesAccessorName	= Json_GetString( Json_GetMemberByName( attributes, "JOINT" ), "" );
					const char * jointWeightsAccessorName	= Json_GetString( Json_GetMemberByName( attributes, "WEIGHT" ), "" );
					const char * indicesAccessorName		= Json_GetString( Json_GetMemberByName( primitive, "indices" ), "" );

					surface->material = Gltf_GetMaterialByName( scene, Json_GetString( Json_GetMemberByName( primitive, "material" ), "" ) );
					assert( surface->material != NULL );

					const GltfAccessor_t * positionAccessor		= Gltf_GetAccessorByNameAndType( scene, positionAccessorName,		"VEC3",		GL_FLOAT );
					const GltfAccessor_t * normalAccessor		= Gltf_GetAccessorByNameAndType( scene, normalAccessorName,			"VEC3",		GL_FLOAT );
					const GltfAccessor_t * tangentAccessor		= Gltf_GetAccessorByNameAndType( scene, tangentAccessorName,		"VEC3",		GL_FLOAT );
					const GltfAccessor_t * binormalAccessor		= Gltf_GetAccessorByNameAndType( scene, binormalAccessorName,		"VEC3",		GL_FLOAT );
					const GltfAccessor_t * colorAccessor		= Gltf_GetAccessorByNameAndType( scene, colorAccessorName,			"VEC4",		GL_FLOAT );
					const GltfAccessor_t * uv0Accessor			= Gltf_GetAccessorByNameAndType( scene, uv0AccessorName,			"VEC2",		GL_FLOAT );
					const GltfAccessor_t * uv1Accessor			= Gltf_GetAccessorByNameAndType( scene, uv1AccessorName,			"VEC2",		GL_FLOAT );
					const GltfAccessor_t * uv2Accessor			= Gltf_GetAccessorByNameAndType( scene, uv2AccessorName,			"VEC2",		GL_FLOAT );
					const GltfAccessor_t * jointIndicesAccessor	= Gltf_GetAccessorByNameAndType( scene, jointIndicesAccessorName,	"VEC4",		GL_FLOAT );
					const GltfAccessor_t * jointWeightsAccessor	= Gltf_GetAccessorByNameAndType( scene, jointWeightsAccessorName,	"VEC4",		GL_FLOAT );
					const GltfAccessor_t * indicesAccessor		= Gltf_GetAccessorByNameAndType( scene, indicesAccessorName,		"SCALAR",	GL_UNSIGNED_SHORT );

					if ( positionAccessor == NULL || indicesAccessor == NULL )
					{
						assert( false );
						continue;
					}

					surface->mins.x = positionAccessor->floatMin[0];
					surface->mins.y = positionAccessor->floatMin[1];
					surface->mins.z = positionAccessor->floatMin[2];
					surface->maxs.x = positionAccessor->floatMax[0];
					surface->maxs.y = positionAccessor->floatMax[1];
					surface->maxs.z = positionAccessor->floatMax[2];

					assert( normalAccessor			== NULL || normalAccessor->count		== positionAccessor->count );
					assert( tangentAccessor			== NULL || tangentAccessor->count		== positionAccessor->count );
					assert( binormalAccessor		== NULL || binormalAccessor->count		== positionAccessor->count );
					assert( colorAccessor			== NULL || colorAccessor->count			== positionAccessor->count );
					assert( uv0Accessor				== NULL || uv0Accessor->count			== positionAccessor->count );
					assert( uv1Accessor				== NULL || uv1Accessor->count			== positionAccessor->count );
					assert( uv2Accessor				== NULL || uv2Accessor->count			== positionAccessor->count );
					assert( jointIndicesAccessor	== NULL || jointIndicesAccessor->count	== positionAccessor->count );
					assert( jointWeightsAccessor	== NULL || jointWeightsAccessor->count	== positionAccessor->count );

					const int attribFlags = ( positionAccessor != NULL		? VERTEX_ATTRIBUTE_FLAG_POSITION : 0 ) |
											( normalAccessor != NULL		? VERTEX_ATTRIBUTE_FLAG_NORMAL : 0 ) |
											( tangentAccessor != NULL		? VERTEX_ATTRIBUTE_FLAG_TANGENT : 0 ) |
											( binormalAccessor != NULL		? VERTEX_ATTRIBUTE_FLAG_BINORMAL : 0 ) |
											( colorAccessor != NULL			? VERTEX_ATTRIBUTE_FLAG_COLOR : 0 ) |
											( uv0Accessor != NULL			? VERTEX_ATTRIBUTE_FLAG_UV0 : 0 ) |
											( uv1Accessor != NULL			? VERTEX_ATTRIBUTE_FLAG_UV1 : 0 ) |
											( uv2Accessor != NULL			? VERTEX_ATTRIBUTE_FLAG_UV2 : 0 ) |
											( jointIndicesAccessor != NULL	? VERTEX_ATTRIBUTE_FLAG_JOINT_INDICES : 0 ) |
											( jointWeightsAccessor != NULL	? VERTEX_ATTRIBUTE_FLAG_JOINT_WEIGHTS : 0 );

					GpuVertexAttributeArrays_t attribs;
					GpuVertexAttributeArrays_Alloc( &attribs.base, DefaultVertexAttributeLayout, positionAccessor->count, attribFlags );

					if ( positionAccessor != NULL )		memcpy( attribs.position,		Gltf_GetBufferData( positionAccessor ),		positionAccessor->count		* sizeof( attribs.position[0] ) );
					if ( normalAccessor != NULL )		memcpy( attribs.normal,			Gltf_GetBufferData( normalAccessor ),		normalAccessor->count		* sizeof( attribs.normal[0] ) );
					if ( tangentAccessor != NULL )		memcpy( attribs.tangent,		Gltf_GetBufferData( tangentAccessor ),		tangentAccessor->count		* sizeof( attribs.tangent[0] ) );
					if ( binormalAccessor != NULL )		memcpy( attribs.binormal,		Gltf_GetBufferData( binormalAccessor ),		binormalAccessor->count		* sizeof( attribs.binormal[0] ) );
					if ( colorAccessor != NULL )		memcpy( attribs.color,			Gltf_GetBufferData( colorAccessor ),		colorAccessor->count		* sizeof( attribs.color[0] ) );
					if ( uv0Accessor != NULL )			memcpy( attribs.uv0,			Gltf_GetBufferData( uv0Accessor ),			uv0Accessor->count			* sizeof( attribs.uv0[0] ) );
					if ( uv1Accessor != NULL )			memcpy( attribs.uv1,			Gltf_GetBufferData( uv1Accessor ),			uv1Accessor->count			* sizeof( attribs.uv1[0] ) );
					if ( uv2Accessor != NULL )			memcpy( attribs.uv2,			Gltf_GetBufferData( uv2Accessor ),			uv2Accessor->count			* sizeof( attribs.uv2[0] ) );
					if ( jointIndicesAccessor != NULL )	memcpy( attribs.jointIndices,	Gltf_GetBufferData( jointIndicesAccessor ),	jointIndicesAccessor->count	* sizeof( attribs.jointIndices[0] ) );
					if ( jointWeightsAccessor != NULL )	memcpy( attribs.jointWeights,	Gltf_GetBufferData( jointWeightsAccessor ),	jointWeightsAccessor->count	* sizeof( attribs.jointWeights[0] ) );

					GpuTriangleIndex_t * indices = (GpuTriangleIndex_t *)Gltf_GetBufferData( indicesAccessor );

					GpuGeometry_Create( context, &surface->geometry, &attribs.base, positionAccessor->count, indices, indicesAccessor->count );

					GpuVertexAttributeArrays_Free( &attribs.base );

					GpuGraphicsPipelineParms_t pipelineParms;
					GpuGraphicsPipelineParms_Init( &pipelineParms );

					pipelineParms.renderPass = renderPass;
					pipelineParms.program = &surface->material->technique->program;
					pipelineParms.geometry = &surface->geometry;
					pipelineParms.rop = surface->material->technique->rop;

					GpuGraphicsPipeline_Create( context, &surface->pipeline, &pipelineParms );
				}
			}
			Gltf_CreateModelNameHash( scene );

			const Microseconds_t endTime = GetTimeMicroseconds();
			Print( "%1.3f seconds to load models\n", ( endTime - startTime ) * 1e-6f );
		}

		//
		// glTF animations
		//
		{
			const Microseconds_t startTime = GetTimeMicroseconds();

			const Json_t * animations = Json_GetMemberByName( rootNode, "animations" );
			scene->animationCount = Json_GetMemberCount( animations );
			scene->animations = (GltfAnimation_t *) calloc( scene->animationCount, sizeof( GltfAnimation_t ) );
			for ( int animationIndex = 0; animationIndex < scene->animationCount; animationIndex++ )
			{
				const Json_t * animation = Json_GetMemberByIndex( animations, animationIndex );
				scene->animations[animationIndex].name = Gltf_strdup( Json_GetMemberName( animation ) );

				const Json_t * parameters = Json_GetMemberByName( animation, "parameters" );
				const Json_t * samplers = Json_GetMemberByName( animation, "samplers" );

				const char * timeAccessor = Json_GetString( Json_GetMemberByName( parameters, "TIME" ), "" );
				const GltfAccessor_t * access_time = Gltf_GetAccessorByNameAndType( scene, timeAccessor, "SCALAR", GL_FLOAT );

				if ( access_time == NULL || access_time->count <= 0 )
				{
					assert( false );
					continue;
				}

				scene->animations[animationIndex].sampleCount = access_time->count;
				scene->animations[animationIndex].sampleTimes = (float *)Gltf_GetBufferData( access_time );

				const Json_t * channels = Json_GetMemberByName( animation, "channels" );
				scene->animations[animationIndex].channelCount = Json_GetMemberCount( channels );
				scene->animations[animationIndex].channels = (GltfAnimationChannel_t *) calloc( scene->animations[animationIndex].channelCount, sizeof( GltfAnimationChannel_t ) );
				int newChannelCount = 0;
				for ( int channelIndex = 0; channelIndex < scene->animations[animationIndex].channelCount; channelIndex++ )
				{
					const Json_t * channel = Json_GetMemberByIndex( channels, channelIndex );
					const char * samplerName = Json_GetString( Json_GetMemberByName( channel, "sampler" ), "" );
					const Json_t * sampler = Json_GetMemberByName( samplers, samplerName );
					const char * inputName = Json_GetString( Json_GetMemberByName( sampler, "input" ), "" );
					const char * interpolation = Json_GetString( Json_GetMemberByName( sampler, "interpolation" ), "" );
					const char * outputName = Json_GetString( Json_GetMemberByName( sampler, "output" ), "" );
					const char * accessorName = Json_GetString( Json_GetMemberByName( parameters, outputName ), "" );

					assert( strcmp( inputName, "TIME" ) == 0 );
					assert( strcmp( interpolation, "LINEAR" ) == 0 );
					assert( outputName[0] != '\0' );
					assert( accessorName[0] != '\0' );

					UNUSED_PARM( inputName );
					UNUSED_PARM( interpolation );

					const Json_t * target = Json_GetMemberByName( channel, "target" );
					const char * nodeName = Json_GetString( Json_GetMemberByName( target, "id" ), "" );
					const char * pathName = Json_GetString( Json_GetMemberByName( target, "path" ), "" );

					Vector3f_t * translation = NULL;
					Quatf_t * rotation = NULL;
					Vector3f_t * scale = NULL;

					if ( strcmp( pathName, "translation" ) == 0 )
					{
						const GltfAccessor_t * accessor	= Gltf_GetAccessorByNameAndType( scene, accessorName, "VEC3", GL_FLOAT );
						assert( accessor != NULL );
						translation = (Vector3f_t *) Gltf_GetBufferData( accessor );
					}
					else if ( strcmp( pathName, "rotation" ) == 0 )
					{
						const GltfAccessor_t * accessor	= Gltf_GetAccessorByNameAndType( scene, accessorName, "VEC4", GL_FLOAT );
						assert( accessor != NULL );
						rotation = (Quatf_t *) Gltf_GetBufferData( accessor );
					}
					else if ( strcmp( pathName, "scale" ) == 0 )
					{
						const GltfAccessor_t * accessor	= Gltf_GetAccessorByNameAndType( scene, accessorName, "VEC3", GL_FLOAT );
						assert( accessor != NULL );
						scale = (Vector3f_t *) Gltf_GetBufferData( accessor );
					}

					// Try to merge this channel with a previous channel for the same node.
					for ( int k = 0; k < newChannelCount; k++ )
					{
						if ( strcmp( nodeName, scene->animations[animationIndex].channels[k].nodeName ) == 0 )
						{
							if ( translation != NULL )
							{
								scene->animations[animationIndex].channels[k].translation = translation;
								translation = NULL;
							}
							if ( rotation != NULL )
							{
								scene->animations[animationIndex].channels[k].rotation = rotation;
								rotation = NULL;
							}
							if ( scale != NULL )
							{
								scene->animations[animationIndex].channels[k].scale = scale;
								scale = NULL;
							}
							break;
						}
					}

					// Only store the channel if it was not merged.
					if ( translation != NULL || rotation != NULL || scale != NULL )
					{
						scene->animations[animationIndex].channels[newChannelCount].nodeName = Gltf_strdup( nodeName );
						scene->animations[animationIndex].channels[newChannelCount].node = NULL; // linked up once the nodes are loaded
						scene->animations[animationIndex].channels[newChannelCount].translation = translation;
						scene->animations[animationIndex].channels[newChannelCount].rotation = rotation;
						scene->animations[animationIndex].channels[newChannelCount].scale = scale;
						newChannelCount++;
					}
				}
				scene->animations[animationIndex].channelCount = newChannelCount;
			}
			Gltf_CreateAnimationNameHash( scene );

			const Microseconds_t endTime = GetTimeMicroseconds();
			Print( "%1.3f seconds to load animations\n", ( endTime - startTime ) * 1e-6f );
		}

		//
		// glTF skins
		//
		{
			const Microseconds_t startTime = GetTimeMicroseconds();

			const Json_t * skins = Json_GetMemberByName( rootNode, "skins" );
			scene->skinCount = Json_GetMemberCount( skins );
			scene->skins = (GltfSkin_t *) calloc( scene->skinCount, sizeof( GltfSkin_t ) );
			for ( int skinIndex = 0; skinIndex < scene->skinCount; skinIndex++ )
			{
				const Json_t * skin = Json_GetMemberByIndex( skins, skinIndex );
				scene->skins[skinIndex].name = Gltf_strdup( Json_GetMemberName( skin ) );
				Gltf_ParseFloatArray( scene->skins[skinIndex].bindShapeMatrix.m[0], 16, Json_GetMemberByName( skin, "bindShapeMatrix" ) );

				const char * bindAccessorName = Json_GetString( Json_GetMemberByName( skin, "inverseBindMatrices" ), "" );
				const GltfAccessor_t * bindAccess = Gltf_GetAccessorByNameAndType( scene, bindAccessorName, "MAT4", GL_FLOAT );
				scene->skins[skinIndex].inverseBindMatrices = Gltf_GetBufferData( bindAccess );

				const char * minsAccessorName = Json_GetString( Json_GetMemberByName( skin, "jointGeometryMins" ), "" );
				const GltfAccessor_t * minsAccess = Gltf_GetAccessorByNameAndType( scene, minsAccessorName, "VEC3", GL_FLOAT );
				scene->skins[skinIndex].jointGeometryMins = Gltf_GetBufferData( minsAccess );

				const char * maxsAccessorName = Json_GetString( Json_GetMemberByName( skin, "jointGeometryMaxs" ), "" );
				const GltfAccessor_t * maxsAccess = Gltf_GetAccessorByNameAndType( scene, maxsAccessorName, "VEC3", GL_FLOAT );
				scene->skins[skinIndex].jointGeometryMaxs = Gltf_GetBufferData( maxsAccess );

				assert( scene->skins[skinIndex].name[0] != '\0' );
				assert( scene->skins[skinIndex].inverseBindMatrices != NULL );

				const Json_t * jointNames = Json_GetMemberByName( skin, "jointNames" );
				scene->skins[skinIndex].jointCount = Json_GetMemberCount( jointNames );
				scene->skins[skinIndex].joints = (GltfJoint_t *) calloc( scene->skins[skinIndex].jointCount, sizeof( GltfJoint_t ) );
				assert( scene->skins[skinIndex].jointCount <= MAX_JOINTS );
				for ( int jointIndex = 0; jointIndex < scene->skins[skinIndex].jointCount; jointIndex++ )
				{
					scene->skins[skinIndex].joints[jointIndex].name = Gltf_strdup( Json_GetString( Json_GetMemberByIndex( jointNames, jointIndex ), "" ) );
					scene->skins[skinIndex].joints[jointIndex].node = NULL; // linked up once the nodes are loaded
				}
				assert( bindAccess->count == scene->skins[skinIndex].jointCount );

				GpuBuffer_Create( context, &scene->skins[skinIndex].jointBuffer, GPU_BUFFER_TYPE_UNIFORM, scene->skins[skinIndex].jointCount * sizeof( Matrix4x4f_t ), NULL, false );
			}
			Gltf_CreateSkinNameHash( scene );

			const Microseconds_t endTime = GetTimeMicroseconds();
			Print( "%1.3f seconds to load skins\n", ( endTime - startTime ) * 1e-6f );
		}

		//
		// glTF cameras
		//
		{
			const Microseconds_t startTime = GetTimeMicroseconds();

			const Json_t * cameras = Json_GetMemberByName( rootNode, "cameras" );
			scene->cameraCount = Json_GetMemberCount( cameras );
			scene->cameras = (GltfCamera_t *) calloc( scene->cameraCount, sizeof( GltfCamera_t ) );
			for ( int cameraIndex = 0; cameraIndex < scene->cameraCount; cameraIndex++ )
			{
				const Json_t * camera = Json_GetMemberByIndex( cameras, cameraIndex );
				const char * type = Json_GetString( Json_GetMemberByName( camera, "type" ), "" );
				scene->cameras[cameraIndex].name = Gltf_strdup( Json_GetMemberName( camera ) );
				if ( strcmp( type, "perspective" ) == 0 )
				{
					const Json_t * perspective = Json_GetMemberByName( camera, "perspective" );
					const float aspectRatio = Json_GetFloat( Json_GetMemberByName( perspective, "aspectRatio" ), 0.0f );
					const float yfov = Json_GetFloat( Json_GetMemberByName( perspective, "yfov" ), 0.0f );
					scene->cameras[cameraIndex].type = GLTF_CAMERA_TYPE_PERSPECTIVE;
					scene->cameras[cameraIndex].perspective.fovDegreesX = ( 180.0f / MATH_PI ) * 2.0f * atanf( tanf( yfov * 0.5f ) * aspectRatio );
					scene->cameras[cameraIndex].perspective.fovDegreesY = ( 180.0f / MATH_PI ) * yfov;
					scene->cameras[cameraIndex].perspective.nearZ = Json_GetFloat( Json_GetMemberByName( perspective, "znear" ), 0.0f );
					scene->cameras[cameraIndex].perspective.farZ = Json_GetFloat( Json_GetMemberByName( perspective, "zfar" ), 0.0f );
					assert( scene->cameras[cameraIndex].perspective.fovDegreesX > 0.0f );
					assert( scene->cameras[cameraIndex].perspective.fovDegreesY > 0.0f );
					assert( scene->cameras[cameraIndex].perspective.nearZ > 0.0f );
				}
				else
				{
					const Json_t * orthographic = Json_GetMemberByName( camera, "orthographic" );
					scene->cameras[cameraIndex].type = GLTF_CAMERA_TYPE_ORTHOGRAPHIC;
					scene->cameras[cameraIndex].orthographic.magX = Json_GetFloat( Json_GetMemberByName( orthographic, "xmag" ), 0.0f );
					scene->cameras[cameraIndex].orthographic.magY = Json_GetFloat( Json_GetMemberByName( orthographic, "ymag" ), 0.0f );
					scene->cameras[cameraIndex].orthographic.nearZ = Json_GetFloat( Json_GetMemberByName( orthographic, "znear" ), 0.0f );
					scene->cameras[cameraIndex].orthographic.farZ = Json_GetFloat( Json_GetMemberByName( orthographic, "zfar" ), 0.0f );
					assert( scene->cameras[cameraIndex].orthographic.magX > 0.0f );
					assert( scene->cameras[cameraIndex].orthographic.magY > 0.0f );
					assert( scene->cameras[cameraIndex].orthographic.nearZ > 0.0f );
				}
			}
			Gltf_CreateCameraNameHash( scene );

			const Microseconds_t endTime = GetTimeMicroseconds();
			Print( "%1.3f seconds to load cameras\n", ( endTime - startTime ) * 1e-6f );
		}

		//
		// glTF nodes
		//
		{
			const Microseconds_t startTime = GetTimeMicroseconds();

			const Json_t * nodes = Json_GetMemberByName( rootNode, "nodes" );
			scene->nodeCount = Json_GetMemberCount( nodes );
			scene->nodes = (GltfNode_t *) calloc( scene->nodeCount, sizeof( GltfNode_t ) );
			for ( int nodeIndex = 0; nodeIndex < scene->nodeCount; nodeIndex++ )
			{
				const Json_t * node = Json_GetMemberByIndex( nodes, nodeIndex );
				scene->nodes[nodeIndex].name = Gltf_strdup( Json_GetMemberName( node ) );
				scene->nodes[nodeIndex].jointName = Gltf_strdup( Json_GetString( Json_GetMemberByName( node, "jointName" ), "" ) );
				const Json_t * matrix = Json_GetMemberByName( node, "matrix" );
				if ( Json_IsArray( matrix ) )
				{
					Gltf_ParseFloatArray( scene->nodes[nodeIndex].localTransform.m[0], 16, matrix );
					Matrix4x4f_GetTranslation( &scene->nodes[nodeIndex].translation, &scene->nodes[nodeIndex].localTransform );
					Matrix4x4f_GetRotation( &scene->nodes[nodeIndex].rotation, &scene->nodes[nodeIndex].localTransform );
					Matrix4x4f_GetScale( &scene->nodes[nodeIndex].scale, &scene->nodes[nodeIndex].localTransform );
				}
				else
				{
					Gltf_ParseFloatArray( &scene->nodes[nodeIndex].rotation.x, 4, Json_GetMemberByName( node, "rotation" ) );
					Gltf_ParseFloatArray( &scene->nodes[nodeIndex].scale.x, 3, Json_GetMemberByName( node, "scale" ) );
					Gltf_ParseFloatArray( &scene->nodes[nodeIndex].translation.x, 3, Json_GetMemberByName( node, "translation" ) );
					Matrix4x4f_CreateTranslationRotationScale( &scene->nodes[nodeIndex].localTransform,
																&scene->nodes[nodeIndex].scale,
																&scene->nodes[nodeIndex].rotation,
																&scene->nodes[nodeIndex].translation );
				}
				scene->nodes[nodeIndex].globalTransform = scene->nodes[nodeIndex].localTransform;	// transformed to global space later

				assert( scene->nodes[nodeIndex].name[0] != '\0' );
				assert( Matrix4x4f_IsAffine( &scene->nodes[nodeIndex].localTransform, 1e-4f ) );
				assert( Matrix4x4f_IsAffine( &scene->nodes[nodeIndex].globalTransform, 1e-4f ) );

				const Json_t * children = Json_GetMemberByName( node, "children" );
				scene->nodes[nodeIndex].childCount = Json_GetMemberCount( children );
				scene->nodes[nodeIndex].childNames = (char **) calloc( scene->nodes[nodeIndex].childCount, sizeof( char * ) );
				for ( int c = 0; c < scene->nodes[nodeIndex].childCount; c++ )
				{
					scene->nodes[nodeIndex].childNames[c] = Gltf_strdup( Json_GetString( Json_GetMemberByIndex( children, c ), "" ) );
				}
				scene->nodes[nodeIndex].camera = Gltf_GetCameraByName( scene, Json_GetString( Json_GetMemberByName( node, "camera" ), "" ) );
				scene->nodes[nodeIndex].skin = Gltf_GetSkinByName( scene, Json_GetString( Json_GetMemberByName( node, "skin" ), "" ) );
				const Json_t * meshes = Json_GetMemberByName( node, "meshes" );
				scene->nodes[nodeIndex].modelCount = Json_GetMemberCount( meshes );
				scene->nodes[nodeIndex].models = (GltfModel_t **) calloc( scene->nodes[nodeIndex].modelCount, sizeof( GltfModel_t ** ) );
				for ( int m = 0; m < scene->nodes[nodeIndex].modelCount; m++ )
				{
					scene->nodes[nodeIndex].models[m] = Gltf_GetModelByName( scene, Json_GetString( Json_GetMemberByIndex( meshes, m ), "" ) );
					assert( scene->nodes[nodeIndex].models[m] != NULL );
				}
			}
			Gltf_SortNodes( scene->nodes, scene->nodeCount );
			Gltf_CreateNodeNameHash( scene );
			Gltf_CreateNodeJointNameHash( scene );

			const Microseconds_t endTime = GetTimeMicroseconds();
			Print( "%1.3f seconds to load nodes\n", ( endTime - startTime ) * 1e-6f );
		}

		//
		// Assign node pointers now that the nodes are sorted and the hash is setup.
		//
		{
			// Get the node children and parents.
			for ( int nodeIndex = 0; nodeIndex < scene->nodeCount; nodeIndex++ )
			{
				GltfNode_t * node = &scene->nodes[nodeIndex];
				node->children = (GltfNode_t **) calloc( node->childCount, sizeof( GltfNode_t * ) );
				for ( int childIndex = 0; childIndex < node->childCount; childIndex++ )
				{
					node->children[childIndex] = Gltf_GetNodeByName( scene, node->childNames[childIndex] );
					node->children[childIndex]->parent = node;
				}
			}
			// Get the animated nodes.
			for ( int animationIndex = 0; animationIndex < scene->animationCount; animationIndex++ )
			{
				for ( int channelIndex = 0; channelIndex < scene->animations[animationIndex].channelCount; channelIndex++ )
				{
					scene->animations[animationIndex].channels[channelIndex].node = Gltf_GetNodeByName( scene, scene->animations[animationIndex].channels[channelIndex].nodeName );
					assert( scene->animations[animationIndex].channels[channelIndex].node != NULL );
				}
			}
			// Get the skin joint nodes.
			for ( int skinIndex = 0; skinIndex < scene->skinCount; skinIndex++ )
			{
				for ( int jointIndex = 0; jointIndex < scene->skins[skinIndex].jointCount; jointIndex++ )
				{
					scene->skins[skinIndex].joints[jointIndex].node = Gltf_GetNodeByJointName( scene, scene->skins[skinIndex].joints[jointIndex].name );
					assert( scene->skins[skinIndex].joints[jointIndex].node != NULL );
				}
				// Find the parent of the root node of the skin.
				GltfNode_t * root = NULL;
				for ( int jointIndex = 0; jointIndex < scene->skins[skinIndex].jointCount && root == NULL; jointIndex++ )
				{
					root = scene->skins[skinIndex].joints[jointIndex].node;
					for ( int k = 0; k < scene->skins[skinIndex].jointCount; k++ )
					{
						if ( root->parent == scene->skins[skinIndex].joints[k].node )
						{
							root = NULL;
							break;
						}
					}
				}
				scene->skins[skinIndex].parent = root->parent;
			}
		}

		//
		// glTF sub-scenes
		//
		{
			const Json_t * subScenes = Json_GetMemberByName( rootNode, "scenes" );
			scene->subSceneCount = Json_GetMemberCount( subScenes );
			scene->subScenes = (GltfSubScene_t *) calloc( scene->subSceneCount, sizeof( GltfSubScene_t ) );
			for ( int subSceneIndex = 0; subSceneIndex < scene->subSceneCount; subSceneIndex++ )
			{
				const Json_t * subScene = Json_GetMemberByIndex( subScenes, subSceneIndex );
				scene->subScenes[subSceneIndex].name = Gltf_strdup( Json_GetMemberName( subScene ) );

				const Json_t * nodes = Json_GetMemberByName( subScene, "nodes" );
				scene->subScenes[subSceneIndex].subTreeCount = Json_GetMemberCount( nodes );
				scene->subScenes[subSceneIndex].subTrees = (GltfSubTree_t *) calloc( scene->subScenes[subSceneIndex].subTreeCount, sizeof( GltfSubTree_t ) );
				for ( int subTreeIndex = 0; subTreeIndex < scene->subScenes[subSceneIndex].subTreeCount; subTreeIndex++ )
				{
					GltfSubTree_t * subTree = &scene->subScenes[subSceneIndex].subTrees[subTreeIndex];
					const char * nodeName = Json_GetString( Json_GetMemberByIndex( nodes, subTreeIndex ), "" );
					subTree->nodes = Gltf_GetNodeByName( scene, nodeName );
					assert( subTree->nodes != NULL );
					subTree->nodeCount = subTree->nodes->subTreeNodeCount;
				}
			}
			Gltf_CreateSubSceneNameHash( scene );
		}

		//
		// glTF default scene
		//

		const char * defaultSceneName = Json_GetString( Json_GetMemberByName( rootNode, "scene" ), "" );
		scene->currentSubScene = Gltf_GetSubSceneByName( scene, defaultSceneName );
		assert( scene->currentSubScene != NULL );
	}
	Json_Destroy( rootNode );

	// Create a default joint buffer.
	{
		Matrix4x4f_t * data = malloc( MAX_JOINTS * sizeof( Matrix4x4f_t ) );
		for ( int jointIndex = 0; jointIndex < MAX_JOINTS; jointIndex++ )
		{
			Matrix4x4f_CreateIdentity( &data[jointIndex] );
		}
		GpuBuffer_Create( context, &scene->defaultJointBuffer, GPU_BUFFER_TYPE_UNIFORM, MAX_JOINTS * sizeof( Matrix4x4f_t ), data, false );
		free( data );
	}

	// Create unit cube.
	{
		GpuGeometry_CreateCube( context, &scene->unitCubeGeometry, 0.0f, 1.0f );
		GpuGraphicsProgram_Create( context, &scene->unitCubeFlatShadeProgram,
									PROGRAM( unitCubeFlatShadeVertexProgram ), sizeof( PROGRAM( unitCubeFlatShadeVertexProgram ) ),
									PROGRAM( unitCubeFlatShadeFragmentProgram ), sizeof( PROGRAM( unitCubeFlatShadeFragmentProgram ) ),
									unitCubeFlatShadeProgramParms, ARRAY_SIZE( unitCubeFlatShadeProgramParms ),
									scene->unitCubeGeometry.layout, VERTEX_ATTRIBUTE_FLAG_POSITION | VERTEX_ATTRIBUTE_FLAG_NORMAL );

		GpuGraphicsPipelineParms_t pipelineParms;
		GpuGraphicsPipelineParms_Init( &pipelineParms );

		pipelineParms.renderPass = renderPass;
		pipelineParms.program = &scene->unitCubeFlatShadeProgram;
		pipelineParms.geometry = &scene->unitCubeGeometry;

		GpuGraphicsPipeline_Create( context, &scene->unitCubePipeline, &pipelineParms );
	}

	const Microseconds_t t1 = GetTimeMicroseconds();

	Print( "%1.3f seconds to load %s\n", ( t1 - t0 ) * 1e-6f, fileName );

	return true;
}

static void GltfScene_Destroy( GpuContext_t * context, GltfScene_t * scene )
{
	GpuContext_WaitIdle( context );

	{
		for ( int bufferIndex = 0; bufferIndex < scene->bufferCount; bufferIndex++ )
		{
			free( scene->buffers[bufferIndex].name );
			free( scene->buffers[bufferIndex].type );
			free( scene->buffers[bufferIndex].bufferData );
		}
		free( scene->buffers );
		free( scene->bufferNameHash );
	}
	{
		for ( int bufferViewIndex = 0; bufferViewIndex < scene->bufferViewCount; bufferViewIndex++ )
		{
			free( scene->bufferViews[bufferViewIndex].name );
		}
		free( scene->bufferViews );
		free( scene->bufferViewNameHash );
	}
	{
		for ( int accessorIndex = 0; accessorIndex < scene->accessorCount; accessorIndex++ )
		{
			free( scene->accessors[accessorIndex].name );
			free( scene->accessors[accessorIndex].type );
		}
		free( scene->accessors );
		free( scene->accessorNameHash );
	}
	{
		for ( int imageIndex = 0; imageIndex < scene->imageCount; imageIndex++ )
		{
			free( scene->images[imageIndex].name );
			free( scene->images[imageIndex].uri );
		}
		free( scene->images );
		free( scene->imageNameHash );
	}
	{
		for ( int textureIndex = 0; textureIndex < scene->textureCount; textureIndex++ )
		{
			free( scene->textures[textureIndex].name );
			GpuTexture_Destroy( context, &scene->textures[textureIndex].texture );
		}
		free( scene->textures );
		free( scene->textureNameHash );
	}
	{
		for ( int shaderIndex = 0; shaderIndex < scene->shaderCount; shaderIndex++ )
		{
			free( scene->shaders[shaderIndex].name );
			free( scene->shaders[shaderIndex].uriGlslOpenGL );
			free( scene->shaders[shaderIndex].uriGlslVulkan );
			free( scene->shaders[shaderIndex].uriSpirvOpenGL );
			free( scene->shaders[shaderIndex].uriSpirvVulkan );
		}
		free( scene->shaders );
		free( scene->shaderNameHash );
	}
	{
		for ( int programIndex = 0; programIndex < scene->programCount; programIndex++ )
		{
			free( scene->programs[programIndex].name );
			free( scene->programs[programIndex].vertexSource );
			free( scene->programs[programIndex].fragmentSource );
		}
		free( scene->programs );
		free( scene->programNameHash );
	}
	{
		for ( int techniqueIndex = 0; techniqueIndex < scene->techniqueCount; techniqueIndex++ )
		{
			for ( int uniformIndex = 0; uniformIndex < scene->techniques[techniqueIndex].uniformCount; uniformIndex++ )
			{
				free( (void *)scene->techniques[techniqueIndex].parms[uniformIndex].name );
				free( scene->techniques[techniqueIndex].uniforms[uniformIndex].name );
			}
			free( scene->techniques[techniqueIndex].name );
			free( scene->techniques[techniqueIndex].parms );
			free( scene->techniques[techniqueIndex].uniforms );
			GpuGraphicsProgram_Destroy( context, &scene->techniques[techniqueIndex].program );
		}
		free( scene->techniques );
		free( scene->techniqueNameHash );
	}
	{
		for ( int materialIndex = 0; materialIndex < scene->materialCount; materialIndex++ )
		{
			free( scene->materials[materialIndex].name );
			free( scene->materials[materialIndex].values );
		}
		free( scene->materials );
		free( scene->materialNameHash );
	}
	{
		for ( int modelIndex = 0; modelIndex < scene->modelCount; modelIndex++ )
		{
			for ( int surfaceIndex = 0; surfaceIndex < scene->models[modelIndex].surfaceCount; surfaceIndex++ )
			{
				GpuGeometry_Destroy( context, &scene->models[modelIndex].surfaces[surfaceIndex].geometry );
				GpuGraphicsPipeline_Destroy( context, &scene->models[modelIndex].surfaces[surfaceIndex].pipeline );
			}
			free( scene->models[modelIndex].name );
			free( scene->models[modelIndex].surfaces );
		}
		free( scene->models );
		free( scene->modelNameHash );
	}
	{
		for ( int animationIndex = 0; animationIndex < scene->animationCount; animationIndex++ )
		{
			for ( int channelIndex = 0; channelIndex < scene->animations[animationIndex].channelCount; channelIndex++ )
			{
				free( scene->animations[animationIndex].channels[channelIndex].nodeName );
			}
			free( scene->animations[animationIndex].name );
			free( scene->animations[animationIndex].channels );
		}
		free( scene->animations );
		free( scene->animationNameHash );
	}
	{
		for ( int skinIndex = 0; skinIndex < scene->skinCount; skinIndex++ )
		{
			for ( int jointIndex = 0; jointIndex < scene->skins[skinIndex].jointCount; jointIndex++ )
			{
				free( scene->skins[skinIndex].joints[jointIndex].name );
			}
			free( scene->skins[skinIndex].name );
			free( scene->skins[skinIndex].joints );
			GpuBuffer_Destroy( context, &scene->skins[skinIndex].jointBuffer );
		}
		free( scene->skins );
		free( scene->skinNameHash );
	}
	{
		for ( int cameraIndex = 0; cameraIndex < scene->cameraCount; cameraIndex++ )
		{
			free( scene->cameras[cameraIndex].name );
		}
		free( scene->cameras );
		free( scene->cameraNameHash );
	}
	{
		for ( int nodeIndex = 0; nodeIndex < scene->nodeCount; nodeIndex++ )
		{
			for ( int childIndex = 0; childIndex < scene->nodes[nodeIndex].childCount; childIndex++ )
			{
				free( scene->nodes[nodeIndex].childNames[childIndex] );
			}
			free( scene->nodes[nodeIndex].name );
			free( scene->nodes[nodeIndex].jointName );
			free( scene->nodes[nodeIndex].children );
			free( scene->nodes[nodeIndex].childNames );
			free( scene->nodes[nodeIndex].models );
		}
		free( scene->nodes );
		free( scene->nodeNameHash );
		free( scene->nodeJointNameHash );
	}
	{
		for ( int subSceneIndex = 0; subSceneIndex < scene->subSceneCount; subSceneIndex++ )
		{
			free( scene->subScenes[subSceneIndex].subTrees );
		}
		free( scene->subScenes );
		free( scene->subSceneNameHash );
	}

	GpuBuffer_Destroy( context, &scene->defaultJointBuffer );
	GpuGraphicsPipeline_Destroy( context, &scene->unitCubePipeline );
	GpuGraphicsProgram_Destroy( context, &scene->unitCubeFlatShadeProgram );
	GpuGeometry_Destroy( context, &scene->unitCubeGeometry );

	memset( scene, 0, sizeof( GltfScene_t ) );
}

static void GltfScene_Simulate( GltfScene_t * scene, ViewState_t * viewState, GpuWindowInput_t * input, const Microseconds_t time )
{
	// Apply animations to the nodes in the hierarchy.
	for ( int animIndex = 0; animIndex < scene->animationCount; animIndex++ )
	{
		const GltfAnimation_t * animation = &scene->animations[animIndex];
		if ( animation->sampleTimes == NULL || animation->sampleCount < 2 )
		{
			continue;
		}

		const float timeInSeconds = fmodf( time * 1e-6f, animation->sampleTimes[animation->sampleCount - 1] - animation->sampleTimes[0] );
		int frame = 0;
		for ( int sampleCount = animation->sampleCount; sampleCount > 1; sampleCount >>= 1 )
		{
			const int mid = sampleCount >> 1;
			if ( timeInSeconds >= animation->sampleTimes[frame + mid] )
			{
				frame += mid;
				sampleCount = ( sampleCount - mid ) * 2;
			}
		}
		assert( timeInSeconds >= animation->sampleTimes[frame] && timeInSeconds < animation->sampleTimes[frame + 1] );
		const float fraction = ( timeInSeconds - animation->sampleTimes[frame] ) / ( animation->sampleTimes[frame + 1] - animation->sampleTimes[frame] );

		for ( int channelIndex = 0; channelIndex < animation->channelCount; channelIndex++ )
		{
			const GltfAnimationChannel_t * channel = &animation->channels[channelIndex];
			if ( channel->translation != NULL )
			{
				Vector3f_Lerp( &channel->node->translation, &channel->translation[frame], &channel->translation[frame + 1], fraction );
			}
			if ( channel->rotation != NULL )
			{
				Quatf_Lerp( &channel->node->rotation, &channel->rotation[frame], &channel->rotation[frame + 1], fraction );
			}
			if ( channel->scale != NULL )
			{
				Vector3f_Lerp( &channel->node->scale, &channel->scale[frame], &channel->scale[frame + 1], fraction );
			}
		}
	}

	// Transform the node hierarchy into global space.
	for ( int subTreeIndex = 0; subTreeIndex < scene->currentSubScene->subTreeCount; subTreeIndex++ )
	{
		GltfSubTree_t * subTree = &scene->currentSubScene->subTrees[subTreeIndex];
		for ( int nodeIndex = 0; nodeIndex < subTree->nodeCount; nodeIndex++ )
		{
			GltfNode_t * node = &subTree->nodes[nodeIndex];

			Matrix4x4f_CreateTranslationRotationScale( &node->localTransform, &node->scale, &node->rotation, &node->translation );
			if ( node->parent != NULL )
			{
				assert( node->parent < node );
				Matrix4x4f_Multiply( &node->globalTransform, &node->parent->globalTransform, &node->localTransform );
			}
			else
			{
				node->globalTransform = node->localTransform;
			}
		}
	}

	// Find the first camera.
	const GltfNode_t * cameraNode = NULL;
	for ( int subTreeIndex = 0; subTreeIndex < scene->currentSubScene->subTreeCount; subTreeIndex++ )
	{
		GltfSubTree_t * subTree = &scene->currentSubScene->subTrees[subTreeIndex];
		for ( int nodeIndex = 0; nodeIndex < subTree->nodeCount; nodeIndex++ )
		{
			GltfNode_t * node = &subTree->nodes[nodeIndex];
			if ( node->camera != NULL )
			{
				cameraNode = node;
				break;
			}
		}
	}

	// Use the camera if there is one, otherwise use input to move the view point.
	if ( cameraNode != NULL )
	{
		GetHmdViewMatrixForTime( &viewState->hmdViewMatrix, time );

		Matrix4x4f_t cameraViewMatrix;
		Matrix4x4f_Invert( &cameraViewMatrix, &cameraNode->globalTransform );

		Matrix4x4f_Multiply( &viewState->centerViewMatrix, &viewState->hmdViewMatrix, &cameraViewMatrix );

		for ( int eye = 0; eye < NUM_EYES; eye++ )
		{
			Matrix4x4f_t eyeOffsetMatrix;
			Matrix4x4f_CreateTranslation( &eyeOffsetMatrix, ( eye ? -0.5f : 0.5f ) * viewState->interpupillaryDistance, 0.0f, 0.0f );

			Matrix4x4f_Multiply( &viewState->viewMatrix[eye], &eyeOffsetMatrix, &viewState->centerViewMatrix );
			Matrix4x4f_CreateProjectionFov( &viewState->projectionMatrix[eye],
											cameraNode->camera->perspective.fovDegreesX,
											cameraNode->camera->perspective.fovDegreesY,
											0.0f, 0.0f,
											cameraNode->camera->perspective.nearZ, cameraNode->camera->perspective.farZ );

			ViewState_DerivedData( viewState );
		}
	}
	else if ( input != NULL )
	{
		ViewState_HandleInput( viewState, input, time );
	}
	else
	{
		ViewState_HandleHmd( viewState, time );
	}
}

static void GltfScene_UpdateBuffers( GpuCommandBuffer_t * commandBuffer, const GltfScene_t * scene, const ViewState_t * viewState, const int eye )
{
	UNUSED_PARM( eye );

	for ( int subTreeIndex = 0; subTreeIndex < scene->currentSubScene->subTreeCount; subTreeIndex++ )
	{
		GltfSubTree_t * subTree = &scene->currentSubScene->subTrees[subTreeIndex];
		for ( int nodeIndex = 0; nodeIndex < subTree->nodeCount; nodeIndex++ )
		{
			GltfNode_t * node = &subTree->nodes[nodeIndex];
			GltfSkin_t * skin = node->skin;
			if ( skin == NULL )
			{
				continue;
			}
	
			// Exclude the transform of the whole skeleton because that transform will be
			// passed down the vertex shader as the model matrix.
			Matrix4x4f_t inverseGlobalSkeletonTransfom;
			Matrix4x4f_Invert( &inverseGlobalSkeletonTransfom, &skin->parent->globalTransform );

			// Calculate the skin bounds.
			Vector3f_Set( &skin->mins, FLT_MAX );
			Vector3f_Set( &skin->maxs, -FLT_MAX );
			if ( skin->jointGeometryMins != NULL && skin->jointGeometryMaxs != NULL )
			{
				for ( int jointIndex = 0; jointIndex < skin->jointCount; jointIndex++ )
				{
					Matrix4x4f_t localJointTransform;
					Matrix4x4f_Multiply( &localJointTransform, &inverseGlobalSkeletonTransfom, &skin->joints[jointIndex].node->globalTransform );

					Vector3f_t jointMins;
					Vector3f_t jointMaxs;
					Matrix4x4f_TransformBounds( &jointMins, &jointMaxs, &localJointTransform, &skin->jointGeometryMins[jointIndex], &skin->jointGeometryMaxs[jointIndex] );
					Vector3f_Min( &skin->mins, &skin->mins, &jointMins );
					Vector3f_Max( &skin->maxs, &skin->maxs, &jointMaxs );
				}
			}

			// Do not update the joint buffer if the skin bounds are culled.
			{
				Matrix4x4f_t modelViewProjectionCullMatrix;
				Matrix4x4f_Multiply( &modelViewProjectionCullMatrix, &viewState->combinedViewProjectionMatrix, &skin->parent->globalTransform );

				skin->culled = Matrix4x4f_CullBounds( &modelViewProjectionCullMatrix, &skin->mins, &skin->maxs );
				if ( skin->culled )
				{
					continue;
				}
			}

			// Update the skin joint buffer.
			Matrix4x4f_t * joints = NULL;
			GpuBuffer_t * mappedJointBuffer = GpuCommandBuffer_MapBuffer( commandBuffer, &skin->jointBuffer, (void **)&joints );

			for ( int jointIndex = 0; jointIndex < skin->jointCount; jointIndex++ )
			{
				Matrix4x4f_t inverseBindMatrix;
				Matrix4x4f_Multiply( &inverseBindMatrix, &skin->inverseBindMatrices[jointIndex], &skin->bindShapeMatrix );

				Matrix4x4f_t localJointTransform;
				Matrix4x4f_Multiply( &localJointTransform, &inverseGlobalSkeletonTransfom, &skin->joints[jointIndex].node->globalTransform );

				Matrix4x4f_Multiply( &joints[jointIndex], &localJointTransform, &inverseBindMatrix );
			}

			GpuCommandBuffer_UnmapBuffer( commandBuffer, &skin->jointBuffer, mappedJointBuffer, GPU_BUFFER_UNMAP_TYPE_COPY_BACK );
		}
	}
}

static void GltfScene_SetUniformValue( GpuGraphicsCommand_t * command, const GltfUniform_t * uniform, const GltfUniformValue_t * value )
{
	switch ( uniform->type )
	{
		case GPU_PROGRAM_PARM_TYPE_TEXTURE_SAMPLED:					GpuGraphicsCommand_SetParmTextureSampled( command, uniform->index, &value->texture->texture ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT:				GpuGraphicsCommand_SetParmInt( command, uniform->index, value->intValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR2:		GpuGraphicsCommand_SetParmIntVector2( command, uniform->index, (const Vector2i_t *)value->intValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR3:		GpuGraphicsCommand_SetParmIntVector3( command, uniform->index, (const Vector3i_t *)value->intValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_INT_VECTOR4:		GpuGraphicsCommand_SetParmIntVector4( command, uniform->index, (const Vector4i_t *)value->intValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT:				GpuGraphicsCommand_SetParmFloat( command, uniform->index, value->floatValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR2:		GpuGraphicsCommand_SetParmFloatVector2( command, uniform->index, (const Vector2f_t *)value->floatValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR3:		GpuGraphicsCommand_SetParmFloatVector3( command, uniform->index, (const Vector3f_t *)value->floatValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_VECTOR4:		GpuGraphicsCommand_SetParmFloatVector4( command, uniform->index, (const Vector4f_t *)value->floatValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X2:	GpuGraphicsCommand_SetParmFloatMatrix2x2( command, uniform->index, (const Matrix2x2f_t *)value->floatValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X3:	GpuGraphicsCommand_SetParmFloatMatrix2x3( command, uniform->index, (const Matrix2x3f_t *)value->floatValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX2X4:	GpuGraphicsCommand_SetParmFloatMatrix2x4( command, uniform->index, (const Matrix2x4f_t *)value->floatValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X2:	GpuGraphicsCommand_SetParmFloatMatrix3x2( command, uniform->index, (const Matrix3x2f_t *)value->floatValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X3:	GpuGraphicsCommand_SetParmFloatMatrix3x3( command, uniform->index, (const Matrix3x3f_t *)value->floatValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX3X4:	GpuGraphicsCommand_SetParmFloatMatrix3x4( command, uniform->index, (const Matrix3x4f_t *)value->floatValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X2:	GpuGraphicsCommand_SetParmFloatMatrix4x2( command, uniform->index, (const Matrix4x2f_t *)value->floatValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X3:	GpuGraphicsCommand_SetParmFloatMatrix4x3( command, uniform->index, (const Matrix4x3f_t *)value->floatValue ); break;
		case GPU_PROGRAM_PARM_TYPE_PUSH_CONSTANT_FLOAT_MATRIX4X4:	GpuGraphicsCommand_SetParmFloatMatrix4x4( command, uniform->index, (const Matrix4x4f_t *)value->floatValue ); break;
		default: break;
	}
}

typedef struct
{
	Vector4f_t		viewport;
	Matrix4x4f_t	viewMatrix;
	Matrix4x4f_t	projectionMatrix;
	Matrix4x4f_t	viewInverseMatrix;
	Matrix4x4f_t	projectionInverseMatrix;
	Matrix4x4f_t	localMatrix;
	Matrix4x4f_t	modelMatrix;
	Matrix4x4f_t	modelViewMatrix;
	Matrix4x4f_t	modelViewProjectionMatrix;
	Matrix4x4f_t	modelInverseMatrix;
	Matrix4x4f_t	modelViewInverseMatrix;
	Matrix4x4f_t	modelViewProjectionInverseMatrix;
	Matrix3x3f_t	modelInverseTransposeMatrix;
	Matrix3x3f_t	modelViewInverseTransposeMatrix;
} GltfBuiltinUniforms_t;

static void GltfScene_Render( GpuCommandBuffer_t * commandBuffer, const GltfScene_t * scene, const ViewState_t * viewState, const int eye )
{
	GltfBuiltinUniforms_t builtin;

	builtin.viewMatrix = viewState->viewMatrix[eye];
	builtin.projectionMatrix = viewState->projectionMatrix[eye];
	builtin.viewInverseMatrix = viewState->viewInverseMatrix[eye];
	builtin.projectionInverseMatrix = viewState->projectionInverseMatrix[eye];
	builtin.viewport.x = viewState->viewport.x;
	builtin.viewport.y = viewState->viewport.y;
	builtin.viewport.z = viewState->viewport.z;
	builtin.viewport.w = viewState->viewport.w;

	for ( int subTreeIndex = 0; subTreeIndex < scene->currentSubScene->subTreeCount; subTreeIndex++ )
	{
		GltfSubTree_t * subTree = &scene->currentSubScene->subTrees[subTreeIndex];
		for ( int nodeIndex = 0; nodeIndex < subTree->nodeCount; nodeIndex++ )
		{
			GltfNode_t * node = &subTree->nodes[nodeIndex];
			if ( node->modelCount == 0 )
			{
				continue;
			}

			const GltfSkin_t * skin = node->skin;
			const GpuBuffer_t * jointBuffer = ( skin != NULL ) ? &skin->jointBuffer : &scene->defaultJointBuffer;
			const GltfNode_t * parent = ( skin != NULL ) ? skin->parent : node;

			builtin.localMatrix = parent->localTransform;
			builtin.modelMatrix = parent->globalTransform;
			Matrix4x4f_Multiply( &builtin.modelViewMatrix, &builtin.viewMatrix, &builtin.modelMatrix );
			Matrix4x4f_Multiply( &builtin.modelViewProjectionMatrix, &builtin.projectionMatrix, &builtin.modelViewMatrix );
			Matrix4x4f_Invert( &builtin.modelInverseMatrix, &builtin.modelMatrix );
			Matrix4x4f_Invert( &builtin.modelViewInverseMatrix, &builtin.modelViewMatrix );
			Matrix4x4f_Invert( &builtin.modelViewProjectionInverseMatrix, &builtin.modelViewProjectionMatrix );
			Matrix3x3f_CreateTransposeFromMatrix4x4f( &builtin.modelInverseTransposeMatrix, &builtin.modelInverseMatrix );
			Matrix3x3f_CreateTransposeFromMatrix4x4f( &builtin.modelViewInverseTransposeMatrix, &builtin.modelViewInverseMatrix );

			Matrix4x4f_t modelViewProjectionCullMatrix;
			Matrix4x4f_Multiply( &modelViewProjectionCullMatrix, &viewState->combinedViewProjectionMatrix, &builtin.modelMatrix );

			bool showSkinBounds = false;
			if ( skin != NULL && showSkinBounds )
			{
				Matrix4x4f_t unitCubeMatrix;
				Matrix4x4f_CreateOffsetScaleForBounds( &unitCubeMatrix, &builtin.modelMatrix, &skin->mins, &skin->maxs );

				GpuGraphicsCommand_t command;
				GpuGraphicsCommand_Init( &command );

				GpuGraphicsCommand_SetPipeline( &command, &scene->unitCubePipeline );
				GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, 0, &unitCubeMatrix );
				GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, 1, &builtin.viewMatrix );
				GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, 2, &builtin.projectionMatrix );

				GpuCommandBuffer_SubmitGraphicsCommand( commandBuffer, &command );
			}

			if ( skin != NULL && skin->culled )
			{
				continue;
			}

			for ( int modelIndex = 0; modelIndex < node->modelCount; modelIndex++ )
			{
				const GltfModel_t * model = node->models[modelIndex];

				for ( int surfaceIndex = 0; surfaceIndex < model->surfaceCount; surfaceIndex++ )
				{
					const GltfSurface_t * surface = &model->surfaces[surfaceIndex];

					if ( skin == NULL )
					{
						if ( Matrix4x4f_CullBounds( &modelViewProjectionCullMatrix, &surface->mins, &surface->maxs ) )
						{
							continue;
						}
					}

					GpuGraphicsCommand_t command;
					GpuGraphicsCommand_Init( &command );

					GpuGraphicsCommand_SetPipeline( &command, &surface->pipeline );

					const GltfTechnique_t * technique = surface->material->technique;
					for ( int uniformIndex = 0; uniformIndex < technique->uniformCount; uniformIndex++ )
					{
						const GltfUniform_t * uniform = &technique->uniforms[uniformIndex];
						switch ( uniform->semantic )
						{
							case GLTF_UNIFORM_SEMANTIC_DEFAULT_VALUE:					GltfScene_SetUniformValue( &command, uniform, &uniform->defaultValue ); break;
							case GLTF_UNIFORM_SEMANTIC_VIEW:							GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, uniform->index, &builtin.viewMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_VIEW_INVERSE:					GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, uniform->index, &builtin.viewInverseMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_PROJECTION:						GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, uniform->index, &builtin.projectionMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_PROJECTION_INVERSE:				GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, uniform->index, &builtin.projectionInverseMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_LOCAL:							GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, uniform->index, &builtin.localMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_MODEL:							GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, uniform->index, &builtin.modelMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE:					GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, uniform->index, &builtin.modelInverseMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_MODEL_INVERSE_TRANSPOSE:			GpuGraphicsCommand_SetParmFloatMatrix3x3( &command, uniform->index, &builtin.modelInverseTransposeMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_MODEL_VIEW:						GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, uniform->index, &builtin.modelViewMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE:				GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, uniform->index, &builtin.modelViewInverseMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_INVERSE_TRANSPOSE:	GpuGraphicsCommand_SetParmFloatMatrix3x3( &command, uniform->index, &builtin.modelViewInverseTransposeMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION:			GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, uniform->index, &builtin.modelViewProjectionMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_MODEL_VIEW_PROJECTION_INVERSE:	GpuGraphicsCommand_SetParmFloatMatrix4x4( &command, uniform->index, &builtin.modelViewProjectionInverseMatrix ); break;
							case GLTF_UNIFORM_SEMANTIC_VIEWPORT:						GpuGraphicsCommand_SetParmFloatVector4( &command, uniform->index, &builtin.viewport ); break;
							case GLTF_UNIFORM_SEMANTIC_JOINTMATRIX:						GpuGraphicsCommand_SetParmBufferUniform( &command, uniform->index, jointBuffer ); break;
							default: break;
						}
					}

					for ( int valueIndex = 0; valueIndex < surface->material->valueCount; valueIndex++ )
					{
						const GltfMaterialValue_t * value = &surface->material->values[valueIndex];
						if ( value->uniform != NULL )
						{
							GltfScene_SetUniformValue( &command, value->uniform, &value->value );
						}
					}

					GpuCommandBuffer_SubmitGraphicsCommand( commandBuffer, &command );
				}
			}
		}
	}
}

#endif // USE_GLTF == 1

/*
================================================================================================================================

Stats

================================================================================================================================
*/

static void PrintStats( const GpuWindow_t * window )
{
	Print( "--------------------------------\n" );
	Print( "OS     : %s\n", GetOSVersion() );
	Print( "CPU    : %s\n", GetCPUVersion() );
	Print( "GPU    : %s\n", glGetString( GL_RENDERER ) );
	Print( "OpenGL : %s\n", glGetString( GL_VERSION ) );
	Print( "Mode   : %s %dx%d %1.0f Hz\n", window->windowFullscreen ? "fullscreen" : "windowed",
					window->windowWidth, window->windowHeight, window->windowRefreshRate );
	Print( "--------------------------------\n" );
}

/*
================================================================================================================================

Dump GLSL

================================================================================================================================
*/

static void WriteTextFile( const char * path, const char * text )
{
	FILE * fp = fopen( path, "wb" );
	if ( fp == NULL )
	{
		Print( "Failed to write %s\n", path );
		return;
	}
	fwrite( text, strlen( text ), 1, fp );
	fclose( fp );
	Print( "Wrote %s\n", path );
}

typedef struct
{
	const char * fileName;
	const char * extension;
	const char * glsl;
} glsl_t;

static void DumpGLSL()
{
	glsl_t glsl[] =
	{
		{ "barGraphVertexProgram",					"vert",	barGraphVertexProgramGLSL },
		{ "barGraphFragmentProgram",				"frag",	barGraphFragmentProgramGLSL },
		{ "timeWarpSpatialVertexProgram",			"vert",	timeWarpSpatialVertexProgramGLSL },
		{ "timeWarpSpatialFragmentProgram",			"frag",	timeWarpSpatialFragmentProgramGLSL },
		{ "timeWarpChromaticVertexProgram",			"vert",	timeWarpChromaticVertexProgramGLSL },
		{ "timeWarpChromaticFragmentProgram",		"frag",	timeWarpChromaticFragmentProgramGLSL },
		{ "flatShadedVertexProgram",				"vert",	flatShadedVertexProgramGLSL },
		{ "flatShadedMultiViewVertexProgram",		"vert",	flatShadedMultiViewVertexProgramGLSL },
		{ "flatShadedFragmentProgram",				"frag",	flatShadedFragmentProgramGLSL },
		{ "normalMappedVertexProgram",				"vert",	normalMappedVertexProgramGLSL },
		{ "normalMappedMultiViewVertexProgram",		"vert",	normalMappedMultiViewVertexProgramGLSL },
		{ "normalMapped100LightsFragmentProgram",	"frag",	normalMapped100LightsFragmentProgramGLSL },
		{ "normalMapped1000LightsFragmentProgram",	"frag",	normalMapped1000LightsFragmentProgramGLSL },
		{ "normalMapped2000LightsFragmentProgram",	"frag",	normalMapped2000LightsFragmentProgramGLSL },

#if OPENGL_COMPUTE_ENABLED == 1
		{ "barGraphComputeProgram",					"comp", barGraphComputeProgramGLSL },
		{ "timeWarpTransformComputeProgram",		"comp", timeWarpTransformComputeProgramGLSL },
		{ "timeWarpSpatialComputeProgram",			"comp", timeWarpSpatialComputeProgramGLSL },
		{ "timeWarpChromaticComputeProgram",		"comp", timeWarpChromaticComputeProgramGLSL },
#endif
	};

	char path[1024];
	char batchFileBin[4096];
	char batchFileHex[4096];
	size_t batchFileBinLength = 0;
	size_t batchFileHexLength = 0;
	for ( size_t i = 0; i < ARRAY_SIZE( glsl ); i++ )
	{
		sprintf( path, "glsl/%sGLSL.%s", glsl[i].fileName, glsl[i].extension );
		WriteTextFile( path, glsl[i].glsl );

		batchFileBinLength += sprintf( batchFileBin + batchFileBinLength,
									"glslangValidator -G -o %sSPIRV.spv %sGLSL.%s\r\n",
									glsl[i].fileName, glsl[i].fileName, glsl[i].extension );
		batchFileHexLength += sprintf( batchFileHex + batchFileHexLength,
									"glslangValidator -G -x -o %sSPIRV.h %sGLSL.%s\r\n",
									glsl[i].fileName, glsl[i].fileName, glsl[i].extension );
	}

	WriteTextFile( "glsl/spirv_bin.bat", batchFileBin );
	WriteTextFile( "glsl/spirv_hex.bat", batchFileHex );
}

/*
================================================================================================================================

Startup settings.

StartupSettings_t

static int StartupSettings_StringToLevel( const char * string, const int maxLevels );
static int StartupSettings_StringToRenderMode( const char * string );
static int StartupSettings_StringToTimeWarpImplementation( const char * string );

================================================================================================================================
*/

typedef enum
{
	RENDER_MODE_ASYNC_TIME_WARP,
	RENDER_MODE_TIME_WARP,
	RENDER_MODE_SCENE,
	RENDER_MODE_MAX
} RenderMode_t;

typedef struct
{
	bool						fullscreen;
	bool						simulationPaused;
	bool						headRotationDisabled;
	int							displayResolutionLevel;
	int							eyeImageResolutionLevel;
	int							eyeImageSamplesLevel;
	int							drawCallLevel;
	int							triangleLevel;
	int							fragmentLevel;
	bool						useMultiView;
	bool						correctChromaticAberration;
	bool						hideGraphs;
	TimeWarpImplementation_t	timeWarpImplementation;
	RenderMode_t				renderMode;
	Microseconds_t				startupTimeMicroseconds;
	Microseconds_t				noVSyncMicroseconds;
	Microseconds_t				noLogMicroseconds;
} StartupSettings_t;

static int StartupSettings_StringToLevel( const char * string, const int maxLevels )
{
	const int level = atoi( string );
	return ( level >= 0 ) ? ( ( level < maxLevels ) ? level : maxLevels - 1 ) : 0;
}

static int StartupSettings_StringToRenderMode( const char * string )
{
	return	( ( strcmp( string, "atw" ) == 0 ) ? RENDER_MODE_ASYNC_TIME_WARP:
			( ( strcmp( string, "tw"  ) == 0 ) ? RENDER_MODE_TIME_WARP :
			RENDER_MODE_SCENE ) );
}

static int StartupSettings_StringToTimeWarpImplementation( const char * string )
{
	return	( ( strcmp( string, "graphics" ) == 0 ) ? TIMEWARP_IMPLEMENTATION_GRAPHICS :
			( ( strcmp( string, "compute"  ) == 0 ) ? TIMEWARP_IMPLEMENTATION_COMPUTE :
			TIMEWARP_IMPLEMENTATION_GRAPHICS ) );
}

/*
================================================================================================================================

Asynchronous time warp.

================================================================================================================================
*/

enum
{
	QUEUE_INDEX_TIMEWARP	= 0,
	QUEUE_INDEX_SCENE		= 1
};

// Two should be enough but use three to make absolutely sure there are no stalls due to buffer locking.
#define NUM_EYE_BUFFERS			3

#if defined( OS_ANDROID )
#define WINDOW_RESOLUTION( x, fullscreen )	(x)		// always fullscreen
#else
#define WINDOW_RESOLUTION( x, fullscreen )	( (fullscreen) ? (x) : ROUNDUP( x / 2, 8 ) )
#endif

typedef struct
{
	Signal_t				initialized;
	GpuContext_t *			shareContext;
	TimeWarp_t *			timeWarp;
	SceneSettings_t *		sceneSettings;
	GpuWindowInput_t *		input;

	volatile bool			terminate;
	volatile bool			openFrameLog;
} SceneThreadData_t;

void SceneThread_Render( SceneThreadData_t * threadData )
{
	Thread_SetAffinity( THREAD_AFFINITY_BIG_CORES );

	GpuContext_t context;
	GpuContext_CreateShared( &context, threadData->shareContext, QUEUE_INDEX_SCENE );
	GpuContext_SetCurrent( &context );

	const int resolutionTable[] =
	{
		1024,
		1536,
		2048,
		4096
	};
	const int resolution = resolutionTable[threadData->sceneSettings->eyeImageResolutionLevel];

	const GpuSampleCount_t sampleCountTable[] =
	{
		GPU_SAMPLE_COUNT_1,
		GPU_SAMPLE_COUNT_2,
		GPU_SAMPLE_COUNT_4,
		GPU_SAMPLE_COUNT_8
	};
	const GpuSampleCount_t sampleCount = sampleCountTable[threadData->sceneSettings->eyeImageSamplesLevel];

	GpuRenderPass_t renderPass;
	GpuRenderPass_Create( &context, &renderPass, GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_DEPTH_FORMAT_D24,
							sampleCount, GPU_RENDERPASS_TYPE_INLINE,
							GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER |
							GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER );

	GpuFramebuffer_t framebuffer;
	GpuFramebuffer_CreateFromTextureArrays( &context, &framebuffer, &renderPass,
				resolution, resolution, NUM_EYES, NUM_EYE_BUFFERS, threadData->sceneSettings->useMultiView );

	const int numPasses = threadData->sceneSettings->useMultiView ? 1 : NUM_EYES;

	GpuCommandBuffer_t eyeCommandBuffer[NUM_EYES];
	GpuTimer_t eyeTimer[NUM_EYES];

	for ( int eye = 0; eye < numPasses; eye++ )
	{
		GpuCommandBuffer_Create( &context, &eyeCommandBuffer[eye], GPU_COMMAND_BUFFER_TYPE_PRIMARY, NUM_EYE_BUFFERS );
		GpuTimer_Create( &context, &eyeTimer[eye] );
	}

	const BodyInfo_t * bodyInfo = GetDefaultBodyInfo();

	ViewState_t viewState;
	ViewState_Init( &viewState, bodyInfo->interpupillaryDistance );

#if USE_GLTF == 1
	GltfScene_t scene;
	GltfScene_CreateFromFile( &context, &scene, "models.gltf", &renderPass );
#else
	PerfScene_t scene;
	PerfScene_Create( &context, &scene, threadData->sceneSettings, &renderPass );
#endif

	Signal_Raise( &threadData->initialized );

	for ( int frameIndex = 0; !threadData->terminate; frameIndex++ )
	{
		if ( threadData->openFrameLog )
		{
			threadData->openFrameLog = false;
			FrameLog_Open( OUTPUT_PATH "framelog_scene.txt", 10 );
		}

		const Microseconds_t nextDisplayTime = TimeWarp_GetPredictedDisplayTime( threadData->timeWarp, frameIndex );

#if USE_GLTF == 1
		GltfScene_Simulate( &scene, &viewState, threadData->input, nextDisplayTime );
#else
		PerfScene_Simulate( &scene, &viewState, nextDisplayTime );
#endif

		FrameLog_BeginFrame();

		const Microseconds_t t0 = GetTimeMicroseconds();

		GpuTexture_t * eyeTexture[NUM_EYES] = { 0 };
		GpuFence_t * eyeCompletionFence[NUM_EYES] = { 0 };
		int eyeArrayLayer[NUM_EYES] = { 0, 1 };

		for ( int eye = 0; eye < numPasses; eye++ )
		{
			const ScreenRect_t screenRect = GpuFramebuffer_GetRect( &framebuffer );

			GpuCommandBuffer_BeginPrimary( &eyeCommandBuffer[eye] );
			GpuCommandBuffer_BeginFramebuffer( &eyeCommandBuffer[eye], &framebuffer, eye, GPU_TEXTURE_USAGE_COLOR_ATTACHMENT );

#if USE_GLTF == 1
			GltfScene_UpdateBuffers( &eyeCommandBuffer[eye], &scene, &viewState, eye );
#else
			PerfScene_UpdateBuffers( &eyeCommandBuffer[eye], &scene, &viewState, eye );
#endif

			GpuCommandBuffer_BeginTimer( &eyeCommandBuffer[eye], &eyeTimer[eye] );
			GpuCommandBuffer_BeginRenderPass( &eyeCommandBuffer[eye], &renderPass, &framebuffer, &screenRect );

			GpuCommandBuffer_SetViewport( &eyeCommandBuffer[eye], &screenRect );
			GpuCommandBuffer_SetScissor( &eyeCommandBuffer[eye], &screenRect );

#if USE_GLTF == 1
			GltfScene_Render( &eyeCommandBuffer[eye], &scene, &viewState, eye );
#else
			PerfScene_Render( &eyeCommandBuffer[eye], &scene );
#endif

			GpuCommandBuffer_EndRenderPass( &eyeCommandBuffer[eye], &renderPass );
			GpuCommandBuffer_EndTimer( &eyeCommandBuffer[eye], &eyeTimer[eye] );

			GpuCommandBuffer_EndFramebuffer( &eyeCommandBuffer[eye], &framebuffer, eye, GPU_TEXTURE_USAGE_SAMPLED );
			GpuCommandBuffer_EndPrimary( &eyeCommandBuffer[eye] );

			eyeTexture[eye] = GpuFramebuffer_GetColorTexture( &framebuffer );
			eyeCompletionFence[eye] = GpuCommandBuffer_SubmitPrimary( &eyeCommandBuffer[eye] );
		}

		if ( threadData->sceneSettings->useMultiView )
		{
			eyeTexture[1] = eyeTexture[0];
			eyeCompletionFence[1] = eyeCompletionFence[0];
		}

		const Microseconds_t t1 = GetTimeMicroseconds();

		const float eyeTexturesCpuTime = ( t1 - t0 ) * ( 1.0f / 1000.0f );
		const float eyeTexturesGpuTime = GpuTimer_GetMilliseconds( &eyeTimer[0] ) + GpuTimer_GetMilliseconds( &eyeTimer[1] );

		FrameLog_EndFrame( eyeTexturesCpuTime, eyeTexturesGpuTime, GPU_TIMER_FRAMES_DELAYED );

		Matrix4x4f_t projectionMatrix;
		Matrix4x4f_CreateProjectionFov( &projectionMatrix, 80.0f, 80.0f, 0.0f, 0.0f, 0.1f, 0.0f );

		TimeWarp_SubmitFrame( threadData->timeWarp, frameIndex, nextDisplayTime,
								&viewState.hmdViewMatrix, &projectionMatrix,
								eyeTexture, eyeCompletionFence, eyeArrayLayer,
								eyeTexturesCpuTime, eyeTexturesGpuTime );
	}

#if USE_GLTF == 1
	GltfScene_Destroy( &context, &scene );
#else
	PerfScene_Destroy( &context, &scene );
#endif

	for ( int eye = 0; eye < numPasses; eye++ )
	{
		GpuTimer_Destroy( &context, &eyeTimer[eye] );
		GpuCommandBuffer_Destroy( &context, &eyeCommandBuffer[eye] );
	}

	GpuFramebuffer_Destroy( &context, &framebuffer );
	GpuRenderPass_Destroy( &context, &renderPass );
	GpuContext_Destroy( &context );
}

void SceneThread_Create( Thread_t * sceneThread, SceneThreadData_t * sceneThreadData,
							GpuWindow_t * window, TimeWarp_t * timeWarp, SceneSettings_t * sceneSettings )
{
	Signal_Create( &sceneThreadData->initialized, true );
	sceneThreadData->shareContext = &window->context;
	sceneThreadData->timeWarp = timeWarp;
	sceneThreadData->sceneSettings = sceneSettings;
	sceneThreadData->input = &window->input;
	sceneThreadData->terminate = false;
	sceneThreadData->openFrameLog = false;

	// On MacOS context creation fails if the share context is current on another thread.
	GpuContext_UnsetCurrent( &window->context );

	Thread_Create( sceneThread, "atw:scene", (threadFunction_t) SceneThread_Render, sceneThreadData );
	Thread_Signal( sceneThread );
	Signal_Wait( &sceneThreadData->initialized, -1 );

	GpuContext_SetCurrent( &window->context );
}

void SceneThread_Destroy( Thread_t * sceneThread, SceneThreadData_t * sceneThreadData )
{
	sceneThreadData->terminate = true;
	// The following assumes the time warp thread is blocked when this function is called.
	Signal_Raise( &sceneThreadData->timeWarp->newEyeTexturesConsumed );
	Signal_Raise( &sceneThreadData->timeWarp->vsyncSignal );
	Signal_Destroy( &sceneThreadData->initialized );
	Thread_Destroy( sceneThread );
}

bool RenderAsyncTimeWarp( StartupSettings_t * startupSettings )
{
	Thread_SetAffinity( THREAD_AFFINITY_BIG_CORES );
	Thread_SetRealTimePriority( 1 );

	DriverInstance_t instance;
	DriverInstance_Create( &instance );

	const GpuQueueInfo_t queueInfo =
	{
		2,
		GPU_QUEUE_PROPERTY_GRAPHICS | GPU_QUEUE_PROPERTY_COMPUTE,
		{ GPU_QUEUE_PRIORITY_HIGH, GPU_QUEUE_PRIORITY_MEDIUM }
	};

	GpuWindow_t window;
	GpuWindow_Create( &window, &instance, &queueInfo, QUEUE_INDEX_TIMEWARP,
						GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_DEPTH_FORMAT_NONE, GPU_SAMPLE_COUNT_1,
						WINDOW_RESOLUTION( displayResolutionTable[startupSettings->displayResolutionLevel * 2 + 0], startupSettings->fullscreen ),
						WINDOW_RESOLUTION( displayResolutionTable[startupSettings->displayResolutionLevel * 2 + 1], startupSettings->fullscreen ),
						startupSettings->fullscreen );

	int swapInterval = ( startupSettings->noVSyncMicroseconds <= 0 );
	GpuWindow_SwapInterval( &window, swapInterval );

	TimeWarp_t timeWarp;
	TimeWarp_Create( &timeWarp, &window );
	TimeWarp_SetBarGraphState( &timeWarp, startupSettings->hideGraphs ? BAR_GRAPH_HIDDEN : BAR_GRAPH_VISIBLE );
	TimeWarp_SetImplementation( &timeWarp, startupSettings->timeWarpImplementation );
	TimeWarp_SetChromaticAberrationCorrection( &timeWarp, startupSettings->correctChromaticAberration );
	TimeWarp_SetMultiView( &timeWarp, startupSettings->useMultiView );
	TimeWarp_SetDisplayResolutionLevel( &timeWarp, startupSettings->displayResolutionLevel );
	TimeWarp_SetEyeImageResolutionLevel( &timeWarp, startupSettings->eyeImageResolutionLevel );
	TimeWarp_SetEyeImageSamplesLevel( &timeWarp, startupSettings->eyeImageSamplesLevel );
	TimeWarp_SetDrawCallLevel( &timeWarp, startupSettings->drawCallLevel );
	TimeWarp_SetTriangleLevel( &timeWarp, startupSettings->triangleLevel );
	TimeWarp_SetFragmentLevel( &timeWarp, startupSettings->fragmentLevel );

	SceneSettings_t sceneSettings;
	SceneSettings_Init( &window.context, &sceneSettings );
	SceneSettings_SetSimulationPaused( &sceneSettings, startupSettings->simulationPaused );
	SceneSettings_SetMultiView( &sceneSettings, startupSettings->useMultiView );
	SceneSettings_SetDisplayResolutionLevel( &sceneSettings, startupSettings->displayResolutionLevel );
	SceneSettings_SetEyeImageResolutionLevel( &sceneSettings, startupSettings->eyeImageResolutionLevel );
	SceneSettings_SetEyeImageSamplesLevel( &sceneSettings, startupSettings->eyeImageSamplesLevel );
	SceneSettings_SetDrawCallLevel( &sceneSettings, startupSettings->drawCallLevel );
	SceneSettings_SetTriangleLevel( &sceneSettings, startupSettings->triangleLevel );
	SceneSettings_SetFragmentLevel( &sceneSettings, startupSettings->fragmentLevel );

	Thread_t sceneThread;
	SceneThreadData_t sceneThreadData;
	SceneThread_Create( &sceneThread, &sceneThreadData, &window, &timeWarp, &sceneSettings );

	hmd_headRotationDisabled = startupSettings->headRotationDisabled;

	Microseconds_t startupTimeMicroseconds = startupSettings->startupTimeMicroseconds;
	Microseconds_t noVSyncMicroseconds = startupSettings->noVSyncMicroseconds;
	Microseconds_t noLogMicroseconds = startupSettings->noLogMicroseconds;

	Thread_SetName( "atw:timewarp" );

	bool exit = false;
	while ( !exit )
	{
		const Microseconds_t time = GetTimeMicroseconds();

		const GpuWindowEvent_t handleEvent = GpuWindow_ProcessEvents( &window );
		if ( handleEvent == GPU_WINDOW_EVENT_ACTIVATED )
		{
			PrintStats( &window );
		}
		else if ( handleEvent == GPU_WINDOW_EVENT_EXIT )
		{
			exit = true;
			break;
		}

		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_ESCAPE ) )
		{
			GpuWindow_Exit( &window );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_Z ) )
		{
			startupSettings->renderMode = (RenderMode_t) ( ( startupSettings->renderMode + 1 ) % RENDER_MODE_MAX );
			break;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_F ) )
		{
			startupSettings->fullscreen = !startupSettings->fullscreen;
			break;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_V ) ||
			( noVSyncMicroseconds > 0 && time - startupTimeMicroseconds > noVSyncMicroseconds ) )
		{
			swapInterval = !swapInterval;
			GpuWindow_SwapInterval( &window, swapInterval );
			noVSyncMicroseconds = 0;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_L ) ||
			( noLogMicroseconds > 0 && time - startupTimeMicroseconds > noLogMicroseconds ) )
		{
			FrameLog_Open( OUTPUT_PATH "framelog_timewarp.txt", 10 );
			sceneThreadData.openFrameLog = true;
			noLogMicroseconds = 0;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_H ) )
		{
			hmd_headRotationDisabled = !hmd_headRotationDisabled;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_P ) )
		{
			SceneSettings_ToggleSimulationPaused( &sceneSettings );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_G ) )
		{
			TimeWarp_CycleBarGraphState( &timeWarp );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_R ) )
		{
			SceneSettings_CycleDisplayResolutionLevel( &sceneSettings );
			startupSettings->displayResolutionLevel = sceneSettings.displayResolutionLevel;
			break;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_B ) )
		{
			SceneSettings_CycleEyeImageResolutionLevel( &sceneSettings );
			startupSettings->eyeImageResolutionLevel = sceneSettings.eyeImageResolutionLevel;
			break;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_S ) )
		{
			SceneSettings_CycleEyeImageSamplesLevel( &sceneSettings );
			startupSettings->eyeImageSamplesLevel = sceneSettings.eyeImageSamplesLevel;
			break;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_Q ) )
		{
			SceneSettings_CycleDrawCallLevel( &sceneSettings );
			TimeWarp_SetDrawCallLevel( &timeWarp, SceneSettings_GetDrawCallLevel( &sceneSettings ) );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_W ) )
		{
			SceneSettings_CycleTriangleLevel( &sceneSettings );
			TimeWarp_SetTriangleLevel( &timeWarp, SceneSettings_GetTriangleLevel( &sceneSettings ) );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_E ) )
		{
			SceneSettings_CycleFragmentLevel( &sceneSettings );
			TimeWarp_SetFragmentLevel( &timeWarp, SceneSettings_GetFragmentLevel( &sceneSettings ) );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_I ) )
		{
			TimeWarp_CycleImplementation( &timeWarp );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_C ) )
		{
			TimeWarp_ToggleChromaticAberrationCorrection( &timeWarp );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_M ) )
		{
			if ( glExtensions.multi_view )
			{
				SceneSettings_ToggleMultiView( &sceneSettings );
				break;
			}
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_D ) )
		{
			DumpGLSL();
		}

		if ( window.windowActive )
		{
			TimeWarp_Render( &timeWarp );
		}
	}

	GpuContext_WaitIdle( &window.context );
	SceneThread_Destroy( &sceneThread, &sceneThreadData );
	TimeWarp_Destroy( &timeWarp, &window );
	GpuWindow_Destroy( &window );
	DriverInstance_Destroy( &instance );

	return exit;
}

/*
================================================================================================================================

Time warp rendering test.

================================================================================================================================
*/

bool RenderTimeWarp( StartupSettings_t * startupSettings )
{
	Thread_SetAffinity( THREAD_AFFINITY_BIG_CORES );

	DriverInstance_t instance;
	DriverInstance_Create( &instance );

	const GpuQueueInfo_t queueInfo =
	{
		1,
		GPU_QUEUE_PROPERTY_GRAPHICS | GPU_QUEUE_PROPERTY_COMPUTE,
		{ GPU_QUEUE_PRIORITY_MEDIUM }
	};

	GpuWindow_t window;
	GpuWindow_Create( &window, &instance, &queueInfo, 0,
						GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_DEPTH_FORMAT_NONE, GPU_SAMPLE_COUNT_1,
						WINDOW_RESOLUTION( displayResolutionTable[startupSettings->displayResolutionLevel * 2 + 0], startupSettings->fullscreen ),
						WINDOW_RESOLUTION( displayResolutionTable[startupSettings->displayResolutionLevel * 2 + 1], startupSettings->fullscreen ),
						startupSettings->fullscreen );

	int swapInterval = ( startupSettings->noVSyncMicroseconds <= 0 );
	GpuWindow_SwapInterval( &window, swapInterval );

	TimeWarp_t timeWarp;
	TimeWarp_Create( &timeWarp, &window );
	TimeWarp_SetBarGraphState( &timeWarp, startupSettings->hideGraphs ? BAR_GRAPH_HIDDEN : BAR_GRAPH_VISIBLE );
	TimeWarp_SetImplementation( &timeWarp, startupSettings->timeWarpImplementation );
	TimeWarp_SetChromaticAberrationCorrection( &timeWarp, startupSettings->correctChromaticAberration );
	TimeWarp_SetDisplayResolutionLevel( &timeWarp, startupSettings->displayResolutionLevel );

	hmd_headRotationDisabled = startupSettings->headRotationDisabled;

	Microseconds_t startupTimeMicroseconds = startupSettings->startupTimeMicroseconds;
	Microseconds_t noVSyncMicroseconds = startupSettings->noVSyncMicroseconds;
	Microseconds_t noLogMicroseconds = startupSettings->noLogMicroseconds;

	Thread_SetName( "atw:timewarp" );

	bool exit = false;
	while ( !exit )
	{
		const Microseconds_t time = GetTimeMicroseconds();

		const GpuWindowEvent_t handleEvent = GpuWindow_ProcessEvents( &window );
		if ( handleEvent == GPU_WINDOW_EVENT_ACTIVATED )
		{
			PrintStats( &window );
		}
		else if ( handleEvent == GPU_WINDOW_EVENT_EXIT )
		{
			exit = true;
		}

		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_ESCAPE ) )
		{
			GpuWindow_Exit( &window );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_Z ) )
		{
			startupSettings->renderMode = (RenderMode_t) ( ( startupSettings->renderMode + 1 ) % RENDER_MODE_MAX );
			break;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_F ) )
		{
			startupSettings->fullscreen = !startupSettings->fullscreen;
			break;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_V ) ||
			( noVSyncMicroseconds > 0 && time - startupTimeMicroseconds > noVSyncMicroseconds ) )
		{
			swapInterval = !swapInterval;
			GpuWindow_SwapInterval( &window, swapInterval );
			noVSyncMicroseconds = 0;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_L ) ||
			( noLogMicroseconds > 0 && time - startupTimeMicroseconds > noLogMicroseconds ) )
		{
			FrameLog_Open( OUTPUT_PATH "framelog_timewarp.txt", 10 );
			noLogMicroseconds = 0;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_H ) )
		{
			hmd_headRotationDisabled = !hmd_headRotationDisabled;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_G ) )
		{
			TimeWarp_CycleBarGraphState( &timeWarp );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_I ) )
		{
			TimeWarp_CycleImplementation( &timeWarp );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_C ) )
		{
			TimeWarp_ToggleChromaticAberrationCorrection( &timeWarp );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_D ) )
		{
			DumpGLSL();
		}

		if ( window.windowActive )
		{
			TimeWarp_Render( &timeWarp );
		}
	}

	GpuContext_WaitIdle( &window.context );
	TimeWarp_Destroy( &timeWarp, &window );
	GpuWindow_Destroy( &window );
	DriverInstance_Destroy( &instance );

	return exit;
}

/*
================================================================================================================================

Scene rendering test.

================================================================================================================================
*/

bool RenderScene( StartupSettings_t * startupSettings )
{
	Thread_SetAffinity( THREAD_AFFINITY_BIG_CORES );

	DriverInstance_t instance;
	DriverInstance_Create( &instance );

	const GpuSampleCount_t sampleCountTable[] =
	{
		GPU_SAMPLE_COUNT_1,
		GPU_SAMPLE_COUNT_2,
		GPU_SAMPLE_COUNT_4,
		GPU_SAMPLE_COUNT_8
	};
	const GpuSampleCount_t sampleCount = sampleCountTable[startupSettings->eyeImageSamplesLevel];

	const GpuQueueInfo_t queueInfo =
	{
		1,
		GPU_QUEUE_PROPERTY_GRAPHICS,
		{ GPU_QUEUE_PRIORITY_MEDIUM }
	};

	GpuWindow_t window;
	GpuWindow_Create( &window, &instance, &queueInfo, 0,
						GPU_SURFACE_COLOR_FORMAT_R8G8B8A8, GPU_SURFACE_DEPTH_FORMAT_D24, sampleCount,
						WINDOW_RESOLUTION( displayResolutionTable[startupSettings->displayResolutionLevel * 2 + 0], startupSettings->fullscreen ),
						WINDOW_RESOLUTION( displayResolutionTable[startupSettings->displayResolutionLevel * 2 + 1], startupSettings->fullscreen ),
						startupSettings->fullscreen );

	int swapInterval = ( startupSettings->noVSyncMicroseconds <= 0 );
	GpuWindow_SwapInterval( &window, swapInterval );

	GpuRenderPass_t renderPass;
	GpuRenderPass_Create( &window.context, &renderPass, window.colorFormat, window.depthFormat,
							sampleCount, GPU_RENDERPASS_TYPE_INLINE,
							GPU_RENDERPASS_FLAG_CLEAR_COLOR_BUFFER |
							GPU_RENDERPASS_FLAG_CLEAR_DEPTH_BUFFER );

	GpuFramebuffer_t framebuffer;
	GpuFramebuffer_CreateFromSwapchain( &window, &framebuffer, &renderPass );

	GpuCommandBuffer_t commandBuffer;
	GpuCommandBuffer_Create( &window.context, &commandBuffer, GPU_COMMAND_BUFFER_TYPE_PRIMARY, GpuFramebuffer_GetBufferCount( &framebuffer ) );

	GpuTimer_t timer;
	GpuTimer_Create( &window.context, &timer );

	BarGraph_t frameCpuTimeBarGraph;
	BarGraph_CreateVirtualRect( &window.context, &frameCpuTimeBarGraph, &renderPass, &frameCpuTimeBarGraphRect, 64, 1, &colorDarkGrey );

	BarGraph_t frameGpuTimeBarGraph;
	BarGraph_CreateVirtualRect( &window.context, &frameGpuTimeBarGraph, &renderPass, &frameGpuTimeBarGraphRect, 64, 1, &colorDarkGrey );

	SceneSettings_t sceneSettings;
	SceneSettings_Init( &window.context, &sceneSettings );
	SceneSettings_SetSimulationPaused( &sceneSettings, startupSettings->simulationPaused );
	SceneSettings_SetDisplayResolutionLevel( &sceneSettings, startupSettings->displayResolutionLevel );
	SceneSettings_SetEyeImageResolutionLevel( &sceneSettings, startupSettings->eyeImageResolutionLevel );
	SceneSettings_SetEyeImageSamplesLevel( &sceneSettings, startupSettings->eyeImageSamplesLevel );
	SceneSettings_SetDrawCallLevel( &sceneSettings, startupSettings->drawCallLevel );
	SceneSettings_SetTriangleLevel( &sceneSettings, startupSettings->triangleLevel );
	SceneSettings_SetFragmentLevel( &sceneSettings, startupSettings->fragmentLevel );

	ViewState_t viewState;
	ViewState_Init( &viewState, 0.0f );

#if USE_GLTF == 1
	GltfScene_t scene;
	GltfScene_CreateFromFile( &window.context, &scene, "models.gltf", &renderPass );
#else
	PerfScene_t scene;
	PerfScene_Create( &window.context, &scene, &sceneSettings, &renderPass );
#endif

	hmd_headRotationDisabled = startupSettings->headRotationDisabled;

	Microseconds_t startupTimeMicroseconds = startupSettings->startupTimeMicroseconds;
	Microseconds_t noVSyncMicroseconds = startupSettings->noVSyncMicroseconds;
	Microseconds_t noLogMicroseconds = startupSettings->noLogMicroseconds;

	Thread_SetName( "atw:scene" );

	bool exit = false;
	while ( !exit )
	{
		const Microseconds_t time = GetTimeMicroseconds();

		const GpuWindowEvent_t handleEvent = GpuWindow_ProcessEvents( &window );
		if ( handleEvent == GPU_WINDOW_EVENT_ACTIVATED )
		{
			PrintStats( &window );
		}
		else if ( handleEvent == GPU_WINDOW_EVENT_EXIT )
		{
			exit = true;
			break;
		}

		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_ESCAPE ) )
		{
			GpuWindow_Exit( &window );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_Z ) )
		{
			startupSettings->renderMode = (RenderMode_t) ( ( startupSettings->renderMode + 1 ) % RENDER_MODE_MAX );
			break;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_F ) )
		{
			startupSettings->fullscreen = !startupSettings->fullscreen;
			break;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_V ) ||
			( noVSyncMicroseconds > 0 && time - startupTimeMicroseconds > noVSyncMicroseconds ) )
		{
			swapInterval = !swapInterval;
			GpuWindow_SwapInterval( &window, swapInterval );
			noVSyncMicroseconds = 0;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_L ) ||
			( noLogMicroseconds > 0 && time - startupTimeMicroseconds > noLogMicroseconds ) )
		{
			FrameLog_Open( OUTPUT_PATH "framelog_scene.txt", 10 );
			noLogMicroseconds = 0;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_H ) )
		{
			hmd_headRotationDisabled = !hmd_headRotationDisabled;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_P ) )
		{
			SceneSettings_ToggleSimulationPaused( &sceneSettings );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_R ) )
		{
			SceneSettings_CycleDisplayResolutionLevel( &sceneSettings );
			startupSettings->displayResolutionLevel = sceneSettings.displayResolutionLevel;
			break;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_B ) )
		{
			SceneSettings_CycleEyeImageResolutionLevel( &sceneSettings );
			startupSettings->eyeImageResolutionLevel = sceneSettings.eyeImageResolutionLevel;
			break;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_S ) )
		{
			SceneSettings_CycleEyeImageSamplesLevel( &sceneSettings );
			startupSettings->eyeImageSamplesLevel = sceneSettings.eyeImageSamplesLevel;
			break;
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_Q ) )
		{
			SceneSettings_CycleDrawCallLevel( &sceneSettings );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_W ) )
		{
			SceneSettings_CycleTriangleLevel( &sceneSettings );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_E ) )
		{
			SceneSettings_CycleFragmentLevel( &sceneSettings );
		}
		if ( GpuWindowInput_ConsumeKeyboardKey( &window.input, KEY_D ) )
		{
			DumpGLSL();
		}

		if ( window.windowActive )
		{
			const Microseconds_t nextSwapTime = GpuWindow_GetNextSwapTimeMicroseconds( &window );

#if USE_GLTF == 1
			GltfScene_Simulate( &scene, &viewState, &window.input, nextSwapTime );
#else
			PerfScene_Simulate( &scene, &viewState, nextSwapTime );
#endif

			FrameLog_BeginFrame();

			const Microseconds_t t0 = GetTimeMicroseconds();

			const ScreenRect_t screenRect = GpuFramebuffer_GetRect( &framebuffer );

			GpuCommandBuffer_BeginPrimary( &commandBuffer );
			GpuCommandBuffer_BeginFramebuffer( &commandBuffer, &framebuffer, 0, GPU_TEXTURE_USAGE_COLOR_ATTACHMENT );

#if USE_GLTF == 1
			GltfScene_UpdateBuffers( &commandBuffer, &scene, &viewState, 0 );
#else
			PerfScene_UpdateBuffers( &commandBuffer, &scene, &viewState, 0 );
#endif

			BarGraph_UpdateGraphics( &commandBuffer, &frameCpuTimeBarGraph );
			BarGraph_UpdateGraphics( &commandBuffer, &frameGpuTimeBarGraph );

			GpuCommandBuffer_BeginTimer( &commandBuffer, &timer );
			GpuCommandBuffer_BeginRenderPass( &commandBuffer, &renderPass, &framebuffer, &screenRect );

			GpuCommandBuffer_SetViewport( &commandBuffer, &screenRect );
			GpuCommandBuffer_SetScissor( &commandBuffer, &screenRect );

#if USE_GLTF == 1
			GltfScene_Render( &commandBuffer, &scene, &viewState, 0 );
#else
			PerfScene_Render( &commandBuffer, &scene );
#endif

			BarGraph_RenderGraphics( &commandBuffer, &frameCpuTimeBarGraph );
			BarGraph_RenderGraphics( &commandBuffer, &frameGpuTimeBarGraph );

			GpuCommandBuffer_EndRenderPass( &commandBuffer, &renderPass );
			GpuCommandBuffer_EndTimer( &commandBuffer, &timer );

			GpuCommandBuffer_EndFramebuffer( &commandBuffer, &framebuffer, 0, GPU_TEXTURE_USAGE_PRESENTATION );
			GpuCommandBuffer_EndPrimary( &commandBuffer );

			GpuCommandBuffer_SubmitPrimary( &commandBuffer );

			const Microseconds_t t1 = GetTimeMicroseconds();

			const float sceneCpuTimeMilliseconds = ( t1 - t0 ) * ( 1.0f / 1000.0f );
			const float sceneGpuTimeMilliseconds = GpuTimer_GetMilliseconds( &timer );

			FrameLog_EndFrame( sceneCpuTimeMilliseconds, sceneGpuTimeMilliseconds, GPU_TIMER_FRAMES_DELAYED );

			BarGraph_AddBar( &frameCpuTimeBarGraph, 0, sceneCpuTimeMilliseconds * window.windowRefreshRate * ( 1.0f / 1000.0f ), &colorGreen, true );
			BarGraph_AddBar( &frameGpuTimeBarGraph, 0, sceneGpuTimeMilliseconds * window.windowRefreshRate * ( 1.0f / 1000.0f ), &colorGreen, true );

			GpuWindow_SwapBuffers( &window );
		}
	}

#if USE_GLTF == 1
	GltfScene_Destroy( &window.context, &scene );
#else
	PerfScene_Destroy( &window.context, &scene );
#endif
	BarGraph_Destroy( &window.context, &frameGpuTimeBarGraph );
	BarGraph_Destroy( &window.context, &frameCpuTimeBarGraph );
	GpuTimer_Destroy( &window.context, &timer );
	GpuCommandBuffer_Destroy( &window.context, &commandBuffer );
	GpuFramebuffer_Destroy( &window.context, &framebuffer );
	GpuRenderPass_Destroy( &window.context, &renderPass );
	GpuWindow_Destroy( &window );
	DriverInstance_Destroy( &instance );

	return exit;
}

/*
================================================================================================================================

Startup

================================================================================================================================
*/

static int StartApplication( int argc, char * argv[] )
{
	StartupSettings_t startupSettings;
	memset( &startupSettings, 0, sizeof( startupSettings ) );
	startupSettings.startupTimeMicroseconds = GetTimeMicroseconds();
	
	for ( int i = 1; i < argc; i++ )
	{
		const char * arg = argv[i];
		if ( arg[0] == '-' ) { arg++; }

		if ( strcmp( arg, "f" ) == 0 && i + 0 < argc )		{ startupSettings.fullscreen = true; }
		else if ( strcmp( arg, "v" ) == 0 && i + 1 < argc )	{ startupSettings.noVSyncMicroseconds = (Microseconds_t)( atof( argv[++i] ) * 1000 * 1000 ); }
		else if ( strcmp( arg, "h" ) == 0 && i + 0 < argc )	{ startupSettings.headRotationDisabled = true; }
		else if ( strcmp( arg, "p" ) == 0 && i + 0 < argc )	{ startupSettings.simulationPaused = true; }
		else if ( strcmp( arg, "r" ) == 0 && i + 1 < argc )	{ startupSettings.displayResolutionLevel = StartupSettings_StringToLevel( argv[++i], MAX_DISPLAY_RESOLUTION_LEVELS ); }
		else if ( strcmp( arg, "b" ) == 0 && i + 1 < argc )	{ startupSettings.eyeImageResolutionLevel = StartupSettings_StringToLevel( argv[++i], MAX_EYE_IMAGE_RESOLUTION_LEVELS ); }
		else if ( strcmp( arg, "s" ) == 0 && i + 1 < argc )	{ startupSettings.eyeImageSamplesLevel = StartupSettings_StringToLevel( argv[++i], MAX_EYE_IMAGE_SAMPLES_LEVELS ); }
		else if ( strcmp( arg, "q" ) == 0 && i + 1 < argc )	{ startupSettings.drawCallLevel = StartupSettings_StringToLevel( argv[++i], MAX_SCENE_DRAWCALL_LEVELS ); }
		else if ( strcmp( arg, "w" ) == 0 && i + 1 < argc )	{ startupSettings.triangleLevel = StartupSettings_StringToLevel( argv[++i], MAX_SCENE_TRIANGLE_LEVELS ); }
		else if ( strcmp( arg, "e" ) == 0 && i + 1 < argc )	{ startupSettings.fragmentLevel = StartupSettings_StringToLevel( argv[++i], MAX_SCENE_FRAGMENT_LEVELS ); }
		else if ( strcmp( arg, "m" ) == 0 && i + 0 < argc )	{ startupSettings.useMultiView = ( atoi( argv[++i] ) != 0 ); }
		else if ( strcmp( arg, "c" ) == 0 && i + 1 < argc )	{ startupSettings.correctChromaticAberration = ( atoi( argv[++i] ) != 0 ); }
		else if ( strcmp( arg, "i" ) == 0 && i + 1 < argc )	{ startupSettings.timeWarpImplementation = (TimeWarpImplementation_t)StartupSettings_StringToTimeWarpImplementation( argv[++i] ); }
		else if ( strcmp( arg, "z" ) == 0 && i + 1 < argc )	{ startupSettings.renderMode = StartupSettings_StringToRenderMode( argv[++i] ); }
		else if ( strcmp( arg, "g" ) == 0 && i + 0 < argc )	{ startupSettings.hideGraphs = true; }
		else if ( strcmp( arg, "l" ) == 0 && i + 1 < argc )	{ startupSettings.noLogMicroseconds = (Microseconds_t)( atof( argv[++i] ) * 1000 * 1000 ); }
		else if ( strcmp( arg, "d" ) == 0 && i + 0 < argc )	{ DumpGLSL(); exit( 0 ); }
		else
		{
			Print( "Unknown option: %s\n"
				   "atw_opengl [options]\n"
				   "options:\n"
				   "   -f          start fullscreen\n"
				   "   -v <s>      start with V-Sync disabled for this many seconds\n"
				   "   -h          start with head rotation disabled\n"
				   "   -p          start with the simulation paused\n"
				   "   -r <0-3>    set display resolution level\n"
				   "   -b <0-3>    set eye image resolution level\n"
				   "   -s <0-3>    set multi-sampling level\n"
				   "   -q <0-3>    set per eye draw calls level\n"
				   "   -w <0-3>    set per eye triangles per draw call level\n"
				   "   -e <0-3>    set per eye fragment program complexity level\n"
				   "   -m <0-1>    enable/disable multi-view\n"
				   "   -c <0-1>    enable/disable correction for chromatic aberration\n"
				   "   -i <name>   set time warp implementation: graphics, compute\n"
				   "   -z <name>   set the render mode: atw, tw, scene\n"
				   "   -g          hide graphs\n"
				   "   -l <s>      log 10 frames of OpenGL commands after this many seconds\n"
				   "   -d          dump GLSL to files for conversion to SPIR-V\n",
				   arg );
			return 1;
		}
	}

	//startupSettings.headRotationDisabled = true;
	//startupSettings.simulationPaused = true;
	//startupSettings.eyeImageSamplesLevel = 0;
	//startupSettings.useMultiView = true;
	//startupSettings.correctChromaticAberration = true;
	//startupSettings.renderMode = RENDER_MODE_SCENE;
	//startupSettings.timeWarpImplementation = TIMEWARP_IMPLEMENTATION_COMPUTE;

	Print( "    fullscreen = %d\n",					startupSettings.fullscreen );
	Print( "    noVSyncMicroseconds = %lld\n",		startupSettings.noVSyncMicroseconds );
	Print( "    headRotationDisabled = %d\n",		startupSettings.headRotationDisabled );
	Print( "    simulationPaused = %d\n",			startupSettings.simulationPaused );
	Print( "    displayResolutionLevel = %d\n",		startupSettings.displayResolutionLevel );
	Print( "    eyeImageResolutionLevel = %d\n",	startupSettings.eyeImageResolutionLevel );
	Print( "    eyeImageSamplesLevel = %d\n",		startupSettings.eyeImageSamplesLevel );
	Print( "    drawCallLevel = %d\n",				startupSettings.drawCallLevel );
	Print( "    triangleLevel = %d\n",				startupSettings.triangleLevel );
	Print( "    fragmentLevel = %d\n",				startupSettings.fragmentLevel );
	Print( "    useMultiView = %d\n",				startupSettings.useMultiView );
	Print( "    correctChromaticAberration = %d\n",	startupSettings.correctChromaticAberration );
	Print( "    timeWarpImplementation = %d\n",		startupSettings.timeWarpImplementation );
	Print( "    renderMode = %d\n",					startupSettings.renderMode );
	Print( "    hideGraphs = %d\n",					startupSettings.hideGraphs );
	Print( "    noLogMicroseconds = %lld\n",		startupSettings.noLogMicroseconds );

	for ( bool exit = false; !exit; )
	{
		if ( startupSettings.renderMode == RENDER_MODE_ASYNC_TIME_WARP )
		{
			exit = RenderAsyncTimeWarp( &startupSettings );
		}
		else if ( startupSettings.renderMode == RENDER_MODE_TIME_WARP )
		{
			exit = RenderTimeWarp( &startupSettings );
		}
		else if ( startupSettings.renderMode == RENDER_MODE_SCENE )
		{
			exit = RenderScene( &startupSettings );
		}
	}

	return 0;
}

#if defined( OS_WINDOWS )

int APIENTRY WinMain( HINSTANCE hCurrentInst, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow )
{
	UNUSED_PARM( hCurrentInst );
	UNUSED_PARM( hPreviousInst );
	UNUSED_PARM( nCmdShow );

	int		argc = 0;
	char *	argv[32];

	char filename[_MAX_PATH];
	GetModuleFileNameA( NULL, filename, _MAX_PATH );
	argv[argc++] = filename;

	while ( argc < 32 )
	{
		while ( lpszCmdLine[0] != '\0' && lpszCmdLine[0] == ' ' ) { lpszCmdLine++; }
		if ( lpszCmdLine[0] == '\0' ) { break; }

		argv[argc++] = lpszCmdLine;
        
		while ( lpszCmdLine[0] != '\0' && lpszCmdLine[0] != ' ' ) { lpszCmdLine++; }
		if ( lpszCmdLine[0] == '\0' ) { break; }
        
		*lpszCmdLine++ = '\0';
	}

	return StartApplication( argc, argv );
}

#elif defined( OS_APPLE_IOS )

static int argc_deferred;
static char** argv_deferred;

@interface MyAppDelegate : NSObject <UIApplicationDelegate> {}
@end

@implementation MyAppDelegate

-(BOOL) application: (UIApplication*) application didFinishLaunchingWithOptions: (NSDictionary*) launchOptions {

	CGRect screenRect = UIScreen.mainScreen.bounds;
	myUIView = [[MyUIView alloc] initWithFrame: screenRect];
	UIViewController* myUIVC = [[[MyUIViewController alloc] init] autorelease];
	myUIVC.view = myUIView;

	myUIWindow = [[UIWindow alloc] initWithFrame: screenRect];
	[myUIWindow addSubview: myUIView];
	myUIWindow.rootViewController = myUIVC;
	[myUIWindow makeKeyAndVisible];

	// Delay to allow startup runloop to complete.
	[self performSelector: @selector(startApplication:) withObject: nil afterDelay: 0.25f];

	return YES;
}

-(void) startApplication: (id) argObj {
	autoReleasePool = [[NSAutoreleasePool alloc] init];
	StartApplication( argc_deferred, argv_deferred );
}

@end

int main( int argc, char * argv[] )
{
	argc_deferred = argc;
	argv_deferred = argv;

	return UIApplicationMain( argc, argv, nil, @"MyAppDelegate" );
}

#elif defined( OS_APPLE_MACOS )

static const char * FormatString( const char * format, ... )
{
	static int index = 0;
	static char buffer[2][4096];
	index ^= 1;
	va_list args;
	va_start( args, format );
	vsnprintf( buffer[index], sizeof( buffer[index] ), format, args );
	va_end( args );
	return buffer[index];
}

void SystemCommandVerbose( const char * command )
{
	int result = system( command );
	printf( "%s : %s\n", command, ( result == 0 ) ? "\e[0;32msuccessful\e[0m" : "\e[0;31mfailed\e[0m" );
}

void WriteTextFileVerbose( const char * fileName, const char * text )
{
	FILE * file = fopen( fileName, "wb" );
	int elem = 0;
	if ( file != NULL )
	{
		elem = fwrite( text, strlen( text ), 1, file );
		fclose( file );
	}
	printf( "write %s %s\n", fileName, ( elem == 1 ) ? "\e[0;32msuccessful\e[0m" : "\e[0;31mfailed\e[0m" );
}

void CreateBundle( const char * exePath )
{
	const char * bundleIdentifier = "ATW";
	const char * bundleName = "ATW";
	const char * bundleSignature = "atwx";

	const char * infoPlistText =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<plist version=\"1.0\">\n"
		"<dict>\n"
		"	<key>BuildMachineOSBuild</key>\n"
		"	<string>13F34</string>\n"
		"	<key>CFBundleDevelopmentRegion</key>\n"
		"	<string>en</string>\n"
		"	<key>CFBundleExecutable</key>\n"
		"	<string>%s</string>\n"					// %s for executable name
		"	<key>CFBundleIdentifier</key>\n"
		"	<string>%s</string>\n"					// %s for bundleIdentifier
		"	<key>CFBundleInfoDictionaryVersion</key>\n"
		"	<string>6.0</string>\n"
		"	<key>CFBundleName</key>\n"
		"	<string>%s</string>\n"					// %s for bundleName
		"	<key>CFBundlePackageType</key>\n"
		"	<string>APPL</string>\n"
		"	<key>CFBundleShortVersionString</key>\n"
		"	<string>1.0</string>\n"
		"	<key>CFBundleSignature</key>\n"
		"	<string>atwx</string>\n"				// %s for bundleSignature
		"	<key>CFBundleVersion</key>\n"
		"	<string>1</string>\n"
		"	<key>DTCompiler</key>\n"
		"	<string>com.apple.compilers.llvm.clang.1_0</string>\n"
		"	<key>DTPlatformBuild</key>\n"
		"	<string>6A2008a</string>\n"
		"	<key>DTPlatformVersion</key>\n"
		"	<string>GM</string>\n"
		"	<key>DTSDKBuild</key>\n"
		"	<string>14A382</string>\n"
		"	<key>DTSDKName</key>\n"
		"	<string>macosx10.10</string>\n"
		"	<key>DTXcode</key>\n"
		"	<string>0611</string>\n"
		"	<key>DTXcodeBuild</key>\n"
		"	<string>6A2008a</string>\n"
		"	<key>LSMinimumSystemVersion</key>\n"
		"	<string>10.9</string>\n"
		"	<key>NSMainNibFile</key>\n"
		"	<string>MainMenu</string>\n"
		"	<key>NSPrincipalClass</key>\n"
		"	<string>NSApplication</string>\n"
		"</dict>\n"
		"</plist>\n";

	const char * exeName = exePath + strlen( exePath ) - 1;
	for ( ; exeName > exePath && exeName[-1] != '/'; exeName-- ) {}

	SystemCommandVerbose( FormatString( "rm -r %s.app", exePath ) );
	SystemCommandVerbose( FormatString( "mkdir %s.app", exePath ) );
	SystemCommandVerbose( FormatString( "mkdir %s.app/Contents", exePath ) );
	SystemCommandVerbose( FormatString( "mkdir %s.app/Contents/MacOS", exePath ) );
	SystemCommandVerbose( FormatString( "cp %s %s.app/Contents/MacOS", exePath, exePath ) );
	WriteTextFileVerbose( FormatString( "%s.app/Contents/Info.plist", exePath ),
							FormatString( infoPlistText, exeName, bundleIdentifier, bundleName, bundleSignature ) );
}

void LaunchBundle( int argc, char * argv[] )
{
	// Print command to open the bundled application.
	char command[2048];
	int length = snprintf( command, sizeof( command ), "open %s.app", argv[0] );

	// Append all the original command-line arguments.
	const char * argsParm = " --args";
	const int argsParmLength = strlen( argsParm );
	if ( argc > 1 && argsParmLength + 1 < sizeof( command ) - length )
	{
		strcpy( command + length, argsParm );
		length += argsParmLength;
		for ( int i = 1; i < argc; i++ )
		{
			const int argLength = strlen( argv[i] );
			if ( argLength + 2 > sizeof( command ) - length )
			{
				break;
			}
			strcpy( command + length + 0, " " );
			strcpy( command + length + 1, argv[i] );
			length += 1 + argLength;
		}
	}

	// Launch the bundled application with the original command-line arguments.
	SystemCommandVerbose( command );
}

void SetBundleCWD( const char * bundledExecutablePath )
{
	// Inside a bundle, an executable lives three folders and
	// four forward slashes deep: /name.app/Contents/MacOS/name
	char cwd[1024];
	strcpy( cwd, bundledExecutablePath );
	for ( int i = strlen( cwd ) - 1, slashes = 0; i >= 0 && slashes < 4; i-- )
	{
		slashes += ( cwd[i] == '/' );
		cwd[i] = '\0';
	}
	int result = chdir( cwd );
	Print( "chdir( \"%s\" ) %s", cwd, ( result == 0 ) ? "successful" : "failed" );
}

int main( int argc, char * argv[] )
{
	/*
		When an application executable is not launched from a bundle, macOS
		considers the application to be a console application with only text output
		and console keyboard input. As a result, an application will not receive
		keyboard events unless the application is launched from a bundle.
		Programmatically created graphics windows are also unable to properly
		acquire focus unless the application is launched from a bundle.

		If the executable was not launched from a bundle then automatically create
		a bundle right here and then launch the bundled application.
	*/
	if ( strstr( argv[0], "/Contents/MacOS/" ) == NULL )
	{
		CreateBundle( argv[0] );
		LaunchBundle( argc, argv );
		return 0;
	}

	SetBundleCWD( argv[0] );

	autoReleasePool = [[NSAutoreleasePool alloc] init];
	
	[NSApplication sharedApplication];
	[NSApp finishLaunching];
	[NSApp activateIgnoringOtherApps:YES];

	return StartApplication( argc, argv );
}

#elif defined( OS_LINUX )

int main( int argc, char * argv[] )
{
	return StartApplication( argc, argv );
}

#elif defined( OS_ANDROID )

#define MAX_ARGS		32
#define MAX_ARGS_BUFFER	1024

typedef struct
{
	char	buffer[MAX_ARGS_BUFFER];
	char *	argv[MAX_ARGS];
	int		argc;
} AndroidParm_t;

// adb shell am start -n com.vulkansamples.atw_opengl/android.app.NativeActivity -a "android.intent.action.MAIN" --es "args" "\"-r tw\""
void GetIntentParms( AndroidParm_t * parms )
{
	parms->buffer[0] = '\0';
	parms->argv[0] = "atw_vulkan";
	parms->argc = 1;

	Java_t java;
	java.vm = global_app->activity->vm;
	(*java.vm)->AttachCurrentThread( java.vm, &java.env, NULL );
	java.activity = global_app->activity->clazz;

	jclass activityClass = (*java.env)->GetObjectClass( java.env, java.activity );
	jmethodID getIntenMethodId = (*java.env)->GetMethodID( java.env, activityClass, "getIntent", "()Landroid/content/Intent;" );
	jobject intent = (*java.env)->CallObjectMethod( java.env, java.activity, getIntenMethodId );
	(*java.env)->DeleteLocalRef( java.env, activityClass );

	jclass intentClass = (*java.env)->GetObjectClass( java.env, intent );
	jmethodID getStringExtraMethodId = (*java.env)->GetMethodID( java.env, intentClass, "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;" );

	jstring argsJstring = (*java.env)->NewStringUTF( java.env, "args" );
	jstring extraJstring = (*java.env)->CallObjectMethod( java.env, intent, getStringExtraMethodId, argsJstring );
	if ( extraJstring != NULL )
	{
		const char * args = (*java.env)->GetStringUTFChars( java.env, extraJstring, 0 );
		strncpy( parms->buffer, args, sizeof( parms->buffer ) - 1 );
		parms->buffer[sizeof( parms->buffer ) - 1] = '\0';
		(*java.env)->ReleaseStringUTFChars( java.env, extraJstring, args );

		Print( "    args = %s\n", args );

		char * ptr = parms->buffer;
		while ( parms->argc < MAX_ARGS )
		{
			while ( ptr[0] != '\0' && ptr[0] == ' ' ) { ptr++; }
			if ( ptr[0] == '\0' ) { break; }

			parms->argv[parms->argc++] = ptr;

			while ( ptr[0] != '\0' && ptr[0] != ' ' ) { ptr++; }
			if ( ptr[0] == '\0' ) { break; }

			*ptr++ = '\0';
		}
	}

	(*java.env)->DeleteLocalRef( java.env, argsJstring );
	(*java.env)->DeleteLocalRef( java.env, intentClass );
	(*java.vm)->DetachCurrentThread( java.vm );
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events.
 */
void android_main( struct android_app * app )
{
	Print( "----------------------------------------------------------------" );
	Print( "onCreate()" );
	Print( "    android_main()" );

	// Make sure the native app glue is not stripped.
	app_dummy();

	global_app = app;

	AndroidParm_t parms;
	GetIntentParms( &parms );

	StartApplication( parms.argc, parms.argv );
}

#endif
