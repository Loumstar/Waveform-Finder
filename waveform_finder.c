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

bool is_point_of_inflection(const int32_t* frame){
    /*
    Method to determine whether a sample is a point of inflection along the waveform. 

    The point of inflection is evaluated by comparing the previous second derivative to
    the next second derivative at that point.

    If between these two points, the second derivative passes through zero, there must be
    a point of inflection.
    
    Note this point could be anywhere between the first and second samples.
    */
    double sd0 = second_derivative(frame);
    double sd1 = second_derivative(frame + 1);

    return (sd0 <= 0 && sd1 > 0) || (sd0 >= 0 && sd1 < 0);
}

uint64_t find_square_area(const curve* c){
    /*
    Method to determine the square area underneath a curve.
    This is equivalent to integrating the square of the function with respect to each
    sample.

    ∫ c ** 2 ds ~ sum(c[i] ** 2) * ∆s = sum(c2[i] ** 2)
    */
    uint64_t square_difference = 0;

    for(size_t i = 0; i < c->length; i++){
        square_difference += (int64_t) pow((c->data)[i], 2);
    }

    return square_difference;
}

curve blank_curve(){
    curve c;

    c.length = 0;
    c.square_area = 0;
    c.data = NULL;

    return c;
}

curve new_curve(int32_t* frame_start){
    curve c;
    
    c.length = 0;
    c.square_area = 0;
    c.data = frame_start;

    return c;
}

void set_curve_length(curve* c, size_t length){
    if(c->data){
        c->length = length;
        c->square_area = find_square_area(c);
    } else {
        printf("Curve has no data pointer. Length not saved\n");
    }
}

uint64_t compare_curves(const curve* c1, const curve* c2){
    /*
    Method to evaluate how similar two curves are.

    This is done by finding the square difference in area of the two curves.

    ∫ (c2 - c1) ** 2 ds ~ sum((c2[i] - c1[i]) ** 2) * ∆s = sum((c2[i] - c1[i]) ** 2)

    This is loosely based off of chi squared tests, where squaring the value means any
    discrepency between the two curves is always positive, and can be used to indicate
    the error between them.
    */
    uint64_t square_difference = 0;

    if(c1->data && c2->data){
        size_t length = (c1->length > c2->length) ? c1->length : c1->length;
        for(size_t i = 0; i < length; i++){
            square_difference += pow(((c1->data)[i] - (c2->data)[i]), 2);
        }
    } else if(c1->data || c2->data){
        square_difference = c1->data ? c1->square_area : c2->square_area;
    }
    
    return square_difference;
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
    size_t square_area = (c1->square_area > c2->square_area) ? c1->square_area : c2->square_area;
    double error_of_fit = square_area ? compare_curves(c1, c2) / square_area : DBL_MAX;

    return error_of_fit < CURVE_ERROR_THRESHOLD;
}

int recurse_through_curves(const curve* curves, int i, int j, size_t curves_array_length){
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
    j += j < 0 ? curves_array_length : 0;

    if(is_same_curve(&curves[i], &curves[j])){
        bool intermediate_curves_same = true;
        int m = i, n = j;

        while(m != j){
            m += m <= 0 ? curves_array_length - 1 : -1;
            n += n <= 0 ? curves_array_length - 1 : -1;
            
            if(!is_same_curve(&curves[m], &curves[n])){
                intermediate_curves_same = false;
                break;
            }
        }
        if(intermediate_curves_same || i == j){
            return j;
        }
    }
    return recurse_through_curves(curves, i, j - 1, curves_array_length);
}

waveform blank_waveform(){
    waveform w;

    w.length = 0;
    w.numberof_curves = 0;

    return w;
}

waveform find_waveform(const curve* curves, int i, size_t curves_array_length){    
    /*
    Method to return a waveform struct if a waveform is found by comparing curves using
    the recurse_through_curves() method.

    If index i and j are equal, this implies no curve has been found. If the length of
    the waveform is greater than WAVEFORM_MAX_CURVES, then the waveform is discarded.

    In both cases, a blank waveform will be returned.

    Else, a fully initialised waveform, with pointers to each curve, will be returned.
    */
    waveform w = blank_waveform();
    
    int j = recurse_through_curves(curves, i, i - 1, curves_array_length);
    size_t numberof_curves = i - j < 0 ? curves_array_length + i - j : i - j;

    if(i == j){
        printf("No waveform pattern found for curve at %p\n", (void*) &curves[i]);
    } else if(numberof_curves > WAVEFORM_MAX_CURVES){
        printf("Waveform length greater than WAVEFORM_MAX_CURVES\n");
        printf("    %zu > %d\n", numberof_curves, WAVEFORM_MAX_CURVES);
    } else {
        w.numberof_curves = numberof_curves;
        //printf("WAVEFORM FOUND\n   LENGTH %zu\n", w.numberof_curves);
        for(size_t k = 0; k < w.numberof_curves; k++){
            w.curves[k] = curves[(j + k) % curves_array_length];
            w.length += w.curves[k].length;
        }
    }

    return w;
}
