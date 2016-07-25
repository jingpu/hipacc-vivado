#define HIPACC_MAX_WIDTH     1024
#define HIPACC_MAX_HEIGHT    1024
#define HIPACC_WINDOW_SIZE_X 3
#define HIPACC_WINDOW_SIZE_Y 3
#define BORDER_FILL_VALUE    0
#define HIPACC_II_TARGET     1
#define HIPACC_PPT           1

#include "hipacc_vivado_types.hpp"
#include "hipacc_vivado_filter.hpp"

#include "ccSobelDerivX.cc"
#include "ccGaussGaussY.cc"
#include "ccHarrisCornerHC.cc"
#include "ccSquare1SquareY.cc"
#include "ccSquare1SquareX.cc"
#include "ccGaussGaussXY.cc"
#include "ccGaussGaussX.cc"
#include "ccSquare2SquareXY.cc"
#include "ccSobelDerivY.cc"

void hipaccRun(hls::stream<uchar > &_strmOut0, hls::stream<uchar > &_strmIN, float k, float threshold) {
#pragma HLS dataflow
  hls::stream<uchar > _strmTmp11;
  hls::stream<uchar > _strmTmp13;
  splitStream<HIPACC_II_TARGET,HIPACC_MAX_WIDTH,HIPACC_MAX_HEIGHT,HIPACC_WINDOW_SIZE_X,HIPACC_WINDOW_SIZE_Y>(_strmIN, _strmTmp11, _strmTmp13, HIPACC_MAX_WIDTH, HIPACC_MAX_HEIGHT);
  hls::stream<float > _strmTmp6;
  ccSobelDerivYKernel(_strmTmp6, _strmTmp13, HIPACC_MAX_WIDTH, HIPACC_MAX_HEIGHT);
  hls::stream<float > _strmTmp7;
  hls::stream<float > _strmTmp12;
  splitStream<HIPACC_II_TARGET,HIPACC_MAX_WIDTH,HIPACC_MAX_HEIGHT,HIPACC_WINDOW_SIZE_X,HIPACC_WINDOW_SIZE_Y>(_strmTmp6, _strmTmp7, _strmTmp12, HIPACC_MAX_WIDTH, HIPACC_MAX_HEIGHT);
  hls::stream<float > _strmTmp2;
  ccSobelDerivXKernel(_strmTmp2, _strmTmp11, HIPACC_MAX_WIDTH, HIPACC_MAX_HEIGHT);
  hls::stream<float > _strmTmp3;
  hls::stream<float > _strmTmp10;
  splitStream<HIPACC_II_TARGET,HIPACC_MAX_WIDTH,HIPACC_MAX_HEIGHT,HIPACC_WINDOW_SIZE_X,HIPACC_WINDOW_SIZE_Y>(_strmTmp2, _strmTmp3, _strmTmp10, HIPACC_MAX_WIDTH, HIPACC_MAX_HEIGHT);
  hls::stream<float > _strmTmp9;
  ccSquare2SquareXYKernel(_strmTmp9, _strmTmp10, _strmTmp12, HIPACC_MAX_WIDTH, HIPACC_MAX_HEIGHT);
  hls::stream<float > _strmTmp8;
  ccGaussGaussXYKernel(_strmTmp8, _strmTmp9, HIPACC_MAX_WIDTH, HIPACC_MAX_HEIGHT);
  hls::stream<float > _strmTmp5;
  ccSquare1SquareYKernel(_strmTmp5, _strmTmp7, HIPACC_MAX_WIDTH, HIPACC_MAX_HEIGHT);
  hls::stream<float > _strmTmp4;
  ccGaussGaussYKernel(_strmTmp4, _strmTmp5, HIPACC_MAX_WIDTH, HIPACC_MAX_HEIGHT);
  hls::stream<float > _strmTmp1;
  ccSquare1SquareXKernel(_strmTmp1, _strmTmp3, HIPACC_MAX_WIDTH, HIPACC_MAX_HEIGHT);
  hls::stream<float > _strmTmp0;
  ccGaussGaussXKernel(_strmTmp0, _strmTmp1, HIPACC_MAX_WIDTH, HIPACC_MAX_HEIGHT);
  ccHarrisCornerHCKernel(_strmOut0, _strmTmp0, _strmTmp4, _strmTmp8, k, threshold, HIPACC_MAX_WIDTH, HIPACC_MAX_HEIGHT);
}

