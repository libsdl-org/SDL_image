/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2022 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/**
 *  \file SDL_image.h
 *
 *  Header file for SDL_image library
 *
 * A simple library to load images of various formats as SDL surfaces
 */
#ifndef SDL_IMAGE_H_
#define SDL_IMAGE_H_

#include "SDL.h"
#include "SDL_version.h"
#include "begin_code.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Printable format: "%d.%d.%d", MAJOR, MINOR, PATCHLEVEL
 */
#define SDL_IMAGE_MAJOR_VERSION 2
#define SDL_IMAGE_MINOR_VERSION 5
#define SDL_IMAGE_PATCHLEVEL    2

/**
 * This macro can be used to fill a version structure with the compile-time
 * version of the SDL_image library.
 */
#define SDL_IMAGE_VERSION(X)                        \
{                                                   \
    (X)->major = SDL_IMAGE_MAJOR_VERSION;           \
    (X)->minor = SDL_IMAGE_MINOR_VERSION;           \
    (X)->patch = SDL_IMAGE_PATCHLEVEL;              \
}

#if SDL_IMAGE_MAJOR_VERSION < 3 && SDL_MAJOR_VERSION < 3
/**
 *  This is the version number macro for the current SDL_image version.
 *
 *  In versions higher than 2.9.0, the minor version overflows into
 *  the thousands digit: for example, 2.23.0 is encoded as 4300.
 *  This macro will not be available in SDL 3.x or SDL_image 3.x.
 *
 *  Deprecated, use SDL_IMAGE_VERSION_ATLEAST or SDL_IMAGE_VERSION instead.
 */
#define SDL_IMAGE_COMPILEDVERSION \
    SDL_VERSIONNUM(SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL)
#endif /* SDL_IMAGE_MAJOR_VERSION < 3 && SDL_MAJOR_VERSION < 3 */

/**
 *  This macro will evaluate to true if compiled with SDL_image at least X.Y.Z.
 */
#define SDL_IMAGE_VERSION_ATLEAST(X, Y, Z) \
    ((SDL_IMAGE_MAJOR_VERSION >= X) && \
     (SDL_IMAGE_MAJOR_VERSION > X || SDL_IMAGE_MINOR_VERSION >= Y) && \
     (SDL_IMAGE_MAJOR_VERSION > X || SDL_IMAGE_MINOR_VERSION > Y || SDL_IMAGE_PATCHLEVEL >= Z))

/**
 * This function gets the version of the dynamically linked SDL_image library.
 *
 * it should NOT be used to fill a version structure, instead you should use
 * the SDL_IMAGE_VERSION() macro.
 *
 * \returns SDL_image version
 */
extern DECLSPEC const SDL_version * SDLCALL IMG_Linked_Version(void);

/**
 * Initialization flags
 */
typedef enum
{
    IMG_INIT_JPG    = 0x00000001,
    IMG_INIT_PNG    = 0x00000002,
    IMG_INIT_TIF    = 0x00000004,
    IMG_INIT_WEBP   = 0x00000008,
    IMG_INIT_JXL    = 0x00000010,
    IMG_INIT_AVIF   = 0x00000020
} IMG_InitFlags;

/**
 * Loads dynamic libraries and prepares them for use.
 *
 * Flags should be one or more flags from IMG_InitFlags OR'd together.
 *
 * \param flags initialization flags
 * \returns flags successfully initialized, or 0 on failure.
 *
 * \sa IMG_Quit
 * \sa IMG_InitFlags
 * \sa IMG_Load
 * \sa IMG_LoadTexture
 */
extern DECLSPEC int SDLCALL IMG_Init(int flags);

/**
 * Unloads libraries loaded with IMG_Init.
 *
 * \sa IMG_Init
 */
extern DECLSPEC void SDLCALL IMG_Quit(void);

/**
 * Load an image from an SDL data source.
 *
 * If the image format supports a transparent pixel, SDL will set the colorkey
 * for the surface. You can enable RLE acceleration on the surface afterwards
 * by calling: SDL_SetColorKey(image, SDL_RLEACCEL, image->format->colorkey);
 *
 * \param src RWops
 * \param freesrc can be set so that the RWops is freed after this function is
 *                called
 * \param type may be one of: "BMP", "GIF", "PNG", etc.
 * \returns SDL surface, or NULL on error.
 *
 * \sa IMG_Load
 * \sa IMG_Load_RW
 * \sa SDL_FreeSurface
 */
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadTyped_RW(SDL_RWops *src, int freesrc, const char *type);

/**
 * Load an image from file
 *
 * \param file file name
 * \returns SDL surface, or NULL on error.
 *
 * \sa IMG_LoadTyped_RW
 * \sa IMG_Load_RW
 * \sa SDL_FreeSurface
 */
extern DECLSPEC SDL_Surface * SDLCALL IMG_Load(const char *file);

