cmake_minimum_required(VERSION 3.10)
project(CameraJSON VERSION 1.0.0 LANGUAGES CXX)

# Настройка стандарта C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройка компилятора
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Поиск необходимых пакетов
find_package(PkgConfig REQUIRED)

# Поиск GStreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GSTREAMER_BASE REQUIRED gstreamer-base-1.0)

# Поиск JSON-GLib
pkg_check_modules(JSON_GLIB REQUIRED json-glib-1.0)

# Поиск GLib
pkg_check_modules(GLIB REQUIRED glib-2.0)

# Объединение всех флагов
set(ALL_INCLUDE_DIRS
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_APP_INCLUDE_DIRS}
    ${GSTREAMER_BASE_INCLUDE_DIRS}
    ${JSON_GLIB_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)

set(ALL_LIBRARIES
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_APP_LIBRARIES}
    ${GSTREAMER_BASE_LIBRARIES}
    ${JSON_GLIB_LIBRARIES}
    ${GLIB_LIBRARIES}
)

set(ALL_LDFLAGS
    ${GSTREAMER_LDFLAGS}
    ${GSTREAMER_APP_LDFLAGS}
    ${GSTREAMER_BASE_LDFLAGS}
    ${JSON_GLIB_LDFLAGS}
    ${GLIB_LDFLAGS}
)

# Создание исполняемого файла
add_executable(main
    main.cpp
    main.h
    usb_cameras.cpp
    usb_cameras.h
    utils.cpp
    utils.h
)

# Установка путей для заголовков
target_include_directories(main PRIVATE ${ALL_INCLUDE_DIRS})

# Установка флагов компиляции
target_compile_options(main PRIVATE ${GSTREAMER_CFLAGS_OTHER})
target_compile_options(main PRIVATE ${GSTREAMER_APP_CFLAGS_OTHER})
target_compile_options(main PRIVATE ${GSTREAMER_BASE_CFLAGS_OTHER})
target_compile_options(main PRIVATE ${JSON_GLIB_CFLAGS_OTHER})
target_compile_options(main PRIVATE ${GLIB_CFLAGS_OTHER})

# Установка библиотек для линковки
target_link_libraries(main ${ALL_LIBRARIES})
target_link_options(main PRIVATE ${ALL_LDFLAGS})

# Копирование флагов компиляции
target_compile_options(main PRIVATE ${GSTREAMER_CFLAGS})
target_compile_options(main PRIVATE ${GSTREAMER_APP_CFLAGS})
target_compile_options(main PRIVATE ${GSTREAMER_BASE_CFLAGS})
target_compile_options(main PRIVATE ${JSON_GLIB_CFLAGS})
target_compile_options(main PRIVATE ${GLIB_CFLAGS})

# Установка свойств целевого файла
set_target_properties(main PROPERTIES
    OUTPUT_NAME "main"
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Проверка зависимостей во время компиляции
add_custom_target(check-deps ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Проверка зависимостей..."
    COMMAND pkg-config --exists gstreamer-1.0
    COMMAND pkg-config --exists json-glib-1.0
    COMMAND pkg-config --exists glib-2.0
    COMMENT "Проверка наличия всех зависимостей"
)

# Цель для отладки - показать найденные пакеты
add_custom_target(show-deps
    COMMAND ${CMAKE_COMMAND} -E echo "GStreamer: ${GSTREAMER_VERSION}"
    COMMAND ${CMAKE_COMMAND} -E echo "JSON-GLib: ${JSON_GLIB_VERSION}"
    COMMAND ${CMAKE_COMMAND} -E echo "GLib: ${GLIB_VERSION}"
    COMMENT "Показать информацию о зависимостях"
)

# Установка (опционально)
install(TARGETS main
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Сообщение о конфигурации
message(STATUS " ")
message(STATUS "Конфигурация CameraJSON:")
message(STATUS "   Версия CMake: ${CMAKE_VERSION}")
message(STATUS "   Компилятор C++: ${CMAKE_CXX_COMPILER}")
message(STATUS "   Стандарт C++: ${CMAKE_CXX_STANDARD}")
message(STATUS "   GStreamer найдена: ${GSTREAMER_FOUND} (${GSTREAMER_VERSION})")
message(STATUS "   JSON-GLib найден: ${JSON_GLIB_FOUND} (${JSON_GLIB_VERSION})")
message(STATUS "   GLib найден: ${GLIB_FOUND} (${GLIB_VERSION})")
message(STATUS " ")