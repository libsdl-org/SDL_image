// Animation Decoder/Encoder Test Suite written by Xen

#include <SDL3/SDL.h>
#include <SDL3/SDL_test.h>
#include <SDL3_image/SDL_image.h>

static bool apply_dummy_metadata;

#if defined(SDL_PLATFORM_OS2) || defined(SDL_PLATFORM_WIN32)
static const char pathsep[] = "\\";
#elif defined(SDL_PLATFORM_RISCOS)
static const char pathsep[] = ".";
#else
static const char pathsep[] = "/";
#endif

#define MAX_METADATA 6

// Add a minimum of two frames so we don't conflict with specific formats (like WEBP).
#define MIN_FRAMES 2

typedef struct {
    int width;
    int height;
    Uint64 duration;
} FrameData;

typedef struct
{
    const char *metadata;
    const void *value;
    SDL_PropertyType type;
} MetadataInfo;

typedef struct {
    const char* format;
    MetadataInfo availableMetadata[MAX_METADATA];
} FormatInfo;

static const Sint64 default_loop_count = 1;
#define DEFAULT_TITLE "Lorem ipsum dolor sit amet"
#define DEFAULT_AUTHOR "consectetur adipiscing elit"
#define DEFAULT_DESCRIPTION "sed do eiusmod tempor <xml escape test> incididunt ut labore et dolore magna aliqua"
#define DEFAULT_CREATION_TIME "Ut enim ad minim veniam"
#define DEFAULT_COPYRIGHT     "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat"

static const FormatInfo formatInfo[] = {
    { "ANI", { { IMG_PROP_METADATA_TITLE_STRING, (const void*)DEFAULT_TITLE, SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_AUTHOR_STRING, (const void*)DEFAULT_AUTHOR, SDL_PROPERTY_TYPE_STRING } } },

    { "APNG", { { IMG_PROP_METADATA_TITLE_STRING, (const void*)DEFAULT_TITLE, SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_AUTHOR_STRING, (const void*)DEFAULT_AUTHOR, SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_DESCRIPTION_STRING, (const void*)DEFAULT_DESCRIPTION, SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_LOOP_COUNT_NUMBER, (const void *)&default_loop_count, SDL_PROPERTY_TYPE_NUMBER }, { IMG_PROP_METADATA_CREATION_TIME_STRING, (const void*)DEFAULT_CREATION_TIME, SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_COPYRIGHT_STRING, (const void*)DEFAULT_COPYRIGHT, SDL_PROPERTY_TYPE_STRING } } },

    { "AVIFS", { { IMG_PROP_METADATA_TITLE_STRING, (const void*)DEFAULT_TITLE, SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_AUTHOR_STRING, (const void*)DEFAULT_AUTHOR, SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_DESCRIPTION_STRING, (const void*)DEFAULT_DESCRIPTION, SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_LOOP_COUNT_NUMBER, (const void *)&default_loop_count, SDL_PROPERTY_TYPE_NUMBER }, { IMG_PROP_METADATA_CREATION_TIME_STRING, (const void*)DEFAULT_CREATION_TIME, SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_COPYRIGHT_STRING, (const void*)DEFAULT_COPYRIGHT, SDL_PROPERTY_TYPE_STRING } } },

    { "GIF", { { IMG_PROP_METADATA_DESCRIPTION_STRING, (const void*)DEFAULT_DESCRIPTION, SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_LOOP_COUNT_NUMBER, (const void *)&default_loop_count, SDL_PROPERTY_TYPE_NUMBER } } },

    { "WEBP", { { IMG_PROP_METADATA_TITLE_STRING, (const void*)DEFAULT_TITLE, SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_AUTHOR_STRING, (const void*)DEFAULT_AUTHOR, SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_DESCRIPTION_STRING, (const void*)DEFAULT_DESCRIPTION, SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_LOOP_COUNT_NUMBER, (const void *)&default_loop_count, SDL_PROPERTY_TYPE_NUMBER }, { IMG_PROP_METADATA_CREATION_TIME_STRING, (const void*)DEFAULT_CREATION_TIME, SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_COPYRIGHT_STRING, (const void*)DEFAULT_COPYRIGHT, SDL_PROPERTY_TYPE_STRING } } },
};


static const struct {
    const char *format;
    const char *filename;
} inputImages[] = {
    { "ANI", "rgbrgb.ani" },
    { "APNG", "rgbrgb.png" },
    { "AVIFS", "rgbrgb.avifs" },
    { "GIF", "rgbrgb.gif" },
    { "WEBP", "rgbrgb.webp" }
};
static const char *outputImageFormats[] = { "ANI", "APNG", "AVIFS", "GIF", "WEBP" };

static const char *GetAnimationDecoderStatusString(IMG_AnimationDecoderStatus status)
{
    switch (status) {
    case IMG_DECODER_STATUS_OK:
        return "IMG_DECODER_STATUS_OK";
    case IMG_DECODER_STATUS_COMPLETE:
        return "IMG_DECODER_STATUS_COMPLETE";
    case IMG_DECODER_STATUS_FAILED:
        return "IMG_DECODER_STATUS_FAILED";
    case IMG_DECODER_STATUS_INVALID:
    default:
        return "IMG_DECODER_STATUS_INVALID";
    }
}

static bool FormatAnimationEnabled(const char *format)
{
    char env_name[64];
    SDL_snprintf(env_name, sizeof(env_name), "SDL_IMAGE_ANIM_%s", format);
    SDL_strupr(env_name);
    const char *env_value = SDL_getenv(env_name);
    if (!env_value) {
        return true;
    }
    if (env_value[0] == '\0' || SDL_strcmp(env_value, "0") == 0 || SDL_strcasecmp(env_value, "no") == 0 || SDL_strcasecmp(env_value, "false") == 0) {
        return false;
    }
    return true;
}

/*
 * Return the absolute path to a resource file, similar to GLib's
 * g_test_build_filename().
 *
 * If type is TEST_FILE_DIST, look for it in $SDL_TEST_SRCDIR or next
 * to the executable.
 *
 * Fails and returns NULL if out of memory.
 */
