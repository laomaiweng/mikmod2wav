/* mikmod2wav.c */
/* MikMod to WAV converter */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <mikmod.h>

#define PROGRAM "mikmod2wav"

int main(int argc, char *argv[])
{
    int rc = EXIT_FAILURE;
    const char *input_file = NULL;
    const char *output_file = NULL;
    size_t drv_cmdline_sz = 0;
    char *drv_cmdline = NULL;
    bool mikmod_init = false;
    MODULE *module = NULL;

    /* check argc */
    bool help = argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0);
    if (argc != 3 || help) {
        fprintf(stderr, "Usage: " PROGRAM " <mikmod input file> <.wav output file>\n");
        exit(help || argc == 1 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    /* filenames */
    input_file = argv[1];
    output_file = argv[2];

    long engineversion = MikMod_GetVersion();
    printf("***** " PROGRAM " *****\n"
           "-- MikMod v%ld.%ld.%ld --\n\n",
           (engineversion >> 16) & 0xFF,
           (engineversion >> 8) & 0xFF,
           (engineversion) & 0xFF);

    /* version check */
    if (engineversion < LIBMIKMOD_VERSION) {
        fprintf(stderr, "This program requires MikMod v%ld.%ld.%ld.\n",
                LIBMIKMOD_VERSION_MAJOR,
                LIBMIKMOD_VERSION_MINOR,
                LIBMIKMOD_REVISION);
        goto cleanup;
    }

    /* register the WAV disk writer driver */
    MikMod_RegisterDriver(&drv_wav);

    /* register all module loaders */
    MikMod_RegisterAllLoaders();

    /* prepare the driver command line */
    /* explicitly set md_device to use the WAV driver,
     * otherwise the command line gets ignored */
    md_device = 1;
    drv_cmdline_sz = sizeof("file=") + strlen(output_file);
    if ((drv_cmdline = (char*) malloc(drv_cmdline_sz)) == NULL) {
        fprintf(stderr, "Could not allocate driver command line\n");
        goto cleanup;
    }
    snprintf(drv_cmdline, drv_cmdline_sz, "file=%s", output_file);

    /* initialize the library */
    if (MikMod_Init(drv_cmdline)) {
        fprintf(stderr, "Could not initialize sound, reason: %s\n",
                MikMod_strerror(MikMod_errno));
        goto cleanup;
    }
    mikmod_init = true;

    /* load module */
    module = Player_Load(input_file, 64, 0);
    if (module) {
        printf("-- Module info --\n"
               "File: %s\n"
               "Title: %s\n"
               "Comment: %s\n"
               "Format: %s\n\n",
               input_file,
               module->songname,
               module->comment ? module->comment : "",
               module->modtype);

        /* start module */
        printf("-- Conversion --\n");
        Player_Start(module);

        while (Player_Active()) {
            /* we're playing */
            MikMod_Update();
            printf("\rpat:%" PRId16 "/%" PRIu16 " pos:%2.2" PRIx16 " time:%" PRIu32 ":%02" PRIu32,
                   module->sngpos, module->numpos, module->patpos,
                   (module->sngtime >> 10) / 60, (module->sngtime >> 10) % 60);
            fflush(stdout);
        }

        Player_Stop();
        printf("\n");
        Player_Free(module);
    } else {
        fprintf(stderr, "Could not load module, reason: %s\n",
                MikMod_strerror(MikMod_errno));
        goto cleanup;
    }

    /* all good */
    rc = EXIT_SUCCESS;

cleanup:
    free(drv_cmdline);
    if (mikmod_init) {
        MikMod_Exit();
    }
    return rc;
}
