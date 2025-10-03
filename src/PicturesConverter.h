#pragma once
#include <gtk/gtk.h>
#include <libintl.h>
#include <locale.h>

#define _(STRING) gettext(STRING)

#define PICTURES_CONVERTER_VERSION "1.1"


struct inFile{
    gchar *filepath;
    bool selected;
    uint32_t pages;
};

struct outFile{
    const gchar *outFilePath;
    const gchar *inFilePath;
    uint32_t *pages;
    uint32_t numPages;
    uint32_t max_pages;
};
struct inJobFile{
    const gchar *inFilePath;
    uint32_t *pages;
    uint32_t numPages;

};
struct convertJobFile{
    const gchar *outFilePath;
    GList *inJobFiles;
};
typedef struct {
    GtkProgressBar *bar;
    GtkLabel       *label;
    GtkWindow      *window;
} UIInfo;

inline GList *choosed_file_paths = NULL;
inline GList *convert_file_paths=NULL;
inline GList *computed_file_paths = NULL;

//FileManager
gboolean is_multiside_format(const gchar *filename);
gchar *ensure_unique_filename(const gchar *path);
void add_file(const gchar *filepath);
void clear_lists();
void convert_checked_files_to(const gchar *new_ext,GtkWidget *window);

//Ui
void refresh_computedlistbox(GList *paths);
void refresh_coosedlistbox(GList *paths);
void convert_files(GtkWidget* main_window);
void on_activate(GtkApplication *app, gpointer user_data);

//Converter
void* run_convert_files(void* data);

//Cnfig
void build_config_path();
void free_config_path();
void save_settings();
void load_settings();