static char *
GetTestFilename(const char *file)
{
    const char *base;
    char *path = NULL;
    bool needPathSep = true;

    base = SDL_getenv("SDL_TEST_SRCDIR");

    if (base == NULL) {
        base = SDL_GetBasePath();
        /* SDL_GetBasePath() guarantees a trailing path separator */
        needPathSep = false;
    }

    if (base != NULL) {
        size_t len = SDL_strlen(base) + SDL_strlen(pathsep) + SDL_strlen(file) + 1;

        path = SDL_malloc(len);
        if (path == NULL) {
            return NULL;
        }

        if (needPathSep) {
            SDL_snprintf(path, len, "%s%s%s", base, pathsep, file);
        } else {
            SDL_snprintf(path, len, "%s%s", base, file);
        }
    } else {
        path = SDL_strdup(file);
        if (path == NULL) {
            return NULL;
        }
    }

    return path;
}

static int SDLCALL testDecodeEncode(void *args) {
    (void) args;
    SDLTest_Log("Starting Animation Decoder/Encoder Tests -- Decode, encode decoded frames then decode again to compare...");

    SDLTest_Log("--TESTS--");
    for (size_t cim = 0; cim < SDL_arraysize(inputImages); ++cim) {
        for (size_t com = 0; com < SDL_arraysize(outputImageFormats); ++com) {
            const char *inputImage = inputImages[cim].filename;
            const char *outputImageFormat = outputImageFormats[com];
            SDLTest_Log("%s >> %s == %s", inputImage, outputImageFormat, inputImage);
            if (com == 0) {
                SDLTest_Log("--- Rewind Tests for %s ---", inputImage);
                SDLTest_Log("--- Metadata Tests for %s ---", inputImage);
            }
        }
    }

    for (size_t cim = 0; cim < SDL_arraysize(inputImages); ++cim) {
        for (size_t com = 0; com < SDL_arraysize(outputImageFormats); ++com) {
            const char *inputImage = inputImages[cim].filename;
            const char *outputImageFormat = outputImageFormats[com];

            if (!FormatAnimationEnabled(inputImages[cim].format)) {
                SDLTest_Log("Animation format %s disabled (input)", inputImages[cim].format);
                continue;
            }
            if (!FormatAnimationEnabled(outputImageFormats[com])) {
                SDLTest_Log("animation format %s disabled (output)", outputImageFormats[com]);
                continue;
            }

            char *inputImagePath = GetTestFilename(inputImage);
            if (!inputImagePath) {
                SDLTest_LogError("Failed to convert '%s' to absolute path", inputImage);
                return TEST_ABORTED;
            }

            SDLTest_Log("Input Image: %s (%s)", inputImage, inputImagePath);
            SDLTest_Log("Output Format: %s", outputImageFormat);

            IMG_AnimationDecoder *decoder = IMG_CreateAnimationDecoder(inputImagePath);
            SDL_free(inputImagePath);
            SDLTest_AssertCheck(decoder != NULL, "IMG_CreateAnimationDecoder: Failed to create animation decoder for input image: %s", SDL_GetError());
            if (!decoder) {
                continue;
            }
            SDL_IOStream *encoderIO = SDL_IOFromDynamicMem();
            SDLTest_AssertCheck(encoderIO != NULL, "SDL_IOFromDynamicMem");
            if (!encoderIO) {
                SDLTest_LogError("Failed to create IO stream for output image: %s", SDL_GetError());
                IMG_CloseAnimationDecoder(decoder);
                return TEST_ABORTED;
            }

            SDL_PropertiesID encoderProps = SDL_CreateProperties();
            SDLTest_AssertCheck(encoderProps, "SDL_CreateProperties");
            if (!encoderProps) {
                SDLTest_LogError("Failed to create properties for encoder: %s", SDL_GetError());
                IMG_CloseAnimationDecoder(decoder);
                SDL_CloseIO(encoderIO);
                return TEST_ABORTED;
            }
            SDL_SetPointerProperty(encoderProps, IMG_PROP_ANIMATION_ENCODER_CREATE_IOSTREAM_POINTER, encoderIO);
            SDL_SetBooleanProperty(encoderProps, IMG_PROP_ANIMATION_ENCODER_CREATE_IOSTREAM_AUTOCLOSE_BOOLEAN,
                                   false);
            SDL_SetStringProperty(encoderProps, IMG_PROP_ANIMATION_ENCODER_CREATE_TYPE_STRING, outputImageFormat);

            if (apply_dummy_metadata) {
                SDLTest_Log("Applying dummy metadata...");
                // Most formats hold metadata in the beginning of the file (except WEBP and AVIF since those are EXIF and XMP),
                // when that is the case, the changes to metadata parser can destroy the reading of the frames as well since they
                // might do modifications to the stream. For that reason we add dummy metadata to the encoder properties to make
                // sure that the metadata code does not interfere with the frame reading process.
                for (size_t k = 0; k < SDL_arraysize(formatInfo); ++k) {
                    const char *format = formatInfo[k].format;
                    for (int mi = 0; mi < MAX_METADATA; ++mi) {
                        const MetadataInfo *metadata = &formatInfo[k].availableMetadata[mi];
                        if (!metadata || !metadata->metadata || !metadata->value) {
                            continue;
                        }

                        switch (metadata->type) {
                        case SDL_PROPERTY_TYPE_BOOLEAN:
                            SDL_SetBooleanProperty(encoderProps, metadata->metadata, *(bool *) metadata->value);
                            break;
                        case SDL_PROPERTY_TYPE_NUMBER:
                            SDL_SetNumberProperty(encoderProps, metadata->metadata, *(Sint64 *) metadata->value);
                            break;
                        case SDL_PROPERTY_TYPE_STRING:
                            SDL_SetStringProperty(encoderProps, metadata->metadata, (const char *)metadata->value);
                            break;
                        default:
                            SDLTest_AssertCheck(false, "ERROR: Unsupported metadata type for %s: %d", format, metadata->type);
                            SDL_DestroyProperties(encoderProps);
                            IMG_CloseAnimationDecoder(decoder);
                            SDL_CloseIO(encoderIO);
                            return TEST_ABORTED;
                        }
                    }
                }
            }

            IMG_AnimationEncoder *encoder = IMG_CreateAnimationEncoderWithProperties(encoderProps);
            SDLTest_AssertCheck(encoder != NULL, "IMG_CreateAnimationEncoderWithProperties");
            SDL_DestroyProperties(encoderProps);

            int arrayCapacity = 32;
            int arraySize = 0;
            FrameData **decodedFrameData = (FrameData **) SDL_calloc(arrayCapacity, sizeof(*decodedFrameData));

            if (encoder) {
                Uint64 duration;
                SDL_Surface *frame;
                int i = 0, ii = 0;
                while (IMG_GetAnimationDecoderFrame(decoder, &frame, &duration)) {
                    if (frame) {
                        SDLTest_Log("Frame Duration (%i): %" SDL_PRIu64 " ms", i, duration);
                        SDLTest_Log("Frame Format (%i): %s", i, SDL_GetPixelFormatName(frame->format));
                        SDLTest_Log("Frame Size (%i): %ix%i", i, frame->w, frame->h);

                        bool result = IMG_AddAnimationEncoderFrame(encoder, frame, duration);
                        SDLTest_AssertCheck(result, "IMG_AddAnimationEncoderFrame");

                        if (result) {
                            SDLTest_Log("Frame (%i) added to encoder.", i);
                            ii++;
                        } else {
                            SDLTest_Log("ERROR: Failed to add frame (%i) to encoder: %s", i, SDL_GetError());
                            for (int ai = 0; ai < arraySize; ++ai) {
                                SDL_free(decodedFrameData[ai]);
                            }
                            SDL_free(decodedFrameData);
                            IMG_CloseAnimationEncoder(encoder);
                            IMG_CloseAnimationDecoder(decoder);
                            return TEST_ABORTED;
                        }

                        if (arraySize >= arrayCapacity) {
                            arrayCapacity *= 2;
                            FrameData **temp = (FrameData **) SDL_realloc(decodedFrameData, arrayCapacity *
                                                                                            sizeof(*decodedFrameData));
                            SDLTest_AssertCheck(temp != NULL, "SDL_realloc");
                            if (!temp) {
                                SDLTest_LogError("ERROR: Failed to reallocate memory for decoded frames.");
                                for (int ai = 0; ai < arraySize; ++ai) {
                                    SDL_free(decodedFrameData[ai]);
                                }
                                SDL_free(decodedFrameData);
                                IMG_CloseAnimationEncoder(encoder);
                                IMG_CloseAnimationDecoder(decoder);
                                return TEST_ABORTED;
                            }
                            decodedFrameData = temp;
                        }
                        decodedFrameData[arraySize] = (FrameData *) SDL_calloc(1, sizeof(FrameData));
                        SDLTest_AssertCheck(decodedFrameData[arraySize] != NULL, "decodedFrameData[arraySize]");
                        if (!decodedFrameData[arraySize]) {
                            SDLTest_LogError("ERROR: Failed to allocate memory for frame data.");
                            for (int ai = 0; ai < arraySize; ++ai) {
                                SDL_free(decodedFrameData[ai]);
                            }
                            SDL_free(decodedFrameData);
                            IMG_CloseAnimationEncoder(encoder);
                            IMG_CloseAnimationDecoder(decoder);
                            return TEST_ABORTED;
                        }
                        decodedFrameData[arraySize]->width = frame->w;
                        decodedFrameData[arraySize]->height = frame->h;
                        decodedFrameData[arraySize]->duration = duration;
                        arraySize++;
                    } else {
                        SDLTest_AssertCheck(false, "IMG_GetAnimationDecoderFrame");
                        SDLTest_LogError("Failed to get frame: %s", SDL_GetError());
                        for (int ai = 0; ai < arraySize; ++ai) {
                            SDL_free(decodedFrameData[ai]);
                        }
                        SDL_free(decodedFrameData);
                        IMG_CloseAnimationEncoder(encoder);
                        IMG_CloseAnimationDecoder(decoder);
                        return TEST_ABORTED;
                    }
                    i++;
                }

                if (!IMG_CloseAnimationEncoder(encoder)) {
                    SDLTest_AssertCheck(false, "IMG_CloseAnimationEncoder");
                    SDLTest_LogError("Failed to close animation encoder: %s", SDL_GetError());
                    for (int ai = 0; ai < arraySize; ++ai) {
                        SDL_free(decodedFrameData[ai]);
                    }
                    SDL_free(decodedFrameData);
                    IMG_CloseAnimationDecoder(decoder);
                    SDL_CloseIO(encoderIO);
                    return TEST_COMPLETED;
                } else {
                    SDLTest_AssertPass("Encoder closed successfully after adding %i frames.", ii);
                }

                SDLTest_AssertCheck(ii == i, "All frames were added to the encoder. Added %i, Total: %i", ii, i);
                if (ii != i) {
                    for (int ai = 0; ai < arraySize; ++ai) {
                        SDL_free(decodedFrameData[ai]);
                    }
                    SDL_free(decodedFrameData);
                    IMG_CloseAnimationDecoder(decoder);
                    SDL_CloseIO(encoderIO);
                    return TEST_COMPLETED;
                } else {

                    SDLTest_Log("All frames added to the encoder successfully.");
                    SDLTest_Log("Total frames processed: %i", i);
                    SDLTest_Log(
                            "Loading memory stream for %s back to IMG_AnimationDecoder to see if it results the same as %s",
                            outputImageFormat, inputImage);

                    Sint64 newpos = SDL_SeekIO(encoderIO, 0, SDL_IO_SEEK_SET);
                    SDLTest_AssertCheck(newpos == 0, "Failed to seek to the beginning of the IO stream: %s",
                                        SDL_GetError());
                    if (newpos != 0) {
                        for (int ai = 0; ai < arraySize; ++ai) {
                            SDL_free(decodedFrameData[ai]);
                        }
                        SDL_free(decodedFrameData);
                        IMG_CloseAnimationDecoder(decoder);
                        SDL_CloseIO(encoderIO);
                        return TEST_ABORTED;
                    }

                    IMG_AnimationDecoder *decoder2 = IMG_CreateAnimationDecoder_IO(encoderIO, false,
                                                                                   outputImageFormat);
                    SDLTest_AssertCheck(decoder2 != NULL, "IMG_CreateAnimationDecoder_IO");
                    if (decoder2) {
                        bool check_duration = true;
                        Uint64 duration2;
                        SDL_Surface *frame2;
                        int j = 0;
                        if (SDL_strcmp(inputImages[cim].format, "ANI") == 0  || SDL_strcmp(outputImageFormat, "ANI") == 0) {
                            /* ANI uses 1/60 time units which can't represent our 20 ms sample frame durations.
                             * If we switched the sample data to be 100 FPS, that would work for both, but as-is...
                             */
                            check_duration = false;
                        }
                        while (IMG_GetAnimationDecoderFrame(decoder2, &frame2, &duration2)) {
                            SDLTest_Log("Reloaded Frame Duration (%i): %" SDL_PRIu64 " ms", j, duration2);
                            SDLTest_Log("Reloaded Frame Format (%i): %s", j, SDL_GetPixelFormatName(frame2->format));
                            SDLTest_Log("Reloaded Frame Size (%i): %ix%i", j, frame2->w, frame2->h);

                            if (arraySize < j) {
                                SDLTest_LogError("Reloaded frame index %i exceeds decoded frame data size %d", j, arraySize);
                                for (int ai = 0; ai < arraySize; ++ai) {
                                    SDL_free(decodedFrameData[ai]);
                                }
                                SDL_free(decodedFrameData);
                                IMG_CloseAnimationDecoder(decoder2);
                                IMG_CloseAnimationDecoder(decoder);
                                SDL_CloseIO(encoderIO);
                                return TEST_ABORTED;
                            }

                            if (decodedFrameData[j]->width != frame2->w ||
                                decodedFrameData[j]->height != frame2->h ||
                                (check_duration && decodedFrameData[j]->duration != duration2)) {
                                SDLTest_LogError("Frame data mismatch at index %i. Expected (%i, %i, %" SDL_PRIu64 "), Got (%i, %i, %" SDL_PRIu64 ")",
                                    j, decodedFrameData[j]->width, decodedFrameData[j]->height, decodedFrameData[j]->duration, frame2->w, frame2->h, duration2);
                                for (int ai = 0; ai < arraySize; ++ai) {
                                    SDL_free(decodedFrameData[ai]);
                                }
                                SDL_free(decodedFrameData);
                                IMG_CloseAnimationDecoder(decoder2);
                                IMG_CloseAnimationDecoder(decoder);
                                SDL_CloseIO(encoderIO);
                                return TEST_ABORTED;
                            } else {
                                SDLTest_Log("Reloaded frame matches original frame data at index %i.", j);
                            }

                            j++;
                        }

                        if (j != i) {
                            SDLTest_LogError("Frame count mismatch after reloading output. Expected %i, Got %i", i, j);
                            for (int ai = 0; ai < arraySize; ++ai) {
                                SDL_free(decodedFrameData[ai]);
                            }
                            SDL_free(decodedFrameData);
                            IMG_CloseAnimationDecoder(decoder2);
                            IMG_CloseAnimationDecoder(decoder);
                            SDL_CloseIO(encoderIO);
                            return TEST_ABORTED;
                        } else {
                            SDLTest_Log("All frames reloaded successfully from memory stream for %s with frame count %i.",
                                   outputImageFormat, j);
                        }

                        IMG_CloseAnimationDecoder(decoder2);
                    } else {
                        SDLTest_LogError("Failed to create animation decoder for output image: %s",
                                    SDL_GetError());
                        for (int ai = 0; ai < arraySize; ++ai) {
                            SDL_free(decodedFrameData[ai]);
                        }
                        SDL_free(decodedFrameData);
                        IMG_CloseAnimationDecoder(decoder);
                        SDL_CloseIO(encoderIO);
                        return TEST_ABORTED;
                    }
                }

                SDL_CloseIO(encoderIO);
            } else {
                SDLTest_LogError("Failed to create animation encoder for output format %s: %s", outputImageFormat, SDL_GetError());
                for (int ai = 0; ai < arraySize; ++ai) {
                    SDL_free(decodedFrameData[ai]);
                }
                SDL_free(decodedFrameData);
                IMG_CloseAnimationDecoder(decoder);
                return TEST_ABORTED;
            }

            for (int ai = 0; ai < arraySize; ++ai) {
                SDL_free(decodedFrameData[ai]);
            }
            SDL_free(decodedFrameData);

            IMG_CloseAnimationDecoder(decoder);
        }
    }

    SDLTest_Log("Finished test 'Starting Animation Decoder/Encoder Tests'.");
    return TEST_COMPLETED;
}

