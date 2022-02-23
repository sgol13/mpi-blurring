// MPI blurrer
// Szymon Gołębiowski

#ifndef IMAGE_BLURRER_HPP
#define IMAGE_BLURRER_HPP

#include "mpi.h"
#include <iostream>

using namespace std;

class ImageBlurrer {
  public:
    ImageBlurrer();
    ~ImageBlurrer();

    void setImage(double **image, int size_R, int size_C);
    double **getImage();
    int getSizeX();
    int getSizeY();
    void execute(int iterations, int margin);

  private:
    int margin;
    double **image;
    double **next_image;
    int size_R, size_C;

    double *margin_buffer;

    int my_rank;
    int processes_number;
    int process_rows;

    void shareData();
    void collectData(bool swap_images);
    void singleIteration();
    double blurPixel(int r, int c);

    void sendMargins();
    void receiveMargins();

    void copyToBuffer(double *buffer, int first_row_copy, int rows_num_copy);
    void copyFromBuffer(double *buffer, int first_row_copy, int rows_num_copy);

    int calcRowsNum(int process_rank);
    int calcRowsNumWithMargins(int process_rank);
    int calcFirstRow(int process_rank);
    int calcLastRow(int process_rank);

    static constexpr int CONFIG_TAG = 0;
    static constexpr int FRAME_TAG = 1;
    static constexpr int LEFT_MARGIN_TAG = 2;
    static constexpr int RIGHT_MARGIN_TAG = 3;
};

#endif
