#ifndef _CCHARRISCORNERHC_CC_
#define _CCHARRISCORNERHC_CC_


struct ccHarrisCornerHCKernelKernel {
  float k;
  float threshold;

  ccHarrisCornerHCKernelKernel(float k, float threshold) {
    this->k = k;
    this->threshold = threshold;
  }

  uchar operator()(float Dx, float Dy, float Dxy) {
    {
        float x = Dx;
        float y = Dy;
        float xy = Dxy;
        float R = 0;
        R = ((x * y) - (xy * xy)) - (k * (x + y) * (x + y));
        uchar out = 0;
        if (R > threshold)
            out = 1;
        return out;
    }
}
};

void ccHarrisCornerHCKernel(hls::stream<uchar > &Output, hls::stream<float > &Dx, hls::stream<float > &Dy, hls::stream<float > &Dxy, float k, float threshold, int IS_width, int IS_height) {
    struct ccHarrisCornerHCKernelKernel kernel(k, threshold);
    processPixels3<HIPACC_II_TARGET,HIPACC_MAX_WIDTH,HIPACC_MAX_HEIGHT,3,3>(Dx, Dy, Dxy, Output, IS_width, IS_height, kernel);
}

#endif //_CCHARRISCORNERHC_CC_

