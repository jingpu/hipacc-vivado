#ifndef _CCFILTER2DLFC_CC_
#define _CCFILTER2DLFC_CC_


struct ccFilter2DLFCKernelKernel {
  Window<5, 5, uchar > mask;

  ccFilter2DLFCKernelKernel(Window<5, 5, uchar > &m)
    : mask(m) {
#pragma HLS ARRAY_PARTITION variable=&mask complete dim=0
  }

  uchar4 operator()(uchar4 Input[5][5]) {
    {
#pragma HLS ARRAY_PARTITION variable=&mask complete dim=0
        ushort4 _tmp0 = { 0, 0, 0, 0 };
        for (int i = 0; i < 5; i++)
          for (int j = 0; j < 5; j++) {
            _tmp0 += convert_ushort4(getWindowAt(Input, i, j)) * ((ushort) mask(i, j));
          }
        uchar4 sum =  convert_uchar4(_tmp0 >> 8);
          //return convert_ushort4(_tmp0, true);
        return sum;
    }
}
};

void ccFilter2DLFCKernel(hls::stream<uchar4 > &Output, hls::stream<uchar4 > &Input, int IS_width, int IS_height, Window<5, 5, uchar > &mask) {
#pragma HLS ARRAY_PARTITION variable=&mask complete dim=0
  struct ccFilter2DLFCKernelKernel kernel(mask);
    process<HIPACC_II_TARGET,HIPACC_MAX_WIDTH,HIPACC_MAX_HEIGHT,5,5>(Input, Output, IS_width, IS_height, kernel, BorderPadding::BORDER_CLAMP);
}

#endif //_CCFILTER2DLFC_CC_

