/*
  Copyright 1997-2025 Sam Lantinga
  Copyright 2022 Collabora Ltd.

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

#include <SDL3_image/SDL_image.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_test.h>

#if defined(SDL_PLATFORM_OS2) || defined(SDL_PLATFORM_WIN32)
static const char pathsep[] = "\\";
#elif defined(SDL_PLATFORM_RISCOS)
static const char pathsep[] = ".";
#else
static const char pathsep[] = "/";
#endif

#if defined(__APPLE__) && !defined(SDL_IMAGE_USE_COMMON_BACKEND)
# define USING_IMAGEIO 1
#else
# define USING_IMAGEIO 0
#endif

typedef enum
{
    TEST_FILE_DIST,
    TEST_FILE_BUILT
} TestFileType;

static bool
GetStringBoolean(const char *value, bool default_value)
{
    if (!value || !*value) {
        return default_value;
    }
    if (*value == '0' || SDL_strcasecmp(value, "false") == 0) {
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
 * If type is TEST_FILE_BUILT, look for it in $SDL_TEST_BUILDDIR or next
 * to the executable.
 *
 * Fails and returns NULL if out of memory.
 */
static char *
GetTestFilename(TestFileType type, const char *file)
{
    const char *base;
    char *path = NULL;
    bool needPathSep = true;

    if (type == TEST_FILE_DIST) {
        base = SDL_getenv("SDL_TEST_SRCDIR");
    } else {
        base = SDL_getenv("SDL_TEST_BUILDDIR");
    }

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

static SDLTest_CommonState *state;

typedef struct
{
    const char *name;
    const char *sample;
    const char *reference;
    int w;
    int h;
    int tolerance;
    bool canLoad;
    bool canSave;
    bool (SDLCALL * checkFunction)(SDL_IOStream *src);
    SDL_Surface *(SDLCALL * loadFunction)(SDL_IOStream *src);
} Format;

static const Format formats[] =
{
    {
        "AVIF",
        "sample.avif",
        "sample.bmp",
        23,
        42,
        300,
#ifdef LOAD_AVIF
        true,
#else
        false,
#endif
        SDL_IMAGE_SAVE_AVIF,
        IMG_isAVIF,
        IMG_LoadAVIF_IO,
    },
    {
        "BMP",
        "sample.bmp",
        "sample.png",
        23,
        42,
        0,              /* lossless */
#ifdef LOAD_BMP
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isBMP,
        IMG_LoadBMP_IO,
    },
    {
        "CUR",
        "sample.cur",
        "sample.bmp",
        23,
        42,
        0,              /* lossless */
#ifdef LOAD_BMP
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isCUR,
        IMG_LoadCUR_IO,
    },
    {
        "GIF",
        "palette.gif",
        "palette.bmp",
        23,
        42,
        0,              /* lossless */
#if USING_IMAGEIO || defined(LOAD_GIF)
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isGIF,
        IMG_LoadGIF_IO,
    },
    {
        "ICO",
        "sample.ico",
        "sample.bmp",
        23,
        42,
        0,              /* lossless */
#ifdef LOAD_BMP
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isICO,
        IMG_LoadICO_IO,
    },
    {
        "JPG",
        "sample.jpg",
        "sample.bmp",
        23,
        42,
        100,
#if (USING_IMAGEIO && defined(JPG_USES_IMAGEIO)) || defined(SDL_IMAGE_USE_WIC_BACKEND) || defined(LOAD_JPG)
        true,
#else
        false,
#endif
        SDL_IMAGE_SAVE_JPG,
        IMG_isJPG,
        IMG_LoadJPG_IO,
    },
#if 0 /* Different versions of JXL yield different output images */
    {
        "JXL",
        "sample.jxl",
        "sample.bmp",
        23,
        42,
        300,
#ifdef LOAD_JXL
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isJXL,
        IMG_LoadJXL_IO,
    },
#endif
#if 0
    {
        "LBM",
        "sample.lbm",
        "sample.bmp",
        23,
        42,
        0,              /* lossless? */
#ifdef LOAD_LBM
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isLBM,
        IMG_LoadLBM_IO,
    },
#endif
    {
        "PCX",
        "sample.pcx",
        "sample.bmp",
        23,
        42,
        0,              /* lossless? */
#ifdef LOAD_PCX
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isPCX,
        IMG_LoadPCX_IO,
    },
    {
        "PNG",
        "sample.png",
        "sample.bmp",
        23,
        42,
        0,              /* lossless */
#if (USING_IMAGEIO && defined(PNG_USES_IMAGEIO)) || defined(SDL_IMAGE_USE_WIC_BACKEND) || defined(LOAD_PNG)
        true,
#else
        false,
#endif
        SDL_IMAGE_SAVE_PNG,
        IMG_isPNG,
        IMG_LoadPNG_IO,
    },
    {
        "PNM",
        "sample.pnm",
        "sample.bmp",
        23,
        42,
        0,              /* lossless */
#ifdef LOAD_PNM
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isPNM,
        IMG_LoadPNM_IO,
    },
    {
        "QOI",
        "sample.qoi",
        "sample.bmp",
        23,
        42,
        0,              /* lossless */
#ifdef LOAD_QOI
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isQOI,
        IMG_LoadQOI_IO,
    },
    {
        "SVG",
        "svg.svg",
        "svg.bmp",
        32,
        32,
        100,
#ifdef LOAD_SVG
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isSVG,
        IMG_LoadSVG_IO,
    },
    {
        "SVG-sized",
        "svg.svg",
        "svg64.bmp",
        64,
        64,
        100,
#ifdef LOAD_SVG
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isSVG,
        IMG_LoadSVG_IO,
    },
    {
        "SVG-class",
        "svg-class.svg",
        "svg-class.bmp",
        82,
        82,
        0,              /* lossless? */
#ifdef LOAD_SVG
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isSVG,
        IMG_LoadSVG_IO,
    },
    {
        "TGA",
        "sample.tga",
        "sample.bmp",
        23,
        42,
        0,              /* lossless? */
#if USING_IMAGEIO || defined(LOAD_TGA)
        true,
#else
        false,
#endif
        false,      /* can save */
        NULL,
        IMG_LoadTGA_IO,
    },
    {
        "TIF",
        "sample.tif",
        "sample.bmp",
        23,
        42,
        0,              /* lossless */
#if USING_IMAGEIO || defined(SDL_IMAGE_USE_WIC_BACKEND) || defined(LOAD_TIF)
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isTIF,
        IMG_LoadTIF_IO,
    },
    {
        "WEBP",
        "sample.webp",
        "sample.bmp",
        23,
        42,
        0,              /* lossless */
#ifdef LOAD_WEBP
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isWEBP,
        IMG_LoadWEBP_IO,
    },
    {
        "XCF",
        "sample.xcf",
        "sample.bmp",
        23,
        42,
        0,              /* lossless */
#ifdef LOAD_XCF
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isXCF,
        IMG_LoadXCF_IO,
    },
    {
        "XPM",
        "sample.xpm",
        "sample.bmp",
        23,
        42,
        0,              /* lossless */
#ifdef LOAD_XPM
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isXPM,
        IMG_LoadXPM_IO,
    },
#if 0
    {
        "XV",
        "sample.xv",
        "sample.bmp",
        23,
        42,
        0,              /* lossless? */
#ifdef LOAD_XV
        true,
#else
        false,
#endif
        false,      /* can save */
        IMG_isXV,
        IMG_LoadXV_IO,
    },
#endif
};

static bool
StrHasSuffix(const char *str, const char *suffix)
{
    size_t str_len = SDL_strlen(str);
    size_t suffix_len = SDL_strlen(suffix);

    return (str_len >= suffix_len
            && SDL_strcmp(str + (str_len - suffix_len), suffix) == 0);
}

typedef enum
{
    LOAD_CONVENIENCE = 0,
    LOAD_IO,
    LOAD_TYPED_IO,
    LOAD_FORMAT_SPECIFIC,
    LOAD_SIZED
} LoadMode;

/* Convert to RGBA for comparison, if necessary */
static bool
ConvertToRgba32(SDL_Surface **surface_p)
{
    if ((*surface_p)->format != SDL_PIXELFORMAT_RGBA32) {
        SDL_Surface *temp;

        SDL_ClearError();
        temp = SDL_ConvertSurface(*surface_p, SDL_PIXELFORMAT_RGBA32);
        SDLTest_AssertCheck(temp != NULL,
                            "Converting to RGBA should succeed (%s)",
                            SDL_GetError());
        if (temp == NULL) {
            return false;
        }
        SDL_DestroySurface(*surface_p);
        *surface_p = temp;
    }
    return true;
}

static void
DumpPixels(const char *filename, SDL_Surface *surface)
{
    const SDL_Palette *palette = SDL_GetSurfacePalette(surface);
    const unsigned char *pixels = surface->pixels;
    const unsigned char *p;
    size_t w, h, pitch;
    size_t i, j;

    SDL_Log("%s:\n", filename);

    if (palette) {
        size_t n = 0;

        if (palette->ncolors >= 0) {
            n = (size_t) palette->ncolors;
        }

        SDL_Log("  Palette:\n");
        for (i = 0; i < n; i++) {
            SDL_Log("    RGBA[0x%02x] = %02x%02x%02x%02x\n",
                    (unsigned) i,
                    palette->colors[i].r,
                    palette->colors[i].g,
                    palette->colors[i].b,
                    palette->colors[i].a);
        }
    }

    if (surface->w < 0) {
        SDL_Log("    Invalid width %d\n", surface->w);
        return;
    }

    if (surface->h < 0) {
        SDL_Log("    Invalid height %d\n", surface->h);
        return;
    }

    if (surface->pitch < 0) {
        SDL_Log("    Invalid pitch %d\n", surface->pitch);
        return;
    }

    w = (size_t) surface->w;
    h = (size_t) surface->h;
    pitch = (size_t) surface->pitch;

    SDL_Log("  Pixels:\n");

    for (j = 0; j < h; j++) {
        SDL_Log("    ");

        for (i = 0; i < w; i++) {
            p = pixels + (j * pitch) + (i * SDL_BYTESPERPIXEL(surface->format));

            switch (SDL_BITSPERPIXEL(surface->format)) {
                case 1:
                case 4:
                case 8:
                    SDL_Log("%02x ", *p);
                    break;

                case 12:
                case 15:
                case 16:
                    SDL_Log("%02x", *p++);
                    SDL_Log("%02x ", *p);
                    break;

                case 24:
                    SDL_Log("%02x", *p++);
                    SDL_Log("%02x", *p++);
                    SDL_Log("%02x ", *p);
                    break;

                case 32:
                    SDL_Log("%02x", *p++);
                    SDL_Log("%02x", *p++);
                    SDL_Log("%02x", *p++);
                    SDL_Log("%02x ", *p);
                    break;
            }
        }

        SDL_Log("\n");
    }
}

static void
FormatLoadTest(const Format *format,
               LoadMode mode)
{
    SDL_Surface *reference = NULL;
    SDL_Surface *surface = NULL;
    SDL_IOStream *src = NULL;
    char *filename = NULL;
    char *refFilename = NULL;
    int diff;

    SDL_ClearError();
    filename = GetTestFilename(TEST_FILE_DIST, format->sample);
    if (!SDLTest_AssertCheck(filename != NULL,
                             "Building filename should succeed (%s)",
                             SDL_GetError())) {
        goto out;
    }

    SDL_ClearError();
    refFilename = GetTestFilename(TEST_FILE_DIST, format->reference);
    if (!SDLTest_AssertCheck(refFilename != NULL,
                             "Building ref filename should succeed (%s)",
                             SDL_GetError())) {
        goto out;
    }

    if (StrHasSuffix(format->reference, ".bmp")) {
        SDL_ClearError();
        SDLTest_AssertPass("About to call SDL_LoadBMP(\"%s\")", refFilename);
        reference = SDL_LoadBMP(refFilename);
        if (!SDLTest_AssertCheck(reference != NULL,
                                 "Loading reference should succeed (%s)",
                                 SDL_GetError())) {
            goto out;
        }
    }
    else if (StrHasSuffix (format->reference, ".png")) {
#ifdef LOAD_PNG
        SDL_ClearError();
        SDLTest_AssertPass("About to call IMG_Load(\"%s\")", refFilename);
        reference = IMG_Load(refFilename);
        if (!SDLTest_AssertCheck(reference != NULL,
                                 "Loading reference should succeed (%s)",
                                 SDL_GetError())) {
            goto out;
        }
#endif
    }

    if (mode != LOAD_CONVENIENCE) {
        SDL_ClearError();
        SDLTest_AssertPass("About to call SDL_IOFromFile(\"%s\", \"rb\")", refFilename);
        src = SDL_IOFromFile(filename, "rb");
        SDLTest_AssertCheck(src != NULL,
                            "Opening %s should succeed (%s)",
                            filename, SDL_GetError());
        if (src == NULL)
            goto out;
    }

    SDL_ClearError();
    switch (mode) {
        case LOAD_CONVENIENCE:
            SDLTest_AssertPass("About to call IMG_Load(\"%s\")", filename);
            surface = IMG_Load(filename);
            break;

        case LOAD_IO:
            if (format->checkFunction != NULL) {
                SDL_IOStream *ref_src;
                int check;

                SDLTest_AssertPass("About to call SDL_IOFromFile(\"%s\", \"rb\")", refFilename);
                ref_src = SDL_IOFromFile(refFilename, "rb");
                SDLTest_AssertCheck(ref_src != NULL,
                                    "Opening %s should succeed (%s)",
                                    refFilename, SDL_GetError());
                if (ref_src != NULL) {
                    SDLTest_AssertPass("About to call IMG_Is%s(<reference>)", format->name);
                    check = format->checkFunction(ref_src);
                    SDLTest_AssertCheck(!check,
                                        "Should not detect %s as %s -> %d",
                                        refFilename, format->name, check);
                    SDL_CloseIO(ref_src);
                }
            }

            if (format->checkFunction != NULL) {
                SDLTest_AssertPass("About to call IMG_Is%s(<src>)", format->name);
                int check = format->checkFunction(src);

                SDLTest_AssertCheck(check,
                                    "Should detect %s as %s -> %d",
                                    filename, format->name, check);
            }

            SDL_ClearError();
            SDLTest_AssertPass("About to call IMG_Load_IO(<src>, true)");
            surface = IMG_Load_IO(src, true);
            src = NULL;      /* ownership taken */
            break;

        case LOAD_TYPED_IO:
            SDLTest_AssertPass("About to call IMG_LoadTyped_IO(<src>, true, \"%s\")", format->name);
            surface = IMG_LoadTyped_IO(src, true, format->name);
            src = NULL;      /* ownership taken */
            break;

        case LOAD_FORMAT_SPECIFIC:
            SDLTest_AssertPass("About to call IMG_Load%s_IO(<src>)", format->name);
            surface = format->loadFunction(src);
            break;

        case LOAD_SIZED:
            if (SDL_strcmp(format->name, "SVG-sized") == 0) {
                surface = IMG_LoadSizedSVG_IO(src, 64, 64);
            }
            break;
    }

    if (!SDLTest_AssertCheck(surface != NULL,
                             "Load %s (%s)", filename, SDL_GetError())) {
        goto out;
    }

    SDLTest_AssertCheck(surface->w == format->w,
                        "Expected width %d px, got %d",
                        format->w, surface->w);
    SDLTest_AssertCheck(surface->h == format->h,
                        "Expected height %d px, got %d",
                        format->h, surface->h);

    if (GetStringBoolean(SDL_getenv("SDL_IMAGE_TEST_DEBUG"), false)) {
        DumpPixels(filename, surface);
    }

    if (reference != NULL) {
        ConvertToRgba32(&reference);
        ConvertToRgba32(&surface);
        diff = SDLTest_CompareSurfaces(surface, reference, format->tolerance);
        SDLTest_AssertCheck(diff == 0,
                            "Surface differed from reference by at most %d in %d pixels",
                            format->tolerance, diff);
        if (diff != 0 || GetStringBoolean(SDL_getenv("SDL_IMAGE_TEST_DEBUG"), false)) {
            DumpPixels(filename, surface);
            DumpPixels(refFilename, reference);
        }
    }

out:
    if (surface != NULL) {
        SDL_DestroySurface(surface);
    }
    if (reference != NULL) {
        SDL_DestroySurface(reference);
    }
    if (src != NULL) {
        SDL_CloseIO(src);
    }
    if (refFilename != NULL) {
        SDL_free(refFilename);
    }
    if (filename != NULL) {
        SDL_free(filename);
    }
}

static void
FormatSaveTest(const Format *format,
               bool rw)
{
    char *refFilename = NULL;
    char filename[64] = { 0 };
    SDL_Surface *reference = NULL;
    SDL_Surface *surface = NULL;
    SDL_IOStream *dest = NULL;
    int diff;
    bool result;

    SDL_snprintf(filename, sizeof(filename),
                 "save%s.%s",
                 rw ? "Rwops" : "",
                 format->name);

    SDL_ClearError();
    refFilename = GetTestFilename(TEST_FILE_DIST, "sample.bmp");
    if (!SDLTest_AssertCheck(refFilename != NULL,
                             "Building ref filename should succeed (%s)",
                             SDL_GetError())) {
        goto out;
    }

    SDL_ClearError();
    reference = SDL_LoadBMP(refFilename);
    if (!SDLTest_AssertCheck(reference != NULL,
                             "Loading reference should succeed (%s)",
                             SDL_GetError())) {
        goto out;
    }

    SDL_ClearError();
    if (SDL_strcmp (format->name, "AVIF") == 0) {
        if (rw) {
            dest = SDL_IOFromFile(filename, "wb");
            result = IMG_SaveAVIF_IO(reference, dest, false, 90);
            SDL_CloseIO(dest);
        } else {
            result = IMG_SaveAVIF(reference, filename, 90);
        }
    } else if (SDL_strcmp(format->name, "JPG") == 0) {
        if (rw) {
            dest = SDL_IOFromFile(filename, "wb");
            result = IMG_SaveJPG_IO(reference, dest, false, 90);
            SDL_CloseIO(dest);
        } else {
            result = IMG_SaveJPG(reference, filename, 90);
        }
    } else if (SDL_strcmp (format->name, "PNG") == 0) {
        if (rw) {
            dest = SDL_IOFromFile(filename, "wb");
            result = IMG_SavePNG_IO(reference, dest, false);
            SDL_CloseIO(dest);
        } else {
            result = IMG_SavePNG(reference, filename);
        }
    } else {
        SDLTest_AssertCheck(false, "How do I save %s?", format->name);
        goto out;
    }

    SDLTest_AssertCheck(result, "Save %s (%s)", filename, SDL_GetError());

    if (format->canLoad) {
        SDL_ClearError();
        SDLTest_AssertPass("About to call IMG_Load(\"%s\")", filename);
        surface = IMG_Load(filename);

        if (!SDLTest_AssertCheck(surface != NULL,
                                 "Load %s (%s)", "saved file", SDL_GetError())) {
            goto out;
        }

        ConvertToRgba32(&reference);
        ConvertToRgba32(&surface);

        SDLTest_AssertCheck(surface->w == format->w,
                            "Expected width %d px, got %d",
                            format->w, surface->w);
        SDLTest_AssertCheck(surface->h == format->h,
                            "Expected height %d px, got %d",
                            format->h, surface->h);

        diff = SDLTest_CompareSurfaces(surface, reference, format->tolerance);
        SDLTest_AssertCheck(diff == 0,
                            "Surface differed from reference by at most %d in %d pixels",
                            format->tolerance, diff);
        if (diff != 0 || GetStringBoolean(SDL_getenv("SDL_IMAGE_TEST_DEBUG"), false)) {
            DumpPixels(filename, surface);
            DumpPixels(refFilename, reference);
        }
    }

out:
    if (surface != NULL) {
        SDL_DestroySurface(surface);
    }
    if (reference != NULL) {
        SDL_DestroySurface(reference);
    }
    if (refFilename != NULL) {
        SDL_free(refFilename);
    }
}

static void
FormatTest(const Format *format)
{
    bool forced;
    char envVar[64] = { 0 };

    SDL_snprintf(envVar, sizeof(envVar), "SDL_IMAGE_TEST_REQUIRE_LOAD_%s",
                 format->name);

    forced = GetStringBoolean(SDL_getenv(envVar), false);
    if (forced) {
        SDLTest_AssertCheck(format->canLoad,
                            "%s loading should be enabled", format->name);
    }

    if (format->canLoad || forced) {
        SDLTest_Log("Testing ability to load format %s", format->name);

        if (SDL_strcmp(format->name, "SVG-sized") == 0) {
            FormatLoadTest(format, LOAD_SIZED);
        } else {
            FormatLoadTest(format, LOAD_CONVENIENCE);

            if (SDL_strcmp(format->name, "TGA") == 0) {
                SDLTest_Log("SKIP: Recognising %s by magic number is not supported", format->name);
            } else {
                FormatLoadTest(format, LOAD_IO);
            }

            FormatLoadTest(format, LOAD_TYPED_IO);

            if (format->loadFunction != NULL) {
                FormatLoadTest(format, LOAD_FORMAT_SPECIFIC);
            }
        }
    } else {
        SDLTest_Log("Format %s is not supported", format->name);
    }

    SDL_snprintf(envVar, sizeof(envVar), "SDL_IMAGE_TEST_REQUIRE_SAVE_%s",
                 format->name);

    forced = GetStringBoolean(SDL_getenv(envVar), false);
    if (forced) {
        SDLTest_AssertCheck(format->canSave,
                            "%s saving should be enabled", format->name);
    }

    if (format->canSave || forced) {
        SDLTest_Log("Testing ability to save format %s", format->name);
        FormatSaveTest(format, false);
        FormatSaveTest(format, true);
    } else {
        SDLTest_Log("Saving format %s is not supported", format->name);
    }
}

static int SDLCALL
TestFormats(void *arg)
{
    size_t i;
    (void)arg;

    for (i = 0; i < SDL_arraysize(formats); i++) {
        FormatTest(&formats[i]);
    }

    return TEST_COMPLETED;
}

static const SDLTest_TestCaseReference formatsTestCase = {
    TestFormats, "Images", "Load and save various image formats", TEST_ENABLED
};

static const SDLTest_TestCaseReference *testCases[] =  {
    &formatsTestCase,
    NULL
};
static SDLTest_TestSuiteReference testSuite = {
    "img",
    NULL,
    testCases,
    NULL
};
static SDLTest_TestSuiteReference *testSuites[] =  {
    &testSuite,
    NULL
};

int
main(int argc, char *argv[])
{
    int result;
    int i, done;
    SDL_Event event;
    SDLTest_TestSuiteRunner *runner;

    /* Initialize test framework */
    state = SDLTest_CommonCreateState(argv, SDL_INIT_VIDEO);
    if (!state) {
        return 1;
    }

    runner = SDLTest_CreateTestSuiteRunner(state, testSuites);

    if (!SDLTest_CommonDefaultArgs(state, argc, argv)) {
        return 1;
    }

    /* Initialize common state */
    if (!SDLTest_CommonInit(state)) {
        goto failure;
    }

    /* Create the windows, initialize the renderers */
    for (i = 0; i < state->num_windows; ++i) {
        SDL_Renderer *renderer = state->renderers[i];
        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(renderer);
    }

    /* Call Harness */
    result = SDLTest_ExecuteTestSuiteRunner(runner);

    /* Empty event queue */
    done = 0;
    for (i=0; i<100; i++)  {
      while (SDL_PollEvent(&event)) {
        SDLTest_CommonEvent(state, &event, &done);
      }
      SDL_Delay(10);
    }

    /* Shutdown everything */
    SDLTest_CommonQuit(state);
    return result;
failure:
    SDLTest_CommonQuit(state);
    return 1;
}
