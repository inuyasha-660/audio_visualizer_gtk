#include "audio.h"
#include "glib.h"
#include "ui.h"
#include <adwaita.h>
#include <fftw3.h>
#include <gtk/gtk.h>
#include <math.h>
#include <portaudio.h>
#include <stdlib.h>

extern PlayData *playData;
extern float     buffer_draw[BUFFER_LEN * 2];

static const char *TITLE_HOME = "Audio Visualizer";
static int         Height = 1000;
static int         Width = 1650;

static GtkWidget *Listbox_music;
static GtkWidget *Btn_play;
static GtkWidget *Spin_volume;
static GdkRGBA    color_draw = {0, 1, 0, 1};

/* 自定义控件 DrawForm */
typedef struct _DrawForm {
    GtkWidget parent_instance;
} _DrawForm;

G_DECLARE_FINAL_TYPE(DrawForm, draw_form, DRAWFORM, WIDGET, GtkWidget)
G_DEFINE_TYPE(DrawForm, draw_form, GTK_TYPE_WIDGET)

static void update_draw_color(GtkWidget *color_button, gpointer *user_data)
{
    color_draw = *gtk_color_dialog_button_get_rgba(
        GTK_COLOR_DIALOG_BUTTON(color_button));
}

static void btn_play_update()
{
    if (playData->status == PLAYING) {
        gtk_button_set_icon_name(GTK_BUTTON(Btn_play), "media-playback-pause");
    } else {
        gtk_button_set_icon_name(GTK_BUTTON(Btn_play), "media-playback-start");
    }
}

static void draw_form_xy(GtkSnapshot *snapshot, int width, int height,
                         GdkRGBA color)
{
    for (int i = 0; i < BUFFER_LEN; i++) {
        float left = buffer_draw[2 * i];
        float right = buffer_draw[(2 * i) + 1];

        int x, y;
        if (playData->xy_reverse) {
            x = (int)(width / 2) - (left * 300);
            y = (int)(height / 2) + (right * 300);
        } else {
            x = (int)(width / 2) + (left * 300);
            y = (int)(height / 2) - (right * 300);
        }

        graphene_rect_t rect;
        graphene_rect_init(&rect, x, y, 4, 4);
        gtk_snapshot_append_color(snapshot, &color, &rect);
    }
}

static void draw_form_wave(GtkSnapshot *snapshot, int width, int height,
                           GdkRGBA color)
{
    float mid_y = height / 2.0;
    float step_x = (float)width / BUFFER_LEN;

    for (int i = 0; i < BUFFER_LEN; i++) {
        if ((i % 2) != 0) {
            continue;
        }

        float           x = i * step_x;
        float           y = mid_y - (buffer_draw[i] * (height / 4.0));
        graphene_rect_t rect;

        graphene_rect_init(&rect, x, fmin(mid_y, y), step_x, fabs(y - mid_y));
        gtk_snapshot_append_color(snapshot, &color, &rect);
    }
}

/* FFTW */
int            has_init = 0;
int            Scale = 10;
fftwf_complex *fftw_out;
fftwf_plan     fft_plan;
static void    fft_init()
{
    fftw_out = fftwf_alloc_complex(BUFFER_LEN + 1);
    fft_plan = fftwf_plan_dft_r2c_1d(BUFFER_LEN * 2, buffer_draw, fftw_out,
                                     FFTW_ESTIMATE);

    has_init = 1;
}

void fft_clean()
{
    fftwf_destroy_plan(fft_plan);
    fftwf_free(fftw_out);
}

static void draw_form_fft(GtkSnapshot *snapshot, int width, int height,
                          GdkRGBA color)
{
    float step_x = (float)width / (BUFFER_LEN / 2.0 + 1);

    fftwf_execute(fft_plan);
    for (int i = 0; i < (BUFFER_LEN / 2 + 1); i++) {
        double fftw_h = sqrt(fftw_out[i][0] * fftw_out[i][0] +
                             (fftw_out[i][1] * fftw_out[i][1])) /
                        (BUFFER_LEN * 2);

        float y = fftw_h * height * Scale;
        float x = i * step_x;

        graphene_rect_t rect;

        graphene_rect_init(&rect, x, height - y, step_x, y);
        gtk_snapshot_append_color(snapshot, &color, &rect);
    }
}

static void draw_form_snapshot(GtkWidget *widget, GtkSnapshot *snapshot)
{
    int width = gtk_widget_get_width(widget);
    int height = gtk_widget_get_height(widget);

    gtk_snapshot_append_color(snapshot, &(GdkRGBA){0, 0, 0, 1.0},
                              &GRAPHENE_RECT_INIT(0, 0, width, height));

    if (playData->status == PAUSE) {
        return;
    }

    switch (playData->draw_mode) {
    case WAVE: {
        draw_form_wave(snapshot, width, height, color_draw);
        break;
    }
    case XY: {
        draw_form_xy(snapshot, width, height, color_draw);
        break;
    }
    case FREQUENCY:
        if (!has_init) {
            fft_init();
        }
        draw_form_fft(snapshot, width, height, color_draw);
        break;
    }
}

