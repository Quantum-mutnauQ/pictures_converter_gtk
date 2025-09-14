#include <gtk/gtk.h>
#include <libintl.h>
#include <locale.h>
#include <libconfig.h>
#include <magic.h>
#include <poppler/glib/poppler.h> // Für PDF
#include <tiffio.h> // Für TIFF
#include <cairo-pdf.h>
#include <pthread.h>

struct inFile{
    gchar *filepath;
    bool selected;
    uint32_t pages;
};

struct outFile{
    const gchar *outFilePath;
    const gchar *inFilePath;
    uint32_t *pages;
    uint32_t max_pages;
};
struct inJobFile{
    const gchar *inFilePath;
    uint32_t *pages;
};
struct convertJobFile{
    const gchar *outFilePath;
    GList *inJobFiles;
};

#define _(STRING) gettext(STRING)
#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

GList *choosed_file_paths = NULL;
GList *convert_file_paths=NULL;
GList *computed_file_paths = NULL;

// UI widgets
GtkWidget *choosed_files_listbox;
GtkWidget *computed_files_listbox;

gchar *config_file_path = NULL;

void save_settings() {
    config_t cfg;
    config_init(&cfg);

    config_setting_t *root = config_root_setting(&cfg);

    if (!config_write_file(&cfg, config_file_path)) {
        fprintf(stderr, _("Error while writing file.\n"));
    }


    config_destroy(&cfg);
}
void load_settings() {
    config_t cfg;
    config_init(&cfg);

    if (!config_read_file(&cfg, config_file_path)) {
        fprintf(stderr, _("Error while reading file: %s:%d - %s\n"),
                config_error_file(&cfg),
                config_error_line(&cfg),
                config_error_text(&cfg));
        fprintf(stderr, _("Trye to corrct\n"));
        save_settings();
        config_destroy(&cfg);
        return;
    }

    config_destroy(&cfg);
}
void on_check_button_toggled(GtkCheckButton *toggle_button, gpointer user_data) {
    gboolean active = gtk_check_button_get_active(toggle_button);
    ((inFile*) user_data)->selected =active;
}

void refresh_coosedlistbox(GtkWidget *listbox, GList *paths) {
    gtk_list_box_remove_all(GTK_LIST_BOX(listbox));

    for (GList *l = paths; l != NULL; l = l->next) {
        inFile* infile = ((inFile*) l->data);
        const gchar *filepath = (const gchar*)infile->filepath;

        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

        GtkWidget *checkButton = gtk_check_button_new();
        g_signal_connect(checkButton, "toggled", G_CALLBACK(on_check_button_toggled), ((inFile*) l->data));

        gtk_box_append(GTK_BOX(row), checkButton);
        gtk_check_button_set_active(GTK_CHECK_BUTTON(checkButton),((inFile*) l->data)->selected);

        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(filepath, 48, 48, TRUE, NULL);
        GtkWidget *image;

        if (pixbuf) {
            GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
            image = gtk_image_new_from_paintable(GDK_PAINTABLE(texture));
            g_object_unref(texture);
            g_object_unref(pixbuf);
        } else {
            image = gtk_image_new_from_icon_name("image-missing");
        }

        gtk_box_append(GTK_BOX(row), image);

        const gchar *filename = g_path_get_basename(filepath);
        GtkWidget *label = gtk_label_new(filename);
        gtk_widget_set_hexpand(label, TRUE);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_box_append(GTK_BOX(row), label);

        gtk_widget_set_tooltip_text(row, filepath);

        GtkWidget *list_row = gtk_list_box_row_new();
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(list_row), row);
        g_object_set_data(G_OBJECT(list_row), "file-data",(gpointer)infile);

        // Rechtsklick-Kontextmenü
        GtkGestureClick *click = GTK_GESTURE_CLICK(gtk_gesture_click_new());
        gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_SECONDARY);
        gtk_widget_add_controller(list_row, GTK_EVENT_CONTROLLER(click));


        gtk_list_box_append(GTK_LIST_BOX(listbox), list_row);
    }
}

void refresh_computedlistbox(GtkWidget *listbox, GList *paths) {
    gtk_list_box_remove_all(GTK_LIST_BOX(listbox));

    for (GList *l = paths; l != NULL; l = l->next) {
        outFile *out_File=(outFile*) l->data;
        const gchar *filepath = (const gchar*)out_File->outFilePath;

        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale((const gchar*)out_File->inFilePath, 48, 48, TRUE, NULL);
        GtkWidget *image;

        if (pixbuf) {
            GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
            image = gtk_image_new_from_paintable(GDK_PAINTABLE(texture));
            g_object_unref(texture);
            g_object_unref(pixbuf);
        } else {
            image = gtk_image_new_from_icon_name("image-missing");
        }

        gtk_box_append(GTK_BOX(row), image);

        const gchar *filename = g_path_get_basename(filepath);
        GtkWidget *label = gtk_label_new(filename);
        gtk_widget_set_hexpand(label, TRUE);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_box_append(GTK_BOX(row), label);

        //gtk_widget_set_tooltip_text(row, g_strdup_printf(_("Original Datei: %s\nWandelnde Datei: %s\nSeite: %d von %d"), out_File->inFilePath, out_File->outFilePath,out_File->pages+1,out_File->max_pages+1));

        GtkWidget *list_row = gtk_list_box_row_new();
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(list_row), row);
        g_object_set_data(G_OBJECT(list_row), "file-data", (gpointer)out_File);

        // Rechtsklick-Kontextmenü
        GtkGestureClick *click = GTK_GESTURE_CLICK(gtk_gesture_click_new());
        gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_SECONDARY);
        gtk_widget_add_controller(list_row, GTK_EVENT_CONTROLLER(click));


        gtk_list_box_append(GTK_LIST_BOX(listbox), list_row);

    }
}

