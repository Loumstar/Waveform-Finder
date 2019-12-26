#include "../waveform_finder.h"
#include "Write-WAV-File/wave_file.h"

#define MAX_SAVED_CURVES 50

int main(void){
    char wave_filename[] = "a_note.wav";

    curve saved_curves[MAX_SAVED_CURVES];

    for(size_t i = 0; i < MAX_SAVED_CURVES; i++){
        saved_curves[i].length = 0;
    }

    waveform main_waveform;

    Wave a_note = read_wave_metadata(wave_filename);

    int32_t* a_note_array = malloc(sizeof(int32_t) * a_note.numberof_samples);
    read_wave_data_to_array(&a_note, a_note_array);

    int curve_index = 0;
    size_t curve_data_index = 2;
    
    for(size_t i = 0; i < a_note.numberof_samples; i++){
        saved_curves[curve_index].data[curve_data_index] = a_note_array[i];
        
        if(DELTA_S < i && is_point_of_inflection(&a_note_array[i])){
            saved_curves[curve_index].length = curve_data_index;
            saved_curves[curve_index].square_area = find_square_area(&saved_curves[curve_index]);

            //printf("LENGTH %zu, SQ AREA %llu\n", saved_curves[curve_index].length, saved_curves[curve_index].square_area);

            size_t relative_index = find_waveform(saved_curves, curve_index, MAX_SAVED_CURVES);

            if(relative_index){
                //printf("WAVEFORM FOUND WITH LENGTH %zu\n", relative_index);
                main_waveform.length = relative_index;
                for(size_t j = 0; j < relative_index; j++){
                    main_waveform.curves[j] = saved_curves[(curve_index + relative_index + j) % MAX_SAVED_CURVES];
                }
            }

            curve_index = (curve_index + 1) % MAX_SAVED_CURVES;

            saved_curves[curve_index].data[0] = a_note_array[i];
            curve_data_index = 1;

        } else {
            curve_data_index++;
        }
    }

    free(a_note_array);

    return 0;
}
