#include "audio.h"
#include "ui.h"
#include <pipewire/pipewire.h>
#include <portaudio.h>
#include <sndfile.h>

float buffer_draw[BUFFER_LEN * 2];

PlayData *playData = {0};

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
