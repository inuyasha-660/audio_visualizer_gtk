#include "audio.h"
#include "ui.h"
#include <adwaita.h>
#include <portaudio.h>

const char *APPLICATION_NAME = "audio_visualizer";
const char *APPLICATION_ID = "net.fsunix.audio-visualizer";

extern PlayData *playData;

int main(int argc, char *argv[])
{
    g_autoptr(AdwApplication) app = NULL;
    int status = 0;

    if (audio_init() < 0) {
        return 1;
    }

    app = adw_application_new(APPLICATION_ID, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(draw_ui_main), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    audio_clean();
    Pa_Terminate();
    free(playData->audiopath);
    fft_clean();

    return status;
}
