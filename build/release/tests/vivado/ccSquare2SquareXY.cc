#ifndef _CCSQUARE2SQUAREXY_CC_
#define _CCSQUARE2SQUAREXY_CC_


struct ccSquare2SquareXYKernelKernel {

  ccSquare2SquareXYKernelKernel() {
  }

  float operator()(float Input1, float Input2) {
    {
        return Input1 * Input2;
    }
}
};

void ccSquare2SquareXYKernel(hls::stream<float > &Output, hls::stream<float > &Input1, hls::stream<float > &Input2, int IS_width, int IS_height) {
    struct ccSquare2SquareXYKernelKernel kernel;
    processPixels2<HIPACC_II_TARGET,HIPACC_MAX_WIDTH,HIPACC_MAX_HEIGHT,3,3>(Input1, Input2, Output, IS_width, IS_height, kernel);
}

#endif //_CCSQUARE2SQUAREXY_CC_

