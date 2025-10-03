# Bilderumwandler
Der Bildkonvert ist ein Programm, mit dem an verschieden Dateiformate mit einer grafischen Oberfläche umwandeln kann. Mit dem Programm können folgende Format in alle angebotenen Formate um wandel werden:
jpg, png, tiff, pdf
Es können aus verschiedenen Verzeichnisse Bilder ausgewählt werden. Durch ein doppelklickt springt man automatisch in das Verzeichnis, in dem das Bild gespeichert ist. Nach der Auswahl des neuen Formates (Schaltflächen in der Mitte) werden die Dateien in das gewünschte Format gewandelt. Sie werden im selben Verzeichnis abgelegt, aus dem sie ausgewählt wurden.
Die tiff und pdf Formate können mehrere Seite beinhalten. Diese werden mit eine nachlaufenden Nummer in einzelne Formate gewandelt.

 ## Screenshots
***Dunkles Thema:***

<img width="721" height="273" alt="Screenshot Dark Theme 1" src="https://github.com/user-attachments/assets/66dc0c1b-af8c-43c2-b6c0-f38bbc7f068b" />


***Helles Thema:***

<img width="721" height="273" alt="Screenshot Light Theme1" src="https://github.com/user-attachments/assets/77c6297c-1e5a-4712-82b1-bdab1d417c6e" />

## Erste Schritte

Es gibt zwei Installationsmethoden, aus denen Sie wählen können:

**1. Installation über [Flatpak auf Flathub](https://flathub.org/apps/io.github.quantum_mutnauq.pictures_converter_gtk)**

[![Bekomme es von Flathub](https://flathub.org/api/badge?svg&locale=de)](https://flathub.org/apps/io.github.quantum_mutnauq.pictures_converter_gtk) 

**2. Aus der Binärdatei ausführen**

Wenn Sie Pictures Converter aus der Binärdatei ausführen möchten, folgen Sie einfach diesen einfachen Schritten:

1. **Installation**: Laden Sie die [neueste Version](https://github.com/Quantum-mutnauQ/Fast_Reader_GTK/releases) herunter und entpacken Sie den Inhalt der Zip-Datei.
2. **Ausführung**: Führen Sie die kompilierte "PicturesConverter"-Executable aus. Sie können sie ausführen, indem Sie `./PicturesConverter` im Ordner eingeben, in dem das Programm gespeichert ist, oder indem Sie einfach doppelt darauf klicken.
3. **Fehlerbehebung**: Sie müssen `libgtk-4-1`, `libconfig9`,`libmagic-mgc`, `libpoppler-glib8t64` und `libtiff6` installieren.

## Selbst kompilieren
### Build-Anweisungen

1. **Code herunterladen**: Beginnen Sie mit dem Herunterladen der neuesten Version des Codes.
2. **Vorbereitung**: Erstellen Sie ein Build-Verzeichnis im Projektordner.
3. **Build-Programm erstellen**: Navigieren Sie zum Build-Verzeichnis und führen Sie den folgenden Befehl aus: `cmake ..`
4. **Build ausführen**: Führen Sie im Build-Verzeichnis den folgenden Befehl aus, um den Code zu kompilieren:
       `make -j[Thread-Anzahl]` Ersetzen Sie `[Thread-Anzahl]` durch die Anzahl der Threads, die Sie für den Build-Prozess nutzen möchten.