/**
 * Load an image from a SDL datasource
 *
 * \param src RWops
 * \param freesrc can be set so that the RWops is freed after this function is
 *                called
 * \returns SDL surface, or NULL on error
 *
 * \sa IMG_Load
 * \sa IMG_LoadTyped_RW
 * \sa SDL_FreeSurface
 */
extern DECLSPEC SDL_Surface * SDLCALL IMG_Load_RW(SDL_RWops *src, int freesrc);

#if SDL_VERSION_ATLEAST(2,0,0)

/**
 * Load an image directly into a render texture.
 *
 * \param renderer SDL Render
 * \param file image file name
 * \returns SDL Texture, or NULL on error.
 *
 * \sa IMG_LoadTexture_RW
 * \sa IMG_LoadTextureTyped_RW
 * \sa SDL_DestroyTexture
 */
extern DECLSPEC SDL_Texture * SDLCALL IMG_LoadTexture(SDL_Renderer *renderer, const char *file);

/**
 * Load an image directly into a render texture.
 *
 * \param renderer SDL Render
 * \param src RWops
 * \param freesrc can be set so that the RWops is freed after this function is
 *                called
 * \returns SDL Texture, or NULL on error
 *
 * \sa IMG_LoadTexture
 * \sa IMG_LoadTextureTyped_RW
 * \sa SDL_DestroyTexture
 */
extern DECLSPEC SDL_Texture * SDLCALL IMG_LoadTexture_RW(SDL_Renderer *renderer, SDL_RWops *src, int freesrc);

/**
 * Load an image directly into a render texture.
 *
 * \param renderer SDL Render
 * \param src RWops
 * \param freesrc can be set so that the RWops is freed after this function is
 *                called
 * \param type may be one of: "BMP", "GIF", "PNG", etc.
 * \returns SDL Texture, or NULL on error
 *
 * \sa IMG_LoadTexture
 * \sa IMG_LoadTexture_RW
 * \sa SDL_DestroyTexture
 */
extern DECLSPEC SDL_Texture * SDLCALL IMG_LoadTextureTyped_RW(SDL_Renderer *renderer, SDL_RWops *src, int freesrc, const char *type);
#endif /* SDL 2.0 */

/**
 * Functions to detect a file type, given a seekable source
 *
 * \param src RWops
 * \returns 1 if file type is detected, 0 otherwise
 *
 * \sa IMG_isAVIF
 * \sa IMG_isICO
 * \sa IMG_isCUR
 * \sa IMG_isBMP
 * \sa IMG_isGIF
 * \sa IMG_isJPG
 * \sa IMG_isJXL
 * \sa IMG_isLBM
 * \sa IMG_isPCX
 * \sa IMG_isPNG
 * \sa IMG_isPNM
 * \sa IMG_isSVG
 * \sa IMG_isQOI
 * \sa IMG_isTIF
 * \sa IMG_isXCF
 * \sa IMG_isXPM
 * \sa IMG_isXV
 * \sa IMG_isWEBP
 */
