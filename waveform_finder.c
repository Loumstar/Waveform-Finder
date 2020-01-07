#include "waveform_finder.h"

double derivative(const int32_t* y){
    /*
    Method to return the rate of change of displacement of the waveform with respect each
    sample.

    Using consecutive samples to determine the derivative often gives innacurate results,
    so an average across a set is used instead.
    */
    return (double) (y[DELTA_S] - y[0]) / DELTA_S;
}

double second_derivative(const int32_t* y){
    /*
    Method to return the second derivative, or 'the rate of change of rate of change' of 
    displacement of the waveform with respect to sample count.
    */
    return (double) (derivative(y) - derivative(y - DELTA_S)) / DELTA_S;
}

bool is_point_of_inflection(const int32_t* sample){
    /*
    Method to determine whether a sample is a point of inflection along the waveform. 

    The point of inflection is evaluated by comparing the previous second derivative to
    the next second derivative at that sample.

    If between these two samples, the second derivative passes through zero, there must be
    a point of inflection.
    
    Note this point could be anywhere between this sample and the next.
    */
    double sd0 = second_derivative(sample);
    double sd1 = second_derivative(sample + 1);

    return (sd0 <= 0 && sd1 > 0) || (sd0 >= 0 && sd1 < 0);
}

uint64_t curve_square_area(const curve* c){
    /*
    Method to determine the square area underneath a curve.
    This is equivalent to integrating the square of the function with respect to each
    sample.

    ∫ c ** 2 ds ~ sum(c[i] ** 2) * ∆s = sum(c2[i] ** 2)
    */
    uint64_t square_difference = 0;

    for(size_t i = 0; i < c->numberof_samples; i++){
        square_difference += (uint64_t) pow((c->data)[i], 2);
    }

    return square_difference;
}

uint64_t curves_square_difference(const curve* c1, const curve* c2){
    /*
    Method to determine the square difference in area between two curves.

    ∫ (c2 - c1) ** 2 ds ~ sum((c2[i] - c1[i]) ** 2) * ∆s = sum((c2[i] - c1[i]) ** 2)

    The upper limit of the integral is equal to the length of the longer curve.

    To prevent segfaults, when the index goes above the length of the shorter curve,
    its value will be assumed to be zero.
    */
    uint64_t square_difference = 0;
    const curve *c_long, *c_short;

    int32_t c_short_data;

    if(c1->numberof_samples > c2->numberof_samples){
        c_long = c1; 
        c_short = c2;
    } else {
        c_long = c2; 
        c_short = c1;
    }

    for(size_t i = 0; i < c_long->numberof_samples; i++){
        c_short_data = c_short->numberof_samples < i ? 0 : (c_short->data)[i];
        square_difference += (uint64_t) pow(((c_long->data)[i] - c_short_data), 2);
    }

    return square_difference;
}

curve blank_curve(){
    /*
    Method to return a blank curve.

    As elements in struct do not initialise to zero, this method needs to be run first to
    ensure errors do not occur.
    */
    curve c;

    c.numberof_samples = 0;
    c.square_area = 0;
    c.data = NULL;

    c.__is_valid = false;

    return c;
}

void new_curve(curve* c, int32_t* data_start, size_t numberof_samples){
    /*
    Method to create a new curve.

    The method requires a pointer to the start of the curve in a sample array, and a
    length. If either of these are invalid (0 or NULL) then the curve itself is 
    considered invalid.

    This prevents further analysis from causing segfaults, or reading wild data values.
    */
    c->data = data_start;
    c->numberof_samples = numberof_samples;

    if(c->data && c->numberof_samples){
        c->__is_valid = true;
        c->square_area = curve_square_area(c);
    } else {
        c->__is_valid = false;
        printf("Curve has NULL data pointer. Analysis not run.\n");
    }
}

uint64_t compare_curves(const curve* c1, const curve* c2){
    /*
    Method to evaluate how similar two curves are.

    This is done by finding the square difference in area of the two curves.

    This is loosely based off of chi squared tests, where squaring the value means any
    discrepency between the two curves is always positive, and can be used to indicate
    the error between them.
    */
    if(c1->__is_valid && c2->__is_valid){
        return curves_square_difference(c1, c2);
    } else if(c1->__is_valid || c2->__is_valid){
        return c1->__is_valid ? c1->square_area : c2->square_area;
    } else {
        return UINT64_MAX;
    }
}

bool is_same_curve(const curve* c1, const curve* c2){
    /*
    Method to determine whether two curves are similar enough to be considered the same
    curve.
    
    This is evaluated by finding the error between the two curves and dividing it by the
    largest square area of the two curves. If the 'error of fit' is greater than the 
    threshold, they are considered different.

    If a curve has no square area, it must be blank. Therefore the error of fit is given
    the maximum value so that it fails the test.
    */
    uint64_t square_area = (c1->square_area > c2->square_area) ? c1->square_area : c2->square_area;
    double error_of_fit = square_area ? (double) compare_curves(c1, c2) / square_area : DBL_MAX;

    return error_of_fit < CURVE_ERROR_THRESHOLD;
}