static int SDLCALL testDecoderRewind(void *args)
{
    (void)args;
    SDLTest_Log("Starting test 'Decoder Rewind Test'");

    for (size_t cim = 0; cim < SDL_arraysize(outputImageFormats); ++cim) {

        const char *outputImageFormat = outputImageFormats[cim];
        if (!FormatAnimationEnabled(outputImageFormats[cim])) {
            SDLTest_Log("animation format %s disabled (output)", outputImageFormats[cim]);
            continue;
        }

        SDL_IOStream *rewindEncoderIO = SDL_IOFromDynamicMem();
        SDLTest_AssertCheck(rewindEncoderIO != NULL, "SDL_IOFromDynamicMem");
        if (!rewindEncoderIO) {
            SDLTest_LogError("Failed to create IO stream for frame encoder: %s", SDL_GetError());
            return TEST_ABORTED;
        }

        IMG_AnimationEncoder *encoder = IMG_CreateAnimationEncoder_IO(rewindEncoderIO, false, outputImageFormat);
        if (!encoder) {
            SDLTest_LogError("Failed to create animation encoder for output format %s: %s", outputImageFormat, SDL_GetError());
            SDL_CloseIO(rewindEncoderIO);
            return TEST_ABORTED;
        }

        // We have to create different colors for each frame to ensure they are distinguishable. Otherwise, some formats like WEBP truncates identical repeating frames.
        for (int fi = 0; fi < MIN_FRAMES; ++fi) {
            SDL_Surface *frame = SDL_CreateSurface(256, 256, SDL_PIXELFORMAT_RGBA32);
            SDLTest_AssertCheck(frame != NULL, "SDL_CreateSurface");
            if (!frame) {
                SDLTest_LogError("Failed to create surface for frame: %s", SDL_GetError());
                IMG_CloseAnimationEncoder(encoder);
                SDL_CloseIO(rewindEncoderIO);
                return TEST_ABORTED;
            }

            const SDL_PixelFormatDetails *pixelFormatDetails = SDL_GetPixelFormatDetails(frame->format);
            bool result = SDL_FillSurfaceRect(frame, NULL, SDL_MapRGBA(pixelFormatDetails, NULL, (Uint8)SDL_rand(256), (Uint8)SDL_rand(256), (Uint8)SDL_rand(256), 255));
            SDLTest_AssertCheck(result, "SDL_FillSurfaceRect: Failed to fill surface with red color: %s", SDL_GetError());

            result = IMG_AddAnimationEncoderFrame(encoder, frame, 1000);
            SDLTest_AssertCheck(result, "IMG_AddAnimationEncoderFrame");
            if (result) {
                SDLTest_Log("Frame %i added to encoder successfully.", fi);
                SDL_DestroySurface(frame);
            } else {
                SDLTest_LogError("Failed to add frame to encoder: %sn", SDL_GetError());
                SDL_DestroySurface(frame);
                IMG_CloseAnimationEncoder(encoder);
                SDL_CloseIO(rewindEncoderIO);
                return TEST_ABORTED;
            }
        }

        if (!IMG_CloseAnimationEncoder(encoder)) {
            SDLTest_LogError("Failed to close animation encoder for frame: %s", SDL_GetError());
            SDL_CloseIO(rewindEncoderIO);
            return TEST_ABORTED;
        }
        SDLTest_Log("Frames encoded successfully for output format %s.", outputImageFormat);
        if (SDL_SeekIO(rewindEncoderIO, 0, SDL_IO_SEEK_SET) != 0) {
            SDLTest_AssertCheck(false, "SDL_SeekIO");
            SDLTest_LogError("Failed to seek to the beginning of the IO stream for frame encoder: %s", SDL_GetError());
            SDL_CloseIO(rewindEncoderIO);
            return TEST_ABORTED;
        }

        IMG_AnimationDecoder *rewindDecoder = IMG_CreateAnimationDecoder_IO(rewindEncoderIO, false, outputImageFormat);
        if (rewindDecoder) {
            SDLTest_Log("Rewind decoder created successfully for output format %s.", outputImageFormat);
            SDLTest_Log("Decoding %i frames first.", MIN_FRAMES);
            for (int ri = 0; ri < MIN_FRAMES; ++ri) {
                if (!IMG_GetAnimationDecoderFrame(rewindDecoder, NULL, NULL)) {
                    SDLTest_AssertCheck(false, "IMG_GetAnimationDecoderFrame");
                    SDLTest_LogError("Failed to get frame %i from rewind decoder. status: %s --- SDL Error: %s", ri, GetAnimationDecoderStatusString(IMG_GetAnimationDecoderStatus(rewindDecoder)), SDL_GetError());
                    IMG_CloseAnimationDecoder(rewindDecoder);
                    SDL_CloseIO(rewindEncoderIO);
                    return TEST_ABORTED;
                }
            }

            SDLTest_Log("Now resetting the animation decoder...");

            if (!IMG_ResetAnimationDecoder(rewindDecoder)) {
                SDLTest_AssertCheck(false, "IMG_ResetAnimationDecoder");
                SDLTest_LogError("Failed to reset rewind decoder: %s", SDL_GetError());
                IMG_CloseAnimationDecoder(rewindDecoder);
                SDL_CloseIO(rewindEncoderIO);
                return TEST_ABORTED;
            }

            SDLTest_Log("Decoding %i frames again after reset.", MIN_FRAMES);

            for (int ri = 0; ri < MIN_FRAMES; ++ri) {
                if (!IMG_GetAnimationDecoderFrame(rewindDecoder, NULL, NULL)) {
                    SDLTest_AssertCheck(false, "IMG_GetAnimationDecoderFrame");
                    SDLTest_LogError("Failed to get frame %i from rewind decoder. status: %s --- SDL Error: %s", ri, GetAnimationDecoderStatusString(IMG_GetAnimationDecoderStatus(rewindDecoder)), SDL_GetError());
                    IMG_CloseAnimationDecoder(rewindDecoder);
                    SDL_CloseIO(rewindEncoderIO);
                    return TEST_ABORTED;
                }
            }
        } else {
            SDLTest_AssertCheck(false, "IMG_CreateAnimationDecoder_IO");
            SDLTest_LogError("Failed to create rewind decoder for input image: %s", SDL_GetError());
            SDL_CloseIO(rewindEncoderIO);
            return TEST_ABORTED;
        }

        SDL_CloseIO(rewindEncoderIO);
        SDLTest_Log("Finished rewind test for output format %s.", outputImageFormat);
    }

    SDLTest_Log("Finished test 'Decoder Rewind Test'.");
    return TEST_COMPLETED;
}

