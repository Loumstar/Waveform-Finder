#ifndef WAVEFORM_FINDER_H
    #define WAVEFORM_FINDER_H

    #include <stdbool.h>
    #include <stdint.h>
    #include <stdio.h>
    #include <float.h>
    #include <math.h>

    #define DELTA_S 10
    #define WAVEFORM_MAX_CURVES 15
    #define CURVE_ERROR_THRESHOLD 0.01

    #define MAX_SAVED_CURVES (2 * WAVEFORM_MAX_CURVES)

    typedef struct curve {
        size_t numberof_samples;
        uint64_t square_area;
        
        int32_t* data;

        bool __is_valid;
    } curve;

    typedef struct waveform {
        size_t numberof_samples;
        size_t numberof_curves;
        
        curve curves[WAVEFORM_MAX_CURVES];

        size_t __curve_index;
        bool __is_valid;
    } waveform;

    bool is_point_of_inflection(const int32_t* sample);

    
    curve blank_curve();

    void new_curve(curve* c, int32_t* data_start, size_t numberof_samples);

    
    waveform blank_waveform();

    void find_new_waveform(waveform* w, const curve* curves, int i);

    void update_waveform(waveform* w, const curve* curves, int i);

    
    bool curve_fits_waveform(const curve* c, waveform* w);

    bool is_end_of_waveform(const waveform* w);

#endif
