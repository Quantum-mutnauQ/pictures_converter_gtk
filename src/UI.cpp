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


        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale((const gchar*)out_File->outFilePath, 48, 48, TRUE, NULL);
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

        gtk_widget_set_tooltip_text(row, g_strdup_printf(_("Original Datei: %s\nUmgewandelnde Datei: %s"), out_File->inFilePath, out_File->outFilePath));

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

    GtkWidget* progress_window = gtk_application_window_new(gtk_window_get_application(GTK_WINDOW(main_window)));
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


void on_add_file_clicked(GtkButton *button, gpointer user_data) {
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

}

void on_add_file_and_clear_list_clicked(GtkButton *button, gpointer user_data){

    clear_lists();
    on_add_file_clicked(button,user_data);
}

void on_radio_button_SourceFolder_toggled(GtkCheckButton *toggle_button, gpointer user_data) {
    if (gtk_check_button_get_active(toggle_button))
            destinationDiretryType = SourceFolder;
}
void on_radio_button_PicturesFolder_toggled(GtkCheckButton *toggle_button, gpointer user_data) {
    if (gtk_check_button_get_active(toggle_button))
        destinationDiretryType = PicturesFolder;
}
void on_radio_button_AskForFolder_toggled(GtkCheckButton *toggle_button, gpointer user_data) {
    if (gtk_check_button_get_active(toggle_button))
        destinationDiretryType = AskForFolder;
}

void on_settings_clicked(GSimpleAction *action, GVariant *parameter, gpointer main_window) {
    GtkApplication *app = gtk_window_get_application(GTK_WINDOW(main_window));

    GtkWidget *settings_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(settings_window), _("Einstellungen"));
    gtk_window_set_default_size(GTK_WINDOW(settings_window), 400, 300);

    gtk_window_set_modal(GTK_WINDOW(settings_window), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(settings_window), GTK_WINDOW(main_window));
    gtk_widget_set_visible(settings_window, TRUE);

    // Create a vertical box to hold the label and radio buttons
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_window_set_child(GTK_WINDOW(settings_window), vbox);

    // Create a label for the title
    GtkWidget *title_label = gtk_label_new(_("Speicherpfad:"));
    gtk_box_append(GTK_BOX(vbox), title_label);

    // Create a group for the radio buttons
    GtkWidget *radio_button1 = gtk_check_button_new_with_label(_("Ins Ursprungsverzeichnis"));
    GtkWidget *radio_button2 = gtk_check_button_new_with_label(_("In Bilder/Umgewandelt"));
    GtkWidget *radio_button3 = gtk_check_button_new_with_label(_("Immer Fragen"));


#ifdef USE_CUSTE_SOURCE_PICURE_TEXT
    gtk_check_button_set_label(GTK_CHECK_BUTTON(radio_button1),_("Ins Ursprungsverzeichnis (Benötigt \"filesystem=host\" Zugriff)"));
