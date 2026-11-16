cmake_minimum_required(VERSION 3.22)

project(FDgPE VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

# Windows : runtime statique (évite d'exiger les redistribuables Visual C++)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# --- Dépendances (sous-modules git, voir README.md) ---------------------------
foreach(dep JUCE tracktion_engine)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/${dep}/CMakeLists.txt")
        message(FATAL_ERROR
            "third_party/${dep} est vide ou absent. "
            "Lance : git submodule update --init (sans --recursive), voir README.md.")
    endif()
endforeach()

# JUCE d'abord : les modules Tracktion en ont besoin.
add_subdirectory(third_party/JUCE)

# Les modules Tracktion (cibles tracktion::tracktion_core / _engine / _graph).
# On n'utilise PAS le JUCE embarqué dans tracktion_engine/modules/juce : son
# sous-module pointe vers une URL SSH. On épingle JUCE nous-mêmes (HTTPS).
add_subdirectory(third_party/tracktion_engine/modules tracktion_modules)

add_subdirectory(app)
