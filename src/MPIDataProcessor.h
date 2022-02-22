#include "DataProcessor.h"
#include "mpi.h"
#include <iostream>

using namespace std;

#ifndef MPIDATAPROCESSOR_H_
#define MPIDATAPROCESSOR_H_

#include "DataProcessor.h"

class MPIDataProcessor : public DataProcessor {
  public:
    MPIDataProcessor();
    virtual ~MPIDataProcessor();
    virtual double **getResult() override;

  protected:
    virtual void singleExecution() override;
    virtual void shareData() override;
    virtual void collectData() override;

  private:
    void sendMargins();
    void receiveMargins();

    void createDataPortion(int row, int col, double *buffer);

    int calcRowsNum(int procRank);
    int calcRowsNumWithMargins(int procRank);
    int calcFirstRow(int procRank);
    int calcLastRow(int procRank);

    void copyToBuffer(double *buffer, int firstRowCopy, int rowsNumCopy);
    void copyFromBuffer(double *buffer, int firstRowCopy, int rowsNumCopy);

    double *marginBuffer;
    double *localBuffer;

    int rank;
    int procNum;
    int procRows;

    static constexpr int CONFIG_TAG = 0;
    static constexpr int FRAME_TAG = 1;
    static constexpr int LEFT_MARGIN_TAG = 2;
    static constexpr int RIGHT_MARGIN_TAG = 3;
};

#endif
