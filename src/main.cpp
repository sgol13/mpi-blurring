#include "ImageBlurrer.hpp"
#include "mpi.h"
#include "pnm.hpp"
#include <iomanip>
#include <iostream>
#include <math.h>
#include <thread>

using namespace std;
using namespace pnm::literals;

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

        // for (std::size_t y = 0; y < ppm.y_size(); y += 3) {
        //     for (std::size_t x = 0; x < ppm.x_size(); ++x) {
        //         // cout << (double)ppm[y][x].value << "\n";
        //         ppm[y][x] = 0_gray;
        //     }
        // }

        // pnm::write("out.ppm", ppm, pnm::format::binary);

        image_blurrer.setImage(raw_image, size_R, size_C);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    image_blurrer.execute(5, 3);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));


    if (rank == 0) {


        for (int r = 0; r < size_R; r++) {
            for (int c = 0; c < size_C; c++) {
                // raw_image[r][c] = static_cast<double>(ppm[r][c].value);
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

    // =======================================
    // MPI_Init(&argc, &argv);
    // int rank;
    // MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // const int MARGIN = 2;
    // const int DATA_SIZE = 12;
    // const int REPETITIONS = 4;

    // ImageBlurrer *dp = new ImageBlurrer();

    // if (rank == 0) {

    //     double **initialData = new double *[DATA_SIZE];
    //     for (int i = 0; i < DATA_SIZE; i++)
    //         initialData[i] = new double[DATA_SIZE];

    //     fillWithData(initialData, DATA_SIZE, MARGIN);
    //     showTable(initialData, DATA_SIZE);

    //     dp->setImage(initialData, DATA_SIZE, DATA_SIZE);
    // }

    // std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // dp->execute(REPETITIONS, MARGIN);

    // std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // if (rank == 0) {
    //     double **result = dp->getImage();
    //     showTable(result, DATA_SIZE);
    // }

    // MPI_Finalize();

    return 0;
}
