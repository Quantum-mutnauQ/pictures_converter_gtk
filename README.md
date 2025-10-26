# Pictures-converter
The image converter is a program that allows users to convert various file formats through a graphical interface. With this program, the following formats can be converted into all supported formats: 
JPG, PNG, TIFF, and PDF
Users can select images from different directories. By double-clicking, the program automatically navigates to the directory where the image is stored. After selecting the new format (using the buttons in the middle), the files are converted into the desired format. They are saved in the same directory from which they were selected.
The TIFF and PDF formats can contain multiple pages. These pages are converted into separate files with sequential numbering.
In the settings function, users can choose whether images are always saved in the "Pictures/Converted" directory, the original directory from which they came (note that the Flatpak version requires elevated permissions for this), or if the program should always prompt for the desired save location.


 ## Screenshots
***Dark Theme:***

<img width="721" height="273" alt="Screenshot Dark Theme 1" src="https://github.com/user-attachments/assets/66dc0c1b-af8c-43c2-b6c0-f38bbc7f068b" />


***Light Theme:***

<img width="721" height="273" alt="Screenshot Light Theme1" src="https://github.com/user-attachments/assets/77c6297c-1e5a-4712-82b1-bdab1d417c6e" />

## Getting Started

There are two installation methods available for you to choose from:

**1. Install via [Flatpak on Flathub](https://flathub.org/apps/io.github.quantum_mutnauq.pictures_converter_gtk)**

[![Get it from flathub](https://flathub.org/api/badge?svg&locale=en)](https://flathub.org/apps/io.github.quantum_mutnauq.pictures_converter_gtk) 

**2. Run from the Binary**

If you prefer to run Pictures Converter from the binary, just follow these simple steps:

1. **Installation**: Download the [latest release](https://github.com/Quantum-mutnauQ/pictures_converter_gtk/releases) and extract the contents from the zip file.
2. **Execution**: Run the compiled "PicturesConverter" executable. You can execute it by running `./PicturesConverter` in the folder where the program is saved, or simply double-click on it.
3. **Troubleshooting** You need to install `libgtk-4-1`, `libconfig9`,`libmagic-mgc`, `libpoppler-glib8t64` and `libtiff6`.

## Adding a New Language

1. **Create Files**: Within the "src/locales" folder, create a subfolder named with the short of your language. Inside this folder, create another folder named `LC_MESSAGES`. Then, within this folder, create a file named `PicturesConverter.po`.
2. **Add Translations to File**: Copy the provided template and modify the English phrases to their equivalents in your language in the PicturesConverter.po file.
```po

 ```
3. **Compile**: Use the `msgfmt` command to compile the language file. For example: `msgfmt src/locale/<YourLanguage>/LC_MESSAGES/PicturesConverter.po -o build/locale/<YourLanguage>/LC_MESSAGES/PicturesConverter.mo`.
4. **Testing**: Launch Pictures Converter and verify that the new language is working correctly. If any issues arise, please open an issue and provide your `PicturesConverter.po` file and language details.
5. **Support the Project (Optional)**: Open an issue and share your `PicturesConverter.po` file along with your language details, and we will add it to the project.
6. **Add Language Support for CMake (Optional)**: To include support for an additional language, add the following line under the existing `compile_po_files(en)` entry in your CMake configuration: `compile_po_files([your_language_short_name])` After making this change, reconfigure CMake to apply the updates.
7. 
## Compiling It Yourself
### Build Instructions

1. **Download the Code**: Begin by downloading the latest version of the code.
2. **Preparation**: Create a build directory within the project folder.
3. **Create Build Program**: Navigate to the build directory and run the following command: `cmake ..`
4. **Run the Build**: In the build directory, execute the following command to compile the code:
       `make -j[thread count]` Replace `[thread count]` with the number of threads you wish to utilize for the build process.


Feel free to let me know if you need any further modifications!

