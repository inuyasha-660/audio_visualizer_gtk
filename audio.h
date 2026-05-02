#include <sndfile.h>
#include <portaudio.h>
#include <gtk/gtk.h>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

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
    int        mode; /* 0: 音频文件 1: PipeWire Node */
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

typedef struct {
    GHashTable *table_node_row;

    struct spa_audio_info format;
    struct pw_stream *stream;
    const struct spa_pod  *params[1];
    uint32_t               n_params;
    struct spa_pod_builder builder;

    struct pw_thread_loop *loop;
    struct pw_context *context;
    struct pw_core *core;
    struct pw_registry *registry;
    struct spa_hook registry_listener;
} PWData;

int audio_init();
int audio_play();
void audio_clean();
gboolean pw_setup(gpointer user_data);
void do_quit();
void start_record(const char *node_name);