const char* check_file_type(const char *filename) {
    magic_t magic;
    const char *type;

    // Initialisiere die magic-Bibliothek
    magic = magic_open(MAGIC_MIME_TYPE);
    if (magic == NULL) {
        fprintf(stderr, "Fehler beim Initialisieren von magic.\n");
        return NULL;
    }

    if (magic_load(magic, NULL) != 0) {
        fprintf(stderr, "Fehler beim Laden der magic-Daten: %s\n", magic_error(magic));
        magic_close(magic);
        return NULL;
    }

    // Bestimme den Dateityp
    type = magic_file(magic, filename);
    if (type == NULL) {
        fprintf(stderr, "Fehler beim Bestimmen des Dateityps: %s\n", magic_error(magic));
        magic_close(magic);
        return NULL;
    }

    // Dupliziere den String, um sicherzustellen, dass er nicht verändert wird
    char *type_copy = strdup(type);
    if (type_copy == NULL) {
        fprintf(stderr, "Fehler beim Duplizieren des Dateityps.\n");
        magic_close(magic);
        return NULL;
    }

    // Schließe die magic-Bibliothek
    magic_close(magic);
    return type_copy; // Rückgabe des duplizierten Dateityps
}
char* convert_to_file_url(const char *filepath) {
    // Erstelle eine URL aus dem Dateipfad
    char *url = g_strdup_printf("file://%s", filepath);
    return url;
}
static gboolean is_multiside_format(const gchar *filename) {
    const gchar *ext = strrchr(filename, '.');
    if (!ext) return FALSE;
    ext++; // skip the '.'
    return g_ascii_strcasecmp(ext, "pdf") == 0 || g_ascii_strcasecmp(ext, "tiff") == 0 || g_ascii_strcasecmp(ext, "tif") == 0;
}

