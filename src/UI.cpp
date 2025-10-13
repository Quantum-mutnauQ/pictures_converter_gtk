#include "PicturesConverter.h"
#ifdef USE_ADWAITA
#include <adwaita.h>
#endif
GMainLoop *loop = NULL;

GtkWidget *choosed_files_listbox;
GtkWidget *computed_files_listbox;

void on_check_button_toggled(GtkCheckButton *toggle_button, gpointer user_data) {
    gboolean active = gtk_check_button_get_active(toggle_button);
    ((inFile*) user_data)->selected =active;
}

// Handler für Doppelklick auf das Bild
static void on_image_double_click(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data){
    if (n_press == 2) { // Doppelclick
        const gchar *filepath = (const gchar *)user_data;
        if (filepath && g_file_test(filepath, G_FILE_TEST_EXISTS)){
            GFile* file = g_file_new_for_path(filepath);
            GtkFileLauncher* file_launcher = gtk_file_launcher_new(file);

            gtk_file_launcher_open_containing_folder(file_launcher, NULL, NULL, NULL, file);

            g_object_unref(file_launcher);
            g_object_unref(file);
        }
    }
}
void refresh_coosedlistbox(GList *paths) {
    gtk_list_box_remove_all(GTK_LIST_BOX(choosed_files_listbox));

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


        gtk_list_box_append(GTK_LIST_BOX(choosed_files_listbox), list_row);
    }
}

void refresh_computedlistbox(GList *paths) {
    gtk_list_box_remove_all(GTK_LIST_BOX(computed_files_listbox));

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


        gtk_list_box_append(GTK_LIST_BOX(computed_files_listbox), list_row);

    }
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
    refresh_coosedlistbox(choosed_file_paths);
}

void on_filechooser_response(GDBusConnection *conn,
                             const gchar *sender_name,
                             const gchar *object_path,
                             const gchar *interface_name,
                             const gchar *signal_name,
                             GVariant *parameters,
                             gpointer user_data) {
    guint32 response_code;
    GVariant *results;

    g_variant_get(parameters, "(u@a{sv})", &response_code, &results);

    if (response_code == 0) {
        // Erfolgreiche Auswahl
        GVariant *uris = g_variant_lookup_value(results, "uris", G_VARIANT_TYPE_STRING_ARRAY);
        if (uris) {
            gsize n_uris;
            const gchar **uri_list = g_variant_get_strv(uris, &n_uris);
            for (gsize i = 0; i < n_uris; i++) {
                g_print("Ausgewählte Datei: %s\n", uri_list[i]);

                GFile *file = g_file_new_for_uri(uri_list[i]);
                GFile *parent = g_file_get_parent(file);

                if (parent) {
                    gchar *parent_uri = g_file_get_uri(parent);
                    g_print("Übergeordneter Ordner: %s\n", parent_uri);

                    // 👉 Du brauchst KEIN D-Bus hier:
                    // Du kannst direkt alle Dateien im Ordner auflisten:
                    GFileEnumerator *enumerator = g_file_enumerate_children(
                        parent,
                        "*",
                        G_FILE_QUERY_INFO_NONE,
                        NULL,
                        NULL
                        );

                    while (TRUE) {
                        GFileInfo *info = g_file_enumerator_next_file(enumerator, NULL, NULL);
                        if (!info)
                            break;
                        g_print("Datei im Ordner: %s\n", g_file_info_get_name(info));
                        g_object_unref(info);
                    }

                    g_object_unref(enumerator);
                    g_free(parent_uri);
                    g_object_unref(parent);
                }

                g_object_unref(file);
            }
            g_free(uri_list);
            g_variant_unref(uris);
        }
    } else {
        g_print("Dateiauswahl wurde abgebrochen oder ist fehlgeschlagen.\n");
    }

    g_variant_unref(results);
    if (loop)
        g_main_loop_quit(loop);
}

void on_add_file_clicked(GtkButton *button, gpointer user_data) {

    /*
    GtkWindow *parent_window = GTK_WINDOW(user_data);
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, _("Datei auswälen"));
    gtk_file_dialog_set_modal(dialog,true);

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
                                      refresh_coosedlistbox(choosed_file_paths);
                                      refresh_computedlistbox(computed_file_paths);

                                      g_object_unref(files);
                                  },
                                  NULL);


    g_object_unref(dialog);
*/
    GDBusConnection *connection;
    GError *error = NULL;
    GVariant *response = NULL;
    gchar *handle_token = NULL;
    gchar *object_path = NULL;

    connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!connection) {
        g_printerr("Fehler beim Verbinden mit D-Bus: %s\n", error->message);
        g_error_free(error);
        return;
    }

    handle_token = g_strdup_printf("filechooser%d", g_random_int());
    object_path = g_strdup_printf("/org/freedesktop/portal/desktop/request/%s/%s",
                                  g_get_user_name(), handle_token);

    // Optionen vorbereiten
    GVariantBuilder options;
    g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&options, "{sv}", "multiple", g_variant_new_boolean(TRUE));
    g_variant_builder_add(&options, "{sv}", "use-filechooser", g_variant_new_boolean(FALSE));

    // D-Bus-Aufruf: FileChooser.OpenFile
    response = g_dbus_connection_call_sync(
        connection,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.FileChooser",
        "OpenFile",
        g_variant_new("(ssa{sv})",
                      "", // parent_window leer lassen
                      "Datei auswählen",
                      &options),
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
        );

    if (!response) {
        g_printerr("Fehler beim Öffnen des FileChoosers: %s\n", error->message);
        g_error_free(error);
        g_free(handle_token);
        g_free(object_path);
        g_object_unref(connection);
    }
    g_variant_unref(response);

    // Signal abonnieren
    guint signal_id = g_dbus_connection_signal_subscribe(
        connection,
        "org.freedesktop.portal.Desktop",
        "org.freedesktop.portal.Request",
        "Response",
        NULL,
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_filechooser_response,
        NULL,
        NULL
        );

    loop = g_main_loop_new(NULL, FALSE);
    g_print("Bitte Datei auswählen…\n");
    g_main_loop_run(loop);

    g_dbus_connection_signal_unsubscribe(connection, signal_id);
    g_main_loop_unref(loop);


    g_free(handle_token);
    g_free(object_path);
    g_object_unref(connection);
}

void on_add_file_and_clear_list_clicked(GtkButton *button, gpointer user_data){

    clear_lists();
    on_add_file_clicked(button,user_data);
}
void on_close_request(GtkWidget *widget, gpointer data) {
    save_settings();
}

void on_about_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    GtkWidget *about_dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog), "PicturesConverter");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog), PICTURES_CONVERTER_VERSION);
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog), _("Converting image formats."));
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog), "https://github.com/Quantum-mutnauQ/pictures_converter_gtk");
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
                         refresh_coosedlistbox(choosed_file_paths);
                         refresh_computedlistbox(computed_file_paths);
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
