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

#include <SDL3_image/SDL_image.h>

#include "IMG_anim.h"
#include "IMG_webp.h"


IMG_AnimationStream *IMG_CreateAnimationStream(const char *file)
{
    if (!file) {
        SDL_InvalidParamError("file");
        return NULL;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    if (!props) {
        return NULL;
    }

    SDL_SetStringProperty(props, IMG_PROP_ANIMATION_STREAM_CREATE_FILENAME_STRING, file);
    IMG_AnimationStream *stream = IMG_CreateAnimationStreamWithProperties(props);
    SDL_DestroyProperties(props);
    return stream;
}

IMG_AnimationStream *IMG_CreateAnimationStream_IO(SDL_IOStream *dst, bool closeio, const char *type)
{
    if (!dst) {
        SDL_InvalidParamError("dst");
        return NULL;
    }

    if (!type) {
        SDL_InvalidParamError("type");
        return NULL;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    if (!props) {
        return NULL;
    }

    SDL_SetPointerProperty(props, IMG_PROP_ANIMATION_STREAM_CREATE_IOSTREAM_POINTER, dst);
    SDL_SetBooleanProperty(props, IMG_PROP_ANIMATION_STREAM_CREATE_IOSTREAM_AUTOCLOSE_BOOLEAN, closeio);
    SDL_SetStringProperty(props, IMG_PROP_ANIMATION_STREAM_CREATE_TYPE_STRING, type);
    IMG_AnimationStream *stream = IMG_CreateAnimationStreamWithProperties(props);
    SDL_DestroyProperties(props);
    return stream;
}

IMG_AnimationStream *IMG_CreateAnimationStreamWithProperties(SDL_PropertiesID props)
{
    if (!props) {
        SDL_InvalidParamError("props");
        return NULL;
    }

    const char *file = SDL_GetStringProperty(props, IMG_PROP_ANIMATION_STREAM_CREATE_FILENAME_STRING, NULL);
    SDL_IOStream *dst = SDL_GetPointerProperty(props, IMG_PROP_ANIMATION_STREAM_CREATE_IOSTREAM_POINTER, NULL);
    bool closeio = SDL_GetBooleanProperty(props, IMG_PROP_ANIMATION_STREAM_CREATE_IOSTREAM_AUTOCLOSE_BOOLEAN, false);
    const char *type = SDL_GetStringProperty(props, IMG_PROP_ANIMATION_STREAM_CREATE_TYPE_STRING, NULL);
    int timebase_numerator = (int)SDL_GetNumberProperty(props, IMG_PROP_ANIMATION_STREAM_CREATE_TIMEBASE_NUMERATOR_NUMBER, 1);
    int timebase_denominator = (int)SDL_GetNumberProperty(props, IMG_PROP_ANIMATION_STREAM_CREATE_TIMEBASE_DENOMINATOR_NUMBER, 1000);

    if (!type || !*type) {
        if (file) {
            type = SDL_strrchr(file, '.');
            if (type) {
                // Skip the '.' in the file extension
                ++type;
            }
        }
        if (!type || !*type) {
            SDL_SetError("Couldn't determine file type");
            return NULL;
        }
    }

    if (timebase_numerator <= 0) {
        SDL_SetError("Time base numerator must be > 0");
        return NULL;
    }

    if (timebase_denominator <= 0) {
        SDL_SetError("Time base denominator must be > 0");
        return NULL;
    }

    if (!dst) {
        if (!file) {
            SDL_SetError("No output properties set");
            return NULL;
        }

        dst = SDL_IOFromFile(file, "wb");
        if (!dst) {
            return NULL;
        }
        closeio = true;
    }

    IMG_AnimationStream *stream = (IMG_AnimationStream *)SDL_calloc(1, sizeof(*stream));
    if (!stream) {
        goto error;
    }

    stream->dst = dst;
    stream->start = SDL_TellIO(dst);
    stream->closeio = closeio;
    stream->quality = (int)SDL_GetNumberProperty(props, IMG_PROP_ANIMATION_STREAM_CREATE_QUALITY_NUMBER, -1);
    stream->timebase_numerator = timebase_numerator;
    stream->timebase_denominator = timebase_denominator;

    bool result = false;
    if (SDL_strcasecmp(type, "webp") == 0) {
        result = IMG_CreateWEBPAnimationStream(stream, props);
    } else {
        SDL_SetError("Unrecognized output type");
    }
    if (result) {
        return stream;
    }

error:
    if (dst) {
        if (closeio) {
            SDL_CloseIO(dst);
        } else if (stream && stream->start >= 0) {
            SDL_SeekIO(dst, stream->start, SDL_IO_SEEK_SET);
        }
    }
    if (stream) {
        SDL_free(stream);
    }
    return NULL;
}

bool IMG_AddAnimationFrame(IMG_AnimationStream *stream, SDL_Surface *surface, Uint64 pts)
{
    if (!stream) {
        return SDL_InvalidParamError("stream");
    }
    if (!surface || surface->w <= 0 || surface->h <= 0) {
        return SDL_InvalidParamError("surface");
    }

    // Make sure the presentation timestamp starts at 0
    if (!stream->first_pts) {
        stream->first_pts = pts;
    }
    pts -= stream->first_pts;

    // Make sure the presentation timestamp isn't out of order
    if (pts < stream->last_pts) {
        return SDL_SetError("Frame added out of order");
    }

    bool result = stream->AddFrame(stream, surface, pts);

    // Save the last presentation timestamp
    stream->last_pts = pts;

    return result;
}

bool IMG_CloseAnimationStream(IMG_AnimationStream *stream)
{
    if (!stream) {
        return SDL_InvalidParamError("stream");
    }

    bool result = stream->Close(stream);
    if (stream->closeio) {
        SDL_CloseIO(stream->dst);
    }
    SDL_free(stream);
    return result;
}

int GetStreamPresentationTimestampMS(IMG_AnimationStream *stream, Uint64 pts)
{
    return (int)((pts * 1000 * stream->timebase_numerator) / stream->timebase_denominator);
}

