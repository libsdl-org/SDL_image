#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <stdio.h>
#include <string.h>

static void usage(const char *argv0)
{
    fprintf(stderr,
            "usage: %s <create|frames> <input.apng> [count]\n"
            "  create: create decoder only\n"
            "  frames: create decoder and fetch [count] frames (default: 2)\n",
            argv0);
}

int main(int argc, char **argv)
{
    const char *mode;
    const char *path;
    int count = 2;
    SDL_IOStream *src = NULL;
    IMG_AnimationDecoder *decoder = NULL;
    int rc = 1;

    if (argc < 3) {
        usage(argv[0]);
        return 2;
    }

    mode = argv[1];
    path = argv[2];
    if (argc >= 4) {
        count = SDL_atoi(argv[3]);
        if (count < 1) {
            fprintf(stderr, "invalid frame count: %s\n", argv[3]);
            return 2;
        }
    }

    if (!SDL_Init(0)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    src = SDL_IOFromFile(path, "rb");
    if (!src) {
        fprintf(stderr, "SDL_IOFromFile failed: %s\n", SDL_GetError());
        goto done;
    }

    decoder = IMG_CreateAnimationDecoder_IO(src, true, "apng");
    src = NULL;
    if (!decoder) {
        fprintf(stderr, "IMG_CreateAnimationDecoder_IO failed: %s\n", SDL_GetError());
        goto done;
    }

    fprintf(stdout, "decoder created for %s\n", path);

    if (strcmp(mode, "create") == 0) {
        rc = 0;
        goto done;
    }

    if (strcmp(mode, "frames") != 0) {
        usage(argv[0]);
        goto done;
    }

    for (int i = 0; i < count; ++i) {
        SDL_Surface *frame = NULL;
        Uint64 duration = 0;
        bool ok = IMG_GetAnimationDecoderFrame(decoder, &frame, &duration);

        fprintf(stdout,
                "frame %d: ok=%d status=%d duration=%llu error=%s\n",
                i,
                ok ? 1 : 0,
                (int)IMG_GetAnimationDecoderStatus(decoder),
                (unsigned long long)duration,
                SDL_GetError());

        if (frame) {
            SDL_DestroySurface(frame);
        }
        if (!ok) {
            break;
        }
    }

    rc = 0;

done:
    if (decoder) {
        IMG_CloseAnimationDecoder(decoder);
    } else if (src) {
        SDL_CloseIO(src);
    }
    SDL_Quit();
    return rc;
}
