// Animation Decoder/Encoder Test Suite written by Xen

#include <stddef.h>
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

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
    void *value;
    SDL_PropertyType type;
} MetadataInfo;

typedef struct {
    const char* format;
    MetadataInfo availableMetadata[MAX_METADATA];
} FormatInfo;

static const FormatInfo formatInfo[] = {
        { "PNG", { { IMG_PROP_METADATA_TITLE_STRING, "Test Animation", SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_AUTHOR_STRING, "Xen", SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_DESCRIPTION_STRING, "Love SDL", SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_LOOP_COUNT_NUMBER, (void*)(Sint64)1, SDL_PROPERTY_TYPE_NUMBER }, { IMG_PROP_METADATA_CREATION_TIME_STRING, "2077", SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_COPYRIGHT_STRING, "Copyright 2077 SDL_image Test Suite", SDL_PROPERTY_TYPE_STRING } } },
        { "GIF", { { IMG_PROP_METADATA_DESCRIPTION_STRING, "Love SDL", SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_LOOP_COUNT_NUMBER, (void*)(Sint64)1, SDL_PROPERTY_TYPE_NUMBER } } },
        { "WEBP", { { IMG_PROP_METADATA_TITLE_STRING, "Test Animation", SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_AUTHOR_STRING, "Xen", SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_DESCRIPTION_STRING, "Love SDL", SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_LOOP_COUNT_NUMBER, (void*)(Sint64)1, SDL_PROPERTY_TYPE_NUMBER }, { IMG_PROP_METADATA_CREATION_TIME_STRING, "2077", SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_COPYRIGHT_STRING, "Copyright 2077 SDL_image Test Suite", SDL_PROPERTY_TYPE_STRING } } },
        { "AVIFS", { { IMG_PROP_METADATA_TITLE_STRING, "Test Animation", SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_AUTHOR_STRING, "Xen", SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_DESCRIPTION_STRING, "Love SDL", SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_LOOP_COUNT_NUMBER, (void*)(Sint64)1, SDL_PROPERTY_TYPE_NUMBER }, { IMG_PROP_METADATA_CREATION_TIME_STRING, "2077", SDL_PROPERTY_TYPE_STRING }, { IMG_PROP_METADATA_COPYRIGHT_STRING, "Copyright 2077 SDL_image Test Suite", SDL_PROPERTY_TYPE_STRING } } },
    };

static const char *inputImages[] = { "rgbrgb.png", "rgbrgb.gif", "rgbrgb.webp", "rgbrgb.avifs" };
static const char *outputImageFormats[] = { "PNG", "GIF", "WEBP", "AVIFS" };

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
        return "IMG_DECODER_STATUS_INVALID";
    }
}

