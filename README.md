Установка на Linux:
 -- Fedora: 
       sudo dnf install gstreamer1-devel gstreamer1-plugins-base-tools gstreamer1-doc gstreamer1-plugins-base-devel gstreamer1-plugins-good gstreamer1-plugins-good-extras \
       gstreamer1-plugins-ugly json-glib-devel pkg-config v4l-utils \
       gstreamer1-plugins-bad-free \
       gstreamer1-plugins-bad-free-devel gstreamer1-plugins-bad-free-extras
 -- Ubuntu/Debian:
       sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
       gstreamer1.0-plugins-bad libjson-glib-dev pkg-config v4l-utils \
       gstreamer1.0-plugins-ugly gstreamer1.0-libav \
       gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio

Флаги компиляции: `pkg-config --cflags --libs gstreamer-1.0 json-glib-1.0 glib-2.0` - вот эти кавычки обязательны.
Пример: g++ main.cpp -o main `pkg-config --cflags --libs gstreamer-1.0 json-glib-1.0 glib-2.0`

Можно использовать CMake (что значительно проще):
 -- Если не установлен: sudo apt install cmake
 -- Процесс запуска:
       Для этого нужно зайти в директорию build в папке проекта (с помощью cd (создать в случае отсутствия))
       Далее прописать команду cmake ..
       После чего нужно создать исполняемый файл, для этого следует прописать команду cmake --build .
       Теперь приложение готово к запуску командой ./main (название исполняемого файла можно поменять в CMakeLists.txt)
винда пока сосёт    