GList* group_convert_jobs(GList *outFiles) {
    GList *result = NULL;

    // Gruppiere alle outFile nach outFilePath
    GHashTable *grouped = g_hash_table_new(g_str_hash, g_str_equal);

    for (GList *l = outFiles; l != NULL; l = l->next) {
        struct outFile *of = (struct outFile *)l->data;

        GList *group = (GList *)g_hash_table_lookup(grouped, of->outFilePath);
        group = g_list_append(group, of);
        g_hash_table_insert(grouped, (gpointer)of->outFilePath, group);
    }

    // Verarbeite jede Gruppe
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, grouped);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        const gchar *outFilePath = (const gchar *)key;
        GList *group = (GList *)value;

        // Prüfe Format
        gboolean is_multi = is_multiside_format(outFilePath);

        if (is_multi) {
            // Alle in einer convertJobFile sammeln
            struct convertJobFile *job = g_new0(struct convertJobFile, 1);
            job->outFilePath = outFilePath;

            for (GList *g = group; g != NULL; g = g->next) {
                struct outFile *of = (struct outFile *)g->data;
                struct inJobFile *inj = g_new(struct inJobFile, 1);
                inj->inFilePath = of->inFilePath;
                //------------------------------------------------------------------------
                inj->pages = of->pages;
                job->inJobFiles = g_list_append(job->inJobFiles, inj);
            }

            result = g_list_append(result, job);
        } else {
            // Für jede Eingabedatei eine eigene convertJobFile
            int index = 0;
            for (GList *g = group; g != NULL; g = g->next, index++) {
                struct outFile *of = (struct outFile *)g->data;

                struct convertJobFile *job = g_new0(struct convertJobFile, 1);

                // Neuen Dateinamen erzeugen mit _0, _1, ...
                gchar *newOutPath = g_strdup_printf("%s_%d%s",
                                                    g_strndup(outFilePath, (gint)(strrchr(outFilePath, '.') - outFilePath)), // ohne Erweiterung
                                                    index,
                                                    strrchr(outFilePath, '.')); // Erweiterung

                job->outFilePath = newOutPath;

                struct inJobFile *inj = g_new(struct inJobFile, 1);
                inj->inFilePath = of->inFilePath;
                uint32_t *pages = new uint32_t[1];
                pages[0]=0;
                inj->pages =pages;

                job->inJobFiles = g_list_append(NULL, inj);
                result = g_list_append(result, job);
            }
        }
    }

    g_hash_table_destroy(grouped);
    return result;
}
void free_convertJobFile(gpointer data){
    struct inJobFile* file = (inJobFile*)data;
    g_free((gpointer)file->inFilePath);
    g_free(file);
}
void free_convert_job_file(gpointer data) {
    if (!data) return;

    struct convertJobFile *job = (struct convertJobFile *)data;

    // Gib alle inJobFiles frei
    if (job->inJobFiles) {
        g_list_free_full(job->inJobFiles, (GDestroyNotify)free_convertJobFile);
        job->inJobFiles = NULL;
    }

    // Falls outFilePath dynamisch alloziert wurde (z.B. mit g_strdup), freigeben
    // Nur freigeben, wenn du sicher bist, dass du Besitzer des Strings bist
    //g_free((gpointer)job->outFilePath);

    // Zum Schluss das struct selbst freigeben
    g_free(job);
}
guchar* convert_rgba_to_rgb(const guchar* src, int width, int height, int src_rowstride) {
    guchar *rgb_data = (guchar *)g_malloc(width * height * 3);
    for (int y = 0; y < height; y++) {
        const guchar *row = src + y * src_rowstride;
        for (int x = 0; x < width; x++) {
            rgb_data[(y * width + x) * 3 + 0] = row[x * 4 + 0]; // R
            rgb_data[(y * width + x) * 3 + 1] = row[x * 4 + 1]; // G
            rgb_data[(y * width + x) * 3 + 2] = row[x * 4 + 2]; // B
            // Alpha ignorieren
        }
    }
    return rgb_data;
}
void printConvertJobFileList(GList *convertJobFileList) {
    // Überprüfen, ob die Liste leer ist
    if (convertJobFileList == NULL) {
        printf("Die Liste der ConvertJobFiles ist leer.\n");
        return;
    }

    // Durchlaufe die Liste der convertJobFile-Elemente
    for (GList *node = convertJobFileList; node != NULL; node = node->next) {
        struct convertJobFile *jobFile = (struct convertJobFile *)node->data;

        // Ausgabe der Informationen des convertJobFile
        printf("Ausgabedatei: %s\n", jobFile->outFilePath);
        printf("   Eingabedateien:\n");

        // Durchlaufe die Liste der inJobFiles
        for (GList *inJobNode = jobFile->inJobFiles; inJobNode != NULL; inJobNode = inJobNode->next) {
            struct inJobFile *inJobFile = (struct inJobFile *)inJobNode->data;
            for(uint32_t i = 0; i <ARRAY_COUNT(inJobFile->pages);i++)
            //----------------------------------------------------------------------------------------------------------
            printf("     - Eingabedatei: %s, Seite: %dS\n", inJobFile->inFilePath, i);
        }

        printf("\n"); // Leerzeile für bessere Lesbarkeit
    }
}
gboolean save_pixbuf_as_pdf(GdkPixbuf *pixbuf, const gchar *filename, GError **error) {
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);

    cairo_surface_t *surface = cairo_pdf_surface_create(filename, width, height);
    if (!surface) {
        g_set_error(error, g_quark_from_static_string("save-pdf"), 1, "PDF-Surface konnte nicht erstellt werden.");
        return FALSE;
    }

    cairo_t *cr = cairo_create(surface);

    // Pixbuf → Cairo pattern
    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return TRUE;
}
GdkPixbuf* render_page_to_pixbuf(PopplerPage *page, double dpi) {
    double width, height;
    poppler_page_get_size(page, &width, &height);

    int pixel_width = (int)(width * dpi / 72.0);
    int pixel_height = (int)(height * dpi / 72.0);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, pixel_width, pixel_height);
    cairo_t *cr = cairo_create(surface);

    cairo_scale(cr, dpi / 72.0, dpi / 72.0); // Skalierung für gewünschte DPI
    poppler_page_render(page, cr);          // Seite auf Cairo zeichnen
    cairo_destroy(cr);

    // Oberfläche in Pixbuf umwandeln
    GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, pixel_width, pixel_height);

    cairo_surface_destroy(surface);
    return pixbuf;
}