static void draw_form_class_init(DrawFormClass *class)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);
    widget_class->snapshot = draw_form_snapshot;
}

gboolean redraw(GtkWidget *widget, GdkFrameClock *frame_clock,
                gpointer user_data)
{
    if (playData->status == PLAYING) {
        gtk_widget_queue_draw(widget);
    }
    if (playData->status == END) {
        btn_play_update();
    }

    return G_SOURCE_CONTINUE;
}

static void draw_form_init(DrawForm *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    gtk_widget_set_name(widget, "dramform");
}

GtkWidget *draw_form_new()
{
    GtkWidget *draw_form = g_object_new(draw_form_get_type(), NULL);
    gtk_widget_set_hexpand(draw_form, TRUE);
    gtk_widget_set_vexpand(draw_form, TRUE);
    gtk_widget_add_tick_callback(draw_form, redraw, NULL, NULL);

    return draw_form;
}

/* 自定义控件 DrawForm */

static void start_play(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
    int index = gtk_list_box_row_get_index(row);

    if (playData->status == PLAYING) {
        audio_clean();
    }

    playData->status = PLAYING;
    btn_play_update();

    playData->audiopath = playData->MusicList[index];
    audio_play();
}

static void audio_status_ctl(GtkWidget *Btn_play, gpointer user_data)
{
    if (playData->audiopath == NULL)
        return;

    if (playData->status == PLAYING) {
        gtk_button_set_icon_name(GTK_BUTTON(Btn_play), "media-playback-start");
        Pa_AbortStream(playData->pa_stream);
        playData->status = PAUSE;

    } else if (playData->status == PAUSE) {
        gtk_button_set_icon_name(GTK_BUTTON(Btn_play), "media-playback-pause");
        Pa_StartStream(playData->pa_stream);
        playData->status = PLAYING;

    } else {
        gtk_button_set_icon_name(GTK_BUTTON(Btn_play), "media-playback-pause");
        playData->status = PLAYING;
        audio_clean();
        audio_play();
    }
}

static void volume_update(GtkWidget *spin)
{
    playData->volume = gtk_spin_button_get_value(GTK_SPIN_BUTTON(Spin_volume));
}

static gboolean on_drop(GtkDropTarget *target, const GValue *value, double x,
                        double y, gpointer user_data)
{
    GdkFileList *file_list = g_value_get_boxed(value);
    GSList      *list = gdk_file_list_get_files(file_list);

    for (GSList *l = list; l != NULL; l = l->next) {
        GFile *file = l->data;
        char  *audio_path = g_file_get_path(file);
        gchar *filename = g_path_get_basename(audio_path);

        playData->MusicList = (char **)realloc(
            playData->MusicList, (playData->MusicCount + 1) * sizeof(char *));
        playData->MusicList[playData->MusicCount] = strdup(audio_path);
        ++playData->MusicCount;

        GtkWidget *row_music = adw_action_row_new();
        gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row_music), TRUE);
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row_music), filename);
        gtk_list_box_append(GTK_LIST_BOX(Listbox_music), row_music);

        free(audio_path);
        free(filename);
    }

    return TRUE;
}

static void update_xy_reverse_mode(GtkWidget *btn, gpointer user_data)
{
    playData->xy_reverse = playData->xy_reverse ? 0 : 1;
}

static void update_draw_mode(GtkWidget *toggle, gpointer user_data)
{
    DRAW_MODE mode = GPOINTER_TO_INT(user_data);
    playData->draw_mode = mode;
}