#endif
    // Group the radio buttons
    gtk_check_button_set_group(GTK_CHECK_BUTTON(radio_button2), GTK_CHECK_BUTTON(radio_button1));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(radio_button3), GTK_CHECK_BUTTON(radio_button1));

    g_signal_connect(radio_button1, "toggled", G_CALLBACK(on_radio_button_SourceFolder_toggled), radio_button1);
    g_signal_connect(radio_button2, "toggled", G_CALLBACK(on_radio_button_PicturesFolder_toggled), radio_button2);
    g_signal_connect(radio_button3, "toggled", G_CALLBACK(on_radio_button_AskForFolder_toggled), radio_button3);

    // Set the active radio button based on the selected_directory_type
    switch (destinationDiretryType) {
    case SourceFolder:
        gtk_check_button_set_active(GTK_CHECK_BUTTON(radio_button1), TRUE);
        break;
    case PicturesFolder:
        gtk_check_button_set_active(GTK_CHECK_BUTTON(radio_button2), TRUE);
        break;
    case AskForFolder:
        gtk_check_button_set_active(GTK_CHECK_BUTTON(radio_button3), TRUE);
        break;
    }

    // Add the radio buttons to the vertical box
    gtk_box_append(GTK_BOX(vbox), radio_button1);
    gtk_box_append(GTK_BOX(vbox), radio_button2);
    gtk_box_append(GTK_BOX(vbox), radio_button3);

    // Show all widgets
    gtk_widget_set_visible(settings_window, true);
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

    GSimpleAction *settings_list_action = g_simple_action_new("settings", NULL);
    g_signal_connect(settings_list_action, "activate", G_CALLBACK(on_settings_clicked), window);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(settings_list_action));


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
    g_menu_append(file_menu, _ ("Einstellungen"), "app.settings");
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

    GtkWidget *buttons_start = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_append (GTK_BOX (hbox), buttons_start);

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

    GtkWidget *buttons_end = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_append (GTK_BOX (hbox), buttons_end);


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

    GtkWidget *seperator_quecksettigs = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append (GTK_BOX (box), seperator_quecksettigs);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_start(grid, 6);
    gtk_widget_set_margin_end(grid, 6);
    gtk_widget_set_margin_top(grid, 6);
    gtk_widget_set_margin_bottom(grid, 6);

    /* Überschrift */
    GtkWidget *quicksettings_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(quicksettings_label), _("<b>Schnell einstellungen:</b>"));
    gtk_label_set_xalign(GTK_LABEL(quicksettings_label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), quicksettings_label, 0, 0, 2, 1);

    /* Auflösung skalieren */
    const char* resolution_tooltip= _("Prozentuale Skalierung der Bildauflösung (z. B. 100 = original).");

    GtkWidget *resolution_label = gtk_label_new(_("Auflösung skalieren (%)"));
    gtk_label_set_xalign(GTK_LABEL(resolution_label), 0.0);
    gtk_widget_set_margin_start(resolution_label, 10);
    gtk_grid_attach(GTK_GRID(grid), resolution_label, 0, 1, 1, 1);
    gtk_widget_set_tooltip_text(resolution_label,resolution_tooltip);

    GtkAdjustment *adj_resolution = gtk_adjustment_new(100.0, 1.0, 500.0, 1.0, 10.0, 0.0);
    resolution_spin = gtk_spin_button_new(adj_resolution, 1.0, 0);
    gtk_widget_set_hexpand(resolution_spin, FALSE);
    gtk_grid_attach(GTK_GRID(grid), resolution_spin, 1, 1, 1, 1);
    gtk_widget_set_tooltip_text(resolution_spin,resolution_tooltip);

    /* JPG Komprimierung */
    GtkWidget *JPG_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(JPG_label), _("<b>JPG:</b>"));
    gtk_label_set_xalign(GTK_LABEL(JPG_label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), JPG_label, 0, 2, 1, 1);

    const char* compression_tooltip= _("JPG-Qualität: höher = bessere Qualität, größere Dateien (1–100).");

    GtkWidget *compression_label = gtk_label_new(_("Kompremierung"));
    gtk_label_set_xalign(GTK_LABEL(compression_label), 0.0);
    gtk_widget_set_margin_start(compression_label, 10);
    gtk_grid_attach(GTK_GRID(grid), compression_label, 0, 3, 1, 1);
    gtk_widget_set_tooltip_text(compression_label,compression_tooltip);

    GtkAdjustment *adj_compression = gtk_adjustment_new(75.0, 1.0, 100.0, 1.0, 5.0, 0.0);
    compression_spin = gtk_spin_button_new(adj_compression, 1.0, 0);
    gtk_widget_set_hexpand(compression_spin, FALSE);
    gtk_grid_attach(GTK_GRID(grid), compression_spin, 1, 3, 1, 1);
    gtk_widget_set_tooltip_text(compression_spin,compression_tooltip);

    /* PDF DPI */
    GtkWidget *PDF_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(PDF_label), _("<b>PDF:</b>"));
    gtk_label_set_xalign(GTK_LABEL(PDF_label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), PDF_label, 0, 4, 1, 1);

    const char* dpi_tooltip= _("Scan-Auflösung der PDF in DPI (Dots Per Inch). Höher = schärfer, größere Dateien.");

    GtkWidget *dpi_label = gtk_label_new(_("DPI"));
    gtk_label_set_xalign(GTK_LABEL(dpi_label), 0.0);
    gtk_widget_set_margin_start(dpi_label, 10);
    gtk_grid_attach(GTK_GRID(grid), dpi_label, 0, 5, 1, 1);
    gtk_widget_set_tooltip_text(dpi_label,dpi_tooltip);

    GtkAdjustment *adj_dpi = gtk_adjustment_new(300.0, 72.0, 1200.0, 1.0, 50.0, 0.0);
    dpi_spin = gtk_spin_button_new(adj_dpi, 1.0, 0);
    gtk_widget_set_hexpand(dpi_spin, FALSE);
    gtk_grid_attach(GTK_GRID(grid), dpi_spin, 1, 5, 1, 1);
    gtk_widget_set_tooltip_text(dpi_spin,dpi_tooltip);

    /* Reset-Button unter allem (zentrig links) */
    GtkWidget *reset_button = gtk_button_new_with_label(_("Zurücksetzen"));
    gtk_widget_set_halign(reset_button, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), reset_button, 0, 6, 2, 1);

    g_signal_connect(reset_button, "clicked", G_CALLBACK(+[] (GtkButton *btn, gpointer data) {
                         gtk_spin_button_set_value(GTK_SPIN_BUTTON(resolution_spin),100);
                         gtk_spin_button_set_value(GTK_SPIN_BUTTON(compression_spin),75);
                         gtk_spin_button_set_value(GTK_SPIN_BUTTON(dpi_spin),300);
                     }), NULL);

    gtk_box_append(GTK_BOX(box), grid);


    load_settings();

    g_signal_connect(window, "close-request", G_CALLBACK(on_close_request), NULL);

    gtk_window_set_child(GTK_WINDOW(window), box);
    gtk_widget_set_visible(window, TRUE);
}
