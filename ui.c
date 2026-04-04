#include "ui.h"
#include "glib-object.h"
#include "gtk/gtkshortcut.h"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_VOL 45

typedef enum {
    PLAYING = 1,
    PAUSE = 0,
} PLAYSTATUS;

static const char *TITLE_HOME = "audio_visualizer";
static int         Height = 930;
static int         Width = 1480;

static int    PlayListCount = 0;
static char **PlayList = {0};

static int PlayStatus = PAUSE;

void playlist_free()
{
    for (int i = 0; i < PlayListCount; i++) {
        free(PlayList[i]);
    }
    free(PlayList);
}

static gboolean on_drop(GtkDropTarget *target, const GValue *value, double x,
                        double y, gpointer user_data)
{
    const gchar *audio_path = g_value_get_string(value);
    GtkWidget   *box_audiolist = (GtkWidget *)user_data;

    gchar *filename = g_path_get_basename(audio_path);

    PlayList = (char **)realloc(PlayList, (PlayListCount + 1) * sizeof(char *));
    PlayList[PlayListCount] = strdup(audio_path);
    ++PlayListCount;

    GtkWidget *btn_audio = gtk_button_new_with_label(filename);
    gtk_box_append(GTK_BOX(box_audiolist), btn_audio);

    g_free(filename);
    return TRUE;
}

static void audio_status_ctl(GtkWidget *btn_play, gpointer user_data)
{
    PlayStatus = PlayStatus == PLAYING ? PAUSE : PLAYING;

    if (PlayStatus == PLAYING) {
        gtk_button_set_icon_name(GTK_BUTTON(btn_play), "media-playback-pause");
    } else {
        gtk_button_set_icon_name(GTK_BUTTON(btn_play), "media-playback-start");
    }
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
    GtkWidget *btn_play = gtk_button_new();
    gtk_button_set_icon_name(GTK_BUTTON(btn_play), "media-playback-start");
    g_signal_connect(btn_play, "clicked", G_CALLBACK(audio_status_ctl), NULL);

    GtkWidget *spin_volume = gtk_spin_button_new_with_range(0, 120, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_volume), DEFAULT_VOL);

    gtk_box_append(GTK_BOX(box_play_ctl), btn_play);
    gtk_box_append(GTK_BOX(box_play_ctl), spin_volume);
    gtk_widget_set_margin_start(box_play_ctl, 10);

    // 绘制模式控制
    AdwToggle *toggle_wave = adw_toggle_new();
    AdwToggle *toggle_frequency = adw_toggle_new();
    AdwToggle *toggle_xy = adw_toggle_new();
    adw_toggle_set_label(ADW_TOGGLE(toggle_wave), "波形");
    adw_toggle_set_label(ADW_TOGGLE(toggle_frequency), "频谱");
    adw_toggle_set_label(ADW_TOGGLE(toggle_xy), "XY");

    GtkWidget *toggle_audio_ctl = adw_toggle_group_new();
    adw_toggle_group_add(ADW_TOGGLE_GROUP(toggle_audio_ctl), toggle_wave);
    adw_toggle_group_add(ADW_TOGGLE_GROUP(toggle_audio_ctl), toggle_frequency);
    adw_toggle_group_add(ADW_TOGGLE_GROUP(toggle_audio_ctl), toggle_xy);
    gtk_widget_set_margin_start(toggle_audio_ctl, 10);
    gtk_widget_set_margin_end(toggle_audio_ctl, 10);
    gtk_widget_set_margin_bottom(toggle_audio_ctl, 8);

    gtk_box_append(GTK_BOX(box_sider), scrolled_musiclist);
    gtk_box_append(GTK_BOX(box_sider), box_play_ctl);
    gtk_box_append(GTK_BOX(box_sider), toggle_audio_ctl);

    adw_overlay_split_view_set_sidebar(ADW_OVERLAY_SPLIT_VIEW(siderbar_home),
                                       box_sider);

    gtk_window_set_child(GTK_WINDOW(window), siderbar_home);
    gtk_window_present(GTK_WINDOW(window));
}