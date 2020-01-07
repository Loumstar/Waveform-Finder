#include "../waveform_finder.h"
#include "Write-WAV-File/wave_file.h"

int main(void){
    char wave_filename[] = "a_note.wav";

    curve curves[MAX_SAVED_CURVES];
    waveform current_waveform = blank_waveform();
    
    // Ensure all curves are blank
    for(size_t i = 0; i < MAX_SAVED_CURVES; i++) curves[i] = blank_curve();
    
    // Read data from wave file to an array
    Wave audio_file = read_wave_metadata(wave_filename);
    int32_t* audio_array = malloc(sizeof(int32_t) * audio_file.numberof_samples);
    read_wave_data_to_array(&audio_file, audio_array);

    // Create a counter for updating curves in the array
    size_t curve_index = 0;
    // Create a counter that represents the starting index of a curve in the audio array
    size_t curve_start_index = 0;
    // Create boolean for if a new waveform needs to be found
    bool waveform_up_to_date;
    
    // Loop through each sample of the audio file array
    for(size_t sample_index = DELTA_S; sample_index < audio_file.numberof_samples - DELTA_S; sample_index++){
        // If at a given sample there is a point of inflection
        if(is_point_of_inflection(&audio_array[sample_index])){
            
            /*
            Create a new curve that begins at point of the previous inflection and has a
            length equal to the number of samples between the previous and the new
            points of inflection.
            */

            size_t curve_length = sample_index - curve_start_index;
            new_curve(&curves[curve_index], &audio_array[curve_start_index], curve_length);
            
            // Check if the new curve fits the pattern of the waveform
            waveform_up_to_date = curve_fits_waveform(&curves[curve_index], &current_waveform);

            if(!waveform_up_to_date){
                // If it doesn't fit, start looking for a new waveform, starting with the newest curve
                find_new_waveform(&current_waveform, curves, curve_index);
            } else if(is_end_of_waveform(&current_waveform)){
                // If the waveform has repeated itself, change the waveform to the most recent version
                update_waveform(&current_waveform, curves, curve_index);
            }

            // Move counter to the next curve in the array, if it goes above the size of the array, start again at zero
            curve_index = (curve_index + 1) % MAX_SAVED_CURVES;
            // Set the starting index of the next curve
            curve_start_index = sample_index;
        }
    }
    
    remove_sample_data(&audio_file);
    free(audio_array);
    
    return 0;
}
