/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

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
 * This file contains the XML management functions for SDL_image.
 *
 * It provides functions to parse and manage XML data, which can be used
 * for various purposes such as configuration, metadata, or other structured data.
 */

#ifndef IMG_XML_MAN
#define IMG_XML_MAN
#ifdef __cplusplus
extern "C" {
#endif
extern const char *__xmlman_GetXMPDescription(const uint8_t *data, size_t len);
extern const char *__xmlman_GetXMPCopyright(const uint8_t *data, size_t len);
extern const char *__xmlman_GetXMPTitle(const uint8_t *data, size_t len);
extern const char *__xmlman_GetXMPCreator(const uint8_t *data, size_t len);
extern const char *__xmlman_GetXMPCreateDate(const uint8_t *data, size_t len);
extern uint8_t *__xmlman_ConstructXMPWithRDFDescription(const char *dctitle, const char *dccreator, const char *dcdescription, const char *dcrights, const char *xmpcreatedate, size_t *outlen);
#ifdef __cplusplus
}
#endif
#endif /* IMG_XML_MAN */
