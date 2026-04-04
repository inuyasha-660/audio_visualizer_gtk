#include "ui.h"
#include "audio.h"
#include "glib.h"
#include "glibconfig.h"
#include "gtk/gtkshortcut.h"
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <adwaita.h>
#include <gtk/gtk.h>
#include <portaudio.h>
#include <stdio.h>

extern PlayData *playData;

static const char *TITLE_HOME = "audio_visualizer";
static int         Height = 950;
static int         Width = 1550;

static GtkWidget *Btn_play;
static GtkWidget *Spin_volume;

static void btn_play_update()
{
    if (playData->status == PLAYING) {
        gtk_button_set_icon_name(GTK_BUTTON(Btn_play), "media-playback-pause");
    } else {
        gtk_button_set_icon_name(GTK_BUTTON(Btn_play), "media-playback-start");
    }
}

static void audio_status_ctl(GtkWidget *Btn_play, gpointer user_data)
{
    if (playData->audiopath == NULL)
        return;

    if (playData->status == PLAYING) {
        gtk_button_set_icon_name(GTK_BUTTON(Btn_play), "media-playback-start");
        Pa_AbortStream(playData->pa_stream);
        playData->status = PAUSE;

    } else {
        gtk_button_set_icon_name(GTK_BUTTON(Btn_play), "media-playback-pause");
        Pa_StartStream(playData->pa_stream);
        playData->status = PLAYING;
    }
}

static void start_play(GtkWidget *btn, gpointer user_data)
{
    char *audio_path = (char *)user_data;

    if (playData->status == PLAYING) {
        audio_clean();
    }

    playData->status = PLAYING;
    btn_play_update();

    playData->audiopath = audio_path;
    audio_play();
}

static void volume_update(GtkWidget *spin)
{
    playData->volume = gtk_spin_button_get_value(GTK_SPIN_BUTTON(Spin_volume));
}

static gboolean on_drop(GtkDropTarget *target, const GValue *value, double x,
                        double y, gpointer user_data)
{
    const char *audio_path = g_value_get_string(value);
    GtkWidget  *box_audiolist = (GtkWidget *)user_data;

    gchar *filename = g_path_get_basename(audio_path);

    GtkWidget *btn_audio = gtk_button_new_with_label(filename);
    gtk_box_append(GTK_BOX(box_audiolist), btn_audio);
    g_signal_connect(btn_audio, "clicked", G_CALLBACK(start_play),
                     (gpointer)g_strdup(audio_path));

    g_free(filename);
    return TRUE;
}

static gboolean gl_draw_audio(GtkGLArea *area, GdkGLContext *context)
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glFlush();

    return TRUE;
}

static void update_draw_mode(GtkWidget *toggle, gpointer user_data)
{
    playData->draw_mode = GPOINTER_TO_INT(user_data);
}

void draw_ui_main(GtkApplication *app)
{
    GtkWidget *window = gtk_application_window_new(app);

    gtk_window_set_title(GTK_WINDOW(window), TITLE_HOME);
    gtk_window_set_default_size(GTK_WINDOW(window), Width, Height);

    GtkWidget *siderbar_home = adw_overlay_split_view_new();

    GtkWidget *box_sider = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    GtkWidget *box_audiolist = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(box_audiolist, 10);
    gtk_widget_set_margin_start(box_audiolist, 10);
    gtk_widget_set_margin_end(box_audiolist, 10);

    GtkWidget *scrolled_musiclist = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_musiclist),
                                  box_audiolist);
    gtk_widget_set_vexpand(scrolled_musiclist, TRUE);
    gtk_widget_set_hexpand(scrolled_musiclist, FALSE);

    // 允许拖入文件
    GtkDropTarget *drop_target =
        gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_COPY);

    gtk_widget_add_controller(GTK_WIDGET(scrolled_musiclist),
                              GTK_EVENT_CONTROLLER(drop_target));
    g_signal_connect(drop_target, "drop", G_CALLBACK(on_drop), box_audiolist);

    // 音频控制
    GtkWidget *box_play_ctl = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    Btn_play = gtk_button_new();
    gtk_button_set_icon_name(GTK_BUTTON(Btn_play), "media-playback-start");
    g_signal_connect(Btn_play, "clicked", G_CALLBACK(audio_status_ctl), NULL);

    Spin_volume = gtk_spin_button_new_with_range(0, 120, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(Spin_volume), DEFAULT_VOL);
    g_signal_connect(Spin_volume, "value-changed", G_CALLBACK(volume_update),
                     NULL);

    gtk_box_append(GTK_BOX(box_play_ctl), Btn_play);
    gtk_box_append(GTK_BOX(box_play_ctl), Spin_volume);
    gtk_widget_set_margin_start(box_play_ctl, 10);

    // 绘制模式控制
    GtkWidget *box_audio_toggle = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *toggle_wave = gtk_toggle_button_new_with_label("波形");
    GtkWidget *toggle_frequency = gtk_toggle_button_new_with_label("频谱");
    GtkWidget *toggle_xy = gtk_toggle_button_new_with_mnemonic("XY");

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

    // OpenGL 音频绘制
    GtkWidget *gl_area = gtk_gl_area_new();
    g_signal_connect(gl_area, "render", G_CALLBACK(gl_draw_audio), NULL);

    adw_overlay_split_view_set_content(ADW_OVERLAY_SPLIT_VIEW(siderbar_home),
                                       gl_area);

    gtk_window_set_child(GTK_WINDOW(window), siderbar_home);
    gtk_window_present(GTK_WINDOW(window));
}