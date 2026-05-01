#include "audio.h"
#include "ui.h"
#include <adwaita.h>
#include <pipewire/pipewire.h>
#include <portaudio.h>
#include <sndfile.h>
#include <stdlib.h>

float buffer_draw[BUFFER_LEN * 2];

PlayData *playData = {0};
PWData   *pwData = {0};

int play_callback(const void *input, void *output, unsigned long frameCount,
                  const PaStreamCallbackTimeInfo *timeInfo,
                  PaStreamCallbackFlags statusFlags, void *userData)
{
    int        ret = paContinue;
    float     *out = (float *)output;
    sf_count_t read_count;

    read_count = sf_readf_float(playData->sndfile, out, frameCount);
    if (read_count < frameCount) {
        playData->status = END;

        int rest = (frameCount - read_count) * playData->sf_info.channels;
        for (int i = 0; i < rest; i++) {
            out[read_count * playData->sf_info.channels + i] = 0.0f;
        }
        ret = paComplete;
    } else {
        playData->frames_played += read_count;
    }

    memcpy(buffer_draw, out,
           read_count * playData->sf_info.channels * sizeof(float));

    int total_samples = read_count * playData->sf_info.channels;
    for (int i = 0; i < total_samples; i++) {
        out[i] *= (playData->volume / 100.0f);
    }

    return ret;
}

void audio_clean()
{
    Pa_StopStream(playData->pa_stream);
    sf_close(playData->sndfile);
    Pa_CloseStream(playData->pa_stream);
    playData->frames_played = 0;
}

int audio_play()
{
    PaError pa_err;

    playData->sndfile =
        sf_open(playData->audiopath, SFM_READ, &playData->sf_info);

    pa_err = Pa_OpenDefaultStream(
        &playData->pa_stream, 0, playData->sf_info.channels, paFloat32,
        playData->sf_info.samplerate, BUFFER_LEN, play_callback, playData);
    if (pa_err != paNoError) {
        fprintf(stderr,
                "[Pa_OpenDefaultStream] Failed to open default stream\n");
        fprintf(stderr, "%s\n", Pa_GetErrorText(pa_err));
    }

    pa_err = Pa_StartStream(playData->pa_stream);
    if (pa_err != paNoError) {
        fprintf(stderr, "[Pa_StartStream] Failed to start pa_stream\n");
        fprintf(stderr, "%s\n", Pa_GetErrorText(pa_err));
    }

    return pa_err;
}

int audio_init()
{
    playData = malloc(sizeof(PlayData));
    playData->MusicCount = 0;
    playData->MusicList = NULL;
    playData->audiopath = NULL;
    playData->xy_reverse = 0;
    playData->draw_mode = WAVE;
    playData->status = INITIAL;
    playData->frames_played = 0;
    playData->volume = DEFAULT_VOL;
    playData->pa_stream = NULL;

    PaError pa_err;

    pa_err = Pa_Initialize();
    if (pa_err != paNoError) {
        fprintf(stderr, "[Pa_Initialize] %s\n", Pa_GetErrorText(pa_err));
        free(playData);

        return -1;
    }

    return 0;
}

static void registry_event_global(void *data, uint32_t id, uint32_t permissions,
                                  const char *type, uint32_t version,
                                  const struct spa_dict *props)
{
    if (props == NULL || props->n_items == 0) {
        return;
    }

    const struct spa_dict_item *item;
    spa_dict_for_each(item, props)
    {
        if (strcmp(item->key, "node.name"))
            continue;

        GtkWidget *row_node = adw_action_row_new();
        gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row_node), TRUE);
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row_node),
                                      item->value);

        g_hash_table_insert(pwData->table_node_row, GINT_TO_POINTER(id),
                            row_node);

        PWUpdateCtx *update_ctx = malloc(sizeof(PWUpdateCtx));
        update_ctx->operate = ADD;
        update_ctx->row_node = row_node;

        g_idle_add(ui_update_pw_node, update_ctx);
    }
}

static void registry_event_global_remove(void *data, uint32_t id)
{
    GtkWidget *row_node =
        g_hash_table_lookup(pwData->table_node_row, GINT_TO_POINTER(id));

    if (row_node != NULL) {
        PWUpdateCtx *update_ctx = malloc(sizeof(PWUpdateCtx));
        update_ctx->operate = REMOVE;
        update_ctx->row_node = row_node;
        g_idle_add(ui_update_pw_node, update_ctx);

        g_hash_table_remove(pwData->table_node_row, GINT_TO_POINTER(id));
    }
}

static const struct pw_registry_events registry_events = {
    PW_VERSION_REGISTRY_EVENTS,
    .global = registry_event_global,
    .global_remove = registry_event_global_remove,
};

gboolean pw_setup(gpointer user_data)
{
    int    argc = 0;
    char **argv = NULL;
    pw_init(&argc, &argv);

    pwData = malloc(sizeof(PWData));
    pwData->table_node_row = g_hash_table_new(g_direct_hash, g_direct_equal);
    pwData->context = NULL;
    pwData->core = NULL;
    pwData->loop = NULL;
    pwData->registry = NULL;

    pwData->loop = pw_thread_loop_new("pw-server", NULL);
    if (pwData->loop == NULL) {
        g_warning("Failed to create main loop of PipeWire");
        return G_SOURCE_REMOVE;
    }

    pwData->context =
        pw_context_new(pw_thread_loop_get_loop(pwData->loop), NULL, 0);
    if (pwData->context == NULL) {
        g_warning("Failed to create context\n");
        return G_SOURCE_REMOVE;
    }

    pwData->core = pw_context_connect(pwData->context, NULL, 0);
    if (pwData->core == NULL) {
        g_warning("Failed to connect to context\n");
        return G_SOURCE_REMOVE;
    }

    pwData->registry =
        pw_core_get_registry(pwData->core, PW_VERSION_REGISTRY, 0);
    if (pwData->registry == NULL) {
        g_warning("Failed to connect to context\n");
        return G_SOURCE_REMOVE;
    }

    spa_zero(pwData->registry_listener);
    pw_registry_add_listener(pwData->registry, &pwData->registry_listener,
                             &registry_events, NULL);

    pw_thread_loop_start(pwData->loop);

    return G_SOURCE_REMOVE;
}
