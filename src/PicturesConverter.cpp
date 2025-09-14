#include <gtk/gtk.h>
#ifdef USE_ADWAITA
#include <adwaita.h>
#endif
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

#define _(STRING) gettext(STRING)

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
// Handler für Doppelklick auf das Bild
static void on_image_double_click(GtkGestureClick *gesture,
                                  int n_press,
                                  double x,
                                  double y,
                                  gpointer user_data)
{
    if (n_press == 2) { // Doppelclick
        const gchar *filepath = (const gchar *)user_data;
        if (filepath && g_file_test(filepath, G_FILE_TEST_EXISTS)){

            GError *error = NULL;

            /* Nautilus‑AppInfo holen (Standard‑Dateimanager) */
            GAppInfo *app = g_app_info_get_default_for_type ("inode/directory", FALSE);
            if (!app) {
                g_warning (_("Kein Standard‑Dateimanager gefunden"));
                return;
            }

            /* GFile für den Ordner erzeugen */
            GFile *folder = g_file_new_for_path (filepath);
            GList *files = g_list_append (NULL, folder);

            /* Launch */
            if (!g_app_info_launch (app, files, NULL, &error)) {
                g_warning (_("Start fehlgeschlagen: %s"), error->message);
                g_error_free (error);
            }

            /* Aufräumen */
            g_object_unref (folder);
            g_list_free (files);
            g_object_unref (app);
        }
    }
}


