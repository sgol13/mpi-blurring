#include "MPIDataProcessor.h"

MPIDataProcessor::MPIDataProcessor() {

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &procNum);

    procRows = dataSize / procNum;
}

MPIDataProcessor::~MPIDataProcessor() {}

double **MPIDataProcessor::getResult() { return data; }

void MPIDataProcessor::singleExecution() {

    if (rank % 2 == 0) {

        sendMargins();
        receiveMargins();

    } else {

        receiveMargins();
        sendMargins();
    }

    int procNumWithMargins = calcRowsNumWithMargins(rank);
    int lastRow = procNumWithMargins - margin - 1;
    for (int r = 0; r < procNumWithMargins; r++) {
        for (int c = 0; c < dataSize; c++) {

            if (c >= margin && c + margin < dataSize && r >= margin && r <= lastRow) {

                double sum = 0;
                for (int i = r - margin; i <= r + margin; i++) {
                    for (int j = c - margin; j <= c + margin; j++) {
                        sum += data[i][j];
                    }
                }

                nextData[r][c] = sum / ((2 * margin + 1) * (2 * margin + 1));

            } else {
                nextData[r][c] = data[r][c];
            }
        }
    }

    swap(data, nextData);
}

void MPIDataProcessor::shareData() {

    // SHARE CONFIGURATION
    if (rank == 0) {

        // send procNum and margin to all other processes
        int buffer[2] = {dataSize, margin};
        for (int dest = 1; dest < procNum; dest++) {
            MPI_Send(buffer, 2, MPI_INT, dest, CONFIG_TAG, MPI_COMM_WORLD);
        }

    } else {

        // receive procNum and margin from process 0
        int buffer[2];
        MPI_Status status;
        MPI_Recv(buffer, 2, MPI_INT, 0, CONFIG_TAG, MPI_COMM_WORLD, &status);
        dataSize = buffer[0];
        margin = buffer[1];
    }

    procRows = calcRowsNum(rank);

    // SHARE DATA FRAMES
    if (rank == 0) {

        nextData = new double *[dataSize];
        for (int i = 0; i < dataSize; i++)
            nextData[i] = new double[dataSize];

        // send data frames to all other processes
        for (int dest = 1; dest < procNum; dest++) {

            int sendRows = calcRowsNum(dest);
            int bufferSize = sendRows * dataSize;
            double *buffer = new double[bufferSize];

            copyToBuffer(buffer, calcFirstRow(dest), sendRows);

            MPI_Send(buffer, bufferSize, MPI_DOUBLE, dest, FRAME_TAG, MPI_COMM_WORLD);

            delete[] buffer;
        }

    } else {

        // receive data frame from process 0
        int bufferSize = procRows * dataSize;
        double *buffer = new double[bufferSize];

        int procRowsWithMargins = calcRowsNumWithMargins(rank);
        data = new double *[procRowsWithMargins];
        nextData = new double *[procRowsWithMargins];
        for (int r = 0; r < procRowsWithMargins; r++) {
            data[r] = new double[dataSize];
            nextData[r] = new double[dataSize];
        }

        MPI_Status status;
        MPI_Recv(buffer, bufferSize, MPI_DOUBLE, 0, FRAME_TAG, MPI_COMM_WORLD,
                 &status);

        copyFromBuffer(buffer, margin, procRows);

        delete[] buffer;
    }
}

void MPIDataProcessor::collectData() {

    if (rank == 0) {

        // receive data frames from all other processes
        for (int src = 1; src < procNum; src++) {

            int sendRows = calcRowsNum(src);
            int bufferSize = sendRows * dataSize;
            double *buffer = new double[bufferSize];

            MPI_Status status;
            MPI_Recv(buffer, bufferSize, MPI_DOUBLE, src, FRAME_TAG, MPI_COMM_WORLD,
                     &status);

            copyFromBuffer(buffer, calcFirstRow(src), sendRows);

            delete[] buffer;
        }

    } else {

        // send data frame to process 0
        int bufferSize = procRows * dataSize;
        double *buffer = new double[bufferSize];

        copyToBuffer(buffer, margin, procRows);

        MPI_Send(buffer, bufferSize, MPI_DOUBLE, 0, FRAME_TAG, MPI_COMM_WORLD);

        delete[] buffer;
    }
}