static int SDLCALL testEncodeDecodeMetadata(void *args) {
    (void)args;
    SDLTest_Log("=========================================================");
    SDLTest_Log("=========================================================");
    SDLTest_Log("Starting test 'Encode Metadata and Decode Metadata Test'");

    for (size_t k = 0; k < SDL_arraysize(formatInfo); ++k) {
        const char *format = formatInfo[k].format;
        if (!FormatAnimationEnabled(format)) {
            SDLTest_Log("animation format %s disabled", format);
            continue;
        }
        SDLTest_Log("Testing format: %s", format);
        SDL_IOStream *metadataEncoderIO = SDL_IOFromDynamicMem();
        SDLTest_AssertCheck(metadataEncoderIO != NULL, "SDL_IOFromDynamicMem");
        if (!metadataEncoderIO) {
            SDLTest_LogError("Failed to create IO stream for metadata encoder: %s", SDL_GetError());
            return TEST_ABORTED;
        }

        SDL_PropertiesID metadataProps = SDL_CreateProperties();
        SDLTest_AssertCheck(metadataProps != 0, "SDL_CreateProperties");
        if (!metadataProps) {
            SDLTest_LogError("Failed to create properties for metadata encoder: %s", SDL_GetError());
            SDL_CloseIO(metadataEncoderIO);
            return TEST_ABORTED;
        }

        for (int mi = 0; mi < MAX_METADATA; ++mi) {
            const MetadataInfo *metadata = &formatInfo[k].availableMetadata[mi];
            if (!metadata || !metadata->metadata || !metadata->value) {
                continue;
            }

            switch (metadata->type) {
                case SDL_PROPERTY_TYPE_BOOLEAN:
                    SDL_SetBooleanProperty(metadataProps, metadata->metadata, *(bool *) metadata->value);
                    break;
                case SDL_PROPERTY_TYPE_NUMBER:
                    SDL_SetNumberProperty(metadataProps, metadata->metadata, *(Sint64 *) metadata->value);
                    break;
                case SDL_PROPERTY_TYPE_STRING:
                    SDL_SetStringProperty(metadataProps, metadata->metadata, (const char *) metadata->value);
                    break;
                default:
                    SDLTest_AssertCheck(false, "Unsupported metadata type for %s: %d", metadata->metadata,
                                        metadata->type);
                    SDL_DestroyProperties(metadataProps);
                    SDL_CloseIO(metadataEncoderIO);
                    return TEST_ABORTED;
            }
        }

        SDL_SetStringProperty(metadataProps, IMG_PROP_ANIMATION_ENCODER_CREATE_TYPE_STRING, format);
        SDL_SetPointerProperty(metadataProps, IMG_PROP_ANIMATION_ENCODER_CREATE_IOSTREAM_POINTER, metadataEncoderIO);
        SDL_SetBooleanProperty(metadataProps, IMG_PROP_ANIMATION_ENCODER_CREATE_IOSTREAM_AUTOCLOSE_BOOLEAN, false);

        IMG_AnimationEncoder *metadataEncoder = IMG_CreateAnimationEncoderWithProperties(metadataProps);
        SDLTest_AssertCheck(metadataEncoder != NULL, "IMG_CreateAnimationEncoderWithProperties");
        if (!metadataEncoder) {
            SDLTest_LogError("Failed to create animation encoder for format %s: %s", format, SDL_GetError());
            SDL_DestroyProperties(metadataProps);
            SDL_CloseIO(metadataEncoderIO);
            return TEST_ABORTED;
        }

        SDL_DestroyProperties(metadataProps);

        for (int fi = 0; fi < MIN_FRAMES; ++fi) {
            SDL_Surface *metadataFrame = SDL_CreateSurface(256, 256, SDL_PIXELFORMAT_RGBA32);
            SDLTest_AssertCheck(metadataFrame != NULL, "SDL_CreateSurface");
            if (!metadataFrame) {
                SDLTest_LogError("Failed to create surface for metadata frame: %s", SDL_GetError());
                IMG_CloseAnimationEncoder(metadataEncoder);
                SDL_CloseIO(metadataEncoderIO);
                return TEST_ABORTED;
            }
            const SDL_PixelFormatDetails *metadataPixelFormatDetails = SDL_GetPixelFormatDetails(metadataFrame->format);
            bool result = SDL_FillSurfaceRect(metadataFrame, NULL,
                                              SDL_MapRGBA(metadataPixelFormatDetails, NULL, (Uint8) SDL_rand(256),
                                                          (Uint8) SDL_rand(256), (Uint8) SDL_rand(256), 255));
            SDLTest_AssertCheck(result, "SDL_FillSurfaceRect");
            if (!result) {
                SDLTest_LogError("Failed to fill surface with green color for metadata frame: %s", SDL_GetError());
                SDL_DestroySurface(metadataFrame);
                IMG_CloseAnimationEncoder(metadataEncoder);
                SDL_CloseIO(metadataEncoderIO);
                return TEST_ABORTED;
            }
            if (IMG_AddAnimationEncoderFrame(metadataEncoder, metadataFrame, 1000)) {
                SDLTest_Log("Metadata frame added to encoder successfully.");
                SDL_DestroySurface(metadataFrame);
            } else {
                SDLTest_LogError("Failed to add metadata frame to encoder: %s", SDL_GetError());
                SDL_DestroySurface(metadataFrame);
                IMG_CloseAnimationEncoder(metadataEncoder);
                SDL_CloseIO(metadataEncoderIO);
                return TEST_ABORTED;
            }
        }

        bool result = IMG_CloseAnimationEncoder(metadataEncoder);
        SDLTest_AssertCheck(result, "IMG_CloseAnimationEncoder");
        if (!result) {
            SDLTest_LogError("Failed to close animation encoder for format %s: %s", format, SDL_GetError());
            SDL_CloseIO(metadataEncoderIO);
            return TEST_ABORTED;
        }

        SDLTest_Log("Metadata frame encoded successfully for format %s.", format);
        if (SDL_SeekIO(metadataEncoderIO, 0, SDL_IO_SEEK_SET) != 0) {
            SDLTest_AssertCheck(false, "SDL_SeekIO");
            SDLTest_LogError("Failed to seek to the beginning of the IO stream for metadata encoder: %s",
                             SDL_GetError());
            SDL_CloseIO(metadataEncoderIO);
            return TEST_ABORTED;
        }
        IMG_AnimationDecoder *metadataDecoder = IMG_CreateAnimationDecoder_IO(metadataEncoderIO, false, format);
        SDLTest_AssertCheck(metadataDecoder != NULL, "IMG_CreateAnimationDecoder_IO");
        if (metadataDecoder) {
            SDL_PropertiesID decodedProps = IMG_GetAnimationDecoderProperties(metadataDecoder);
            SDLTest_AssertCheck(decodedProps != 0, "IMG_GetAnimationDecoderProperties");
            if (!decodedProps) {
                SDLTest_LogError("Failed to get properties from metadata decoder: %s", SDL_GetError());
                IMG_CloseAnimationDecoder(metadataDecoder);
                SDL_CloseIO(metadataEncoderIO);
                return TEST_ABORTED;
            }
            for (int i = 0; i < MAX_METADATA; ++i) {
                const MetadataInfo *metadata = &formatInfo[k].availableMetadata[i];
                if (!metadata || !metadata->metadata || !metadata->value) {
                    continue;
                }

                const char *propName = metadata->metadata;
                if (metadata->type == SDL_PROPERTY_TYPE_BOOLEAN) {
                    bool propValue = SDL_GetBooleanProperty(decodedProps, propName, false);
                    SDLTest_Log("Decoded Property %s: %s", propName, propValue ? "true" : "false");
                    const bool expectedValue = *(const bool *) metadata->value;
                    SDLTest_AssertCheck(propValue == expectedValue,
                                        "Decoded boolean property %s should match expected value. Expected: %s, Got: %s",
                                        propName, expectedValue ? "true" : "false", propValue ? "true" : "false");
                    if (propValue != expectedValue) {
                        IMG_CloseAnimationDecoder(metadataDecoder);
                        SDL_CloseIO(metadataEncoderIO);
                        return TEST_ABORTED;
                    }
                } else if (metadata->type == SDL_PROPERTY_TYPE_NUMBER) {
                    Sint64 propValue = SDL_GetNumberProperty(decodedProps, propName, 0);
                    SDLTest_Log("Decoded Property %s: %" SDL_PRIs64 "", propName, propValue);
                    const Sint64 expectedValue = *(const Sint64 *) metadata->value;
                    SDLTest_AssertCheck(propValue == expectedValue,
                                        "Decoded number property %s should match expected value. Expected: %" SDL_PRIs64 ", Got: %" SDL_PRIs64 "",
                                        propName, expectedValue, propValue);
                    if (propValue != expectedValue) {
                        IMG_CloseAnimationDecoder(metadataDecoder);
                        SDL_CloseIO(metadataEncoderIO);
                        return TEST_ABORTED;
                    }
                } else if (metadata->type == SDL_PROPERTY_TYPE_STRING) {
                    const char *propValue = SDL_GetStringProperty(decodedProps, propName, NULL);
                    SDLTest_Log("Decoded Property %s: %s", propName, propValue ? propValue : "NULL");
                    const char *expectedValue = (const char *) metadata->value;
                    if (propValue == NULL && expectedValue == NULL) {
                        SDLTest_Log("Both decoded and expected string properties are NULL for %s.", propName);
                    } else if (propValue == NULL || expectedValue == NULL ||
                               SDL_strcmp(propValue, expectedValue) != 0) {
                        SDLTest_AssertCheck(false,
                                            "Decoded string property %s does not match expected value. Expected: %s, Got: %s",
                                            propName, expectedValue ? expectedValue : "NULL",
                                            propValue ? propValue : "NULL");
                        IMG_CloseAnimationDecoder(metadataDecoder);
                        SDL_CloseIO(metadataEncoderIO);
                        return TEST_ABORTED;
                    }
                } else {
                    SDLTest_LogError("Unsupported metadata type for %s: %d", propName, metadata->type);
                    IMG_CloseAnimationDecoder(metadataDecoder);
                    SDL_CloseIO(metadataEncoderIO);
                    return TEST_ABORTED;
                }
            }
            IMG_CloseAnimationDecoder(metadataDecoder);
        } else {
            SDLTest_AssertCheck(false, "Failed to create animation decoder for format %s: %s", format, SDL_GetError());
            SDL_CloseIO(metadataEncoderIO);
            SDL_Quit();
            return TEST_ABORTED;
        }
        SDL_CloseIO(metadataEncoderIO);
        SDLTest_Log("Finished metadata encoding and decoding test for format %s.", format);
    }
    SDLTest_Log("Finished test 'Encode Metadata and Decode Metadata Test'.");
    return TEST_COMPLETED;
}

