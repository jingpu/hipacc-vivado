#define HIPACC_MAX_WIDTH     1920
#define HIPACC_MAX_HEIGHT    1080
#define HIPACC_WINDOW_SIZE_X 5
#define HIPACC_WINDOW_SIZE_Y 5
#define BORDER_FILL_VALUE    0
#define HIPACC_II_TARGET     1
#define HIPACC_PPT           1

#include "hipacc_vivado_types.hpp"
#include "hipacc_vivado_filter.hpp"

/* Template class of Window */
template<int ROWS, int COLS, typename T>
class Window {
public:
  Window() {
#pragma HLS ARRAY_PARTITION variable=val dim=1 complete
#pragma HLS ARRAY_PARTITION variable=val dim=2 complete
  };
  T& operator ()(int row, int col) {
#pragma HLS inline
    assert(row >= 0 && row < ROWS && col >= 0 && col < COLS);
    return val[row][col];
  }

  T val[ROWS][COLS];
};

#include "ccFilter2DLFC.cc"

void hipaccRun(hls::stream<uchar4 > &_strmOut0, hls::stream<uchar4 > &_strmIN, Window<5, 5, uchar > mask) {
#pragma HLS INTERFACE s_axilite port=mask bundle=config
#pragma HLS dataflow
  ccFilter2DLFCKernel(_strmOut0, _strmIN, HIPACC_MAX_WIDTH, HIPACC_MAX_HEIGHT, mask);
}