const gchar* get_format_from_extension(const gchar *filename) {
    const gchar *ext = strrchr(filename, '.');
    if (!ext || ext == filename) return NULL;

    ext++; // skip the '.'

    if (g_ascii_strcasecmp(ext, "jpg") == 0 || g_ascii_strcasecmp(ext, "jpeg") == 0) return "jpeg";
    if (g_ascii_strcasecmp(ext, "png") == 0) return "png";
    if (g_ascii_strcasecmp(ext, "tiff") == 0 || g_ascii_strcasecmp(ext, "tif") == 0) return "tiff";
    if (g_ascii_strcasecmp(ext, "bmp") == 0) return "bmp";

    return NULL;
}
void perform_conversion_jobs(GList *convertJobFileList) {
    for (GList *node = convertJobFileList; node != NULL; node = node->next) {
        struct convertJobFile *jobFile = (struct convertJobFile *)node->data;
        const gchar *output_path = jobFile->outFilePath;

        gboolean is_tiff_output = g_str_has_suffix(output_path, ".tif") || g_str_has_suffix(output_path, ".tiff");

        TIFF *tiff = NULL;
        if (is_tiff_output) {
            tiff = TIFFOpen(output_path, "w");
            if (!tiff) {
                g_printerr("Fehler beim Öffnen von %s für TIFF-Ausgabe.\n", output_path);
                continue;
            }
        }

        int page_index = 0;

        for (GList *inNode = jobFile->inJobFiles; inNode != NULL; inNode = inNode->next, page_index++) {
            struct inJobFile *inJob = (struct inJobFile *)inNode->data;
            GdkPixbuf *pixbuf = NULL;

            if (g_str_has_suffix(inJob->inFilePath, ".pdf")) {
                // PDF verarbeiten
                GError *error = NULL;
                PopplerDocument *doc = poppler_document_new_from_file(
                    g_strdup_printf("file://%s", inJob->inFilePath),
                    NULL, &error);

                if (!doc) {
                    g_printerr("Fehler beim Laden von PDF %s: %s\n", inJob->inFilePath, error->message);
                    g_error_free(error);
                    continue;
                }

                int total_pages = poppler_document_get_n_pages(doc);
                int maxpage=0;
                int size = sizeof(inJob->pages)/sizeof(inJob->pages[0]);
                for(int i; i > size;i++){
                    if(maxpage < size){
                        maxpage=i;
                    }
                }

                if (maxpage >= total_pages) {
                    //--------------------------------------------------------------------
                    g_printerr("Ungültige Seite %n in %s (nur %d Seiten).\n",
                               inJob->pages, inJob->inFilePath, total_pages);
                    g_object_unref(doc);
                    continue;
                }

                PopplerPage *page = poppler_document_get_page(doc, inJob->pages[0]);
                if (!page) {
                    //-----------------------------------------------------------------
                    g_printerr("Fehler beim Abrufen der Seite %n von %s.\n", inJob->pages, inJob->inFilePath);
                    g_object_unref(doc);
                    continue;
                }

                pixbuf = render_page_to_pixbuf(page, 300.0);
                g_object_unref(page);
                g_object_unref(doc);
            } else {
                // Anderes Bildformat (PNG, JPG, etc.)
                GError *load_error = NULL;
                pixbuf = gdk_pixbuf_new_from_file(inJob->inFilePath, &load_error);
                if (!pixbuf) {
                    g_printerr("Fehler beim Laden von Bild %s: %s\n", inJob->inFilePath, load_error->message);
                    g_error_free(load_error);
                    continue;
                }
            }

            if (!pixbuf) {
                g_printerr("Kein Pixbuf für Datei %s\n", inJob->inFilePath);
                continue;
            }

            if (is_tiff_output) {
                int width = gdk_pixbuf_get_width(pixbuf);
                int height = gdk_pixbuf_get_height(pixbuf);
                int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
                int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
                gboolean has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);
                guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

                guchar *rgb_data = NULL;

                if (has_alpha || n_channels == 4) {
                    rgb_data = convert_rgba_to_rgb(pixels, width, height, rowstride);
                    pixels = rgb_data;
                    rowstride = width * 3;
                    n_channels = 3;
                }

                // TIFF-Felder setzen
                TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, width);
                TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, height);
                TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, n_channels);
                TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8);
                TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
                TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
                TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
                TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_LZW);

                // Zeilen schreiben
                for (int y = 0; y < height; y++) {
                    TIFFWriteScanline(tiff, pixels + y * rowstride, y, 0);
                }

                TIFFWriteDirectory(tiff);

                if (rgb_data)
                    g_free(rgb_data);
            } else {
                // Kein TIFF → wir schreiben direkt als JPG, PNG usw.
                GError *save_error = NULL;

                // Bei Multi-Eingaben: z.B. out.jpg → out_0.jpg, out_1.jpg usw.
                gchar *final_path = (g_list_length(jobFile->inJobFiles) > 1)
                                        ? g_strdup_printf("%s_%d%s",
                                                          g_strndup(output_path, (gint)(strrchr(output_path, '.') - output_path)),
                                                          page_index,
                                                          strrchr(output_path, '.'))
                                        : g_strdup(output_path);

                const gchar *format = get_format_from_extension(final_path);

                if (!format) {
                    // PDF als Sonderfall behandeln
                    if (g_str_has_suffix(final_path, ".pdf")) {
                        if (!save_pixbuf_as_pdf(pixbuf, final_path, &save_error)) {
                            g_printerr("Fehler beim Speichern als PDF: %s\n", save_error ? save_error->message : "Unbekannt");
                            if (save_error) g_error_free(save_error);
                        } else {
                            g_print("PDF erfolgreich gespeichert: %s\n", final_path);
                        }
                        g_object_unref(pixbuf);
                        g_free(final_path);
                        continue;
                    } else {
                        g_printerr("Unbekanntes Bildformat für Datei %s\n", final_path);
                        g_object_unref(pixbuf);
                        g_free(final_path);
                        continue;
                    }
                }

                if (!gdk_pixbuf_save(pixbuf, final_path, format, &save_error, NULL)) {
                    g_printerr("Fehler beim Speichern von %s: %s\n", final_path, save_error->message);
                    g_error_free(save_error);
                } else {
                    g_print("Datei erfolgreich gespeichert: %s\n", final_path);
                }

                g_free(final_path);
            }

            g_object_unref(pixbuf);
        }

        if (tiff)
            TIFFClose(tiff);

        g_print("Konvertierung abgeschlossen: %s\n", output_path);
    }
}
void free_out_file(gpointer data) {
    convertJobFile *file = (convertJobFile*) data;
    //g_free((gpointer)file->outFilePath);
   //g_list_free_full(file->inJobFiles,free_inJobFile);
}
void* run_convert_files(void* data){
    GList *preview_groups = group_convert_jobs(convert_file_paths);
    printConvertJobFileList(preview_groups);
    //perform_conversion_jobs(preview_groups);
    g_list_free_full(preview_groups, (GDestroyNotify)free_convert_job_file);

    g_list_free_full(convert_file_paths, free_out_file);
    convert_file_paths = NULL;
    return NULL;
}
void convert_files(GtkWidget* main_window) {
    if (!convert_file_paths) {
        g_print(_("Keine Dateien zum Konvertieren vorhanden.\n"));
        return;
    }
    
    GtkWidget* progress_window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(progress_window),_("Umwandlungs prozess"));
    // Wichtig: Blockiere Interaktion mit dem Hauptfenster
    gtk_window_set_modal(GTK_WINDOW(progress_window), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(progress_window), GTK_WINDOW(main_window));
    gtk_widget_set_visible(progress_window,true);

    pthread_t thread;
    pthread_create(&thread, NULL, run_convert_files, NULL);
    pthread_detach(thread);
    computed_file_paths=g_list_copy(convert_file_paths);
    
    gtk_widget_set_visible(progress_window,false);
}
void delete_selected_rows() {
    GList *selected_rows = gtk_list_box_get_selected_rows(GTK_LIST_BOX(choosed_files_listbox));

    for (GList *l = selected_rows; l != NULL; l = l->next) {
        GtkListBoxRow *row = GTK_LIST_BOX_ROW(l->data);

        // Hole die gespeicherten Daten
        inFile *file_data =(inFile*) g_object_get_data(G_OBJECT(row), "file-data");

        if (file_data) {
            // Mache etwas mit file_data
            choosed_file_paths = g_list_remove(choosed_file_paths, file_data);
            g_free(file_data);
        }
    }

    // Liste freigeben
    g_list_free(selected_rows);
    refresh_coosedlistbox(choosed_files_listbox,choosed_file_paths);
}

