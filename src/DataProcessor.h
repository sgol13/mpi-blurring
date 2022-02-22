/*
 * DataProcessor.h
 *
 *  Created on: 20 paź 2021
 *      Author: oramus
 */

#ifndef DATAPROCESSOR_H_
#define DATAPROCESSOR_H_

class DataProcessor {
  private:
    int round_near(double b);

  protected:
    int margin;
    double **data;
    double **nextData;
    int dataSize;

    virtual void singleExecution() = 0;
    virtual void shareData() = 0;
    virtual void collectData() = 0;

  public:
    DataProcessor();
    virtual ~DataProcessor();
    virtual double **getResult() = 0;
    void setInitialData(double **data, int size);
    void execute(int repetitions);
    void setMargin(int margin);
};

#endif /* DATAPROCESSOR_H_ */
