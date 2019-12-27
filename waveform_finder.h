#ifndef WAVEFORM_FINDER_H
    #define WAVEFORM_FINDER_H

    #include <stdbool.h>
    #include <stdint.h>
    #include <stddef.h>
    #include <math.h>

    #include <stdio.h>

    #define DELTA_S 10

    #define CURVE_MAX_SAMPLES 100
    #define WAVEFORM_MAX_CURVES 15

    #define CURVE_ERROR_THRESHOLD 0.01

    typedef struct curve {
        size_t length;
        uint64_t square_area;
        int32_t data[CURVE_MAX_SAMPLES];
    } curve;

    typedef struct waveform {
        size_t length;
        curve curves[WAVEFORM_MAX_CURVES];
    } waveform;

    bool is_point_of_inflection(const int32_t* frame);

    uint64_t find_square_area(const curve* c);

    uint64_t compare_curves(const curve* c1, const curve* c2);

    uint64_t compare_waveforms(const waveform* w1, const waveform* w2);

    bool is_same_curve(const curve* c1, const curve* c2);

    bool is_same_waveform(const waveform* w1, const waveform* w2);

    bool find_waveform(waveform w, const curve* curves, int i, size_t curves_array_length);

#endif