int main()
{
    printf("SDL Version: %i\n", SDL_GetVersion());
    printf("SDL_image Version: %i\n", IMG_Version());

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "ERROR: Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    size_t numFormatInfo = sizeof(formatInfo) / sizeof(formatInfo[0]);
    size_t numInputImages = sizeof(inputImages) / sizeof(inputImages[0]);
    size_t numOutputImageFormats = sizeof(outputImageFormats) / sizeof(outputImageFormats[0]);

    printf("Starting Animation Decoder/Encoder Tests -- Decode, encode decoded frames then decode again to compare...\n");

    printf("--TESTS--\n");
    for (int cim = 0; cim < numInputImages; ++cim) {
        for (int com = 0; com < numOutputImageFormats; ++com) {
            const char *inputImage = inputImages[cim];
            const char *outputImageFormat = outputImageFormats[com];
            printf("%s >> %s == %s\n", inputImage, outputImageFormat, inputImage);
            if (com == 0) {
                printf("--- Rewind Tests for %s ---\n", inputImage);
                printf("--- Metadata Tests for %s ---\n", inputImage);
            }
        }
    }

    for (int cim = 0; cim < numInputImages; ++cim) {
        for (int com = 0; com < numOutputImageFormats; ++com) {
            const char *inputImage = inputImages[cim];
            const char *outputImageFormat = outputImageFormats[com];

            printf("=========================================================\n");
            printf("=========================================================\n");
            printf("Input Image: %s\n", inputImage);
            printf("Output Format: %s\n", outputImageFormat);
            printf("=========================================================\n");

            IMG_AnimationDecoder *decoder = IMG_CreateAnimationDecoder(inputImage);
            if (decoder) {

                SDL_IOStream *encoderIO = SDL_IOFromDynamicMem();
                if (!encoderIO) {
                    fprintf(stderr, "ERROR: Failed to create IO stream for output image: %s\n", SDL_GetError());
                    IMG_CloseAnimationDecoder(decoder);
                    SDL_Quit();
                    return 1;
                }

                SDL_PropertiesID encoderProps = SDL_CreateProperties();
                if (!encoderProps) {
                    fprintf(stderr, "ERROR: Failed to create properties for encoder: %s\n", SDL_GetError());
                    IMG_CloseAnimationDecoder(decoder);
                    SDL_CloseIO(encoderIO);
                    SDL_Quit();
                    return 1;
                }
                SDL_SetPointerProperty(encoderProps, IMG_PROP_ANIMATION_ENCODER_CREATE_IOSTREAM_POINTER, encoderIO);
                SDL_SetBooleanProperty(encoderProps, IMG_PROP_ANIMATION_ENCODER_CREATE_IOSTREAM_AUTOCLOSE_BOOLEAN, false);
                SDL_SetStringProperty(encoderProps, IMG_PROP_ANIMATION_ENCODER_CREATE_TYPE_STRING, outputImageFormat);

                // Most formats hold metadata in the beginning of the file (except WEBP and AVIF since those are EXIF and XMP),
                // when that is the case, the changes to metadata parser can destroy the reading of the frames as well since they
                // might do modifications to the stream. For that reason we add dummy metadata to the encoder properties to make
                // sure that the metadata code does not interfere with the frame reading process.
                for (int k = 0; k < numFormatInfo; ++k) {
                    const char *format = formatInfo[k].format;
                    for (int mi = 0; mi < MAX_METADATA; ++mi) {
                        const MetadataInfo *metadata = &formatInfo[k].availableMetadata[mi];
                        if (!metadata || !metadata->metadata || !metadata->value) {
                            continue;
                        }

                        switch (metadata->type) {
                        case SDL_PROPERTY_TYPE_BOOLEAN:
                            SDL_SetBooleanProperty(encoderProps, metadata->metadata, (bool)metadata->value);
                            break;
                        case SDL_PROPERTY_TYPE_NUMBER:
                            SDL_SetNumberProperty(encoderProps, metadata->metadata, (Sint64)metadata->value);
                            break;
                        case SDL_PROPERTY_TYPE_STRING:
                            SDL_SetStringProperty(encoderProps, metadata->metadata, (const char *)metadata->value);
                            break;
                        default:
                            fprintf(stderr, "ERROR: Unsupported metadata type for %s: %d\n", format, metadata->type);
                            SDL_DestroyProperties(encoderProps);
                            IMG_CloseAnimationDecoder(decoder);
                            SDL_CloseIO(encoderIO);
                            SDL_Quit();
                            return 1;
                        }
                    }
                }

                IMG_AnimationEncoder *encoder = IMG_CreateAnimationEncoderWithProperties(encoderProps);
                SDL_DestroyProperties(encoderProps);

                int arrayCapacity = 32;
                int arraySize = 0;
                FrameData **decodedFrameData = (FrameData **)SDL_calloc(arrayCapacity, sizeof(*decodedFrameData));

                if (encoder) {
                    Uint64 duration;
                    SDL_Surface *frame;
                    Uint64 i = 0;
                    Uint64 ii = 0;
                    while (IMG_GetAnimationDecoderFrame(decoder, &frame, &duration)) {
                        if (frame) {
                            printf("Frame Duration (%llu): %llu ms\n", i, duration);
                            printf("Frame Format (%llu): %s\n", i, SDL_GetPixelFormatName(frame->format));
                            printf("Frame Size (%llu): %ix%i\n", i, frame->w, frame->h);

                            if (IMG_AddAnimationEncoderFrame(encoder, frame, duration)) {
                                printf("Frame (%llu) added to encoder.\n", i);
                                ii++;
                            } else {
                                fprintf(stderr, "ERROR: Failed to add frame (%llu) to encoder: %s\n", i, SDL_GetError());
                                for (int ai = 0; ai < arraySize; ++ai) {
                                    SDL_free(decodedFrameData[ai]);
                                }
                                SDL_free(decodedFrameData);
                                IMG_CloseAnimationEncoder(encoder);
                                IMG_CloseAnimationDecoder(decoder);
                                SDL_Quit();
                                return 1;
                            }

                            if (arraySize >= arrayCapacity) {
                                arrayCapacity *= 2;
                                FrameData **temp = (FrameData **)SDL_realloc(decodedFrameData, arrayCapacity * sizeof(*decodedFrameData));
                                if (!temp) {
                                    fprintf(stderr, "ERROR: Failed to reallocate memory for decoded frames.\n");
                                    for (int ai = 0; ai < arraySize; ++ai) {
                                        SDL_free(decodedFrameData[ai]);
                                    }
                                    SDL_free(decodedFrameData);
                                    IMG_CloseAnimationEncoder(encoder);
                                    IMG_CloseAnimationDecoder(decoder);
                                    SDL_Quit();
                                    return 1;
                                }
                                decodedFrameData = temp;
                            }
                            decodedFrameData[arraySize] = (FrameData *)SDL_calloc(1, sizeof(FrameData));
                            if (!decodedFrameData[arraySize]) {
                                fprintf(stderr, "ERROR: Failed to allocate memory for frame data.\n");
                                for (int ai = 0; ai < arraySize; ++ai) {
                                    SDL_free(decodedFrameData[ai]);
                                }
                                SDL_free(decodedFrameData);
                                IMG_CloseAnimationEncoder(encoder);
                                IMG_CloseAnimationDecoder(decoder);
                                SDL_Quit();
                                return 1;
                            }
                            decodedFrameData[arraySize]->width = frame->w;
                            decodedFrameData[arraySize]->height = frame->h;
                            decodedFrameData[arraySize]->duration = duration;
                            arraySize++;
                        } else {
                            fprintf(stderr, "ERROR: Failed to get frame: %s\n", SDL_GetError());
                            for (int ai = 0; ai < arraySize; ++ai) {
                                SDL_free(decodedFrameData[ai]);
                            }
                            SDL_free(decodedFrameData);
                            IMG_CloseAnimationEncoder(encoder);
                            IMG_CloseAnimationDecoder(decoder);
                            SDL_Quit();
                            return 1;
                        }
                        i++;
                    }

                    if (!IMG_CloseAnimationEncoder(encoder)) {
                        fprintf(stderr, "ERROR: Failed to close animation encoder: %s\n", SDL_GetError());
                        for (int ai = 0; ai < arraySize; ++ai) {
                            SDL_free(decodedFrameData[ai]);
                        }
                        SDL_free(decodedFrameData);
                        IMG_CloseAnimationDecoder(decoder);
                        SDL_CloseIO(encoderIO);
                        SDL_Quit();
                        return 1;
                    } else {
                        printf("Encoder closed successfully after adding %llu frames.\n", ii);
                    }

                    if (ii != i) {
                        fprintf(stderr, "ERROR: Not all frames were added teo the encoder. Added %llu, Total: %llu\n", ii, i);
                        for (int ai = 0; ai < arraySize; ++ai) {
                            SDL_free(decodedFrameData[ai]);
                        }
                        SDL_free(decodedFrameData);
                        IMG_CloseAnimationDecoder(decoder);
                        SDL_CloseIO(encoderIO);
                        SDL_Quit();
                        return 1;
                    } else {
                        printf("All frames added to the encoder successfully.\n");
                        printf("Total frames processed: %llu\n", i);
                        printf("Loading memory stream for %s back to IMG_AnimationDecoder to see if it results the same as %s\n", outputImageFormat, inputImage);

                        if (SDL_SeekIO(encoderIO, 0, SDL_IO_SEEK_SET) != 0) {
                            fprintf(stderr, "ERROR: Failed to seek to the beginning of the IO stream: %s\n", SDL_GetError());
                            for (int ai = 0; ai < arraySize; ++ai) {
                                SDL_free(decodedFrameData[ai]);
                            }
                            SDL_free(decodedFrameData);
                            IMG_CloseAnimationDecoder(decoder);
                            SDL_CloseIO(encoderIO);
                            SDL_Quit();
                            return 1;
                        }

                        IMG_AnimationDecoder *decoder2 = IMG_CreateAnimationDecoder_IO(encoderIO, false, outputImageFormat);
                        if (decoder2) {

                            Uint64 duration2;
                            SDL_Surface *frame2;
                            Uint64 j = 0;
                            while (IMG_GetAnimationDecoderFrame(decoder2, &frame2, &duration2)) {
                                printf("Reloaded Frame Duration (%llu): %llu ms\n", j, duration2);
                                printf("Reloaded Frame Format (%llu): %s\n", j, SDL_GetPixelFormatName(frame2->format));
                                printf("Reloaded Frame Size (%llu): %ix%i\n", j, frame2->w, frame2->h);

                                if (arraySize < j) {
                                    fprintf(stderr, "ERROR: Reloaded frame index %llu exceeds decoded frame data size %d\n", j, arraySize);
                                    for (int ai = 0; ai < arraySize; ++ai) {
                                        SDL_free(decodedFrameData[ai]);
                                    }
                                    SDL_free(decodedFrameData);
                                    IMG_CloseAnimationDecoder(decoder2);
                                    IMG_CloseAnimationDecoder(decoder);
                                    SDL_CloseIO(encoderIO);
                                    SDL_Quit();
                                    return 1;
                                }

                                if (decodedFrameData[j]->width != frame2->w || decodedFrameData[j]->height != frame2->h || decodedFrameData[j]->duration != duration2) {
                                    fprintf(stderr, "ERROR: Frame data mismatch at index %llu. Expected (%i, %i, %llu), Got (%i, %i, %llu)\n",
                                            j,
                                            decodedFrameData[j]->width, decodedFrameData[j]->height, decodedFrameData[j]->duration,
                                            frame2->w, frame2->h, duration2);
                                    for (int ai = 0; ai < arraySize; ++ai) {
                                        SDL_free(decodedFrameData[ai]);
                                    }
                                    SDL_free(decodedFrameData);
                                    IMG_CloseAnimationDecoder(decoder2);
                                    IMG_CloseAnimationDecoder(decoder);
                                    SDL_CloseIO(encoderIO);
                                    SDL_Quit();
                                    return 1;
                                } else {
                                    printf("Reloaded frame matches original frame data at index %llu.\n", j);
                                }

                                j++;
                            }

                            if (j != i) {
                                fprintf(stderr, "ERROR: Frame count mismatch after reloading output. Expected %llu, Got %llu\n", i, j);
                                for (int ai = 0; ai < arraySize; ++ai) {
                                    SDL_free(decodedFrameData[ai]);
                                }
                                SDL_free(decodedFrameData);
                                IMG_CloseAnimationDecoder(decoder2);
                                IMG_CloseAnimationDecoder(decoder);
                                SDL_CloseIO(encoderIO);
                                SDL_Quit();
                                return 1;
                            } else {
                                printf("All frames reloaded successfully from memory stream for %s with frame count %llu.\n", outputImageFormat, j);
                            }

                            IMG_CloseAnimationDecoder(decoder2);
                        } else {
                            fprintf(stderr, "ERROR: Failed to create animation decoder for output image: %s\n", SDL_GetError());
                            for (int ai = 0; ai < arraySize; ++ai) {
                                SDL_free(decodedFrameData[ai]);
                            }
                            SDL_free(decodedFrameData);
                            IMG_CloseAnimationDecoder(decoder);
                            SDL_CloseIO(encoderIO);
                            SDL_Quit();
                            return 1;
                        }
                    }

                    SDL_CloseIO(encoderIO);
                } else {
                    fprintf(stderr, "ERROR: Failed to create animation encoder for output format %s: %s\n", outputImageFormat, SDL_GetError());
                    for (int ai = 0; ai < arraySize; ++ai) {
                        SDL_free(decodedFrameData[ai]);
                    }
                    SDL_free(decodedFrameData);
                    IMG_CloseAnimationDecoder(decoder);
                    SDL_Quit();
                    return 1;
                }

                for (int ai = 0; ai < arraySize; ++ai) {
                    SDL_free(decodedFrameData[ai]);
                }
                SDL_free(decodedFrameData);

                IMG_CloseAnimationDecoder(decoder);
            } else {
                fprintf(stderr, "ERROR: Failed to create animation decoder for input image: %s\n", SDL_GetError());
                SDL_Quit();
                return 1;
            }
        }
    }

    printf("Finished test 'Starting Animation Decoder/Encoder Tests'.\n");
    printf("=========================================================\n");
    printf("=========================================================\n");
    printf("Starting test 'Decoder Rewind Test'\n");

    for (int cim = 0; cim < numOutputImageFormats; ++cim) {

        const char *outputImageFormat = outputImageFormats[cim];

        SDL_IOStream *rewindEncoderIO = SDL_IOFromDynamicMem();
        if (!rewindEncoderIO) {
            fprintf(stderr, "ERROR: Failed to create IO stream for frame encoder: %s\n", SDL_GetError());
            SDL_Quit();
            return 1;
        }

        IMG_AnimationEncoder *encoder = IMG_CreateAnimationEncoder_IO(rewindEncoderIO, false, outputImageFormat);
        if (!encoder) {
            fprintf(stderr, "ERROR: Failed to create animation encoder for output format %s: %s\n", outputImageFormat, SDL_GetError());
            SDL_CloseIO(rewindEncoderIO);
            SDL_Quit();
            return 1;
        }

        // We have to create different colors for each frame to ensure they are distinguishable. Otherwise, some formats like WEBP truncates identical repeating frames.
        for (int fi = 0; fi < MIN_FRAMES; ++fi) {
            SDL_Surface *frame = SDL_CreateSurface(256, 256, SDL_PIXELFORMAT_RGBA32);
            if (!frame) {
                fprintf(stderr, "ERROR: Failed to create surface for frame: %s\n", SDL_GetError());
                IMG_CloseAnimationEncoder(encoder);
                SDL_CloseIO(rewindEncoderIO);
                SDL_Quit();
                return 1;
            }

            const SDL_PixelFormatDetails *pixelFormatDetails = SDL_GetPixelFormatDetails(frame->format);
            if (!SDL_FillSurfaceRect(frame, NULL, SDL_MapRGBA(pixelFormatDetails, NULL, (Uint8)SDL_rand(256), (Uint8)SDL_rand(256), (Uint8)SDL_rand(256), 255))) {
                fprintf(stderr, "ERROR: Failed to fill surface with red color: %s\n", SDL_GetError());
            }

            if (IMG_AddAnimationEncoderFrame(encoder, frame, 1000)) {
                printf("Frame %i added to encoder successfully.\n", fi);
                SDL_DestroySurface(frame);
            } else {
                fprintf(stderr, "ERROR: Failed to add frame to encoder: %s\n", SDL_GetError());
                SDL_DestroySurface(frame);
                IMG_CloseAnimationEncoder(encoder);
                SDL_CloseIO(rewindEncoderIO);
                SDL_Quit();
                return 1;
            }
        }

        if (!IMG_CloseAnimationEncoder(encoder)) {
            fprintf(stderr, "ERROR: Failed to close animation encoder for frame: %s\n", SDL_GetError());
            SDL_CloseIO(rewindEncoderIO);
            SDL_Quit();
            return 1;
        }
        printf("Frames encoded successfully for output format %s.\n", outputImageFormat);
        if (SDL_SeekIO(rewindEncoderIO, 0, SDL_IO_SEEK_SET) != 0) {
            fprintf(stderr, "ERROR: Failed to seek to the beginning of the IO stream for frame encoder: %s\n", SDL_GetError());
            SDL_CloseIO(rewindEncoderIO);
            SDL_Quit();
            return 1;
        }

        IMG_AnimationDecoder *rewindDecoder = IMG_CreateAnimationDecoder_IO(rewindEncoderIO, false, outputImageFormat);
        if (rewindDecoder) {
            printf("Rewind decoder created successfully for output format %s.\n", outputImageFormat);
            printf("Decoding %i frames first.\n", MIN_FRAMES);
            for (int ri = 0; ri < MIN_FRAMES; ++ri) {
                if (!IMG_GetAnimationDecoderFrame(rewindDecoder, NULL, NULL)) {
                    fprintf(stderr, "ERROR: Failed to get frame %i from rewind decoder. status: %s --- SDL Error: %s\n", ri, GetAnimationDecoderStatusString(IMG_GetAnimationDecoderStatus(rewindDecoder)), SDL_GetError());
                    IMG_CloseAnimationDecoder(rewindDecoder);
                    SDL_CloseIO(rewindEncoderIO);
                    SDL_Quit();
                    return 1;
                }
            }

            printf("Now resetting the animation decoder...\n");

            if (!IMG_ResetAnimationDecoder(rewindDecoder)) {
                fprintf(stderr, "ERROR: Failed to reset rewind decoder: %s\n", SDL_GetError());
                IMG_CloseAnimationDecoder(rewindDecoder);
                SDL_CloseIO(rewindEncoderIO);
                SDL_Quit();
                return 1;
            }

            printf("Decoding %i frames again after reset.\n", MIN_FRAMES);

            for (int ri = 0; ri < MIN_FRAMES; ++ri) {
                if (!IMG_GetAnimationDecoderFrame(rewindDecoder, NULL, NULL)) {
                    fprintf(stderr, "ERROR: Failed to get frame %i from rewind decoder. status: %s --- SDL Error: %s\n", ri, GetAnimationDecoderStatusString(IMG_GetAnimationDecoderStatus(rewindDecoder)), SDL_GetError());
                    IMG_CloseAnimationDecoder(rewindDecoder);
                    SDL_CloseIO(rewindEncoderIO);
                    SDL_Quit();
                    return 1;
                }
            }
        } else {
            fprintf(stderr, "ERROR: Failed to create rewind decoder for input image: %s\n", SDL_GetError());
            SDL_CloseIO(rewindEncoderIO);
            SDL_Quit();
            return 1;
        }

        SDL_CloseIO(rewindEncoderIO);
        printf("Finished rewind test for output format %s.\n", outputImageFormat);
    }

    printf("Finished test 'Decoder Rewind Test'.\n");
    printf("=========================================================\n");
    printf("=========================================================\n");
    printf("Starting test 'Encode Metadata and Decode Metadata Test'\n");

    for (int k = 0; k < numFormatInfo; ++k) {
        const char *format = formatInfo[k].format;
        printf("Testing format: %s\n", format);
        SDL_IOStream *metadataEncoderIO = SDL_IOFromDynamicMem();
        if (!metadataEncoderIO) {
            fprintf(stderr, "ERROR: Failed to create IO stream for metadata encoder: %s\n", SDL_GetError());
            SDL_Quit();
            return 1;
        }

        SDL_PropertiesID metadataProps = SDL_CreateProperties();
        if (!metadataProps) {
            fprintf(stderr, "ERROR: Failed to create properties for metadata encoder: %s\n", SDL_GetError());
            SDL_CloseIO(metadataEncoderIO);
            SDL_Quit();
            return 1;
        }

        for (int mi = 0; mi < MAX_METADATA; ++mi) {
            const MetadataInfo *metadata = &formatInfo[k].availableMetadata[mi];
            if (!metadata || !metadata->metadata || !metadata->value) {
                continue;
            }

            switch (metadata->type) {
            case SDL_PROPERTY_TYPE_BOOLEAN:
                SDL_SetBooleanProperty(metadataProps, metadata->metadata, (bool)metadata->value);
                break;
            case SDL_PROPERTY_TYPE_NUMBER:
                SDL_SetNumberProperty(metadataProps, metadata->metadata, (Sint64)metadata->value);
                break;
            case SDL_PROPERTY_TYPE_STRING:
                SDL_SetStringProperty(metadataProps, metadata->metadata, (const char *)metadata->value);
                break;
            default:
                fprintf(stderr, "ERROR: Unsupported metadata type for %s: %d\n", metadata->metadata, metadata->type);
                SDL_DestroyProperties(metadataProps);
                SDL_CloseIO(metadataEncoderIO);
                SDL_Quit();
                return 1;
            }
        }

        SDL_SetStringProperty(metadataProps, IMG_PROP_ANIMATION_ENCODER_CREATE_TYPE_STRING, format);
        SDL_SetPointerProperty(metadataProps, IMG_PROP_ANIMATION_ENCODER_CREATE_IOSTREAM_POINTER, metadataEncoderIO);
        SDL_SetBooleanProperty(metadataProps, IMG_PROP_ANIMATION_ENCODER_CREATE_IOSTREAM_AUTOCLOSE_BOOLEAN, false);

        IMG_AnimationEncoder *metadataEncoder = IMG_CreateAnimationEncoderWithProperties(metadataProps);
        if (!metadataEncoder) {
            fprintf(stderr, "ERROR: Failed to create animation encoder for format %s: %s\n", format, SDL_GetError());
            SDL_DestroyProperties(metadataProps);
            SDL_CloseIO(metadataEncoderIO);
            SDL_Quit();
            return 1;
        }

        SDL_DestroyProperties(metadataProps);

        for (int fi = 0; fi < MIN_FRAMES; ++fi) {
            SDL_Surface *metadataFrame = SDL_CreateSurface(256, 256, SDL_PIXELFORMAT_RGBA32);
            if (!metadataFrame) {
                fprintf(stderr, "ERROR: Failed to create surface for metadata frame: %s\n", SDL_GetError());
                IMG_CloseAnimationEncoder(metadataEncoder);
                SDL_CloseIO(metadataEncoderIO);
                SDL_Quit();
                return 1;
            }
            const SDL_PixelFormatDetails *metadataPixelFormatDetails = SDL_GetPixelFormatDetails(metadataFrame->format);
            if (!SDL_FillSurfaceRect(metadataFrame, NULL, SDL_MapRGBA(metadataPixelFormatDetails, NULL, (Uint8)SDL_rand(256), (Uint8)SDL_rand(256), (Uint8)SDL_rand(256), 255))) {
                fprintf(stderr, "ERROR: Failed to fill surface with green color for metadata frame: %s\n", SDL_GetError());
                SDL_DestroySurface(metadataFrame);
                IMG_CloseAnimationEncoder(metadataEncoder);
                SDL_CloseIO(metadataEncoderIO);
                SDL_Quit();
                return 1;
            }
            if (IMG_AddAnimationEncoderFrame(metadataEncoder, metadataFrame, 1000)) {
                printf("Metadata frame added to encoder successfully.\n");
                SDL_DestroySurface(metadataFrame);
            } else {
                fprintf(stderr, "ERROR: Failed to add metadata frame to encoder: %s\n", SDL_GetError());
                SDL_DestroySurface(metadataFrame);
                IMG_CloseAnimationEncoder(metadataEncoder);
                SDL_CloseIO(metadataEncoderIO);
                SDL_Quit();
                return 1;
            }
        }

        if (!IMG_CloseAnimationEncoder(metadataEncoder)) {
            fprintf(stderr, "ERROR: Failed to close animation encoder for format %s: %s\n", format, SDL_GetError());
            SDL_CloseIO(metadataEncoderIO);
            SDL_Quit();
            return 1;
        }

        printf("Metadata frame encoded successfully for format %s.\n", format);
        if (SDL_SeekIO(metadataEncoderIO, 0, SDL_IO_SEEK_SET) != 0) {
            fprintf(stderr, "ERROR: Failed to seek to the beginning of the IO stream for metadata encoder: %s\n", SDL_GetError());
            SDL_CloseIO(metadataEncoderIO);
            SDL_Quit();
            return 1;
        }
        IMG_AnimationDecoder *metadataDecoder = IMG_CreateAnimationDecoder_IO(metadataEncoderIO, false, format);
        if (metadataDecoder) {
            SDL_PropertiesID decodedProps = IMG_GetAnimationDecoderProperties(metadataDecoder);
            if (!decodedProps) {
                fprintf(stderr, "ERROR: Failed to get properties from metadata decoder: %s\n", SDL_GetError());
                IMG_CloseAnimationDecoder(metadataDecoder);
                SDL_CloseIO(metadataEncoderIO);
                SDL_Quit();
                return 1;
            }
            for (int i = 0; i < MAX_METADATA; ++i) {
                const MetadataInfo *metadata = &formatInfo[k].availableMetadata[i];
                if (!metadata || !metadata->metadata || !metadata->value) {
                    continue;
                }

                const char *propName = metadata->metadata;
                if (metadata->type == SDL_PROPERTY_TYPE_BOOLEAN) {
                    bool propValue = SDL_GetBooleanProperty(decodedProps, propName, false);
                    printf("Decoded Property %s: %s\n", propName, propValue ? "true" : "false");
                    bool expectedValue = (bool)metadata->value;
                    if (propValue != expectedValue) {
                        fprintf(stderr, "ERROR: Decoded boolean property %s does not match expected value. Expected: %s, Got: %s\n",
                                propName, expectedValue ? "true" : "false", propValue ? "true" : "false");
                        IMG_CloseAnimationDecoder(metadataDecoder);
                        SDL_CloseIO(metadataEncoderIO);
                        SDL_Quit();
                        return 1;
                    }
                } else if (metadata->type == SDL_PROPERTY_TYPE_NUMBER) {
                    Sint64 propValue = SDL_GetNumberProperty(decodedProps, propName, 0);
                    printf("Decoded Property %s: %llu\n", propName, propValue);
                    Sint64 expectedValue = (Sint64)metadata->value;
                    if (propValue != expectedValue) {
                        fprintf(stderr, "ERROR: Decoded number property %s does not match expected value. Expected: %llu, Got: %llu\n",
                                propName, expectedValue, propValue);
                        IMG_CloseAnimationDecoder(metadataDecoder);
                        SDL_CloseIO(metadataEncoderIO);
                        SDL_Quit();
                        return 1;
                    }
                } else if (metadata->type == SDL_PROPERTY_TYPE_STRING) {
                    const char *propValue = SDL_GetStringProperty(decodedProps, propName, NULL);
                    printf("Decoded Property %s: %s\n", propName, propValue ? propValue : "NULL");
                    const char *expectedValue = (const char *)metadata->value;
                    if (propValue == NULL && expectedValue == NULL) {
                        printf("Both decoded and expected string properties are NULL for %s.\n", propName);
                    } else if (propValue == NULL || expectedValue == NULL || SDL_strcmp(propValue, expectedValue) != 0) {
                        fprintf(stderr, "ERROR: Decoded string property %s does not match expected value. Expected: %s, Got: %s\n",
                                propName, expectedValue ? expectedValue : "NULL", propValue ? propValue : "NULL");
                        IMG_CloseAnimationDecoder(metadataDecoder);
                        SDL_CloseIO(metadataEncoderIO);
                        SDL_Quit();
                        return 1;
                    }
                } else {
                    fprintf(stderr, "ERROR: Unsupported metadata type for %s: %d\n", propName, metadata->type);
                    IMG_CloseAnimationDecoder(metadataDecoder);
                    SDL_CloseIO(metadataEncoderIO);
                    SDL_Quit();
                    return 1;
                }
            }
            IMG_CloseAnimationDecoder(metadataDecoder);
        } else {
            fprintf(stderr, "ERROR: Failed to create animation decoder for format %s: %s\n", format, SDL_GetError());
            SDL_CloseIO(metadataEncoderIO);
            SDL_Quit();
            return 1;
        }
        SDL_CloseIO(metadataEncoderIO);
        printf("Finished metadata encoding and decoding test for format %s.\n", format);
    }

    printf("All tests completed successfully.\n");

    SDL_Quit();
    return 0;
}