void refresh_coosedlistbox(GtkWidget *listbox, GList *paths) {
    gtk_list_box_remove_all(GTK_LIST_BOX(listbox));

    for (GList *l = paths; l != NULL; l = l->next) {
        inFile* infile = ((inFile*) l->data);
        const gchar *filepath = (const gchar*)infile->filepath;

        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

        gchar *filepath_copy = g_strdup(filepath);

        GtkGestureClick *img_click = GTK_GESTURE_CLICK(gtk_gesture_click_new());
        gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(img_click), GDK_BUTTON_PRIMARY);

        // Daten-Ownership an GTK übergeben → wird mit g_free freigegeben
        g_signal_connect_data(img_click, "pressed",
                              G_CALLBACK(on_image_double_click),
                              filepath_copy,
                              (GClosureNotify)g_free,
                              (GConnectFlags)0);

        gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(img_click));


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

        gchar *filepath_copy = g_strdup(filepath);

        GtkGestureClick *img_click = GTK_GESTURE_CLICK(gtk_gesture_click_new());
        gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(img_click), GDK_BUTTON_PRIMARY);

        // Daten-Ownership an GTK übergeben → wird mit g_free freigegeben
        g_signal_connect_data(img_click, "pressed",
                              G_CALLBACK(on_image_double_click),
                              filepath_copy,
                              (GClosureNotify)g_free,
                              (GConnectFlags)0);

        gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(img_click));


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
GList *group_convert_jobs(GList *outFiles) {
    GList *result = NULL;
    GHashTable *grouped = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_list_free);

    // Gruppierung der outFile-Einträge
    GList *outFileIter;
    for (outFileIter = outFiles; outFileIter != NULL; outFileIter = g_list_next(outFileIter)) {
        struct outFile *outFile = (struct outFile *)outFileIter->data;
        GList **group = (GList **)g_hash_table_lookup(grouped, outFile->outFilePath);

        if (group == NULL) {
            group = g_new0(GList *, 1);
            *group = NULL;
            g_hash_table_insert(grouped, g_strdup(outFile->outFilePath), group);
        }

        *group = g_list_append(*group, outFile);
    }

    // Verarbeitung jeder Gruppe
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, grouped);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        GList *group = *(GList **)value;
        gboolean multiside = is_multiside_format(((struct outFile *)group->data)->outFilePath);

        if (multiside) {
            // Mehrseitiges Format
            struct convertJobFile *convertJobFile = g_new0(struct convertJobFile, 1);
            convertJobFile->outFilePath = g_strdup((const gchar *)key);
            convertJobFile->inJobFiles = NULL;

            GList *inJobFileIter;
            for (inJobFileIter = group; inJobFileIter != NULL; inJobFileIter = g_list_next(inJobFileIter)) {
                struct outFile *outFile = (struct outFile *)inJobFileIter->data;
                struct inJobFile *inJobFile = g_new0(struct inJobFile, 1);
                inJobFile->inFilePath = outFile->inFilePath;
                inJobFile->pages = outFile->pages;
                inJobFile->numPages=outFile->numPages;
                convertJobFile->inJobFiles = g_list_append(convertJobFile->inJobFiles, inJobFile);
            }

            result = g_list_append(result, convertJobFile);
        } else {
            // Einseitiges Format
            GList *inJobFileIter;
            for (inJobFileIter = group; inJobFileIter != NULL; inJobFileIter = g_list_next(inJobFileIter)) {
                struct outFile *outFile = (struct outFile *)inJobFileIter->data;
                struct convertJobFile *convertJobFile = g_new0(struct convertJobFile, 1);
                convertJobFile->outFilePath = g_strdup((const gchar *)key);
                convertJobFile->inJobFiles = NULL;

                struct inJobFile *inJobFile = g_new0(struct inJobFile, 1);
                inJobFile->inFilePath = outFile->inFilePath;
                inJobFile->pages = outFile->pages;
                inJobFile->numPages=outFile->numPages;
                convertJobFile->inJobFiles = g_list_append(convertJobFile->inJobFiles, inJobFile);

                result = g_list_append(result, convertJobFile);
            }
        }
    }

    // Aufräumen
    g_hash_table_destroy(grouped);

    return result;
}
void free_convert_jobs(GList *convertJobFiles) {
    GList *convertJobFileIter;
    for (convertJobFileIter = convertJobFiles; convertJobFileIter != NULL; convertJobFileIter = g_list_next(convertJobFileIter)) {
        struct convertJobFile *convertJobFile = (struct convertJobFile *)convertJobFileIter->data;

        // Freigeben der inJobFiles
        GList *inJobFileIter;
        for (inJobFileIter = convertJobFile->inJobFiles; inJobFileIter != NULL; inJobFileIter = g_list_next(inJobFileIter)) {
            struct inJobFile *inJobFile = (struct inJobFile *)inJobFileIter->data;
            g_free((void *)inJobFile->pages); // Freigeben des Seitenarrays
            g_free(inJobFile); // Freigeben der inJobFile-Struktur
        }
        g_list_free(convertJobFile->inJobFiles); // Freigeben der GList der inJobFiles

        // Freigeben des ausgabepfades
        g_free((void *)convertJobFile->outFilePath); // Freigeben des ausgabepfades
        g_free(convertJobFile); // Freigeben der convertJobFile-Struktur
    }
    g_list_free(convertJobFiles); // Freigeben der GList der convertJobFiles
}
// Funktion zur Ausgabe der Übersicht
void printJobOverview(GList *convertJobFiles) {
    GList *convertJobFileIter;
    GList *inJobFileIter;

    for (convertJobFileIter = convertJobFiles; convertJobFileIter != NULL; convertJobFileIter = g_list_next(convertJobFileIter)) {
        struct convertJobFile *convertJobFile = (struct convertJobFile *)convertJobFileIter->data;

        g_print("Output File: %s\n", convertJobFile->outFilePath);

        for (inJobFileIter = convertJobFile->inJobFiles; inJobFileIter != NULL; inJobFileIter = g_list_next(inJobFileIter)) {
            struct inJobFile *inJobFile = (struct inJobFile *)inJobFileIter->data;

            g_print("  Input File: %s\n", inJobFile->inFilePath);
            g_print("    Pages: ");
            for (uint32_t i = 0; i < inJobFile->numPages; i++) {
                g_print("%u ", inJobFile->pages[i]);
            }
            g_print("\n");
        }
    }
}
// Hilfsfunktion: Falls Datei existiert, neuen Namen mit _1, _2, ... generieren
static gchar *ensure_unique_filename(const gchar *path) {
    gchar *dir = g_path_get_dirname(path);
    gchar *base = g_path_get_basename(path);
    gchar *ext = strrchr(base, '.');
    gchar *name_part;
    if (ext) {
        name_part = g_strndup(base, ext - base);
        ext = g_strdup(ext);
    } else {
        name_part = g_strdup(base);
        ext = g_strdup("");
    }

    gchar *new_path = g_build_filename(dir, base, NULL);
    int counter = 1;
    while (g_file_test(new_path, G_FILE_TEST_EXISTS)) {
        g_free(new_path);
        gchar *new_name = g_strdup_printf("%s_%d%s", name_part, counter++, ext);
        new_path = g_build_filename(dir, new_name, NULL);
        g_free(new_name);
    }

    g_free(dir);
    g_free(base);
    g_free(name_part);
    g_free(ext);

    return new_path;
}

