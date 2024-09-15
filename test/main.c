/*
  Copyright 1997-2024 Sam Lantinga
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

static SDL_bool
GetStringBoolean(const char *value, SDL_bool default_value)
{
    if (!value || !*value) {
        return default_value;
    }
    if (*value == '0' || SDL_strcasecmp(value, "false") == 0) {
        return SDL_FALSE;
    }
    return SDL_TRUE;
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
    SDL_bool needPathSep = SDL_TRUE;

    if (type == TEST_FILE_DIST) {
        base = SDL_getenv("SDL_TEST_SRCDIR");
    } else {
        base = SDL_getenv("SDL_TEST_BUILDDIR");
    }

    if (base == NULL) {
        base = SDL_GetBasePath();
        /* SDL_GetBasePath() guarantees a trailing path separator */
        needPathSep = SDL_FALSE;
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
    int initFlag;
    SDL_bool canLoad;
    SDL_bool canSave;
    SDL_bool (SDLCALL * checkFunction)(SDL_IOStream *src);
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
        IMG_INIT_AVIF,
#ifdef LOAD_AVIF
        SDL_TRUE,
#else
        SDL_FALSE,
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
        0,              /* no initialization */
#ifdef LOAD_BMP
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        0,              /* no initialization */
#ifdef LOAD_BMP
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        0,              /* no initialization */
#if USING_IMAGEIO || defined(LOAD_GIF)
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        0,              /* no initialization */
#ifdef LOAD_BMP
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        IMG_INIT_JPG,
#if (USING_IMAGEIO && defined(JPG_USES_IMAGEIO)) || defined(SDL_IMAGE_USE_WIC_BACKEND) || defined(LOAD_JPG)
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_IMAGE_SAVE_JPG,
        IMG_isJPG,
        IMG_LoadJPG_IO,
    },
    {
        "JXL",
        "sample.jxl",
        "sample.bmp",
        23,
        42,
        300,
        IMG_INIT_JXL,
#ifdef LOAD_JXL
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
        IMG_isJXL,
        IMG_LoadJXL_IO,
    },
#if 0
    {
        "LBM",
        "sample.lbm",
        "sample.bmp",
        23,
        42,
        0,              /* lossless? */
        0,              /* no initialization */
#ifdef LOAD_LBM
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        0,              /* no initialization */
#ifdef LOAD_PCX
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        IMG_INIT_PNG,
#if (USING_IMAGEIO && defined(PNG_USES_IMAGEIO)) || defined(SDL_IMAGE_USE_WIC_BACKEND) || defined(LOAD_PNG)
        SDL_TRUE,
#else
        SDL_FALSE,
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
        0,              /* no initialization */
#ifdef LOAD_PNM
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        0,              /* no initialization */
#ifdef LOAD_QOI
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        0,              /* no initialization */
#ifdef LOAD_SVG
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        0,              /* no initialization */
#ifdef LOAD_SVG
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        0,              /* no initialization */
#ifdef LOAD_SVG
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        0,              /* no initialization */
#if USING_IMAGEIO || defined(LOAD_TGA)
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        IMG_INIT_TIF,
#if USING_IMAGEIO || defined(SDL_IMAGE_USE_WIC_BACKEND) || defined(LOAD_TIF)
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        IMG_INIT_WEBP,
#ifdef LOAD_WEBP
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        0,              /* no initialization */
#ifdef LOAD_XCF
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        0,              /* no initialization */
#ifdef LOAD_XPM
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
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
        0,              /* no initialization */
#ifdef LOAD_XV
        SDL_TRUE,
#else
        SDL_FALSE,
#endif
        SDL_FALSE,      /* can save */
        IMG_isXV,
        IMG_LoadXV_IO,
    },
#endif
};

static SDL_bool
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
static SDL_bool
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
            return SDL_FALSE;
        }
        SDL_DestroySurface(*surface_p);
        *surface_p = temp;
    }
    return SDL_TRUE;
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
    int initResult = 0;
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
        reference = IMG_Load(refFilename);
        if (!SDLTest_AssertCheck(reference != NULL,
                                 "Loading reference should succeed (%s)",
                                 SDL_GetError())) {
            goto out;
        }