void draw_ui_main(GtkApplication *app)
{
    GtkWidget *window = gtk_application_window_new(app);

    // 音频初始化
    if (audio_init() < 0) {
        g_error("Failed to initialize audio");
        exit(1);
    }

    gtk_window_set_title(GTK_WINDOW(window), TITLE_HOME);
    gtk_window_set_default_size(GTK_WINDOW(window), Width, Height);

    GtkWidget *siderbar_home = adw_overlay_split_view_new();

    GtkWidget *box_sider = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    GtkWidget *scrolled_musiclist = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled_musiclist, TRUE);

    // 播放列表
    Listbox_music = gtk_list_box_new();
    gtk_widget_add_css_class(Listbox_music, "boxed-list");
    g_signal_connect(Listbox_music, "row-activated", G_CALLBACK(start_play),
                     NULL);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_musiclist),
                                  Listbox_music);

    // 允许拖入文件
    GtkDropTarget *drop_target =
        gtk_drop_target_new(G_TYPE_INVALID, GDK_ACTION_COPY);
    gtk_drop_target_set_gtypes(drop_target,
                               (GType[1]){
                                   GDK_TYPE_FILE_LIST,
                               },
                               1);

    gtk_widget_add_controller(GTK_WIDGET(scrolled_musiclist),
                              GTK_EVENT_CONTROLLER(drop_target));
    g_signal_connect(drop_target, "drop", G_CALLBACK(on_drop), Listbox_music);

    // 音频控制
    GtkWidget *box_play_ctl = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    Btn_play = gtk_button_new();
    gtk_button_set_icon_name(GTK_BUTTON(Btn_play), "media-playback-start");
    g_signal_connect(Btn_play, "clicked", G_CALLBACK(audio_status_ctl), NULL);

    Spin_volume = gtk_spin_button_new_with_range(0, 120, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(Spin_volume), DEFAULT_VOL);
    g_signal_connect(Spin_volume, "value-changed", G_CALLBACK(volume_update),
                     NULL);

    GtkColorDialog *color_dialog_draw = gtk_color_dialog_new();
    GtkWidget *btn_color_draw = gtk_color_dialog_button_new(color_dialog_draw);
    gtk_color_dialog_button_set_rgba(GTK_COLOR_DIALOG_BUTTON(btn_color_draw),
                                     &color_draw);

    g_signal_connect(btn_color_draw, "notify::rgba",
                     G_CALLBACK(update_draw_color), NULL);

    GtkWidget *btn_xy_reverse = gtk_button_new_with_label("XY反转");
    gtk_widget_set_hexpand(btn_xy_reverse, TRUE);
    g_signal_connect(btn_xy_reverse, "clicked",
                     G_CALLBACK(update_xy_reverse_mode), NULL);

    gtk_box_append(GTK_BOX(box_play_ctl), Btn_play);
    gtk_box_append(GTK_BOX(box_play_ctl), Spin_volume);
    gtk_box_append(GTK_BOX(box_play_ctl), btn_color_draw);
    gtk_box_append(GTK_BOX(box_play_ctl), btn_xy_reverse);
    gtk_widget_set_margin_start(box_play_ctl, 10);
    gtk_widget_set_margin_end(box_play_ctl, 10);

    gtk_widget_set_name(box_play_ctl, "box-play-ctl");

    // 绘制模式控制
    GtkWidget *box_audio_toggle = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *toggle_wave = gtk_toggle_button_new_with_label("波形");
    GtkWidget *toggle_frequency = gtk_toggle_button_new_with_label("频谱");
    GtkWidget *toggle_xy = gtk_toggle_button_new_with_mnemonic("X-Y");

    gtk_widget_set_name(box_audio_toggle, "box-audio-toggle");

    gtk_widget_set_hexpand(toggle_wave, TRUE);
    gtk_widget_set_hexpand(toggle_frequency, TRUE);
    gtk_widget_set_hexpand(toggle_xy, TRUE);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_wave), TRUE);

    g_signal_connect(toggle_wave, "toggled", G_CALLBACK(update_draw_mode),
                     GINT_TO_POINTER(WAVE));
    g_signal_connect(toggle_frequency, "toggled", G_CALLBACK(update_draw_mode),
                     GINT_TO_POINTER(FREQUENCY));
    g_signal_connect(toggle_xy, "toggled", G_CALLBACK(update_draw_mode),
                     GINT_TO_POINTER(XY));

    gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(toggle_frequency),
                                GTK_TOGGLE_BUTTON(toggle_wave));
    gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(toggle_xy),
                                GTK_TOGGLE_BUTTON(toggle_wave));
    gtk_widget_set_margin_start(box_audio_toggle, 10);
    gtk_widget_set_margin_end(box_audio_toggle, 10);
    gtk_widget_set_margin_bottom(box_audio_toggle, 8);

    gtk_box_append(GTK_BOX(box_audio_toggle), toggle_wave);
    gtk_box_append(GTK_BOX(box_audio_toggle), toggle_frequency);
    gtk_box_append(GTK_BOX(box_audio_toggle), toggle_xy);

    // 为 SiderBar 添加部件
    gtk_box_append(GTK_BOX(box_sider), scrolled_musiclist);
    gtk_box_append(GTK_BOX(box_sider), box_play_ctl);
    gtk_box_append(GTK_BOX(box_sider), box_audio_toggle);

    adw_overlay_split_view_set_sidebar(ADW_OVERLAY_SPLIT_VIEW(siderbar_home),
                                       box_sider);

    GtkWidget *draw_form = draw_form_new();
    adw_overlay_split_view_set_content(ADW_OVERLAY_SPLIT_VIEW(siderbar_home),
                                       draw_form);

    gtk_window_set_child(GTK_WINDOW(window), siderbar_home);
    gtk_window_present(GTK_WINDOW(window));
}