// PDF-Seiten als Cairo-Oberfläche rendern
static gboolean render_pdf_page_to_surface(const gchar *pdf_path, guint page_num, double dpi, cairo_surface_t **out_surface) {
    GError *error = NULL;
    gchar *uri = g_strdup_printf("file://%s", pdf_path);
    PopplerDocument *doc = poppler_document_new_from_file(uri, NULL, &error);
    g_free(uri);

    if (!doc) {
        if (error) g_error_free(error);
        return FALSE;
    }

    PopplerPage *page = poppler_document_get_page(doc, page_num);
    if (!page) {
        g_object_unref(doc);
        return FALSE;
    }

    double width_points, height_points;
    poppler_page_get_size(page, &width_points, &height_points);

    // 72 dpi ist Standard in PDF (1 Punkt = 1/72 Zoll)
    double scale = dpi / 72.0;
    int width_px  = (int)(width_points  * scale + 0.5);
    int height_px = (int)(height_points * scale + 0.5);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width_px, height_px);
    cairo_t *cr = cairo_create(surface);

    // Hintergrund auf Weiß setzen
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // Weiß
    cairo_paint(cr);

    // Skalierung setzen, damit Poppler in höherer Auflösung rendert
    cairo_scale(cr, scale, scale);
    poppler_page_render(page, cr);

    cairo_destroy(cr);

    *out_surface = surface;

    g_object_unref(page);
    g_object_unref(doc);
    return TRUE;
}

