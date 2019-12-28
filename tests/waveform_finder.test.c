#include "../waveform_finder.h"
#include "Write-WAV-File/wave_file.h"

#define MAX_SAVED_CURVES (2 * WAVEFORM_MAX_CURVES)

int main(void){
    char wave_filename[] = "a_note.wav";

    curve saved_curves[MAX_SAVED_CURVES];

    for(size_t i = 0; i < MAX_SAVED_CURVES; i++){
        saved_curves[i].length = 0;
        saved_curves[i].square_area = 0;
        saved_curves[i].data = NULL;
    }

    waveform current_waveform;

    Wave a_note = read_wave_metadata(wave_filename);

    int32_t* a_note_array = malloc(sizeof(int32_t) * a_note.numberof_samples);
    read_wave_data_to_array(&a_note, a_note_array);

    size_t curve_index = 0;
    size_t curve_data_index = 0;
    size_t waveforms_found = 0;
    
    saved_curves[curve_index].data = &a_note_array[0];
    
    for(size_t i = 0; i < a_note.numberof_samples; i++){        
        if(DELTA_S < i && is_point_of_inflection(&a_note_array[i])){
            saved_curves[curve_index].length = curve_data_index;
            saved_curves[curve_index].square_area = find_square_area(&saved_curves[curve_index]);

            current_waveform = find_waveform(saved_curves, curve_index, MAX_SAVED_CURVES);

            if(current_waveform.length){
                waveforms_found++;
                printf("WAVEFORM COUNT %zu\n", waveforms_found);
                printf("WAVEFORM LENGTH %zu\n", current_waveform.length);
            }

            curve_index = (curve_index + 1) % MAX_SAVED_CURVES;

            saved_curves[curve_index].data = &a_note_array[i];
            curve_data_index = 1;

        } else {
            curve_data_index++;
        }
    }

    free(a_note_array);

    return 0;
}
