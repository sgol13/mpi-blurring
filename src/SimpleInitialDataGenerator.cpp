/*
 * SimpleInitialDataGenerator.cpp
 *
 *  Created on: 20 paź 2021
 *      Author: oramus
 */

#include "SimpleInitialDataGenerator.h"

SimpleInitialDataGenerator::SimpleInitialDataGenerator(double marginValue,
                                                       double bulkValue) {
    this->marginValue = marginValue;
    this->bulkValue = bulkValue;
}

void SimpleInitialDataGenerator::fillWithData(double **data, int dataSize,
                                              int margin) {
    /* int c = 0;
    for (int i = 0; i < dataSize; i++)
        for (int j = 0; j < dataSize; j++) {
            data[i][j] = c;
            c = (c + 1) % 10;
        } */

    int otherMargin = dataSize - margin - 1;
    for (int i = 0; i < dataSize; i++)
        for (int j = 0; j < dataSize; j++) {
            if (i < margin)
                data[i][j] = marginValue;
            else if (j < margin)
                data[i][j] = marginValue;
            else if (i > otherMargin)
                data[i][j] = marginValue;
            else if (j > otherMargin)
                data[i][j] = marginValue;
            else
                data[i][j] = bulkValue;
        }
}
