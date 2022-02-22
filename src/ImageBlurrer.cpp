// MPI blurrer
// Szymon Gołębiowski

#include "ImageBlurrer.hpp"

ImageBlurrer::ImageBlurrer() {

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &processes_number);

    process_rows = size_R / processes_number;

    // cout << proc_rank << endl;
}

ImageBlurrer::~ImageBlurrer() {}

void ImageBlurrer::setImage(double **image, int size_R, int size_C) {

    this->image = image;
    this->size_R = size_R;
    this->size_C = size_C;
}

double **ImageBlurrer::getImage() { return image; }

int ImageBlurrer::getSizeX() { return size_R; }

int ImageBlurrer::getSizeY() { return size_C; }

void ImageBlurrer::execute(int iterations, int margin) {

    this->margin = margin;

    shareData();

    for (int i = 0; i < iterations; i++) {
        singleExecution();
    }

    collectData();
}

void ImageBlurrer::singleExecution() {

    if (my_rank % 2 == 0) {

        sendMargins();
        receiveMargins();

    } else {

        receiveMargins();
        sendMargins();
    }

    int procNumWithMargins = calcRowsNumWithMargins(my_rank);
    int lastRow = procNumWithMargins - margin - 1;
    for (int r = 0; r < procNumWithMargins; r++) {
        for (int c = 0; c < size_C; c++) {

            if (c >= margin && c + margin < size_C && r >= margin && r <= lastRow) {

                double sum = 0;
                for (int i = r - margin; i <= r + margin; i++) {
                    for (int j = c - margin; j <= c + margin; j++) {
                        sum += image[i][j];
                    }
                }

                next_image[r][c] = sum / ((2 * margin + 1) * (2 * margin + 1));

            } else {
                next_image[r][c] = image[r][c];
            }
        }
    }

    swap(image, next_image);
}

void ImageBlurrer::shareData() {

    // SHARE CONFIGURATION
    if (my_rank == 0) {

        // send procNum and margin to all other processes
        int buffer[3] = {size_R, size_C, margin};
        for (int dest = 1; dest < processes_number; dest++) {
            MPI_Send(buffer, 3, MPI_INT, dest, CONFIG_TAG, MPI_COMM_WORLD);
        }

    } else {

        // receive procNum and margin from process 0
        int buffer[3];
        MPI_Status status;
        MPI_Recv(buffer, 3, MPI_INT, 0, CONFIG_TAG, MPI_COMM_WORLD, &status);
        size_R = buffer[0];
        size_C = buffer[1];
        margin = buffer[2];
    }

    process_rows = calcRowsNum(my_rank);

    // SHARE DATA FRAMES
    if (my_rank == 0) {

        next_image = new double *[size_R];
        for (int i = 0; i < size_R; i++)
            next_image[i] = new double[size_C];

        // send image frames to all other processes
        for (int dest = 1; dest < processes_number; dest++) {

            int sendRows = calcRowsNum(dest);
            int bufferSize = sendRows * size_C;
            double *buffer = new double[bufferSize];

            copyToBuffer(buffer, calcFirstRow(dest), sendRows);

            MPI_Send(buffer, bufferSize, MPI_DOUBLE, dest, FRAME_TAG, MPI_COMM_WORLD);

            delete[] buffer;
        }

    } else {

        // receive image frame from process 0
        int bufferSize = process_rows * size_C;
        double *buffer = new double[bufferSize];

        int procRowsWithMargins = calcRowsNumWithMargins(my_rank);
        image = new double *[procRowsWithMargins];
        next_image = new double *[procRowsWithMargins];
        for (int r = 0; r < procRowsWithMargins; r++) {
            image[r] = new double[size_C];
            next_image[r] = new double[size_C];
        }

        MPI_Status status;
        MPI_Recv(buffer, bufferSize, MPI_DOUBLE, 0, FRAME_TAG, MPI_COMM_WORLD,
                 &status);

        copyFromBuffer(buffer, margin, process_rows);

        delete[] buffer;
    }
}

