cmake_minimum_required(VERSION 3.22)
project(iesu VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(iesu
    COMPANY_NAME "iesu"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Iesu
    PLUGIN_CODE Ies1
    FORMATS VST3 Standalone
    PRODUCT_NAME "iesu"
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(iesu)

juce_add_binary_data(iesuAssets SOURCES Resources/girl.jpg)

target_sources(iesu PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(iesu PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

if(MSVC)
    target_compile_options(iesu PRIVATE /utf-8)
endif()

target_link_libraries(iesu
    PRIVATE
        iesuAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
