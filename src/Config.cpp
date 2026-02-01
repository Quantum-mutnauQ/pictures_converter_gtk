#include "PicturesConverter.h"
#include <libconfig.h>

gchar *config_file_path = NULL;


const gchar* tagged_directory_to_string(TagedDitrectory dir) {
    switch (dir) {
    case SourceFolder:
        return "SourceFolder";
    case PicturesFolder:
        return "PicturesFolder";
    case AskForFolder:
        return "AskForFolder";
    default:
        return "PicturesFolder";
    }
}

TagedDitrectory string_to_tagged_directory(const gchar* str) {
    if (g_strcmp0(str, "SourceFolder") == 0) {
        return SourceFolder;
    } else if (g_strcmp0(str, "PicturesFolder") == 0) {
        return PicturesFolder;
    } else if (g_strcmp0(str, "AskForFolder") == 0) {
        return AskForFolder;
    } else {
        return PicturesFolder; // Default value
    }
}

void build_config_path(){
    const gchar *config_dir = g_get_user_config_dir();
    if (config_dir == NULL) {
        printf(_("Konnte das Standard-Konfigurationsverzeichnis nicht abrufen.\n"));
        return;
    }

    gchar *picturesconverter_dir = g_build_filename(config_dir, "PicturesConverter", NULL);
    if (g_mkdir_with_parents(picturesconverter_dir, 0700) != 0) {
        printf(_("Fehler beim Erstellen des Verzeichnisses PicturesConverter.\n"));
        g_free(picturesconverter_dir);
        return;
    }

    config_file_path = g_build_filename(picturesconverter_dir, "config.cfg", NULL);
    g_free(picturesconverter_dir);

}
void free_config_path(){
        g_free(config_file_path);
}
void save_settings() {
    config_t cfg;
    config_init(&cfg);

    config_setting_t *root = config_root_setting(&cfg);

    config_setting_t *dir_setting = config_setting_add(root, "DestinationDirectory", CONFIG_TYPE_STRING);
    config_setting_set_string(dir_setting, tagged_directory_to_string(destinationDiretryType));

    int resolution = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(resolution_spin));
    config_setting_t *resolution_setting = config_setting_add(root, "resolution", CONFIG_TYPE_INT);
    config_setting_set_int(resolution_setting, resolution);

    int compression = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(compression_spin));
    config_setting_t *compression_setting = config_setting_add(root, "compression", CONFIG_TYPE_INT);
    config_setting_set_int(compression_setting, compression);

    int dpi = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dpi_spin));
    config_setting_t *dpi_setting = config_setting_add(root, "dpi", CONFIG_TYPE_INT);
    config_setting_set_int(dpi_setting, dpi);

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

    const char *dir_str;
    if (config_lookup_string(&cfg, "DestinationDirectory", &dir_str)) {
        destinationDiretryType = string_to_tagged_directory(dir_str);
    }
    int resolution;
    if (config_lookup_int(&cfg, "resolution", &resolution)) {
        if (resolution > 0) {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(resolution_spin), resolution);
        }
    }

    int compression;
    if (config_lookup_int(&cfg, "compression", &compression)) {
        if (compression > 0) {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(compression_spin), compression);
        }
    }

    int dpi;
    if (config_lookup_int(&cfg, "dpi", &dpi)) {
        if (dpi > 0) {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(dpi_spin), dpi);
        }
    }

    config_destroy(&cfg);
}