int count_pages(const gchar *filepath) {
    const char *file_type = check_file_type(filepath);
    if (file_type == NULL) {
        return 0; // Fehler beim Bestimmen des Dateityps
    }

    int page_count = 0;

    // Überprüfen, ob der Dateityp PDF oder TIFF ist
    if (strcmp(file_type, "application/pdf") == 0) {
        char *file_url = convert_to_file_url(filepath);
        PopplerDocument *document = poppler_document_new_from_file(file_url, NULL, NULL);
        g_free(file_url); // Speicher freigeben

        if (!document) {
            printf(_("Fehler beim Öffnen der PDF-Datei: %s\n"), filepath);
            free((void*)file_type);
            return 0; // Fehler beim Öffnen
        }

        page_count = poppler_document_get_n_pages(document)-1;
        g_object_unref(document);
    } else if (strcmp(file_type, "image/tiff") == 0 || strcmp(file_type, "image/tif") == 0) {
        TIFF *tiff = TIFFOpen(filepath, "r");
        if (tiff) {
            uint32_t num_pages = 0;
            do {
                num_pages++;
            } while (TIFFReadDirectory(tiff));
            TIFFClose(tiff);
            page_count = num_pages-1;
        } else {
            printf(_("Fehler beim Öffnen der TIFF-Datei: %s\n"), filepath);
        }
    } else {
        page_count = 0; // Ungültiger Typ
    }

    free((void*)file_type); // Speicher freigeben
    return page_count; // Anzahl der Seiten zurückgeben
}
void add_file(const gchar *filepath) {
    inFile *info = g_new(inFile, 1);
    info->filepath=g_strdup(filepath);
    info->selected=true;
    info->pages=count_pages(info->filepath);

    choosed_file_paths = g_list_append(choosed_file_paths,info );

    refresh_coosedlistbox(choosed_files_listbox, choosed_file_paths);
    refresh_computedlistbox(computed_files_listbox, computed_file_paths);
}
void clear_lists() {
    g_list_free_full(choosed_file_paths, g_free);
    g_list_free_full(computed_file_paths, g_free);
    g_list_free_full(convert_file_paths, g_free);
    choosed_file_paths = NULL;
    computed_file_paths = NULL;
    convert_file_paths=NULL;
    refresh_coosedlistbox(choosed_files_listbox, choosed_file_paths);
    refresh_computedlistbox(computed_files_listbox, computed_file_paths);
}

void on_add_file_clicked(GtkButton *button, gpointer user_data) {
    GtkWindow *parent_window = GTK_WINDOW(user_data);
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, _("Datei auswälen"));

    // Create a filter for images, TIFF, and PDF
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("Erlaubte Dateien"));
    gtk_file_filter_add_mime_type(filter, "image/*");
    gtk_file_filter_add_mime_type(filter, "application/pdf");

    GListStore *filter_store = g_list_store_new(G_TYPE_OBJECT);
    g_list_store_append(filter_store, filter);

    // Set the filters for the dialog
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filter_store));


    gtk_file_dialog_open_multiple(dialog, parent_window, NULL,
                                  (GAsyncReadyCallback)+[](GObject *source_object, GAsyncResult *res, gpointer user_data) {
                                      GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
                                      GtkWindow *parent = GTK_WINDOW(user_data);

                                      GListModel *files = gtk_file_dialog_open_multiple_finish(dialog, res, NULL);
                                      if (!files) return;

                                      guint n_files = g_list_model_get_n_items(files);
                                      for (guint i = 0; i < n_files; i++) {
                                          GFile *file = G_FILE(g_list_model_get_item(files, i));
                                          gchar *path = g_file_get_path(file);
                                          if (path) {
                                              // Nur hinzufügen, wenn Datei noch nicht in der Liste ist
                                              if (true) {
                                                  add_file(path);
                                              }
                                              g_free(path);
                                          }
                                          g_object_unref(file);
                                      }

                                      g_object_unref(files);
                                  },
                                  parent_window);


    g_object_unref(dialog);
}

