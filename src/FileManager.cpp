#include "PicturesConverter.h"
#include <magic.h>
#include <poppler/glib/poppler.h> // Für PDF
#include <tiffio.h> // Für TIFF


const char* check_file_type(const char *filename) {
    magic_t magic;
    const char *type;

    // Initialisiere die magic-Bibliothek
    magic = magic_open(MAGIC_MIME_TYPE);
    if (magic == NULL) {
        fprintf(stderr, _("Fehler beim Initialisieren von magic.\n"));
        return NULL;
    }

    if (magic_load(magic, NULL) != 0) {
        fprintf(stderr, _("Fehler beim Laden der magic-Daten: %s\n"), magic_error(magic));
        magic_close(magic);
        return NULL;
    }

    // Bestimme den Dateityp
    type = magic_file(magic, filename);
    if (type == NULL) {
        fprintf(stderr, _("Fehler beim Bestimmen des Dateityps: %s\n"), magic_error(magic));
        magic_close(magic);
        return NULL;
    }

    // Dupliziere den String, um sicherzustellen, dass er nicht verändert wird
    char *type_copy = strdup(type);
    if (type_copy == NULL) {
        fprintf(stderr, _("Fehler beim Duplizieren des Dateityps.\n"));
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
gboolean is_multiside_format(const gchar *filename) {
    const gchar *ext = strrchr(filename, '.');
    if (!ext) return FALSE;
    ext++; // skip the '.'
    return g_ascii_strcasecmp(ext, "pdf") == 0 || g_ascii_strcasecmp(ext, "tiff") == 0 || g_ascii_strcasecmp(ext, "tif") == 0;
}
// Hilfsfunktion: Falls Datei existiert, neuen Namen mit _1, _2, ... generieren
gchar *ensure_unique_filename(const gchar *path) {
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
    refresh_coosedlistbox(choosed_file_paths);
    refresh_computedlistbox(computed_file_paths);
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

    refresh_coosedlistbox(choosed_file_paths);
    refresh_computedlistbox(computed_file_paths);
}