static int SDLCALL testDecodeThirdPartyMetadata(void *args)
{
    (void)args;
    SDLTest_Log("Starting test 'Decode Third Party Metadata Test'");

    if (!FormatAnimationEnabled("webp")) {
        SDLTest_Log("Animation format webp disabled, skipping test");
        return TEST_SKIPPED;
    }

    IMG_AnimationDecoder *thirdPartyDecoder = IMG_CreateAnimationDecoder("rgbrgb_thirdpartymetadata.webp");
    SDLTest_AssertCheck(thirdPartyDecoder != NULL, "IMG_CreateAnimationDecoder");
    if (thirdPartyDecoder) {
        SDL_PropertiesID thirdPartyMetadata = IMG_GetAnimationDecoderProperties(thirdPartyDecoder);
        SDLTest_AssertCheck(thirdPartyMetadata != 0, "IMG_GetAnimationDecoderProperties");
        if (!thirdPartyMetadata) {
            SDLTest_LogError("Failed to get properties from third party metadata decoder: %s", SDL_GetError());
            IMG_CloseAnimationDecoder(thirdPartyDecoder);
            return TEST_ABORTED;
        }

        const char *xmpDesc = SDL_GetStringProperty(thirdPartyMetadata, IMG_PROP_METADATA_DESCRIPTION_STRING, NULL);
        const char *xmpRights = SDL_GetStringProperty(thirdPartyMetadata, IMG_PROP_METADATA_COPYRIGHT_STRING, NULL);

        // Both of them have to be valid.
        SDLTest_AssertCheck(xmpDesc != NULL && xmpRights != NULL, "Failed to get XMP Description or Rights from third party metadata decoder. Description: %s, Rights: %s",
                xmpDesc ? xmpDesc : "NULL", xmpRights ? xmpRights : "NULL");
        if (!xmpDesc || !xmpRights) {
            IMG_CloseAnimationDecoder(thirdPartyDecoder);
            return TEST_ABORTED;
        }

        size_t descLen = SDL_strnlen(xmpDesc, 256);
        size_t rightsLen = SDL_strnlen(xmpRights, 256);

        SDLTest_AssertCheck(descLen == 18, "XMP Description length mismatch. Expected: 18, Got: %" SDL_PRIu64, (Uint64)descLen);
        if (descLen != 18) {
            IMG_CloseAnimationDecoder(thirdPartyDecoder);
            return TEST_ABORTED;
        }

        SDLTest_AssertCheck(rightsLen == 16, "XMP Rights length mismatch. Expected: 16, Got: %" SDL_PRIu64, (Uint64)rightsLen);
        if (rightsLen != 16) {
            IMG_CloseAnimationDecoder(thirdPartyDecoder);
            return TEST_ABORTED;
        }

        SDLTest_AssertCheck(SDL_strncmp(xmpDesc, "Test <Description>", 18) == 0, "XMP Description content mismatch. Expected: 'Test <Description>', Got: '%s'", xmpDesc);
        if (SDL_strncmp(xmpDesc, "Test <Description>", 18) != 0) {
            IMG_CloseAnimationDecoder(thirdPartyDecoder);
            return TEST_ABORTED;
        }

        SDLTest_AssertCheck(SDL_strncmp(xmpRights, "Test <Copyright>", 16) == 0, "XMP Rights content mismatch. Expected: 'Test <Copyright>', Got: '%s'", xmpRights);
        if (SDL_strncmp(xmpRights, "Test <Copyright>", 16) != 0) {
            IMG_CloseAnimationDecoder(thirdPartyDecoder);
            return TEST_ABORTED;
        }

        SDLTest_Log("Third party metadata decoded successfully. Description: '%s', Rights: '%s'", xmpDesc, xmpRights);
        IMG_CloseAnimationDecoder(thirdPartyDecoder);
    } else {
        SDLTest_LogError("Failed to create animation decoder for third party metadata test image: %s", SDL_GetError());
        return TEST_ABORTED;
    }

    SDLTest_Log("Finished test 'Decode Third Party Metadata Test'.");
    return TEST_COMPLETED;
}

