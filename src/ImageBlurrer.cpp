// MPI blurrer
// Szymon Golebiowski

#include "ImageBlurrer.hpp"

ImageBlurrer::ImageBlurrer() {

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &processes_number);

    process_rows = size_R / processes_number;
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
        singleIteration();
    }

    collectData(iterations % 2);
}

void ImageBlurrer::singleIteration() {

    if (my_rank % 2 == 0) {

        sendMargins();
        receiveMargins();

    } else {

        receiveMargins();
        sendMargins();
    }

    int rows_with_margins = calcRowsNumWithMargins(my_rank);
    int last_row = rows_with_margins - margin - 1;
    for (int r = 0; r < rows_with_margins; r++) {
        for (int c = 0; c < size_C; c++) {

            if (c >= margin && c + margin < size_C && r >= margin && r <= last_row) {
                next_image[r][c] = blurPixel(r, c);

            } else {
                next_image[r][c] = image[r][c];
            }
        }
    }

    swap(image, next_image);
}

double ImageBlurrer::blurPixel(int r, int c) {

    double sum = 0;
    for (int i = r - margin; i <= r + margin; i++) {
        for (int j = c - margin; j <= c + margin; j++) {
            sum += image[i][j];
        }
    }

    int num_pixels = 2 * margin + 1;
    num_pixels *= num_pixels;

    return sum / num_pixels;
}

void ImageBlurrer::shareData() {

    // SHARE CONFIGURATION
    if (my_rank == 0) {

        // send procNum and margin to all other processes
        int buffer[3] = {size_R, size_C, margin};
        for (int destination = 1; destination < processes_number; destination++) {
            MPI_Send(buffer, 3, MPI_INT, destination, CONFIG_TAG, MPI_COMM_WORLD);
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
        for (int i = 0; i < size_R; i++) {
            next_image[i] = new double[size_C];
        }

        // send image frames to all other processes
        for (int destination = 1; destination < processes_number; destination++) {

            int send_rows = calcRowsNum(destination);
            int buffer_size = send_rows * size_C;
            double *buffer = new double[buffer_size];

            copyToBuffer(buffer, calcFirstRow(destination), send_rows);

            MPI_Send(buffer, buffer_size, MPI_DOUBLE, destination, FRAME_TAG,
                     MPI_COMM_WORLD);

            delete[] buffer;
        }

    } else {

        // receive image frame from process 0
        int buffer_size = process_rows * size_C;
        double *buffer = new double[buffer_size];

        int rows_with_margins = calcRowsNumWithMargins(my_rank);
        image = new double *[rows_with_margins];
        next_image = new double *[rows_with_margins];

        for (int r = 0; r < rows_with_margins; r++) {
            image[r] = new double[size_C];
            next_image[r] = new double[size_C];
        }

        MPI_Status status;
        MPI_Recv(buffer, buffer_size, MPI_DOUBLE, 0, FRAME_TAG, MPI_COMM_WORLD,
                 &status);

        copyFromBuffer(buffer, margin, process_rows);

        delete[] buffer;
    }
}

void ImageBlurrer::collectData(bool swap_images) {

    if (my_rank == 0) {

        // receive image frames from all other processes
        for (int src = 1; src < processes_number; src++) {

            int send_rows = calcRowsNum(src);
            int buffer_size = send_rows * size_C;
            double *buffer = new double[buffer_size];

            MPI_Status status;
            MPI_Recv(buffer, buffer_size, MPI_DOUBLE, src, FRAME_TAG, MPI_COMM_WORLD,
                     &status);

            copyFromBuffer(buffer, calcFirstRow(src), send_rows);

            delete[] buffer;
        }

        if (swap_images) {

            for (int r = 0; r < size_R; r++) {
                for (int c = 0; c < size_C; c++) {
                    next_image[r][c] = image[r][c];
                }
            }

            swap(image, next_image);
        }

        for (int r = 0; r < size_R; r++) {
            delete[] next_image[r];
        }
        delete[] next_image;

    } else {

        // send image frame to process 0
        int buffer_size = process_rows * size_C;
        double *buffer = new double[buffer_size];

        copyToBuffer(buffer, margin, process_rows);

        MPI_Send(buffer, buffer_size, MPI_DOUBLE, 0, FRAME_TAG, MPI_COMM_WORLD);

        delete[] buffer;

        int rows_with_margins = calcRowsNumWithMargins(my_rank);
        for (int r = 0; r < rows_with_margins; r++) {
            delete[] image[r];
            delete[] next_image[r];
        }
        delete[] image;
        delete[] next_image;
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

        int first_row = calcRowsNumWithMargins(my_rank) - 2 * margin;
        copyToBuffer(margin_buffer, first_row, margin);

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

        int first_row = calcRowsNumWithMargins(my_rank) - margin;
        copyFromBuffer(margin_buffer, first_row, margin);
    }

    delete[] margin_buffer;
}

void ImageBlurrer::copyToBuffer(double *buffer, int first_row_copy,
                                int rows_num_copy) {

    int last_row_copy = first_row_copy + rows_num_copy - 1;
    int buffer_counter = 0;

    for (int r = first_row_copy; r <= last_row_copy; r++) {
        for (int c = 0; c < size_C; c++) {
            buffer[buffer_counter++] = image[r][c];
        }
    }
}

void ImageBlurrer::copyFromBuffer(double *buffer, int first_row_copy,
                                  int rows_num_copy) {

    int last_row_copy = first_row_copy + rows_num_copy - 1;
    int buffer_counter = 0;

    for (int r = first_row_copy; r <= last_row_copy; r++) {
        for (int c = 0; c < size_C; c++) {
            image[r][c] = buffer[buffer_counter++];
        }
    }
}

int ImageBlurrer::calcRowsNum(int process_rank) {

    int result = size_R / processes_number;

    if (process_rank + 1 == processes_number) {
        result += size_R % processes_number;
    }

    return result;
}

int ImageBlurrer::calcRowsNumWithMargins(int process_rank) {

    int result = calcRowsNum(process_rank);

    if (process_rank > 0 && process_rank + 1 < processes_number) {
        result += 2 * margin;

    } else if (processes_number > 1) {
        result += margin;
    }

    return result;
}

int ImageBlurrer::calcFirstRow(int process_rank) {

    return (size_R / processes_number) * process_rank;
}

int ImageBlurrer::calcLastRow(int process_rank) {

    return calcFirstRow(process_rank) + calcRowsNum(process_rank) - 1;
}