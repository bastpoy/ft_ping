#include "../include/header.h"

float find_max(float *array, int size){
    if(size <= 0){
        return 0;
    }

    float max = array[0];
    for(int i = 1; i < size; i++){
        if(array[i] > max){
            max = array[i];
        }
    }
    return max;
}

float find_min(float *array, int size){
    if(size <= 0){
        return 0;
    }

    float min = array[0];
    for(int i = 1; i < size; i++){
        if(array[i] < min){
            min = array[i];
        }
    }
    return min;
}

float find_mean_deviation(float *array, int size){
    float sum = 0;
    for(int i = 0; i < size; i++){
        sum += array[i];
    }
    float mean = sum / (size);
    sum = 0;
    for(int i = 0; i < size; i++){
        sum += fabs(mean - array[i]);
    }
    float deviateMean = sum / (size);
    return deviateMean;
}

float find_avg(float *array, int size){
    float sum = 0;
    for(int i = 0; i < size; i++){
        sum += array[i];
    }
    return sum /(size);
}
