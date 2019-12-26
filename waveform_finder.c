#include "waveform_finder.h"

double derivative(const int32_t* y){
    /*
    Method to return the rate of change of displacement of the waveform with respect each
    sample.

    Because adjacent samples are one apart, the equation is simplified to a difference in
    displacemnt of the waveform: dy/ds ~ (y[1] - y[0]) / ∆s
    */
    return (double) (y[DELTA_S] - y[0]) / DELTA_S;
}

double second_derivative(const int32_t* y){
    /*
    Method to return the second derivative, or 'the rate of change of rate of change' of 
    displacement of the waveform with respect to sample count.

    Again, ∆s = 1, so the equation is simplified to a difference in derivatives.

    For brevity, pointer arithmetic has been used where the -1 is the previous element
    relative to element y.
    */
    return (double) (derivative(y) - derivative(y - DELTA_S)) / DELTA_S;
}

bool is_point_of_inflection(const int32_t* frame){
    /*
    Method to determine whether a sample is a point of inflection along the waveform. 
    
    The cubic splines are formed between two points of inflection and the maximum 
    absolute second derivative.

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
    uint64_t square_difference = 0;

    for(size_t i = 0; i < c->length; i++){
        square_difference += (int64_t) pow((c->data)[i], 2);
    }

    return square_difference;
}

uint64_t compare_curves(const curve* c1, const curve* c2){
    /*
    Method to evaluate how similar two curves are.

    This is done by finding the square difference in area of the two curves.
    As time increases with each successive sample, this integral can be simplified to a
    riemann sum where the change in sample, ∆s, is always 1:

    ∫ (c2 - c1) ** 2 ds ~ sum((c2[i] - c1[i]) ** 2) * ∆s = sum((c2[i] - c1[i]) ** 2)

    This is loosely based off of chi squared tests, where squaring the value means any
    discrepency between the two curves is always positive, and can be used to indicate
    the error between them.
    */
    size_t length = (c1->length > c2->length) ? c1->length : c1->length;
    uint64_t square_difference = 0;

    for(size_t i = 0; i < length; i++){
        square_difference += pow(((c1->data)[i] - (c2->data)[i]), 2);
    }

    return square_difference;
}

bool is_same_curve(const curve* c1, const curve* c2){
    /*
    Method to determine whether two curves are similar enough to be considered the same
    curve.
    
    This is evaluated by finding the error between the two curves and dividing it by the
    length of the longest curve. If the average error per unit time is greater than the
    threshold, they are considered different.

    The 'per unit time' is not strictly accurate as the length is given in terms of 
    number of samples, but as the period between two samples is constant, these two 
    concepts are roughly the same, and per unit time is more intuitive.
    */
    size_t length = (c1->length > c2->length) ? c1->length : c2->length;
    int error_per_sample = length ? (compare_curves(c1, c2) / length) : 0;
    
    char c1_type[4] = " ve", c2_type[4] = " ve";

    c1_type[0] = c1->data[25] > 0 ? '+' : (c1->data[25] == 0 ? '0' : '-');
    c2_type[0] = c2->data[25] > 0 ? '+' : (c2->data[25] == 0 ? '0' : '-');

    //printf("%s and %s, ERROR / LENGTH: %d\n", c1_type, c2_type, error_per_sample);
    return (size_t) error_per_sample < CURVE_ERROR_THRESHOLD;
}

int recurse_through_curves(curve* curves, int i, int j, size_t curves_array_length){
    /*
    Method to compare a set of curves to determine if there exists a repeating waveform.

    The method is recursive, where i and k point to curves to be compared against, and j
    is the index difference between these curves.
    
    Note i is constant, and j is increasing. As k = i - j, and k is decreasing.

    If curve[i] and curve[k] are the same and i and k are not the same index, this
    implies that there is a repeating pattern.

    The method will then compare all the intermediate curves, i.e. curve[i - 1] will be
    compared to curve[j - 1] and so on until curve[i - n] points to curve[j].

    At this point, if none of the is_same_curve() tests have failed, the sequence from
    j to i is considered a complete waveform.

    This will return the number of curves between curve[i] and the curve[k], which equals
    j. If the return value is zero, this implies all curves have been looped through and 
    no waveform has been found.
    */
    size_t k = i - j < 0 ? i - j + curves_array_length : i - j;

    if(is_same_curve(&curves[i], &curves[k])){
        bool intermediate_curves_same = true;
        int m = i, n = k;

        while(m != k){
            m += m <= 0 ? curves_array_length - 1 : -1;
            n += n <= 0 ? curves_array_length - 1 : -1;
            
            if(!is_same_curve(&curves[m], &curves[n])){
                intermediate_curves_same = false;
                break;
            }
        }
        if(intermediate_curves_same || i == k){
            return j % curves_array_length;
        }
    }
    return recurse_through_curves(curves, i, j + 1, curves_array_length);
}


int find_waveform(curve* curves, int i, size_t curves_array_length){
    return recurse_through_curves(curves, i, 1, curves_array_length);
}