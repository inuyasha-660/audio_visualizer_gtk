#include <sndfile.h>
#include <portaudio.h>

#define BUFFER_LEN 1024

#define DEFAULT_VOL 35.0f

typedef enum {
    PAUSE = 0,
    PLAYING = 1,
    INITIAL = 3, /* 未填入音频 */
    END = 2, /* 播放完毕 */
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

    char      *audiopath;
    size_t     MusicCount;
    char     **MusicList;

    PaStream   *pa_stream;
    sf_count_t frames_played;
    SNDFILE    *sndfile;
    SF_INFO    sf_info;
    float      volume;
} PlayData;

int audio_init();
int audio_play();
void audio_clean();