#endif
    }

    if (format->initFlag) {
        SDL_ClearError();
        initResult = IMG_Init(format->initFlag);
        if (!SDLTest_AssertCheck(initResult != 0,
                                 "Initialization should succeed (%s)",
                                 SDL_GetError())) {
            goto out;
        }
        SDLTest_AssertCheck(initResult & format->initFlag,
                            "Expected at least bit 0x%x set, got 0x%x",
                            format->initFlag, initResult);
    }

    if (mode != LOAD_CONVENIENCE) {
        SDL_ClearError();
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
            surface = IMG_Load(filename);
            break;

        case LOAD_IO:
            if (format->checkFunction != NULL) {
                SDL_IOStream *ref_src;
                int check;

                ref_src = SDL_IOFromFile(refFilename, "rb");
                SDLTest_AssertCheck(ref_src != NULL,
                                    "Opening %s should succeed (%s)",
                                    refFilename, SDL_GetError());
                if (ref_src != NULL) {
                    check = format->checkFunction(ref_src);
                    SDLTest_AssertCheck(!check,
                                        "Should not detect %s as %s -> %d",
                                        refFilename, format->name, check);
                    SDL_CloseIO(ref_src);
                }
            }

            if (format->checkFunction != NULL) {
                int check = format->checkFunction(src);

                SDLTest_AssertCheck(check,
                                    "Should detect %s as %s -> %d",
                                    filename, format->name, check);
            }

            SDL_ClearError();
            surface = IMG_Load_IO(src, SDL_TRUE);
            src = NULL;      /* ownership taken */
            break;

        case LOAD_TYPED_IO:
            surface = IMG_LoadTyped_IO(src, SDL_TRUE, format->name);
            src = NULL;      /* ownership taken */
            break;

        case LOAD_FORMAT_SPECIFIC:
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

    if (GetStringBoolean(SDL_getenv("SDL_IMAGE_TEST_DEBUG"), SDL_FALSE)) {
        DumpPixels(filename, surface);
    }

    if (reference != NULL) {
        ConvertToRgba32(&reference);
        ConvertToRgba32(&surface);
        diff = SDLTest_CompareSurfaces(surface, reference, format->tolerance);
        SDLTest_AssertCheck(diff == 0,
                            "Surface differed from reference by at most %d in %d pixels",
                            format->tolerance, diff);
        if (diff != 0 || GetStringBoolean(SDL_getenv("SDL_IMAGE_TEST_DEBUG"), SDL_FALSE)) {
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
    if (initResult) {
        IMG_Quit();
    }
}

static void
FormatSaveTest(const Format *format,
               SDL_bool rw)
{
    char *refFilename = NULL;
    char filename[64] = { 0 };
    SDL_Surface *reference = NULL;
    SDL_Surface *surface = NULL;
    SDL_IOStream *dest = NULL;
    int initResult = 0;
    int diff;
    SDL_bool result;

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

    if (format->initFlag) {
        SDL_ClearError();
        initResult = IMG_Init(format->initFlag);
        if (!SDLTest_AssertCheck(initResult != 0,
                                 "Initialization should succeed (%s)",
                                 SDL_GetError())) {
            goto out;
        }
        SDLTest_AssertCheck(initResult & format->initFlag,
                            "Expected at least bit 0x%x set, got 0x%x",
                            format->initFlag, initResult);
    }

    SDL_ClearError();
    if (SDL_strcmp (format->name, "AVIF") == 0) {
        if (rw) {
            dest = SDL_IOFromFile(filename, "wb");
            result = IMG_SaveAVIF_IO(reference, dest, SDL_FALSE, 90);
            SDL_CloseIO(dest);
        } else {
            result = IMG_SaveAVIF(reference, filename, 90);
        }
    } else if (SDL_strcmp(format->name, "JPG") == 0) {
        if (rw) {
            dest = SDL_IOFromFile(filename, "wb");
            result = IMG_SaveJPG_IO(reference, dest, SDL_FALSE, 90);
            SDL_CloseIO(dest);
        } else {
            result = IMG_SaveJPG(reference, filename, 90);
        }
    } else if (SDL_strcmp (format->name, "PNG") == 0) {
        if (rw) {
            dest = SDL_IOFromFile(filename, "wb");
            result = IMG_SavePNG_IO(reference, dest, SDL_FALSE);
            SDL_CloseIO(dest);
        } else {
            result = IMG_SavePNG(reference, filename);
        }
    } else {
        SDLTest_AssertCheck(SDL_FALSE, "How do I save %s?", format->name);
        goto out;
    }

    SDLTest_AssertCheck(result, "Save %s (%s)", filename, SDL_GetError());

    if (format->canLoad) {
        SDL_ClearError();
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
        if (diff != 0 || GetStringBoolean(SDL_getenv("SDL_IMAGE_TEST_DEBUG"), SDL_FALSE)) {
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
    if (initResult) {
        IMG_Quit();
    }
}

static void
FormatTest(const Format *format)
{
    SDL_bool forced;
    char envVar[64] = { 0 };

    SDL_snprintf(envVar, sizeof(envVar), "SDL_IMAGE_TEST_REQUIRE_LOAD_%s",
                 format->name);

    forced = GetStringBoolean(SDL_getenv(envVar), SDL_FALSE);
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

    forced = GetStringBoolean(SDL_getenv(envVar), SDL_FALSE);
    if (forced) {
        SDLTest_AssertCheck(format->canSave,
                            "%s saving should be enabled", format->name);
    }

    if (format->canSave || forced) {
        SDLTest_Log("Testing ability to save format %s", format->name);
        FormatSaveTest(format, SDL_FALSE);
        FormatSaveTest(format, SDL_TRUE);
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