// Hauptkonvertierungsfunktion
GList *process_convert_jobs(GList *convert_job_list,UIInfo* info) {
    GList *failed_files = NULL;
    magic_t magic_cookie = magic_open(MAGIC_MIME_TYPE);
    magic_load(magic_cookie, NULL);

    GtkLabel* label = info->label;
    GtkProgressBar* progressBar = info->bar;
    int total_jobs = g_list_length(convert_job_list);
    int processed_jobs = 0;

    for (GList *job_iter = convert_job_list; job_iter != NULL; job_iter = job_iter->next) {
        struct convertJobFile *job = (struct convertJobFile *)job_iter->data;
        gchar *final_out_path = ensure_unique_filename(job->outFilePath);
        gtk_label_set_text(label, final_out_path);
        processed_jobs++;
        gtk_progress_bar_set_fraction(progressBar,(double)processed_jobs / (double)total_jobs);


        gboolean job_success = TRUE;

        // Multiside-Handling abhängig vom Zieltyp
        const char *mime_type = magic_file(magic_cookie, job->outFilePath);
        gboolean is_tiff_out = g_str_has_suffix(final_out_path, ".tiff") || g_str_has_suffix(final_out_path, ".tif");
        gboolean is_pdf_out = g_str_has_suffix(final_out_path, ".pdf");

        TIFF *tiff_out = NULL;
        cairo_surface_t *pdf_surface = NULL;
        cairo_t *pdf_cr = NULL;
        if (is_tiff_out) {
            tiff_out = TIFFOpen(final_out_path, "w8");
            if (!tiff_out) {
                job_success = FALSE;
            }
        } else if (is_pdf_out) {
            pdf_surface = cairo_pdf_surface_create(final_out_path, 595, 842); // A4 Standardgröße
            pdf_cr = cairo_create(pdf_surface);
        }

        for (GList *in_iter = job->inJobFiles; in_iter != NULL && job_success; in_iter = in_iter->next) {
            struct inJobFile *infile = (struct inJobFile *)in_iter->data;
            const char *in_mime = magic_file(magic_cookie, infile->inFilePath);

            for (uint32_t i = 0; i < infile->numPages && job_success; i++) {
                cairo_surface_t *surf = NULL;

                // Quelle behandeln
                if (g_str_has_prefix(in_mime, "application/pdf")) {
                    if (!render_pdf_page_to_surface(infile->inFilePath, infile->pages[i], 300.0, &surf)) {
                        job_success = FALSE;
                        break;
                    }
                } else if (g_str_has_prefix(in_mime, "image/tiff")) {
                    TIFF* tif = TIFFOpen(infile->inFilePath, "r");
                    if (!tif || !TIFFSetDirectory(tif, infile->pages[i])) {
                        if (tif) TIFFClose(tif);
                        job_success = FALSE;
                        break;
                    }

                    uint32_t w, h;
                    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
                    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);

                    uint32_t* raster = (uint32_t*) _TIFFmalloc(w * h * sizeof(uint32_t));
                    if (!raster || !TIFFReadRGBAImageOriented(tif, w, h, raster, ORIENTATION_TOPLEFT, 0)) {
                        if (raster) _TIFFfree(raster);
                        TIFFClose(tif);
                        job_success = FALSE;
                        break;
                    }

                    // wichtig: KEINE neue Variable!
                    surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
                    unsigned char* data = cairo_image_surface_get_data(surf);
                    int stride = cairo_image_surface_get_stride(surf);

                    for (uint32_t y = 0; y < h; y++) {
                        uint32_t* src = raster + y * w;
                        uint32_t* dst = (uint32_t*)(data + y * stride);
                        for (uint32_t x = 0; x < w; x++) {
                            uint32_t px = src[x]; // 0xAARRGGBB (von libtiff als BGRA geliefert)
                            uint8_t r = TIFFGetR(px);
                            uint8_t g = TIFFGetG(px);
                            uint8_t b = TIFFGetB(px);
                            uint8_t a = TIFFGetA(px);

                            // ARGB32 premultiplied
                            if (a < 255) {
                                r = (r * a) / 255;
                                g = (g * a) / 255;
                                b = (b * a) / 255;
                            }

                            dst[x] = (a << 24) | (r << 16) | (g << 8) | b;
                        }
                    }

                    cairo_surface_mark_dirty(surf);
                    _TIFFfree(raster);
                    TIFFClose(tif);
                } else if (g_str_has_prefix(in_mime, "image/")) {
                    surf = cairo_image_surface_create_from_png(infile->inFilePath);
                    if (cairo_surface_status(surf) != CAIRO_STATUS_SUCCESS) {
                        job_success = FALSE;
                        break;
                    }
                } else {
                    job_success = FALSE;
                    break;
                }

                if (is_pdf_out) {
                    /* Bildgröße ermitteln */
                    int w = cairo_image_surface_get_width(surf);
                    int h = cairo_image_surface_get_height(surf);
                    cairo_pdf_surface_set_size(pdf_surface,w,h);
                }


                // Ziel behandeln
                if (is_tiff_out) {
                    int w = cairo_image_surface_get_width(surf);
                    int h = cairo_image_surface_get_height(surf);
                    unsigned char *data = cairo_image_surface_get_data(surf);
                    int stride = cairo_image_surface_get_stride(surf);

                    // Zuerst TIFF-Tags setzen
                    TIFFSetField(tiff_out, TIFFTAG_IMAGEWIDTH, w);
                    TIFFSetField(tiff_out, TIFFTAG_IMAGELENGTH, h);
                    TIFFSetField(tiff_out, TIFFTAG_SAMPLESPERPIXEL, 4);
                    TIFFSetField(tiff_out, TIFFTAG_BITSPERSAMPLE, 8);
                    TIFFSetField(tiff_out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
                    TIFFSetField(tiff_out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
                    TIFFSetField(tiff_out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
                    TIFFSetField(tiff_out, TIFFTAG_COMPRESSION, COMPRESSION_LZW);

                    // Puffer für eine Zeile
                    guchar *rowbuf = g_new(guchar, w * 4);

                    for (int row = 0; row < h; row++) {
                        uint32_t *src = (uint32_t *)(data + row * stride);
                        for (int col = 0; col < w; col++) {
                            uint32_t px = src[col];
                            guchar a = (px >> 24) & 0xFF;
                            guchar r = (px >> 16) & 0xFF;
                            guchar g = (px >> 8) & 0xFF;
                            guchar b = (px) & 0xFF;

                            // Premultiplied Alpha rückgängig machen
                            if (a > 0 && a < 255) {
                                r = (r * 255) / a;
                                g = (g * 255) / a;
                                b = (b * 255) / a;
                            }

                            rowbuf[col * 4 + 0] = r;
                            rowbuf[col * 4 + 1] = g;
                            rowbuf[col * 4 + 2] = b;
                            rowbuf[col * 4 + 3] = a; // Alpha ins TIFF schreiben
                        }

                        if (TIFFWriteScanline(tiff_out, rowbuf, row, 0) < 0) {
                            job_success = FALSE;
                            break;
                        }
                    }

                    g_free(rowbuf);
                    TIFFWriteDirectory(tiff_out);
                } else if (is_pdf_out) {
                    cairo_set_source_surface(pdf_cr, surf, 0, 0);
                    cairo_paint(pdf_cr);
                    cairo_show_page(pdf_cr);
                } else {
                    cairo_surface_write_to_png(surf, final_out_path);
                }

                cairo_surface_destroy(surf);
            }
        }

        // Ausgabedateien schließen
        if (tiff_out) TIFFClose(tiff_out);
        if (pdf_cr) cairo_destroy(pdf_cr);
        if (pdf_surface) cairo_surface_destroy(pdf_surface);

        if (!job_success) {
            failed_files = g_list_append(failed_files, g_strdup(final_out_path));
        }

        g_free(final_out_path);
    }

    magic_close(magic_cookie);
    return failed_files;
}
void free_out_file(gpointer data) {
    convertJobFile *file = (convertJobFile*) data;
    //g_free((gpointer)file->outFilePath);
    //g_list_free_full(file->inJobFiles,free_inJobFile);
}
void* run_convert_files(void* data){

    GList *convert_job_list = group_convert_jobs(convert_file_paths);
    //printJobOverview(convert_job_list);
    GList *failed = process_convert_jobs(convert_job_list,(UIInfo*)data);
    if (failed) {
        // Fehlerliste ausgeben oder verarbeiten
        g_list_free_full(failed, g_free);
    }
    free_convert_jobs(convert_job_list);


    g_list_free_full(convert_file_paths, free_out_file);
    convert_file_paths = NULL;

    gtk_widget_set_visible(GTK_WIDGET(((UIInfo*)data)->window),false);
    return NULL;
}

void convert_files(GtkWidget* main_window) {
    if (!convert_file_paths) {
        g_print(_("Keine Dateien zum Konvertieren vorhanden.\n"));
        return;
    }

    GtkWidget* progress_window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(progress_window), _("Umwandlungsprozess"));
    gtk_window_set_default_size(GTK_WINDOW(progress_window), 100, 100);
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_window_set_child(GTK_WINDOW(progress_window), vbox);

    GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_widget_set_vexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(vbox), spacer);
    // Progress Bar
    GtkWidget* progressBar = gtk_progress_bar_new();
    gtk_box_append(GTK_BOX(vbox), progressBar);

    // Spacer
    GtkWidget* spacer2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(spacer2, TRUE);
    gtk_widget_set_vexpand(spacer2, TRUE);
    gtk_box_append(GTK_BOX(vbox), spacer2);

    // Label
    GtkWidget* label = gtk_label_new(_("hi"));
    gtk_box_append(GTK_BOX(vbox), label);

    // Spacer
    GtkWidget* spacer3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(spacer3, TRUE);
    gtk_widget_set_vexpand(spacer3, TRUE);
    gtk_box_append(GTK_BOX(vbox), spacer3);

    UIInfo *ui = g_new0(UIInfo, 1);
    ui->bar = GTK_PROGRESS_BAR(progressBar);
    ui->label = GTK_LABEL(label);
    ui->window = GTK_WINDOW(progress_window);

    // Wichtig: Blockiere Interaktion mit dem Hauptfenster
    gtk_window_set_modal(GTK_WINDOW(progress_window), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(progress_window), GTK_WINDOW(main_window));
    gtk_widget_set_visible(progress_window, TRUE);

    gtk_window_set_deletable(GTK_WINDOW(progress_window), FALSE);

    
    pthread_t thread;
    pthread_create(&thread, NULL, run_convert_files, ui);
    pthread_detach(thread);
    computed_file_paths=g_list_copy(convert_file_paths);
    

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
                                      refresh_coosedlistbox(choosed_files_listbox, choosed_file_paths);
                                      refresh_computedlistbox(computed_files_listbox, computed_file_paths);

                                      g_object_unref(files);
                                  },
                                  parent_window);


    g_object_unref(dialog);
}