void ImageBlurrer::collectData() {

    if (my_rank == 0) {

        // receive image frames from all other processes
        for (int src = 1; src < processes_number; src++) {

            int sendRows = calcRowsNum(src);
            int bufferSize = sendRows * size_C;
            double *buffer = new double[bufferSize];

            MPI_Status status;
            MPI_Recv(buffer, bufferSize, MPI_DOUBLE, src, FRAME_TAG, MPI_COMM_WORLD,
                     &status);

            copyFromBuffer(buffer, calcFirstRow(src), sendRows);

            delete[] buffer;
        }

    } else {

        // send image frame to process 0
        int bufferSize = process_rows * size_C;
        double *buffer = new double[bufferSize];

        copyToBuffer(buffer, margin, process_rows);

        MPI_Send(buffer, bufferSize, MPI_DOUBLE, 0, FRAME_TAG, MPI_COMM_WORLD);

        delete[] buffer;
    }
}

void ImageBlurrer::sendMargins() {

    double *margin_buffer = new double[size_C * margin];

    // LEFT
    if (my_rank != 0) {

        copyToBuffer(margin_buffer, margin, margin);
        MPI_Send(margin_buffer, margin * size_C, MPI_DOUBLE, my_rank - 1,
                 LEFT_MARGIN_TAG, MPI_COMM_WORLD);
    }

    // RIGHT
    if (my_rank + 1 != processes_number) {

        int firstRow = calcRowsNumWithMargins(my_rank) - 2 * margin;
        copyToBuffer(margin_buffer, firstRow, margin);
        MPI_Send(margin_buffer, margin * size_C, MPI_DOUBLE, my_rank + 1,
                 RIGHT_MARGIN_TAG, MPI_COMM_WORLD);
    }

    delete[] margin_buffer;
}

void ImageBlurrer::receiveMargins() {

    double *margin_buffer = new double[size_C * margin];

    // LEFT
    if (my_rank != 0) {

        MPI_Status status;
        MPI_Recv(margin_buffer, margin * size_C, MPI_DOUBLE, my_rank - 1,
                 RIGHT_MARGIN_TAG, MPI_COMM_WORLD, &status);

        copyFromBuffer(margin_buffer, 0, margin);
    }

    // RIGHT
    if (my_rank + 1 != processes_number) {

        MPI_Status status;
        MPI_Recv(margin_buffer, margin * size_C, MPI_DOUBLE, my_rank + 1,
                 LEFT_MARGIN_TAG, MPI_COMM_WORLD, &status);

        int firstRow = calcRowsNumWithMargins(my_rank) - margin;
        copyFromBuffer(margin_buffer, firstRow, margin);
    }

    delete[] margin_buffer;
}

void ImageBlurrer::copyToBuffer(double *buffer, int firstRowCopy, int rowsNumCopy) {

    int lastRowCopy = firstRowCopy + rowsNumCopy - 1;
    int bufCounter = 0;
    for (int r = firstRowCopy; r <= lastRowCopy; r++)
        for (int c = 0; c < size_C; c++)
            buffer[bufCounter++] = image[r][c];
}

void ImageBlurrer::copyFromBuffer(double *buffer, int firstRowCopy, int rowsNumCopy) {

    int lastRowCopy = firstRowCopy + rowsNumCopy - 1;
    int bufCounter = 0;
    for (int r = firstRowCopy; r <= lastRowCopy; r++)
        for (int c = 0; c < size_C; c++)
            image[r][c] = buffer[bufCounter++];
}

int ImageBlurrer::calcRowsNum(int procRank) {

    int result = size_R / processes_number;

    if (procRank + 1 == processes_number)
        result += size_R % processes_number;

    return result;
}

int ImageBlurrer::calcRowsNumWithMargins(int procRank) {

    int result = calcRowsNum(procRank);

    if (procRank > 0 && procRank + 1 < processes_number)
        result += 2 * margin;
    else if (processes_number > 1)
        result += margin;

    return result;
}

int ImageBlurrer::calcFirstRow(int procRank) {

    return (size_R / processes_number) * procRank;
}

int ImageBlurrer::calcLastRow(int procRank) {

    return calcFirstRow(procRank) + calcRowsNum(procRank) - 1;
}