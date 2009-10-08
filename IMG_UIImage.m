/*
 *  IMG_ImageIO.c
 *  SDL_image
 *
 *  Created by Eric Wing on 1/2/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include "SDL_image.h"
#import <UIKit/UIKit.h>
#import <MobileCoreServices/MobileCoreServices.h> // for UTCoreTypes.h

// Once we have our image, we need to get it into an SDL_Surface
// (Copied straight from the ImageIO backend.)
static SDL_Surface* Create_SDL_Surface_From_CGImage(CGImageRef image_ref)
{
	/* This code is adapted from Apple's Documentation found here:
	 * http://developer.apple.com/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/index.html
	 * Listing 9-4††Using a Quartz image as a texture source.
	 * Unfortunately, this guide doesn't show what to do about
	 * non-RGBA image formats so I'm making the rest up.
	 * All this code should be scrutinized.
	 */
	
	size_t the_width = CGImageGetWidth(image_ref);
	size_t the_height = CGImageGetHeight(image_ref);
	CGRect the_rect = {{0, 0}, {the_width, the_height}};
	
	size_t bits_per_pixel = CGImageGetBitsPerPixel(image_ref);
	size_t bytes_per_row = CGImageGetBytesPerRow(image_ref);
	//	size_t bits_per_component = CGImageGetBitsPerComponent(image_ref);
	size_t bits_per_component = 8;
	
//	CGImageAlphaInfo alpha_info = CGImageGetAlphaInfo(image_ref);
	

	SDL_Surface* sdl_surface = NULL;
	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	Uint32 Amask;

	
	CGContextRef bitmap_context = NULL;
	
	CGColorSpaceRef color_space = NULL;
	CGBitmapInfo bitmap_info = CGImageGetBitmapInfo(image_ref);

	
	switch(bits_per_pixel)
	{
		case 8:
		{
			bytes_per_row = the_width*4;
			//				color_space = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
			color_space = CGColorSpaceCreateDeviceRGB();
			//				bitmap_info = kCGImageAlphaPremultipliedFirst;
#if __BIG_ENDIAN__
			bitmap_info = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Big; /* XRGB Big Endian */
#else
			bitmap_info = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little; /* XRGB Little Endian */
#endif

			Rmask = 0x00FF0000;
			Gmask = 0x0000FF00;
			Bmask = 0x000000FF;
			Amask = 0x00000000;

			sdl_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
											   the_width, the_height, 32, Rmask, Gmask, Bmask, Amask);


			
			break;
		}
		case 15:
		case 16:
		{
			bytes_per_row = the_width*4;

			color_space = CGColorSpaceCreateDeviceRGB();

#if __BIG_ENDIAN__
			bitmap_info = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Big; /* XRGB Big Endian */
#else
			bitmap_info = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little; /* XRGB Little Endian */
#endif
			Rmask = 0x00FF0000;
			Gmask = 0x0000FF00;
			Bmask = 0x000000FF;
			Amask = 0x00000000;


			sdl_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
											   the_width, the_height, 32, Rmask, Gmask, Bmask, Amask);

			break;
		}
		case 24:
		{
			bytes_per_row = the_width*4;
			//			color_space = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
			color_space = CGColorSpaceCreateDeviceRGB();
			//			bitmap_info = kCGImageAlphaNone;
#if __BIG_ENDIAN__
			bitmap_info = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Big; /* XRGB Big Endian */
#else
			bitmap_info = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little; /* XRGB Little Endian */
#endif
			Rmask = 0x00FF0000;
			Gmask = 0x0000FF00;
			Bmask = 0x000000FF;
			Amask = 0x00000000;

			sdl_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
											   the_width, the_height, 32, Rmask, Gmask, Bmask, Amask);

			break;
		}
		case 32:
		{
						
			bytes_per_row = the_width*4;
			//			color_space = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
			color_space = CGColorSpaceCreateDeviceRGB();
			//			bitmap_info = kCGImageAlphaPremultipliedFirst;
#if __BIG_ENDIAN__
			bitmap_info = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Big; /* XRGB Big Endian */
#else
			bitmap_info = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little; /* XRGB Little Endian */
#endif 
			Amask = 0xFF000000;
			Rmask = 0x00FF0000;
			Gmask = 0x0000FF00;
			Bmask = 0x000000FF;

			sdl_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
											   the_width, the_height, 32, Rmask, Gmask, Bmask, Amask);
			break;
		}
		default:
		{
            sdl_surface = NULL;
            break;
		}
			
	}

	if(NULL == sdl_surface)
	{
		if(color_space != NULL)
		{
			CGColorSpaceRelease(color_space);			
		}
		return NULL;
	}


	// Sets up a context to be drawn to with sdl_surface->pixels as the area to be drawn to
	bitmap_context = CGBitmapContextCreate(
														sdl_surface->pixels,
														the_width,
														the_height,
														bits_per_component,
														bytes_per_row,
														color_space,
														bitmap_info
														);
	
	// Draws the image into the context's image_data
	CGContextDrawImage(bitmap_context, the_rect, image_ref);
	
	CGContextRelease(bitmap_context);
	CGColorSpaceRelease(color_space);
	
	return sdl_surface;
	
	
	
}

