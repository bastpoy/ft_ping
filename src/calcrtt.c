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
    if(!sum)
        return 0.0;
    float deviateMean = sum / (size);
    return deviateMean;
}

float find_avg(float *array, int size){
    float sum = 0;
    for(int i = 0; i < size; i++){
        sum += array[i];
    }
    if(!sum)
        return 0.0;
    return sum /(size);
}

void add_time_rtt(float **ptr, float time, int index, int *ptrSize){
    if(index >= *ptrSize){
        *ptrSize = *ptrSize + 15;
        *ptr = (float*)realloc(*ptr, (*ptrSize) * sizeof(float));
    }
    (*ptr)[index] = time;
}