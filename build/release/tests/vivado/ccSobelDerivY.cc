#ifndef _CCSOBELDERIVY_CC_
#define _CCSOBELDERIVY_CC_


struct ccSobelDerivYKernelKernel {

  ccSobelDerivYKernelKernel() {
  }

  float operator()(uchar Input[3][3]) {
    {
        float sum = 0.F;
        float _tmp0 = 0.F;
        {
            _tmp0 += getWindowAt(Input, 0, 0) * -0.166666672F;
        }
        {
            _tmp0 += getWindowAt(Input, 1, 0) * -0.166666672F;
        }
        {
            _tmp0 += getWindowAt(Input, 2, 0) * -0.166666672F;
        }
        {
            _tmp0 += getWindowAt(Input, 0, 2) * 0.166666672F;
        }
        {
            _tmp0 += getWindowAt(Input, 1, 2) * 0.166666672F;
        }
        {
            _tmp0 += getWindowAt(Input, 2, 2) * 0.166666672F;
        }
        sum += _tmp0;
        return sum;
    }
}
};

void ccSobelDerivYKernel(hls::stream<float > &Output, hls::stream<uchar > &Input, int IS_width, int IS_height) {
    struct ccSobelDerivYKernelKernel kernel;
    process<HIPACC_II_TARGET,HIPACC_MAX_WIDTH,HIPACC_MAX_HEIGHT,3,3>(Input, Output, IS_width, IS_height, kernel, BorderPadding::BORDER_CLAMP);
}

#endif //_CCSOBELDERIVY_CC_

