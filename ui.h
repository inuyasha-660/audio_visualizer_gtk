#include <gtk/gtk.h>

typedef enum {
    ADD,
    REMOVE,
} PW_OPERATE;

typedef struct {
    PW_OPERATE operate;
    GtkWidget *row_node;
} PWUpdateCtx;

void draw_ui_main(GtkApplication *app);
void fft_clean();
gboolean ui_update_pw_node(gpointer user_data);
