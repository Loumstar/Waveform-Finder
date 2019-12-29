#include "../waveform_finder.h"
#include "Write-WAV-File/wave_file.h"

#define MAX_SAVED_CURVES (2 * WAVEFORM_MAX_CURVES)

int main(void){
    char wave_filename[] = "a_note.wav";

    curve curves[MAX_SAVED_CURVES];
    waveform current_waveform;
    
    // Ensure all curves are blank
    for(size_t i = 0; i < MAX_SAVED_CURVES; i++){
        curves[i] = blank_curve();
    }
    
    // Read data from wave file to an array
    Wave audio_file = read_wave_metadata(wave_filename);
    int32_t* audio_array = malloc(sizeof(int32_t) * audio_file.numberof_samples);
    read_wave_data_to_array(&audio_file, audio_array);

    // Create a counter for updating curves in the array
    size_t curve_index = 0;
    // Create a counter to determine the number of samples each curve is in length
    size_t curve_data_index = 0;
    
    // Set the starting point for the first curve at the start of the audio file array
    curves[curve_index] = new_curve(&audio_array[0]);
    
    // Loop through each sample of the audio file array
    for(size_t i = DELTA_S; i < audio_file.numberof_samples - DELTA_S; i++){
        // If at a given sample there is a point of inflection
        if(is_point_of_inflection(&audio_array[i])){
            // Set the length of the current curve to the number of samples looped through since previous curve
            set_curve_length(&curves[curve_index], curve_data_index);
            // Run pattern recognition method to find a repeating waveform
            current_waveform = find_waveform(curves, curve_index, MAX_SAVED_CURVES);
            // Move counter to the next curve in the array, if it goes above the size of the array, start again at zero
            curve_index = (curve_index + 1) % MAX_SAVED_CURVES;
            // Reset the next curve, setting the starting point of it to where the previous curve stopped in the audio file
            curves[curve_index] = new_curve(&audio_array[i]);
            // Reset the curve length counter
            curve_data_index = 1;
        } else {
            curve_data_index++;
        }
    }
    
    remove_sample_data(&audio_file);
    free(audio_array);
    
    return 0;
}