static SDL_Surface* LoadImageFromRWops(SDL_RWops* rw_ops, CFStringRef uti_string_hint)
{
	NSAutoreleasePool* autorelease_pool = [[NSAutoreleasePool alloc] init];
	SDL_Surface* sdl_surface;
	UIImage* ui_image;

	CGImageRef image_ref = NULL;
	int bytes_read = 0;
	// I don't know what a good size is. 
	// Max recommended texture size is 1024x1024 on iPhone so maybe base it on that?
	const int block_size = 1024*4;
	char temp_buffer[block_size];
	
	NSMutableData* ns_data = [[NSMutableData alloc] initWithCapacity:1024*1024*4];

	
	do
	{
		bytes_read = SDL_RWread(rw_ops, temp_buffer, 1, block_size);
		[ns_data appendBytes:temp_buffer length:bytes_read];
	} while(bytes_read > 0);

	if(NULL == image_ref)
	{
		return NULL;
	}

	ui_image = [[UIImage alloc] initWithData:ns_data];
	
	sdl_surface = Create_SDL_Surface_From_CGImage([ui_image CGImage]);

	[ui_image release];
	[ns_data release];

	[autorelease_pool drain];

	return sdl_surface;
}

static SDL_Surface* LoadImageFromFile(const char *file)
{
	NSAutoreleasePool* autorelease_pool = [[NSAutoreleasePool alloc] init];
	SDL_Surface* sdl_surface = NULL;
	UIImage* ui_image;
	NSString* ns_string;
	
	ns_string = [[NSString alloc] initWithUTF8String:file];
	ui_image = [[UIImage alloc] initWithContentsOfFile:ns_string];
	
	sdl_surface = Create_SDL_Surface_From_CGImage([ui_image CGImage]);
	
	[ui_image release];
	[ns_string release];
	
	[autorelease_pool drain];
	
	return sdl_surface;
}


/* Since UIImage doesn't really support streams well, we should optimize for the file case. */
SDL_Surface *IMG_Load(const char *file)
{
	SDL_Surface* sdl_surface = NULL;

	sdl_surface = LoadImageFromFile(file);
	if(NULL == sdl_surface)
	{
		// Either the file doesn't exist or ImageIO doesn't understand the format.
		// For the latter case, fallback to the native SDL_image handlers.

		SDL_RWops *src = SDL_RWFromFile(file, "rb");
		char *ext = strrchr(file, '.');
		if(ext) {
			ext++;
		}
		if(!src) {
			/* The error message has been set in SDL_RWFromFile */
			return NULL;
		}
		sdl_surface = IMG_LoadTyped_RW(src, 1, ext);
	}
	return sdl_surface;
}



/* Copied straight from other files so I don't have to alter them. */
static int IMG_isICOCUR(SDL_RWops *src, int type)
{
	int start;
	int is_ICOCUR;

	/* The Win32 ICO file header (14 bytes) */
    Uint16 bfReserved;
    Uint16 bfType;
    Uint16 bfCount;

	if ( !src )
		return 0;
	start = SDL_RWtell(src);
	is_ICOCUR = 0;
    bfReserved = SDL_ReadLE16(src);
    bfType = SDL_ReadLE16(src);
    bfCount = SDL_ReadLE16(src);
    if ((bfReserved == 0) && (bfType == type) && (bfCount != 0)) 
    	is_ICOCUR = 1;
	SDL_RWseek(src, start, SEEK_SET);

	return (is_ICOCUR);
}

int IMG_isICO(SDL_RWops *src)
{
	return IMG_isICOCUR(src, 1);
}

int IMG_isCUR(SDL_RWops *src)
{
	return IMG_isICOCUR(src, 2);
}

int IMG_isBMP(SDL_RWops *src)
{
	int start;
	int is_BMP;
	char magic[2];
	
	if ( !src )
		return 0;
	start = SDL_RWtell(src);
	is_BMP = 0;
	if ( SDL_RWread(src, magic, sizeof(magic), 1) ) {
		if ( strncmp(magic, "BM", 2) == 0 ) {
			is_BMP = 1;
		}
	}
	SDL_RWseek(src, start, SEEK_SET);
	return(is_BMP);
}

int IMG_isGIF(SDL_RWops *src)
{
	int start;
	int is_GIF;
	char magic[6];
	
	if ( !src )
		return 0;
	start = SDL_RWtell(src);
	is_GIF = 0;
	if ( SDL_RWread(src, magic, sizeof(magic), 1) ) {
		if ( (strncmp(magic, "GIF", 3) == 0) &&
			((memcmp(magic + 3, "87a", 3) == 0) ||
			 (memcmp(magic + 3, "89a", 3) == 0)) ) {
			is_GIF = 1;
		}
	}
	SDL_RWseek(src, start, SEEK_SET);
	return(is_GIF);
}

