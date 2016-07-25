#ifndef _CCGAUSSGAUSSY_CC_
#define _CCGAUSSGAUSSY_CC_


struct ccGaussGaussYKernelKernel {

  ccGaussGaussYKernelKernel() {
  }

  float operator()(float Input[3][3]) {
    {
        float sum = 0.F;
        float _tmp0 = 0.F;
        {
            _tmp0 += getWindowAt(Input, 0, 0) * 0.0571179986F;
        }
        {
            _tmp0 += getWindowAt(Input, 1, 0) * 0.124757998F;
        }
        {
            _tmp0 += getWindowAt(Input, 2, 0) * 0.0571179986F;
        }
        {
            _tmp0 += getWindowAt(Input, 0, 1) * 0.124757998F;
        }
        {
            _tmp0 += getWindowAt(Input, 1, 1) * 0.272496015F;
        }
        {
            _tmp0 += getWindowAt(Input, 2, 1) * 0.124757998F;
        }
        {
            _tmp0 += getWindowAt(Input, 0, 2) * 0.0571179986F;
        }
        {
            _tmp0 += getWindowAt(Input, 1, 2) * 0.124757998F;
        }
        {
            _tmp0 += getWindowAt(Input, 2, 2) * 0.0571179986F;
        }
        sum += _tmp0;
        return sum;
    }
}
};

void ccGaussGaussYKernel(hls::stream<float > &Output, hls::stream<float > &Input, int IS_width, int IS_height) {
    struct ccGaussGaussYKernelKernel kernel;
    process<HIPACC_II_TARGET,HIPACC_MAX_WIDTH,HIPACC_MAX_HEIGHT,3,3>(Input, Output, IS_width, IS_height, kernel, BorderPadding::BORDER_CLAMP);
}

#endif //_CCGAUSSGAUSSY_CC_

