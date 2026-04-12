#include <sndfile.h>
#include <portaudio.h>

#define BUFFER_LEN 1024

#define DEFAULT_VOL 35.0f

typedef enum {
    PLAYING = 1,
    PAUSE = 0,
} PLAYSTATUS;

typedef enum {
    WAVE = 100,
    XY = 101,
    FREQUENCY = 102,
} DRAW_MODE;

typedef struct {
    int        status;
    int        xy_reverse; /* 模拟示波器-是否启用反转 */
    DRAW_MODE  draw_mode;
    char       *audiopath;

    PaStream   *pa_stream;
    sf_count_t frames_played;
    SNDFILE    *sndfile;
    SF_INFO    sf_info;
    float      volume;
} PlayData;

int audio_init();
int audio_play();
void audio_clean();
