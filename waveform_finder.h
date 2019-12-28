#ifndef WAVEFORM_FINDER_H
    #define WAVEFORM_FINDER_H

    #include <stdbool.h>
    #include <stdint.h>
    #include <stdio.h>
    #include <math.h>

    #define DELTA_S 10
    #define WAVEFORM_MAX_CURVES 15
    #define CURVE_ERROR_THRESHOLD 0.01

    typedef struct curve {
        size_t length;
        uint64_t square_area;
        int32_t* data;
    } curve;

    typedef struct waveform {
        size_t numberof_curves;
        curve curves[WAVEFORM_MAX_CURVES];
    } waveform;

    bool is_point_of_inflection(const int32_t* frame);

    void set_new_curve(curve* c, int32_t* data_start);

    void set_curve_length(curve* c, size_t length);

    void analyse_curve(curve* c);

    waveform find_waveform(const curve* curves, int i, size_t curves_array_length);

#endif
