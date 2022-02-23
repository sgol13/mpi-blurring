#include "ImageBlurrer.hpp"
#include "mpi.h"
#include "pnm.hpp"
#include <exception>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <thread>

using namespace std;

int main(int argc, char *argv[]) {

    MPI_Init(&argc, &argv);

    int my_rank, processes_number;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &processes_number);

    int error = 0;
    string input_filename, output_filename;
    int iterations;
    int margin;

    ImageBlurrer image_blurrer;

    pnm::image<pnm::gray_pixel> ppm_image;
    double **raw_image;
    int size_R, size_C;

    if (my_rank == 0) {

        if (argc == 5) {

            input_filename = argv[1];
            output_filename = argv[2];
            iterations = atoi(argv[3]);
            margin = atoi(argv[4]);

            if (iterations < 0 || margin < 0) {
                error = 1;
            }

            if (error == 0) {

                try {
                    ppm_image = pnm::read_pgm(input_filename);
                } catch (runtime_error exception) {
                    error = 2;
                }
            }

        } else {
            error = 1;
        }

        int buffer[3] = {error, iterations, margin};
        for (int destination = 1; destination < processes_number; destination++) {
            MPI_Send(&buffer, 3, MPI_INT, destination, 0, MPI_COMM_WORLD);
        }

        if (error == 1) {

            cerr << "ERROR, expected 4 arguments: FILENAME ITERATIONS "
                    "MARGIN\n";
            MPI_Finalize();
            return 1;
        } else if (error == 2) {

            cerr << "ERROR, cannot open file " << input_filename << "\n";
            MPI_Finalize();
            return 2;
        }

    } else {

        int buffer[3];
        MPI_Status status;
        MPI_Recv(&buffer, 3, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        error = buffer[0];
        iterations = buffer[1];
        margin = buffer[2];

        if (error) {
            MPI_Finalize();
            return error;
        }
    }


    if (my_rank == 0) {

        size_R = ppm_image.height();
        size_C = ppm_image.width();

        raw_image = new double *[size_R];
        for (int i = 0; i < size_R; i++) {
            raw_image[i] = new double[size_C];
        }

        for (int r = 0; r < size_R; r++) {
            for (int c = 0; c < size_C; c++) {
                raw_image[r][c] = static_cast<double>(ppm_image[r][c].value);
            }
        }

        image_blurrer.setImage(raw_image, size_R, size_C);
    }

    image_blurrer.execute(iterations, margin);

    if (my_rank == 0) {


        for (int r = 0; r < size_R; r++) {
            for (int c = 0; c < size_C; c++) {

                ppm_image[r][c] = static_cast<uint8_t>(raw_image[r][c]);
            }
        }

        try {
            pnm::write(output_filename, ppm_image, pnm::format::binary);
        } catch (runtime_error exception) {
            error = 3;
        }

        for (int i = 0; i < size_R; i++) {
            delete[] raw_image[i];
        }
        delete[] raw_image;

        for (int destination = 1; destination < processes_number; destination++) {
            MPI_Send(&error, 1, MPI_INT, destination, 0, MPI_COMM_WORLD);
        }

        if (error) {
            cerr << "ERROR, cannot save file " << output_filename << "\n";
            MPI_Finalize();
            return error;
        }

    } else {

        MPI_Status status;
        MPI_Recv(&error, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        if (error) {
            MPI_Finalize();
            return error;
        }
    }

    MPI_Finalize();

    return 0;
}