gchar* replace_extension(int page, const gchar *filepath, const gchar *new_ext) {
    gchar *dirname = g_path_get_dirname(filepath);
    gchar *basename = g_path_get_basename(filepath);
    gchar *dot = strrchr(basename, '.');

    if (dot) *dot = '\0';  // Entferne alte Endung

    gchar *new_filename;

    if (page > 0) {
        // Erstelle den neuen Dateinamen mit der Seitenzahl
        new_filename = g_strconcat(basename, "_", g_strdup_printf("%d", page), ".", new_ext, NULL);
    } else {
        // Erstelle den neuen Dateinamen ohne Seitenzahl
        new_filename = g_strconcat(basename, ".", new_ext, NULL);
    }

    gchar *new_path = g_build_filename(dirname, new_filename, NULL);
    g_free(dirname);
    g_free(basename);
    g_free(new_filename);

    return new_path;
}
void convert_checked_files_to(const gchar *new_ext,GtkWidget *window) {
    GList *to_remove = NULL;

    for (GList *l = choosed_file_paths; l != NULL; l = l->next) {
        inFile *inf = (inFile*) l->data;
        if (inf->selected) {
            if(g_strcmp0(new_ext, "tiff") == 0 || g_strcmp0(new_ext, "pdf") == 0){
                // Dateipfad mit neuer Extension
                gchar *new_path = replace_extension(0,inf->filepath, new_ext);

                // outFile erstellen
                outFile *out = g_new(outFile, 1);
                out->outFilePath = new_path;
                out->inFilePath = g_strdup(inf->filepath); // Kopie des alten Pfads
                out->max_pages=inf->pages;
                uint32_t* pages = new uint32_t[inf->pages];
                for(uint32_t i = 0 ; i <= inf->pages;i++){
                    pages[i]=i;
                }
                out->pages=pages;
                convert_file_paths = g_list_append(convert_file_paths, out);
            }
            else {
                for(uint32_t i = 0 ; i <= inf->pages;i++){
                    // Dateipfad mit neuer Extension
                    gchar *new_path = replace_extension(i,inf->filepath, new_ext);

                    // outFile erstellen
                    outFile *out = g_new(outFile, 1);
                    out->outFilePath = new_path;
                    out->inFilePath = g_strdup(inf->filepath); // Kopie des alten Pfads
                    out->max_pages=inf->pages;
                    uint32_t *pages=new uint32_t[1];
                    pages[0]=i;
                    out->pages=pages;
                    convert_file_paths = g_list_append(convert_file_paths, out);
                }
            }
            // Für späteres Entfernen vormerken
            to_remove = g_list_prepend(to_remove, inf);
        }
    }

    convert_files(window);

    refresh_coosedlistbox(choosed_files_listbox, choosed_file_paths);
    refresh_computedlistbox(computed_files_listbox, computed_file_paths);
}


void on_close_request(GtkWidget *widget, gpointer data) {
    save_settings();
}

void on_about_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    GtkWidget *about_dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog), "PicturesConverter");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog), "1");
    //gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog), _("Fast Reader helps you read quickly by displaying only one word at a time"));
    //gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog), "https://github.com/Quantum-mutnauQ/Pictures-converter");
    gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(about_dialog), GTK_LICENSE_GPL_2_0);

#ifdef ICON_NAME
    gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about_dialog), ICON_NAME);
#else
    gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about_dialog), "picturesconverter");
#endif

    const char *authors[] = {
        "Quantum-mutnauQ",
        NULL
    };
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog), authors);

    //const char *translators = "albanobattistella";
    //gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(about_dialog), translators);

    const char *documenters[] = {
        "Quaste42",
        NULL
    };
    gtk_about_dialog_set_documenters(GTK_ABOUT_DIALOG(about_dialog), documenters);

    gtk_window_present(GTK_WINDOW(about_dialog));
}

void toggle_fullscreen(GSimpleAction *action, GVariant *parameter, gpointer window) {
    if (gtk_window_is_fullscreen(GTK_WINDOW(window)))
        gtk_window_unfullscreen(GTK_WINDOW(window));
    else
        gtk_window_fullscreen(GTK_WINDOW(window));
}

void on_quit_activate(GSimpleAction *action, GVariant *parameter, gpointer app) {
    gtk_window_close(GTK_WINDOW(app));
}