bool check_intermediate_curves(const curve* curves, int i, int j){
    /*
    Method to check the intermdiate curves between two potential waveforms.

    When two curves are found to be similar, they potentially could be the same parts of
    two adjacent waveforms. If this is true, then the curves between the two waveforms 
    should match in sequence to those before the two waveforms.

    The method loops between the curves at index i and j, where i was captured before j.
    If curve[i - 1] == curve[j - 1], check curve[i - 2] == curve[j - 2] and so on until
    curve[j - n] points to curve[i].

    Note here that n therefore equals the number of curves in the waveform.
    
    At this point, the waveform has shown to repeat itself, and the method will return 
    true. If any of the is_same_curve() tests fail, it will return false.
    */
    bool curves_same = true;
    int m = i, n = j;

    while(m != j){
        m += m <= 0 ? MAX_SAVED_CURVES - 1 : -1;
        n += n <= 0 ? MAX_SAVED_CURVES - 1 : -1;
        
        if(!is_same_curve(&curves[m], &curves[n])){
            curves_same = false;
            break;
        }
    }
    return curves_same;
}

int recurse_through_curves(const curve* curves, int i, int j){
    /*
    Method to compare a set of curves to determine if there exists a repeating waveform.

    The method is recursive, where i and k point to curves to be compared against, and j
    is the index difference between these curves.
    
    Note i is constant, and j is decreasing.

    If curve[i] and curve[j] are the same and i and j are not the same index, this
    implies that there is a repeating pattern.

    The method will then compare all the intermediate curves, i.e. curve[i - 1] will be
    compared to curve[j - 1] and so on until curve[i - n] points to curve[j].

    At this point, if none of the is_same_curve() tests have failed, the sequence from
    j to i is considered a complete waveform.

    The method returns j. If j = i, it means that all curves have been looped through and
    no waveform has been found.
    */
    j += j < 0 ? MAX_SAVED_CURVES : 0;

    if(is_same_curve(&curves[i], &curves[j])){
        bool intermediate_curves_same = check_intermediate_curves(curves, i, j);
        
        if(intermediate_curves_same || i == j){
            return j;
        }
    }
    return recurse_through_curves(curves, i, j - 1);
}

waveform blank_waveform(){
    /*
    Method to return a blank waveform.
    */
    waveform w;

    w.numberof_samples = 0;
    w.numberof_curves = 0;
    w.__curve_index = 0;
    w.__is_valid = false;

    return w;
}

void copy_curves_to_waveform(waveform* w, const curve* curves, int i, size_t numberof_curves){
    /*
    Method to copy the curves from an array to a waveform.

    The data is not literally copied, but a pointer to each curve is added to an array 
    within the waveform type.

    Note i denotes the index in the curves array that points to the first curve of the
    waveform.
    */
    w->numberof_curves = numberof_curves;

    w->__is_valid = true;
    w->__curve_index = 0;
    
    for(size_t j = 0; j < w->numberof_curves; j++){
        w->curves[j] = curves[(i + j) % MAX_SAVED_CURVES];
        w->numberof_samples += w->curves[j].numberof_samples;
    }
}

void find_new_waveform(waveform* w, const curve* curves, int i){    
    /*
    Method to return a waveform struct if a waveform is found by comparing curves using
    the recurse_through_curves() method.

    If index i and j are equal, this implies no curve has been found. If the length of
    the waveform is greater than WAVEFORM_MAX_CURVES, then the waveform is discarded.

    In both cases, a blank waveform will be returned.

    Else, a fully initialised waveform, with pointers to each curve, will be returned.
    */  
    *w = blank_waveform();

    int j = recurse_through_curves(curves, i, i - 1);
    size_t numberof_curves = i - j < 0 ? MAX_SAVED_CURVES + i - j : i - j;

    if(!numberof_curves){
        printf("No waveform pattern found for curve beginning at %p\n", (void*) curves[i].data);
    } else if(numberof_curves > WAVEFORM_MAX_CURVES){
        printf("Waveform length greater than WAVEFORM_MAX_CURVES:\n");
        printf("    %zu > %d\n", numberof_curves, WAVEFORM_MAX_CURVES);
    } else {
        copy_curves_to_waveform(w, curves, j + 1, numberof_curves);
        printf("New waveform pattern found:\n");
        printf("    Number of curves: %zu\n", w->numberof_curves);
        printf("    Number of samples: %zu\n", w->numberof_samples);
    }
}

void update_waveform(waveform* w, const curve* curves, int i){
    /*
    Method to update a waveform that is similar to the previous one, but may have minor
    variations. 
    
    This is because the sinusoidal components of each waveform can vary subtly, so the 
    waveform can change shape gradually time. To avoid having to look for a new waveform 
    over time, it is better to update it to latest 'shape'.

    Note i denotes the index of the most recently updated curve in the array.
    
    Assuming the previous and latest waveforms have the same number of curves, the index
    of the first curve of the latest waveform will equal i - numberof_curves.
    */
    size_t starting_curve = i - w->numberof_curves + 1;
    copy_curves_to_waveform(w, curves, starting_curve, w->numberof_curves);
}

bool curve_fits_waveform(const curve* c, waveform* w){
    /*
    Method to check whether a curve fits in the sequence of curves of a waveform, using 
    the is_same_curve() method.

    Every time this method is called on a waveform, the index of the curve within the 
    waveform type is increased so the next curve in the waveform can be compared to the 
    next curve found in the main script.
    */
    return w->__is_valid 
        && w->__curve_index < w->numberof_samples
        && is_same_curve(c, &w->curves[w->__curve_index++]);
}

bool is_end_of_waveform(const waveform* w){
    return w->__curve_index == w->numberof_curves;
}
