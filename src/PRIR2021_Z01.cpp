#include "Alloc.h"
#include "ArithmeticMeanFunction.h"
#include "DataProcessor.h"
#include "MPIDataProcessor.h"
#include "MagicFuntion.h"
#include "SequentialDataProcessor.h"
#include "SimpleInitialDataGenerator.h"
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

int main(int argc, char *argv[]) {

    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    const int MARGIN = 2;
    const int DATA_SIZE = 14;
    const int REPETITIONS = 4;

    int dataPortion = calcDataPortion(MARGIN);
    MagicFuntion *mf = new ArithmeticMeanFunction(dataPortion);
    // DataProcessor *dp = new SequentialDataProcessor();
    DataProcessor *dp = new MPIDataProcessor();

    dp->setMagicFunction(mf);

    if (rank == 0) {
        double **initialData = tableAlloc(DATA_SIZE);
        InitialDataGenerator *generator = new SimpleInitialDataGenerator(1, 10);
        generator->fillWithData(initialData, DATA_SIZE, MARGIN);
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