GtkWidget *create_menu_bar(GtkApplication *app, GtkWidget *window) {
    // Actions definieren und mit Callbacks verbinden
    GSimpleAction *quit_action = g_simple_action_new("quit", NULL);
    g_signal_connect(quit_action, "activate", G_CALLBACK(on_quit_activate), window);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(quit_action));

    GSimpleAction *about_action = g_simple_action_new("about", NULL);
    g_signal_connect(about_action, "activate", G_CALLBACK(on_about_activate), NULL);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(about_action));

    GSimpleAction *toggle_fullscreen_action = g_simple_action_new("toggle-fullscreen", NULL);
    g_signal_connect(toggle_fullscreen_action, "activate", G_CALLBACK(toggle_fullscreen), window);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(toggle_fullscreen_action));

    GSimpleAction *add_immatge_action = g_simple_action_new("addImage", NULL);
    g_signal_connect(add_immatge_action, "activate", G_CALLBACK(on_add_file_clicked), window);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(add_immatge_action));

    const gchar *accels[] = { "F11", NULL };
    gtk_application_set_accels_for_action(app, "app.toggle-fullscreen", accels);

    const gchar *quit_accels[] = { "<Primary>q", NULL };
    gtk_application_set_accels_for_action(app, "app.quit", quit_accels);

    // Hauptmenü
    GMenu *menu_model = g_menu_new();

    // Datei-Menü
    GMenu *file_menu = g_menu_new();

    g_menu_append(file_menu, _("Bild Hinzufügen"), "app.addImage");
    g_menu_append(file_menu, _("Ausgewälte Entfernen"), "app.remove-seleckted");
    g_menu_append(file_menu, _("Schließen"), "app.quit");

    GMenuItem *file_item = g_menu_item_new_submenu(_("Datei"), G_MENU_MODEL(file_menu));
    g_menu_append_item(menu_model, file_item);

    // Ansicht-Menü
    GMenu *ansicht_menu = g_menu_new();
    g_menu_append(ansicht_menu, _("Vollbild"), "app.toggle-fullscreen");
    GMenuItem *ansicht_item = g_menu_item_new_submenu(_("Ansicht"), G_MENU_MODEL(ansicht_menu));
    g_menu_append_item(menu_model, ansicht_item);

    // Hilfe-Menü
    GMenu *help_menu = g_menu_new();
    g_menu_append(help_menu, _("Über"), "app.about");
    GMenuItem *help_item = g_menu_item_new_submenu(_("Hilfe"), G_MENU_MODEL(help_menu));
    g_menu_append_item(menu_model, help_item);

    // Menüleiste erzeugen
    GtkWidget *menu_bar = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(menu_model));
    return menu_bar;
}

void add_popover_menu_to_listbox(GtkWidget *listbox) {
    GtkGesture *right_click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(right_click), GDK_BUTTON_SECONDARY);

    auto right_click_handler = +[](
                                    GtkGestureClick *gesture,
                                    gint n_press,
                                    gdouble x,
                                    gdouble y,
                                    gpointer user_data) -> void {
        GtkWidget *listbox = GTK_WIDGET(user_data);
        GtkWidget *root = GTK_WIDGET(gtk_widget_get_root(listbox));

        GMenu *menu = g_menu_new();
        g_menu_append(menu, _("Ausgewälte Entfernen"), "app.remove-seleckted");

        // Popover erstellen
        GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
        gtk_popover_set_autohide(GTK_POPOVER(popover), TRUE);
        gtk_widget_set_parent(popover, root);

        // Automatisch zerstören, wenn Popover geschlossen wird
        g_signal_connect(popover, "closed", G_CALLBACK(gtk_widget_unparent), NULL);

        graphene_point_t in_point = GRAPHENE_POINT_INIT((float)x,(float) y);
        graphene_point_t out_point;

        if (!gtk_widget_compute_point(GTK_WIDGET(listbox), GTK_WIDGET(root), &in_point, &out_point)) {
            // Fehler oder fallback
            out_point.x = 0;
            out_point.y = 0;
        }

        // Die Werte in int für GdkRectangle casten
        int px = (int)out_point.x;
        int py = (int)out_point.y;

        GdkRectangle rect = { px, py, 1, 1 };
        gtk_popover_set_pointing_to(GTK_POPOVER(popover), &rect);

        gtk_popover_popup(GTK_POPOVER(popover));
    };

    g_signal_connect(right_click, "pressed",
                     G_CALLBACK(right_click_handler),
                     listbox);

    gtk_widget_add_controller(listbox, GTK_EVENT_CONTROLLER(right_click));
}

