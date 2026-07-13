#pragma once
#include <gtk/gtk.h>
#include <libintl.h>
#include <locale.h>

#define _(STRING) gettext(STRING)

#define PICTURES_CONVERTER_VERSION "1.3"

typedef enum{
    png,
    jpg,
    pdf,
    tiff,
} OutPutFormat;

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
    OutPutFormat new_format;
};
struct convertJobFile{
    const gchar *outFilePath;
    OutPutFormat new_format;
    GList *inJobFiles;
};
typedef struct {
    GtkProgressBar *bar;
    GtkLabel       *label;
    GtkWindow      *window;
    GtkButton      *Close_button;
    GtkTextView    *failed_jobs_text_view;
    GtkTextBuffer  *failed_jobs_buffer;
    GList          *faluredJobs;
} UIInfo;

typedef struct {
    UIInfo *uIInfo;
    int total_jobs;
    int processed_jobs;
    const gchar *outFilePath;
} UIInfoTransver;

typedef struct {
    outFile *outFilePath;
    const gchar *reason;
} FailedJob;

struct FilechooserNewPathWindow{
    GtkWidget *window;
    OutPutFormat new_format;
};

enum TagedDitrectory{
    SourceFolder,
    PicturesFolder,
    AskForFolder,
};

inline GList *choosed_file_paths = NULL;
inline GList *convert_file_paths=NULL;
inline GList *computed_file_paths = NULL;
inline TagedDitrectory destinationDiretryType = PicturesFolder;
inline GtkWidget *resolution_spin;
inline GtkWidget *compression_spin;
inline GtkWidget *dpi_spin;

//FileManager
gboolean is_multiside_format(OutPutFormat format);
gchar *ensure_unique_filename(const gchar *path);
void add_file(const gchar *filepath);
void clear_lists();
void convert_checked_files_to(OutPutFormat format,GtkWidget *window);
gboolean ensure_directory_exists(gchar *directory_path);

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
