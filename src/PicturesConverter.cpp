#ifdef USE_ADWAITA
#include <adwaita.h>
#endif
#include <pthread.h>
#include "PicturesConverter.h"

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    bindtextdomain("PicturesConverter", LOCALEDIRR);
    textdomain("PicturesConverter");

    build_config_path();

    GtkApplication *app = gtk_application_new("io.github.quantum_mutnauq.pictures_converter_gtk", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

#ifdef ICON_NAME
    gtk_window_set_default_icon_name(ICON_NAME);
#else
    gtk_window_set_default_icon_name("picturesconverter");
#endif

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    free_config_path();
    return status;
}
