#include "audio.h"
#include "ui.h"
#include <adwaita.h>
#include <pipewire/pipewire.h>
#include <stdlib.h>

const char *APPLICATION_ID = "net.fsunix.audio-visualizer";

extern PlayData *playData;
extern PWData   *pwData;

void clean_global()
{
    if (playData->pa_stream != NULL)
        audio_clean();
    Pa_Terminate();
    for (size_t i = 0; i < playData->MusicCount; i++) {
        free(playData->MusicList[i]);
    }
    free(playData->MusicList);
    free(playData);

    if (pwData->loop != NULL)
        pw_thread_loop_stop(pwData->loop);
    if (pwData->registry != NULL)
        pw_proxy_destroy((struct pw_proxy *)pwData->registry);
    if (pwData->core != NULL)
        pw_core_disconnect(pwData->core);
    if (pwData->context != NULL)
        pw_context_destroy(pwData->context);

    pw_thread_loop_destroy(pwData->loop);

    g_hash_table_destroy(pwData->table_node_row);

    fft_clean();
}

int main(int argc, char *argv[])
{
    g_autoptr(AdwApplication) app = NULL;
    int status = 0;

    app = adw_application_new(APPLICATION_ID, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(draw_ui_main), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    clean_global();
    return status;
}
