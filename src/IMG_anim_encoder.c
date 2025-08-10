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

#include "IMG_anim_encoder.h"
#include "IMG_webp.h"
#include "IMG_libpng.h"
#include "IMG_gif.h"
#include "IMG_avif.h"

IMG_AnimationEncoder *IMG_CreateAnimationEncoder(const char *file)
{
    if (!file || !*file) {
        SDL_InvalidParamError("file");
        return NULL;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    if (!props) {
        return NULL;
    }

    SDL_SetStringProperty(props, IMG_PROP_ANIMATION_ENCODER_CREATE_FILENAME_STRING, file);
    IMG_AnimationEncoder *encoder = IMG_CreateAnimationEncoderWithProperties(props);
    SDL_DestroyProperties(props);
    return encoder;
}

IMG_AnimationEncoder *IMG_CreateAnimationEncoder_IO(SDL_IOStream *dst, bool closeio, const char *type)
{
    if (!dst) {
        SDL_InvalidParamError("dst");
        return NULL;
    }

    if (!type || !*type) {
        SDL_InvalidParamError("type");
        return NULL;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    if (!props) {
        return NULL;
    }

    SDL_SetPointerProperty(props, IMG_PROP_ANIMATION_ENCODER_CREATE_IOSTREAM_POINTER, dst);
    SDL_SetBooleanProperty(props, IMG_PROP_ANIMATION_ENCODER_CREATE_IOSTREAM_AUTOCLOSE_BOOLEAN, closeio);
    SDL_SetStringProperty(props, IMG_PROP_ANIMATION_ENCODER_CREATE_TYPE_STRING, type);
    IMG_AnimationEncoder *encoder = IMG_CreateAnimationEncoderWithProperties(props);
    SDL_DestroyProperties(props);
    return encoder;
}

IMG_AnimationEncoder *IMG_CreateAnimationEncoderWithProperties(SDL_PropertiesID props)
{
    if (!props) {
        SDL_InvalidParamError("props");
        return NULL;
    }

    const char *file = SDL_GetStringProperty(props, IMG_PROP_ANIMATION_ENCODER_CREATE_FILENAME_STRING, NULL);
    SDL_IOStream *dst = SDL_GetPointerProperty(props, IMG_PROP_ANIMATION_ENCODER_CREATE_IOSTREAM_POINTER, NULL);
    bool closeio = SDL_GetBooleanProperty(props, IMG_PROP_ANIMATION_ENCODER_CREATE_IOSTREAM_AUTOCLOSE_BOOLEAN, false);
    const char *type = SDL_GetStringProperty(props, IMG_PROP_ANIMATION_ENCODER_CREATE_TYPE_STRING, NULL);
    int timebase_numerator = (int)SDL_GetNumberProperty(props, IMG_PROP_ANIMATION_ENCODER_CREATE_TIMEBASE_NUMERATOR_NUMBER, 1);
    int timebase_denominator = (int)SDL_GetNumberProperty(props, IMG_PROP_ANIMATION_ENCODER_CREATE_TIMEBASE_DENOMINATOR_NUMBER, 1000);

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

    IMG_AnimationEncoder *encoder = (IMG_AnimationEncoder *)SDL_calloc(1, sizeof(*encoder));
    if (!encoder) {
        goto error;
    }

    encoder->dst = dst;
    encoder->start = SDL_TellIO(dst);
    encoder->closeio = closeio;
    encoder->quality = (int)SDL_GetNumberProperty(props, IMG_PROP_ANIMATION_ENCODER_CREATE_QUALITY_NUMBER, -1);
    encoder->timebase_numerator = timebase_numerator;
    encoder->timebase_denominator = timebase_denominator;

    bool result = false;
    if (SDL_strcasecmp(type, "webp") == 0) {
        result = IMG_CreateWEBPAnimationEncoder(encoder, props);
    } else if (SDL_strcasecmp(type, "png") == 0) {
        result = IMG_CreateAPNGAnimationEncoder(encoder, props);
    } else if (SDL_strcasecmp(type, "gif") == 0) {
        result = IMG_CreateGIFAnimationEncoder(encoder, props);
    } else if (SDL_strcasecmp(type, "avifs") == 0) {
        result = IMG_CreateAVIFAnimationEncoder(encoder, props);
    } else {
        SDL_SetError("Unrecognized output type");
    }
    if (result) {
        return encoder;
    }

error:
    if (dst) {
        if (closeio) {
            SDL_CloseIO(dst);
        } else if (encoder && encoder->start >= 0) {
            SDL_SeekIO(dst, encoder->start, SDL_IO_SEEK_SET);
        }
    }
    if (encoder) {
        SDL_free(encoder);
    }
    return NULL;
}

bool IMG_AddAnimationEncoderFrame(IMG_AnimationEncoder *encoder, SDL_Surface *surface, Uint64 pts)
{
    if (!encoder) {
        return SDL_InvalidParamError("encoder");
    }
    if (!surface || surface->w <= 0 || surface->h <= 0) {
        return SDL_InvalidParamError("surface");
    }

    // Make sure the presentation timestamp starts at 0
    if (!encoder->first_pts) {
        encoder->first_pts = pts;
    }
    pts -= encoder->first_pts;

    // Make sure the presentation timestamp isn't out of order
    if (pts < encoder->last_pts) {
        return SDL_SetError("Frame added out of order");
    }

    bool result = encoder->AddFrame(encoder, surface, pts);

    // Save the last presentation timestamp
    encoder->last_pts = pts;

    return result;
}

bool IMG_CloseAnimationEncoder(IMG_AnimationEncoder *encoder)
{
    if (!encoder) {
        return SDL_InvalidParamError("encoder");
    }

    bool result = encoder->Close(encoder);
    if (encoder->closeio) {
        SDL_CloseIO(encoder->dst);
    }
    SDL_free(encoder);
    return result;
}

int GetStreamPresentationTimestampMS(IMG_AnimationEncoder *encoder, Uint64 pts)
{
    return (int)((pts * 1000 * encoder->timebase_numerator) / encoder->timebase_denominator);
}