extern DECLSPEC int SDLCALL IMG_isAVIF(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isICO(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isCUR(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isBMP(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isGIF(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isJPG(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isJXL(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isLBM(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isPCX(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isPNG(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isPNM(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isSVG(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isQOI(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isTIF(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isXCF(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isXPM(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isXV(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isWEBP(SDL_RWops *src);

/**
 * Individual loading functions
 *
 * \param src RWops
 * \returns SDL surface, or NULL on error
 *
 * \sa IMG_LoadAVIF_RW
 * \sa IMG_LoadICO_RW
 * \sa IMG_LoadCUR_RW
 * \sa IMG_LoadBMP_RW
 * \sa IMG_LoadGIF_RW
 * \sa IMG_LoadJPG_RW
 * \sa IMG_LoadJXL_RW
 * \sa IMG_LoadLBM_RW
 * \sa IMG_LoadPCX_RW
 * \sa IMG_LoadPNG_RW
 * \sa IMG_LoadPNM_RW
 * \sa IMG_LoadSVG_RW
 * \sa IMG_LoadQOI_RW
 * \sa IMG_LoadTIF_RW
 * \sa IMG_LoadXCF_RW
 * \sa IMG_LoadXPM_RW
 * \sa IMG_LoadXV_RW
 * \sa IMG_LoadWEBP_RW
 */
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadAVIF_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadICO_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadCUR_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadBMP_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadGIF_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadJPG_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadJXL_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadLBM_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadPCX_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadPNG_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadPNM_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadSVG_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadQOI_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadTGA_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadTIF_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadXCF_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadXPM_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadXV_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadWEBP_RW(SDL_RWops *src);

/**
 * Load an SVG scaled to a specific size Either width or height may be 0 and
 * will be auto-sized to preserve aspect ratio.
 *
 * \param src RWops
 * \param width width
 * \param height height
 * \returns SDL surface, or NULL on error
 */
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadSizedSVG_RW(SDL_RWops *src, int width, int height);

/**
 * Load a XPM image from a an array Returns an 8bpp indexed surface if
 * possible, otherwise 32bpp.
 *
 * \param xpm null terminated array of strings
 * \returns SDL surface, or NULL on error
 *
 * \sa IMG_ReadXPMFromArrayToRGB888
 */
extern DECLSPEC SDL_Surface * SDLCALL IMG_ReadXPMFromArray(char **xpm);

/**
 * Load a XPM image from a an array Returns always a 32bpp (RGB888) surface
 *
 * \param xpm null terminated array of strings
 * \returns SDL surface, or NULL on error
 *
 * \sa IMG_ReadXPMFromArray
 */
extern DECLSPEC SDL_Surface * SDLCALL IMG_ReadXPMFromArrayToRGB888(char **xpm);

/**
 * Save a SDL_Surface into a PNG image file.
 *
 * \param surface SDL surface to save
 * \param file file name
 * \returns 0 if successful, -1 on error
 *
 * \sa IMG_SavePNG_RW
 * \sa IMG_SaveJPG
 * \sa IMG_SaveJPG_RW
 */
extern DECLSPEC int SDLCALL IMG_SavePNG(SDL_Surface *surface, const char *file);

/**
 * Save a SDL_Surface into a PNG image file.
 *
 * \param surface SDL surface to save
 * \param dst RWops
 * \param freedst free dst
 * \returns 0 if successful, -1 on error
 *
 * \sa IMG_SavePNG
 * \sa IMG_SaveJPG
 * \sa IMG_SaveJPG_RW
 */
extern DECLSPEC int SDLCALL IMG_SavePNG_RW(SDL_Surface *surface, SDL_RWops *dst, int freedst);

/**
 * Save a SDL_Surface into a JPEG image file.
 *
 * \param surface SDL surface to save
 * \param file file name
 * \param quality [0; 33] is Lowest quality, [34; 66] is Middle quality, [67;
 *                100] is Highest quality
 * \returns 0 if successful, -1 on error
 *
 * \sa IMG_SavePNG
 * \sa IMG_SavePNG_RW
 * \sa IMG_SaveJPG_RW
 */
extern DECLSPEC int SDLCALL IMG_SaveJPG(SDL_Surface *surface, const char *file, int quality);

/**
 * Save a SDL_Surface into a JPEG image file.
 *
 * \param surface SDL surface to save
 * \param dst RWops
 * \param freedst free dst
 * \param quality [0; 33] is Lowest quality, [34; 66] is Middle quality, [67;
 *                100] is Highest quality
 * \returns 0 if successful, -1 on error
 *
 * \sa IMG_SavePNG
 * \sa IMG_SavePNG_RW
 * \sa IMG_SaveJPG
 */
extern DECLSPEC int SDLCALL IMG_SaveJPG_RW(SDL_Surface *surface, SDL_RWops *dst, int freedst, int quality);

/**
 * Animated image support
 * Currently only animated GIFs are supported.
 */
typedef struct
{
	int w, h;
	int count;
	SDL_Surface **frames;
	int *delays;
} IMG_Animation;

/**
 * Load an animation from file
 *
 * \param file file name
 * \returns IMG Animation, or NULL on error.
 *
 * \sa IMG_FreeAnimation
 */
extern DECLSPEC IMG_Animation * SDLCALL IMG_LoadAnimation(const char *file);

/**
 * Load an animation from an SDL datasource
 *
 * \param src RWops
 * \param freesrc can be set so that the RWops is freed after this function is
 *                called
 * \returns IMG Animation, or NULL on error
 *
 * \sa IMG_FreeAnimation
 */
extern DECLSPEC IMG_Animation * SDLCALL IMG_LoadAnimation_RW(SDL_RWops *src, int freesrc);

/**
 * Load an animation from an SDL datasource
 *
 * \param src RWops
 * \param freesrc can be set so that the RWops is freed after this function is
 *                called
 * \param type may be one of: "BMP", "GIF", "PNG", etc.
 * \returns IMG Animation, or NULL on error
 *
 * \sa IMG_FreeAnimation
 */
extern DECLSPEC IMG_Animation * SDLCALL IMG_LoadAnimationTyped_RW(SDL_RWops *src, int freesrc, const char *type);

/**
 * Free animation
 *
 * \param anim animation
 *
 * \sa IMG_LoadAnimation
 * \sa IMG_LoadAnimation_RW
 * \sa IMG_LoadAnimationTyped_RW
 */
extern DECLSPEC void SDLCALL IMG_FreeAnimation(IMG_Animation *anim);

/**
 * Load a GIF type animation from an SDL datasource
 *
 * \param src RWops
 * \returns IMG Animation, or NULL on error
 *
 * \sa IMG_FreeAnimation
 */
extern DECLSPEC IMG_Animation * SDLCALL IMG_LoadGIFAnimation_RW(SDL_RWops *src);

/**
 * Report SDL_image errors
 *
 * \sa IMG_GetError
 */
#define IMG_SetError    SDL_SetError

/**
 * Get last SDL_image error
 *
 * \sa IMG_SetError
 */
#define IMG_GetError    SDL_GetError

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* SDL_IMAGE_H_ */
