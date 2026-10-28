cmake_minimum_required(VERSION 3.22)

project(Groover VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

option(GROOVER_BUILD_PLUGIN "Build the JUCE plugin (needs JUCE)" ON)
option(GROOVER_BUILD_TESTS "Build the core unit tests" ON)
option(GROOVER_MIDI_FX "Build as a MIDI effect instead of an instrument (for Logic's MIDI FX slot)" OFF)

# The engine has no JUCE dependency so it can be tested on its own.
add_library(groover_core STATIC
    Source/Core/GrooveLibrary.cpp
    Source/Core/GrooveEngine.cpp)
target_include_directories(groover_core PUBLIC Source/Core)
set_target_properties(groover_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(GROOVER_BUILD_TESTS)
    enable_testing()
    add_executable(groover_tests Tests/CoreTests.cpp)
    target_link_libraries(groover_tests PRIVATE groover_core)
    add_test(NAME groover_tests COMMAND groover_tests)
endif()

if(NOT GROOVER_BUILD_PLUGIN)
    return()
endif()

# Point JUCE_DIR at a local checkout to skip the download.
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout")
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.9
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(GROOVER_MIDI_FX)
    set(GROOVER_IS_SYNTH FALSE)
    set(GROOVER_IS_MIDI_EFFECT TRUE)
else()
    set(GROOVER_IS_SYNTH TRUE)
    set(GROOVER_IS_MIDI_EFFECT FALSE)
endif()

juce_add_plugin(Groover
    COMPANY_NAME "Groover"
    BUNDLE_ID "com.groover.groover"
    PLUGIN_MANUFACTURER_CODE Grvr
    PLUGIN_CODE Grv1
    PRODUCT_NAME "Groover"
    FORMATS VST3 AU
    IS_SYNTH ${GROOVER_IS_SYNTH}
    IS_MIDI_EFFECT ${GROOVER_IS_MIDI_EFFECT}
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Generator
    AU_MAIN_TYPE kAudioUnitType_MusicDevice)

if(GROOVER_MIDI_FX)
    set_target_properties(Groover PROPERTIES JUCE_AU_MAIN_TYPE kAudioUnitType_MIDIProcessor)
endif()

target_sources(Groover PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/UI/GrooverLookAndFeel.cpp
    Source/UI/StepGrid.cpp
    Source/UI/KitPanel.cpp
    Source/MidiExport.cpp)

target_include_directories(Groover PRIVATE Source)

target_compile_definitions(Groover PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Groover
    PRIVATE
        groover_core
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
