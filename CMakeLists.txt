cmake_minimum_required(VERSION 3.21)

project(PumpIt VERSION 1.0.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
)
FetchContent_MakeAvailable(juce)

# Add our plugin target
juce_add_plugin(PumpIt
    COMPANY_NAME "Atharva Chauhan"
    BUNDLE_ID "com.AtharvaChauhan.PumpIt"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE AthC
    PLUGIN_CODE PmIt
    FORMATS VST3 Standalone
    PRODUCT_NAME "PumpIt"
)

# Generate JuceHeader.h for the target
juce_generate_juce_header(PumpIt)

# Add source files
# We will create these in the next steps
target_sources(PumpIt PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/CurveEditorComponent.cpp
)

# Add standard JUCE modules
target_link_libraries(PumpIt PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_dsp
)

# Fix VST2 missing headers error
target_compile_definitions(PumpIt PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# Enable strict warnings
target_compile_options(PumpIt PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra>
)