void on_add_file_and_clear_list_clicked(GtkButton *button, gpointer user_data){

    clear_lists();
    on_add_file_clicked(button,user_data);
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
                out->numPages=0;
                for(uint32_t i = 0 ; i <= inf->pages;i++){
                    pages[i]=i;
                    out->numPages=out->numPages+1;
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
                    out->numPages=1;
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

    GSimpleAction *add_imatge_and_clear_list_action = g_simple_action_new("addImageAndClearList", NULL);
    g_signal_connect(add_imatge_and_clear_list_action, "activate", G_CALLBACK(on_add_file_and_clear_list_clicked), window);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(add_imatge_and_clear_list_action));


    const gchar *accels[] = { "F11", NULL };
    gtk_application_set_accels_for_action(app, "app.toggle-fullscreen", accels);

    const gchar *quit_accels[] = { "<Primary>q", NULL };
    gtk_application_set_accels_for_action(app, "app.quit", quit_accels);

    // Hauptmenü
    GMenu *menu_model = g_menu_new();

    // Datei-Menü
    GMenu *file_menu = g_menu_new();

    g_menu_append(file_menu, _("Bild Hinzufügen"), "app.addImage");
    g_menu_append(file_menu, _("Bild Hinzufügen und Liste löschen"), "app.addImageAndClearList");
    g_menu_append(file_menu, _("Ausgewälte Entfernen"), "app.remove-seleckted");
    g_menu_append(file_menu, _("Alle Entfernen"), "app.remove-all");
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
        g_menu_append(menu, _("Alle Entfernen"), "app.remove-all");

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

#ifdef USE_ADWAITA
    AdwStyleManager* styleManager =adw_style_manager_get_default();
    adw_style_manager_set_color_scheme(styleManager,ADW_COLOR_SCHEME_DEFAULT);
#endif

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget *menu_bar = create_menu_bar(gtk_window_get_application(GTK_WINDOW(window)),window);

    gtk_box_append(GTK_BOX(box), menu_bar);

    GtkWidget *add_file_button = gtk_button_new_with_label(_("Füge Dateien hinzu"));
    gtk_box_append(GTK_BOX(box), add_file_button);
    g_signal_connect(add_file_button, "clicked", G_CALLBACK(on_add_file_clicked), window);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_vexpand(hbox, TRUE);

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

    const gchar *accels[] = {"BackSpace", "Delete", NULL };
    gtk_application_set_accels_for_action(app, "app.remove-seleckted", accels);

    GSimpleAction *remove_all_action = g_simple_action_new("remove-all", NULL);
    g_signal_connect(remove_all_action, "activate", G_CALLBACK(+[](
                                                                          GSimpleAction *action,
                                                                          GVariant *parameter,
                                                                          gpointer user_data) {
                         clear_lists();
                         refresh_coosedlistbox(choosed_files_listbox, choosed_file_paths);
                         refresh_computedlistbox(computed_files_listbox, computed_file_paths);
                     }), NULL);

    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(remove_all_action));

    const gchar *accels_remove_all[] = {"<Control>BackSpace", "<Control>Delete", NULL };
    gtk_application_set_accels_for_action(app, "app.remove-all", accels_remove_all);

    gtk_widget_set_hexpand(choosed_files_listbox, TRUE);
    GtkWidget* choosed_files_listbox_scroller = gtk_scrolled_window_new ();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(choosed_files_listbox_scroller),choosed_files_listbox);
    gtk_box_append(GTK_BOX(hbox), choosed_files_listbox_scroller);
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
    GtkWidget* computed_files_listbox_scroller = gtk_scrolled_window_new ();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(computed_files_listbox_scroller),computed_files_listbox);
    gtk_box_append(GTK_BOX(hbox), computed_files_listbox_scroller);
    gtk_widget_set_hexpand(computed_files_listbox, TRUE);
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(computed_files_listbox),GTK_SELECTION_MULTIPLE);

    add_popover_menu_to_listbox(choosed_files_listbox);


    GtkWidget *line_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand (line_box, TRUE);

    GtkWidget *spacer_left = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand (spacer_left, TRUE);
    gtk_box_append (GTK_BOX (line_box), spacer_left);

    GtkWidget *from_label = gtk_label_new (_("von"));
    gtk_widget_set_halign (from_label, GTK_ALIGN_START);
    gtk_box_append (GTK_BOX (line_box), from_label);

    GtkWidget *spacer_mid = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand (spacer_mid, TRUE);
    gtk_box_append (GTK_BOX (line_box), spacer_mid);

    GtkWidget *to_label = gtk_label_new (_("zu"));
    gtk_widget_set_halign (to_label, GTK_ALIGN_END);
    gtk_box_append (GTK_BOX (line_box), to_label);


    GtkWidget *spacer_right = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand (spacer_right, TRUE);
    gtk_box_append (GTK_BOX (line_box), spacer_right);

    gtk_box_append (GTK_BOX (box), line_box);

    gtk_box_append (GTK_BOX (box), hbox);
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
