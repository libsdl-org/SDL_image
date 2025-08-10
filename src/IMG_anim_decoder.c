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

#include "IMG_anim_decoder.h"
#include "IMG_webp.h"
#include "IMG_libpng.h"
#include "IMG_gif.h"
#include "IMG_avif.h"

IMG_AnimationDecoder *IMG_CreateAnimationDecoder(const char *file)
{
    if (!file) {
        SDL_InvalidParamError("file");
        return NULL;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    if (!props) {
        return NULL;
    }

    SDL_SetStringProperty(props, IMG_PROP_ANIMATION_DECODER_CREATE_FILENAME_STRING, file);
    IMG_AnimationDecoder *decoder = IMG_CreateAnimationDecoderWithProperties(props);
    SDL_DestroyProperties(props);
    return decoder;
}

IMG_AnimationDecoder *IMG_CreateAnimationDecoder_IO(SDL_IOStream *src, bool closeio, const char *type)
{
    if (!src) {
        SDL_InvalidParamError("src");
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

    SDL_SetPointerProperty(props, IMG_PROP_ANIMATION_DECODER_CREATE_IOSTREAM_POINTER, src);
    SDL_SetBooleanProperty(props, IMG_PROP_ANIMATION_DECODER_CREATE_IOSTREAM_AUTOCLOSE_BOOLEAN, closeio);
    SDL_SetStringProperty(props, IMG_PROP_ANIMATION_DECODER_CREATE_TYPE_STRING, type);
    IMG_AnimationDecoder *decoder = IMG_CreateAnimationDecoderWithProperties(props);
    SDL_DestroyProperties(props);
    return decoder;
}

IMG_AnimationDecoder *IMG_CreateAnimationDecoderWithProperties(SDL_PropertiesID props)
{
    if (!props) {
        SDL_InvalidParamError("props");
        return NULL;
    }

    const char *file = SDL_GetStringProperty(props, IMG_PROP_ANIMATION_DECODER_CREATE_FILENAME_STRING, NULL);
    SDL_IOStream *src = SDL_GetPointerProperty(props, IMG_PROP_ANIMATION_DECODER_CREATE_IOSTREAM_POINTER, NULL);
    bool closeio = SDL_GetBooleanProperty(props, IMG_PROP_ANIMATION_DECODER_CREATE_IOSTREAM_AUTOCLOSE_BOOLEAN, false);
    const char *type = SDL_GetStringProperty(props, IMG_PROP_ANIMATION_DECODER_CREATE_TYPE_STRING, NULL);
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

    if (!src) {
        if (!file) {
            SDL_SetError("No output properties set");
            return NULL;
        }

        src = SDL_IOFromFile(file, "rb");
        if (!src) {
            return NULL;
        }
        closeio = true;
    }

    IMG_AnimationDecoder *decoder = (IMG_AnimationDecoder *)SDL_calloc(1, sizeof(*decoder));
    if (!decoder) {
        goto error;
    }

    decoder->src = src;
    decoder->start = SDL_TellIO(src);
    decoder->closeio = closeio;
    decoder->timebase_numerator = timebase_numerator;
    decoder->timebase_denominator = timebase_denominator;

    decoder->metadata = SDL_CreateProperties();
    if (!decoder->metadata)
    {
        SDL_SetError("Failed to create metadata properties");
        goto error;
    }

    bool result = false;
    if (SDL_strcasecmp(type, "webp") == 0) {
        result = IMG_CreateWEBPAnimationDecoder(decoder, props);
    } else if (SDL_strcasecmp(type, "png") == 0) {
        result = IMG_CreateAPNGAnimationDecoder(decoder, props);
    } else if (SDL_strcasecmp(type, "gif") == 0) {
        result = IMG_CreateGIFAnimationDecoder(decoder, props);
    } else if (SDL_strcasecmp(type, "avifs") == 0) {
        result = IMG_CreateAVIFAnimationDecoder(decoder, props);
    } else {
        SDL_SetError("Unrecognized output type");
    }

    if (result) {
        return decoder;
    }

error:
    if (src) {
        if (closeio) {
            SDL_CloseIO(src);
        } else if (decoder && decoder->start >= 0) {
            SDL_SeekIO(src, decoder->start, SDL_IO_SEEK_SET);
        }
    }
    if (decoder) {
        SDL_free(decoder);
    }
    return NULL;
}

SDL_PropertiesID  IMG_GetAnimationDecoderMetadata(IMG_AnimationDecoder* decoder)
{
    if (!decoder) {
        SDL_InvalidParamError("decoder");
        return 0;
    }

    return decoder->metadata;
}

bool IMG_GetNextAnimationDecoderFrame(IMG_AnimationDecoder *decoder, SDL_Surface** frame, Sint64 *pts)
{
    if (!decoder) {
        return SDL_InvalidParamError("decoder");
    }

    return decoder->GetNextFrame(decoder, frame, pts);
}

bool IMG_CloseAnimationDecoder(IMG_AnimationDecoder *decoder)
{
    if (!decoder) {
        return SDL_InvalidParamError("decoder");
    }

    bool result = decoder->Close(decoder);

    if (decoder->metadata) {
        SDL_DestroyProperties(decoder->metadata);
        decoder->metadata = 0;
    }

    if (decoder->closeio) {
        SDL_CloseIO(decoder->src);
    }

    SDL_free(decoder);

    return result;
}

bool IMG_ResetAnimationDecoder(IMG_AnimationDecoder *decoder)
{
    if (!decoder) {
        return SDL_InvalidParamError("decoder");
    }

    return decoder->Reset(decoder);
}

int IMG_GetAnimationDecoderPresentationTimestampMS(IMG_AnimationDecoder *decoder, Sint64 pts)
{
    return (int)((pts * 1000 * decoder->timebase_numerator) / decoder->timebase_denominator);
}

IMG_Animation *IMG_DecodeAsAnimation(SDL_IOStream *src, const char *format, int maxFrames)
{
    IMG_AnimationDecoder *decoder = IMG_CreateAnimationDecoder_IO(src, false, format);
    if (!decoder) {
        return NULL;
    }

    // We do not rely on the metadata for the count of available frames because some
    // formats like GIF only supports continous decoding and doesn't have any data that
    // states the total available frames inside the binary data.
    //
    // For this reason , we will decode frames until we reach the end of the stream or
    // we reach the maximum number of frames specified by the caller.
    int actualCount = 0;
    int currentCount = 32;
    SDL_Surface **frames = (SDL_Surface **)SDL_calloc(currentCount, sizeof(*frames));
    Sint64 *delays = (Sint64 *)SDL_calloc(currentCount, sizeof(*delays));
    while (true) {
        if (maxFrames > 0 && actualCount >= maxFrames)
            break;

        Sint64 pts = 0;
        SDL_Surface *nextFrame;
        if (!IMG_GetNextAnimationDecoderFrame(decoder, &nextFrame, &pts)) {
            for (int i = 0; i < actualCount; ++i) {
                SDL_DestroySurface(frames[i]);
            }

            SDL_free(frames);
            SDL_free(delays);
            IMG_CloseAnimationDecoder(decoder);
            return NULL;
        }

        if (!nextFrame) {
            // No more frames available or an error occurred.
            break;
        }

        actualCount++;
        if (actualCount > currentCount) {
            currentCount *= 2;
            SDL_Surface **tempFrames = (SDL_Surface **)SDL_realloc(frames, currentCount * sizeof(*tempFrames));
            if (!tempFrames) {
                SDL_SetError("Out of memory for animation frames");
                for (int i = 0; i < actualCount; ++i) {
                    SDL_DestroySurface(frames[i]);
                }

                SDL_free(frames);
                SDL_free(delays);
                IMG_CloseAnimationDecoder(decoder);
                return NULL;
            }

            frames = tempFrames;
            Sint64 *tempDelays = (Sint64 *)SDL_realloc(delays, currentCount * sizeof(*delays));
            if (!tempDelays) {
                SDL_SetError("Out of memory for animation delays");
                for (int i = 0; i < actualCount; ++i) {
                    SDL_DestroySurface(frames[i]);
                }

                SDL_free(frames);
                SDL_free(delays);
                IMG_CloseAnimationDecoder(decoder);
                return NULL;
            }
            delays = tempDelays;
        }

        frames[actualCount - 1] = nextFrame;
        delays[actualCount - 1] = pts;
    }

    if (actualCount < currentCount) {
        //Resize the arrays to the actual count of frames.
        SDL_Surface **tempFrames = (SDL_Surface **)SDL_realloc(frames, actualCount * sizeof(*tempFrames));
        if (!tempFrames) {
            SDL_SetError("Out of memory for animation frames");
            for (int i = 0; i < actualCount; ++i) {
                SDL_DestroySurface(frames[i]);
            }

            SDL_free(frames);
            SDL_free(delays);
            IMG_CloseAnimationDecoder(decoder);
            return NULL;
        }

        frames = tempFrames;

        Sint64 *tempDelays = (Sint64 *)SDL_realloc(delays, actualCount * sizeof(*delays));
        if (!tempDelays) {
            SDL_SetError("Out of memory for animation delays");
            for (int i = 0; i < actualCount; ++i) {
                SDL_DestroySurface(frames[i]);
            }

            SDL_free(frames);
            SDL_free(delays);
            IMG_CloseAnimationDecoder(decoder);
            return NULL;
        }
        delays = tempDelays;
    }

    IMG_CloseAnimationDecoder(decoder);

    if (actualCount < 1) {
        SDL_free(frames);
        SDL_free(delays);
        return NULL;
    }

    int width = frames[0]->w;
    int height = frames[0]->h;

    IMG_Animation *anim = (IMG_Animation *)SDL_calloc(1, sizeof(*anim));
    if (!anim) {
        SDL_SetError("Out of memory for IMG_Animation");
        return NULL;
    }
    anim->count = actualCount;
    anim->w = width;
    anim->h = height;
    anim->delays = (int *)SDL_calloc(anim->count, sizeof(*anim->delays));
    if (!anim->delays) {
        SDL_SetError("Out of memory for animation delays");
        SDL_free(anim);
        for (int i = 0; i < actualCount; ++i) {
            SDL_DestroySurface(frames[i]);
        }

        SDL_free(frames);
        SDL_free(delays);
        return NULL;
    }
    anim->frames = (SDL_Surface **)SDL_calloc(anim->count, sizeof(*anim->frames));
    if (!anim->frames) {
        SDL_SetError("Out of memory for animation frames");
        SDL_free(anim->delays);
        SDL_free(anim);
        for (int i = 0; i < actualCount; ++i) {
            SDL_DestroySurface(frames[i]);
        }

        SDL_free(frames);
        SDL_free(delays);
        return NULL;
    }

    for (int i = 0; i < anim->count; ++i) {
        anim->frames[i] = frames[i];

        Sint64 d;
        if (i == 0 && anim->count > 1) {
            d = delays[i + 1];
        } else {
            d = delays[i];
        }

        if (i > 1) {
            d /= i;
        }

        anim->delays[i] = IMG_GetAnimationDecoderPresentationTimestampMS(decoder, d);
    }

    SDL_free(frames);
    SDL_free(delays);

    return anim;
}
