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

typedef struct IMG_AnimationEncoderContext IMG_AnimationEncoderContext;

struct IMG_AnimationEncoder
{
    SDL_IOStream *dst;
    Sint64 start;
    bool closeio;
    int quality;
    int timebase_numerator;
    int timebase_denominator;
    Uint64 accumulated_pts;
    Uint64 last_delay;

    bool (*AddFrame)(IMG_AnimationEncoder *encoder, SDL_Surface *surface, Uint64 duration);
    bool (*Close)(IMG_AnimationEncoder *encoder);

    IMG_AnimationEncoderContext *ctx;
};

extern Uint64 IMG_GetResolvedDuration(IMG_AnimationEncoder *encoder, Uint64 duration, int factor);
extern Uint64 IMG_GetCurrentTimestamp(IMG_AnimationEncoder *encoder, Uint64 duration, int factor);