int IMG_isJPG(SDL_RWops *src)
{
	int start;
	int is_JPG;
	int in_scan;
	Uint8 magic[4];
	
	/* This detection code is by Steaphan Greene <stea@cs.binghamton.edu> */
	/* Blame me, not Sam, if this doesn't work right. */
	/* And don't forget to report the problem to the the sdl list too! */
	
	if ( !src )
		return 0;
	start = SDL_RWtell(src);
	is_JPG = 0;
	in_scan = 0;
	if ( SDL_RWread(src, magic, 2, 1) ) {
		if ( (magic[0] == 0xFF) && (magic[1] == 0xD8) ) {
			is_JPG = 1;
			while (is_JPG == 1) {
				if(SDL_RWread(src, magic, 1, 2) != 2) {
					is_JPG = 0;
				} else if( (magic[0] != 0xFF) && (in_scan == 0) ) {
					is_JPG = 0;
				} else if( (magic[0] != 0xFF) || (magic[1] == 0xFF) ) {
					/* Extra padding in JPEG (legal) */
					/* or this is data and we are scanning */
					SDL_RWseek(src, -1, SEEK_CUR);
				} else if(magic[1] == 0xD9) {
					/* Got to end of good JPEG */
					break;
				} else if( (in_scan == 1) && (magic[1] == 0x00) ) {
					/* This is an encoded 0xFF within the data */
				} else if( (magic[1] >= 0xD0) && (magic[1] < 0xD9) ) {
					/* These have nothing else */
				} else if(SDL_RWread(src, magic+2, 1, 2) != 2) {
					is_JPG = 0;
				} else {
					/* Yes, it's big-endian */
					Uint32 start;
					Uint32 size;
					Uint32 end;
					start = SDL_RWtell(src);
					size = (magic[2] << 8) + magic[3];
					end = SDL_RWseek(src, size-2, SEEK_CUR);
					if ( end != start + size - 2 ) is_JPG = 0;
					if ( magic[1] == 0xDA ) {
						/* Now comes the actual JPEG meat */
#ifdef	FAST_IS_JPEG
						/* Ok, I'm convinced.  It is a JPEG. */
						break;
#else
						/* I'm not convinced.  Prove it! */
						in_scan = 1;
#endif
					}
				}
			}
		}
	}
	SDL_RWseek(src, start, SEEK_SET);
	return(is_JPG);
}

int IMG_isPNG(SDL_RWops *src)
{
	int start;
	int is_PNG;
	Uint8 magic[4];
	
	if ( !src )
		return 0;
	start = SDL_RWtell(src);
	is_PNG = 0;
	if ( SDL_RWread(src, magic, 1, sizeof(magic)) == sizeof(magic) ) {
		if ( magic[0] == 0x89 &&
			magic[1] == 'P' &&
			magic[2] == 'N' &&
			magic[3] == 'G' ) {
			is_PNG = 1;
		}
	}
	SDL_RWseek(src, start, SEEK_SET);
	return(is_PNG);
}

int IMG_isTIF(SDL_RWops* src)
{
	int start;
	int is_TIF;
	Uint8 magic[4];
	
	if ( !src )
		return 0;
	start = SDL_RWtell(src);
	is_TIF = 0;
	if ( SDL_RWread(src, magic, 1, sizeof(magic)) == sizeof(magic) ) {
		if ( (magic[0] == 'I' &&
			  magic[1] == 'I' &&
		      magic[2] == 0x2a &&
			  magic[3] == 0x00) ||
			(magic[0] == 'M' &&
			 magic[1] == 'M' &&
			 magic[2] == 0x00 &&
			 magic[3] == 0x2a) ) {
			is_TIF = 1;
		}
	}
	SDL_RWseek(src, start, SEEK_SET);
	return(is_TIF);
}

SDL_Surface* IMG_LoadCUR_RW(SDL_RWops *src)
{
	/* FIXME: Is this a supported type? */
	return LoadImageFromRWops(src, CFSTR("com.microsoft.cur"));
}
SDL_Surface* IMG_LoadICO_RW(SDL_RWops *src)
{
	return LoadImageFromRWops(src, kUTTypeICO);
}
SDL_Surface* IMG_LoadBMP_RW(SDL_RWops *src)
{
	return LoadImageFromRWops(src, kUTTypeBMP);
}
SDL_Surface* IMG_LoadGIF_RW(SDL_RWops *src)
{
	return LoadImageFromRWops(src, kUTTypeGIF);
}
SDL_Surface* IMG_LoadJPG_RW(SDL_RWops *src)
{
	return LoadImageFromRWops(src, kUTTypeJPEG);
}
SDL_Surface* IMG_LoadPNG_RW(SDL_RWops *src)
{
	return LoadImageFromRWops(src, kUTTypePNG);
}
SDL_Surface* IMG_LoadTGA_RW(SDL_RWops *src)
{
	return LoadImageFromRWops(src, CFSTR("com.truevision.tga-image"));
}
SDL_Surface* IMG_LoadTIF_RW(SDL_RWops *src)
{
	return LoadImageFromRWops(src, kUTTypeTIFF);
}

