#include "DataProcessor.h"
#include "MPIDataProcessor.h"
#include "mpi.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <math.h>
#include <thread>

using namespace std;

int calcDataPortion(int margin) {
    int dataPortion = margin * 2 + 1;
    dataPortion *= dataPortion;
    return dataPortion;
}

void showTable(double **table, int dataSize) {
    cout << "----------------------------------" << endl;
    for (int i = 0; i < dataSize; i++) {
        cout << setw(3) << i << " -> ";
        for (int j = 0; j < dataSize; j++)
            cout << " " << showpoint << setw(4) << setprecision(3) << table[i][j];
        cout << endl;
    }
}

void fillWithData(double **data, int dataSize, int margin) {

    int marginValue = 1;
    int bulkValue = 10;

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

int main(int argc, char *argv[]) {

    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    const int MARGIN = 2;
    const int DATA_SIZE = 12;
    const int REPETITIONS = 4;

    int dataPortion = calcDataPortion(MARGIN);
    DataProcessor *dp = new MPIDataProcessor();
    dp->setMargin(MARGIN);

    if (rank == 0) {

        double **initialData = new double *[DATA_SIZE];
        for (int i = 0; i < DATA_SIZE; i++)
            initialData[i] = new double[DATA_SIZE];

        fillWithData(initialData, DATA_SIZE, MARGIN);
        showTable(initialData, DATA_SIZE);

        dp->setInitialData(initialData, DATA_SIZE);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    dp->execute(REPETITIONS);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    if (rank == 0) {
        double **result = dp->getResult();
        showTable(result, DATA_SIZE);
    }

    MPI_Finalize();

    return 0;
}
