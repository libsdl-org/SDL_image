
SDL_image 2.0

The latest version of this library is available from:
http://www.libsdl.org/projects/SDL_image/

This is a simple library to load images of various formats as SDL surfaces.
This library supports AVIF, BMP, GIF, JPEG, JPEG-XL, LBM, PCX, PNG, PNM (PPM/PGM/PBM),
QOI, TGA, TIFF, WebP, XCF, XPM, and simple SVG formats.

API:
#include "SDL_image.h"

	SDL_Surface *IMG_Load(const char *file);
or
	SDL_Surface *IMG_Load_RW(SDL_RWops *src, int freesrc);
or
	SDL_Surface *IMG_LoadTyped_RW(SDL_RWops *src, int freesrc, char *type);

where type is a string specifying the format (i.e. "PNG" or "pcx").
Note that IMG_Load_RW cannot load TGA images.

To create a surface from an XPM image included in C source, use:

	SDL_Surface *IMG_ReadXPMFromArray(char **xpm);

An example program 'showimage' is included, with source in showimage.c

This library is under the zlib License, see the file "LICENSE.txt" for details.

Note:
Support for AVIF, JPEG-XL, TIFF, and WebP are not included by default because of the
size of the decode libraries, but you can get them by running external/download.sh
- For UNIX platforms, you can build and install them normally and the configure script
  will pick them up and use them.
- For Windows platforms, you will need to build them and then add the appropriate LOAD_*
  preprocessor define to the Visual Studio project.
- For Apple platforms, you can edit the config at the top of the project to enable them,
  and you will need to include the appropriate framework in your application.
- For Android, you can edit the config at the top of Android.mk to enable them.