void MPIDataProcessor::sendMargins() {

    double *marginBuffer = new double[dataSize * margin];

    // LEFT
    if (rank != 0) {

        copyToBuffer(marginBuffer, margin, margin);
        MPI_Send(marginBuffer, margin * dataSize, MPI_DOUBLE, rank - 1,
                 LEFT_MARGIN_TAG, MPI_COMM_WORLD);
    }

    // RIGHT
    if (rank + 1 != procNum) {

        int firstRow = calcRowsNumWithMargins(rank) - 2 * margin;
        copyToBuffer(marginBuffer, firstRow, margin);
        MPI_Send(marginBuffer, margin * dataSize, MPI_DOUBLE, rank + 1,
                 RIGHT_MARGIN_TAG, MPI_COMM_WORLD);
    }

    delete[] marginBuffer;
}

void MPIDataProcessor::receiveMargins() {

    double *marginBuffer = new double[dataSize * margin];

    // LEFT
    if (rank != 0) {

        MPI_Status status;
        MPI_Recv(marginBuffer, margin * dataSize, MPI_DOUBLE, rank - 1,
                 RIGHT_MARGIN_TAG, MPI_COMM_WORLD, &status);

        copyFromBuffer(marginBuffer, 0, margin);
    }

    // RIGHT
    if (rank + 1 != procNum) {

        MPI_Status status;
        MPI_Recv(marginBuffer, margin * dataSize, MPI_DOUBLE, rank + 1,
                 LEFT_MARGIN_TAG, MPI_COMM_WORLD, &status);

        int firstRow = calcRowsNumWithMargins(rank) - margin;
        copyFromBuffer(marginBuffer, firstRow, margin);
    }

    delete[] marginBuffer;
}

void MPIDataProcessor::createDataPortion(int row, int col, double *buffer) {

    int counter = 0;
    for (int i = row - margin; i <= row + margin; i++)
        for (int j = col - margin; j <= col + margin; j++) {
            buffer[counter++] = data[i][j];
        }
}

int MPIDataProcessor::calcRowsNum(int procRank) {

    int result = dataSize / procNum;

    if (procRank + 1 == procNum)
        result += dataSize % procNum;

    return result;
}

int MPIDataProcessor::calcRowsNumWithMargins(int procRank) {

    int result = calcRowsNum(procRank);

    if (procRank > 0 && procRank + 1 < procNum)
        result += 2 * margin;
    else if (procNum > 1)
        result += margin;

    return result;
}

int MPIDataProcessor::calcFirstRow(int procRank) {

    return (dataSize / procNum) * procRank;
}

int MPIDataProcessor::calcLastRow(int procRank) {

    return calcFirstRow(procRank) + calcRowsNum(procRank) - 1;
}

void MPIDataProcessor::copyToBuffer(double *buffer, int firstRowCopy,
                                    int rowsNumCopy) {

    int lastRowCopy = firstRowCopy + rowsNumCopy - 1;
    int bufCounter = 0;
    for (int r = firstRowCopy; r <= lastRowCopy; r++)
        for (int c = 0; c < dataSize; c++)
            buffer[bufCounter++] = data[r][c];
}

void MPIDataProcessor::copyFromBuffer(double *buffer, int firstRowCopy,
                                      int rowsNumCopy) {

    int lastRowCopy = firstRowCopy + rowsNumCopy - 1;
    int bufCounter = 0;
    for (int r = firstRowCopy; r <= lastRowCopy; r++)
        for (int c = 0; c < dataSize; c++)
            data[r][c] = buffer[bufCounter++];
}