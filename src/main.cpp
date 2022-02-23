#include "ImageBlurrer.hpp"
#include "mpi.h"
#include "pnm.hpp"
#include <iomanip>
#include <iostream>
#include <math.h>
#include <thread>

using namespace std;

int main(int argc, char *argv[]) {

    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ImageBlurrer image_blurrer;

    pnm::image<pnm::gray_pixel> ppm;
    double **raw_image;
    int size_R, size_C;
    if (rank == 0) {

        ppm = pnm::read_pgm("e1.pgm");

        std::cout << "width  = " << ppm.width() << " " << ppm.x_size() << std::endl;
        std::cout << "height = " << ppm.height() << " " << ppm.y_size() << std::endl;

        size_R = ppm.height();
        size_C = ppm.width();

        raw_image = new double *[size_R];
        for (int i = 0; i < size_R; i++) {
            raw_image[i] = new double[size_C];
        }

        for (int r = 0; r < size_R; r++) {
            for (int c = 0; c < size_C; c++) {
                raw_image[r][c] = static_cast<double>(ppm[r][c].value);
            }
        }

        image_blurrer.setImage(raw_image, size_R, size_C);
    }

    image_blurrer.execute(5, 3);

    if (rank == 0) {


        for (int r = 0; r < size_R; r++) {
            for (int c = 0; c < size_C; c++) {

                ppm[r][c] = static_cast<uint8_t>(raw_image[r][c]);
            }
        }

        pnm::write("out.ppm", ppm, pnm::format::binary);

        for (int i = 0; i < size_R; i++) {
            delete[] raw_image[i];
        }
        delete[] raw_image;
    }

    MPI_Finalize();

    return 0;
}
