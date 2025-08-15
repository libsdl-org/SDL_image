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

typedef struct IMG_AnimationDecoderContext IMG_AnimationDecoderContext;

struct IMG_AnimationDecoder
{
    IMG_AnimationDecoderStatus status;
    SDL_PropertiesID props;
    SDL_IOStream *src;
    Sint64 start;
    bool closeio;
    int timebase_numerator;
    int timebase_denominator;
    Uint64 accumulated_pts;

    bool (*GetNextFrame)(IMG_AnimationDecoder *decoder, SDL_Surface **frame, Uint64 *duration);
    bool (*Reset)(IMG_AnimationDecoder *decoder);
    bool (*Close)(IMG_AnimationDecoder *decoder);

    IMG_AnimationDecoderContext *ctx;
};

extern Uint64 IMG_TimebaseDuration(Uint64 pts, Uint64 duration, Uint64 src_numerator, Uint64 src_denominator, Uint64 dst_numerator, Uint64 dst_denominator);
extern Uint64 IMG_GetDecoderDuration(IMG_AnimationDecoder *decoder, Uint64 duration, Uint64 timebase_denominator);

extern IMG_Animation *IMG_DecodeAsAnimation(SDL_IOStream *src, const char *format, int maxFrames);
