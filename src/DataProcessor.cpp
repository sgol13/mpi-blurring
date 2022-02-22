/*
 * DataProcessor.cpp
 *
 *  Created on: 20 paź 2021
 *      Author: oramus
 */

#include "DataProcessor.h"
#include <iostream>
#include <math.h>

using namespace std;

DataProcessor::DataProcessor() {}

DataProcessor::~DataProcessor() {}

int DataProcessor::round_near(double b) {
    if ((b - (int)b) < 0.5)
        return (int)b;
    else
        return (int)b + 1;
}


void DataProcessor::setMargin(int margin) {
    this->margin = margin;
    // margin = round_near(sqrt(dataPortionSize) - 1) / 2;
    // cout << "Margin " << margin << endl;
}

void DataProcessor::setInitialData(double **data, int size) {
    this->data = data;
    this->dataSize = size;
}

void DataProcessor::execute(int repetitions) {
    shareData();
    for (int i = 0; i < repetitions; i++)
        singleExecution();
    collectData();
}