static const SDLTest_TestCaseReference decodeEncodeAnimations = {
    testDecodeEncode, "decode_encode_animation", "Animation Decoder/Encoder Tests -- Decode, encode decoded frames then decode again to compare...", TEST_ENABLED
};

static const SDLTest_TestCaseReference decoderRewindAnimations = {
    testDecoderRewind, "decoder_rewind", "Rewind Animatino decoder", TEST_ENABLED
};

static const SDLTest_TestCaseReference animationMetadata = {
    testEncodeDecodeMetadata, "animation_metadata", "Encode Metadata and Decode Metadata", TEST_ENABLED
};

static const SDLTest_TestCaseReference decodeThirdPartyMetadata = {
    testDecodeThirdPartyMetadata, "animation_decodeThirdPartyMetadata", "Decode Third Party Metadata", TEST_ENABLED
};

static const SDLTest_TestCaseReference *animationTests[] = {
    &decodeEncodeAnimations,
    &decoderRewindAnimations,
    &animationMetadata,
    &decodeThirdPartyMetadata,
    NULL
};

static SDLTest_TestSuiteReference animationTestSuite = {
    "animation",
    NULL,
    animationTests,
    NULL
};

static SDLTest_TestSuiteReference *testSuites[] = {
    &animationTestSuite,
    NULL
};