void on_activate(GtkApplication *app, gpointer user_data) {
#ifndef ICON_NAME
    GtkIconTheme *icon_theme = gtk_icon_theme_get_for_display(gdk_display_get_default());
    gtk_icon_theme_add_search_path(icon_theme, "assets/");
    gtk_icon_theme_add_resource_path(icon_theme, "assets/");

    if (gtk_icon_theme_has_icon(icon_theme, "picturesconverter")) {
        g_print(_("Icon mit standart Namen gefunden.\n"));
    } else {
        g_print(_("Icon nicht gefunden.\n"));
    }
#else
    g_print(_("ICON_NAME ist definiert: %s\n"), ICON_NAME);
#endif
    GSettings *settings;

#ifdef HANDLE_GSHEMATIG_DIR
    const gchar *schema_dir = "assets/glib-2.0/schemas";
    GError *error = NULL;

    GSettingsSchemaSource *source = g_settings_schema_source_new_from_directory(
        schema_dir,
        NULL,     // kein Parent-Schema-Source nötig
        TRUE,     // trusted, weil lokal kompiliert
        &error
        );

    if (!source) {
        g_printerr("Fehler beim Laden der Schema-Quelle: %s\n", error->message);
        g_error_free(error);
        return;
    }

    GSettingsSchema *schema = g_settings_schema_source_lookup(source, "io.github.quantum_mutnauq.pictures_converter_gtk.State", TRUE);
    if (!schema) {
        g_printerr("Schema nicht gefunden!\n");
        return;
    }
    settings = g_settings_new_full(schema, NULL, NULL);
#else
    settings = g_settings_new ("io.github.quantum_mutnauq.pictures_converter_gtk.State");
#endif
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), _("Pictures Converter"));
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 620);
    if(settings){
        g_settings_bind (settings, "width",
                        window, "default-width",
                        G_SETTINGS_BIND_DEFAULT);
        g_settings_bind (settings, "height",
                        window, "default-height",
                        G_SETTINGS_BIND_DEFAULT);
        g_settings_bind (settings, "is-maximized",
                        window, "maximized",
                        G_SETTINGS_BIND_DEFAULT);
        g_settings_bind (settings, "is-fullscreen",
                        window, "fullscreened",
                        G_SETTINGS_BIND_DEFAULT);
    }

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget *menu_bar = create_menu_bar(gtk_window_get_application(GTK_WINDOW(window)),window);

    gtk_box_append(GTK_BOX(box), menu_bar);

    GtkWidget *add_file_button = gtk_button_new_with_label(_("Füge Dateien hinzu"));
    gtk_box_append(GTK_BOX(box), add_file_button);
    g_signal_connect(add_file_button, "clicked", G_CALLBACK(on_add_file_clicked), window);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    choosed_files_listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(choosed_files_listbox),GTK_SELECTION_MULTIPLE);


    GSimpleAction *remove_seleckted_action = g_simple_action_new("remove-seleckted", NULL);
    g_signal_connect(remove_seleckted_action, "activate", G_CALLBACK(+[](
                                                                          GSimpleAction *action,
                                                                          GVariant *parameter,
                                                                          gpointer user_data) {
                         delete_selected_rows();
                     }), NULL);

    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(remove_seleckted_action));
    const gchar *accels[] = {"<Ctrl>r", NULL };
    gtk_application_set_accels_for_action(app, "app.remove-seleckted", accels);


    gtk_widget_set_hexpand(choosed_files_listbox, TRUE);
    gtk_box_append(GTK_BOX(hbox), choosed_files_listbox);
    g_object_set_data(G_OBJECT(window), "choosed-files", choosed_files_listbox);

    GtkWidget *vtoolBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(hbox), vtoolBox);

    GtkWidget *convert_to_PNG = gtk_button_new_with_label(_("PNG"));
    gtk_box_append(GTK_BOX(vtoolBox), convert_to_PNG);

    GtkWidget *convert_to_JPG = gtk_button_new_with_label(_("JPG"));
    gtk_box_append(GTK_BOX(vtoolBox), convert_to_JPG);

    GtkWidget *convert_to_PDF = gtk_button_new_with_label(_("PDF"));
    gtk_box_append(GTK_BOX(vtoolBox), convert_to_PDF);

    GtkWidget *convert_to_TIFF = gtk_button_new_with_label(_("TIFF"));
    gtk_box_append(GTK_BOX(vtoolBox), convert_to_TIFF);

    g_signal_connect(convert_to_PNG, "clicked", G_CALLBACK(+[] (GtkButton *btn, gpointer data) {
                         convert_checked_files_to("png",GTK_WIDGET(data));
                     }), window);

    g_signal_connect(convert_to_JPG, "clicked", G_CALLBACK(+[] (GtkButton *btn, gpointer data) {
                         convert_checked_files_to("jpg",GTK_WIDGET(data));
                     }), window);

    g_signal_connect(convert_to_PDF, "clicked", G_CALLBACK(+[] (GtkButton *btn, gpointer data) {
                         convert_checked_files_to("pdf",GTK_WIDGET(data));
                     }), window);

    g_signal_connect(convert_to_TIFF, "clicked", G_CALLBACK(+[] (GtkButton *btn, gpointer data) {
                         convert_checked_files_to("tiff",GTK_WIDGET(data));
                     }), window);



    computed_files_listbox = gtk_list_box_new();
    gtk_box_append(GTK_BOX(hbox), computed_files_listbox);
    gtk_widget_set_hexpand(computed_files_listbox, TRUE);
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(computed_files_listbox),GTK_SELECTION_MULTIPLE);

    add_popover_menu_to_listbox(choosed_files_listbox);

    gtk_box_append(GTK_BOX(box), hbox);

    load_settings();

    g_signal_connect(window, "close-request", G_CALLBACK(on_close_request), NULL);

    gtk_window_set_child(GTK_WINDOW(window), box);
    gtk_widget_set_visible(window, TRUE);
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    bindtextdomain("PicturesConverter", LOCALEDIRR);
    textdomain("PicturesConverter");

    const gchar *config_dir = g_get_user_config_dir();
    if (config_dir == NULL) {
        printf(_("Konnte das Standard-Konfigurationsverzeichnis nicht abrufen.\n"));
        return 1;
    }

    gchar *picturesconverter_dir = g_build_filename(config_dir, "PicturesConverter", NULL);
    if (g_mkdir_with_parents(picturesconverter_dir, 0700) != 0) {
        printf(_("Fehler beim Erstellen des Verzeichnisses PicturesConverter.\n"));
        g_free(picturesconverter_dir);
        return 1;
    }

    config_file_path = g_build_filename(picturesconverter_dir, "config.cfg", NULL);
    g_free(picturesconverter_dir);

    GtkApplication *app = gtk_application_new("io.github.quantum_mutnauq.pictures_converter_gtk", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

#ifdef ICON_NAME
    gtk_window_set_default_icon_name(ICON_NAME);
#else
    gtk_window_set_default_icon_name("picturesconverter");
#endif

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    g_free(config_file_path);
    return status;
}
