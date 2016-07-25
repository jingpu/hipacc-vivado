#ifndef _CCSQUARE1SQUAREX_CC_
#define _CCSQUARE1SQUAREX_CC_


struct ccSquare1SquareXKernelKernel {

  ccSquare1SquareXKernelKernel() {
  }

  float operator()(float Input) {
    {
        float in = Input;
        return in * in;
    }
}
};

void ccSquare1SquareXKernel(hls::stream<float > &Output, hls::stream<float > &Input, int IS_width, int IS_height) {
    struct ccSquare1SquareXKernelKernel kernel;
    processPixels<HIPACC_II_TARGET,HIPACC_MAX_WIDTH,HIPACC_MAX_HEIGHT,3,3>(Input, Output, IS_width, IS_height, kernel);
}

#endif //_CCSQUARE1SQUAREX_CC_