int main(int argc, char *argv[])
{
    int result;
    SDLTest_CommonState *state;
    SDLTest_TestSuiteRunner *runner;

    /* Initialize test framework */
    state = SDLTest_CommonCreateState(argv, 0);
    if (!state) {
        return 1;
    }

    runner = SDLTest_CreateTestSuiteRunner(state, testSuites);

    /* Parse commandline */
    for (int i = 1; i < argc;) {
        int consumed;

        consumed = SDLTest_CommonArg(state, i);
        if (!consumed) {
            if (SDL_strcmp(argv[i], "--no-dummy-metadata") == 0) {
                apply_dummy_metadata = false;
                consumed = 1;
            }
        }
        if (consumed <= 0) {
            const char *options[] = {
                "[--no-dummy-metadata]",
                NULL
            };
            SDLTest_CommonLogUsage(state, argv[0], options);
            return 1;
        }

        i += consumed;
    }

    SDL_Log("      SDL version: %i.%i.%i", SDL_VERSIONNUM_MAJOR(SDL_GetVersion()), SDL_VERSIONNUM_MINOR(SDL_GetVersion()), SDL_VERSIONNUM_MICRO(SDL_GetVersion()));
    SDL_Log("SDL_image version: %i.%i.%i", SDL_VERSIONNUM_MAJOR(IMG_Version()), SDL_VERSIONNUM_MINOR(IMG_Version()), SDL_VERSIONNUM_MICRO(IMG_Version()));

    result = SDLTest_ExecuteTestSuiteRunner(runner);

    SDL_Quit();
    SDLTest_DestroyTestSuiteRunner(runner);
    SDLTest_CommonDestroyState(state);
    return result;
}
