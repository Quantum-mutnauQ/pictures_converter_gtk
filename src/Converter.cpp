#include "PicturesConverter.h"
#include <poppler/glib/poppler.h> // Für PDF
#include <tiffio.h> // Für TIFF
#include <cairo-pdf.h>
#include <magic.h>

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

        g_print(_("Output File: %s\n"), convertJobFile->outFilePath);

        for (inJobFileIter = convertJobFile->inJobFiles; inJobFileIter != NULL; inJobFileIter = g_list_next(inJobFileIter)) {
            struct inJobFile *inJobFile = (struct inJobFile *)inJobFileIter->data;

            g_print(_("  Input File: %s\n"), inJobFile->inFilePath);
            g_print(_("    Pages: "));
            for (uint32_t i = 0; i < inJobFile->numPages; i++) {
                g_print("%u ", inJobFile->pages[i]);
            }
            g_print("\n");
        }
    }
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
