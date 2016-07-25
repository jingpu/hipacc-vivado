#ifndef _CCLAPLACEFILTERLFC_CC_
#define _CCLAPLACEFILTERLFC_CC_


struct ccLaplaceFilterLFCKernelKernel {

  ccLaplaceFilterLFCKernelKernel() {
  }

  ap_uint<32> operator()(uchar4 Input[3][3]) {
    {
        int4 _tmp0 = { 0, 0, 0, 0 };
        {
            _tmp0 += convert_int4(getWindowAt(Input, 0, 0));
        }
        {
            _tmp0 += convert_int4(getWindowAt(Input, 1, 0));
        }
        {
            _tmp0 += convert_int4(getWindowAt(Input, 2, 0));
        }
        {
            _tmp0 += convert_int4(getWindowAt(Input, 0, 1));
        }
        {
            _tmp0 += -8 * convert_int4(getWindowAt(Input, 1, 1));
        }
        {
            _tmp0 += convert_int4(getWindowAt(Input, 2, 1));
        }
        {
            _tmp0 += convert_int4(getWindowAt(Input, 0, 2));
        }
        {
            _tmp0 += convert_int4(getWindowAt(Input, 1, 2));
        }
        {
            _tmp0 += convert_int4(getWindowAt(Input, 2, 2));
        }
        int4 sum = _tmp0;
        sum = min(sum, 255);
        sum = max(sum, 0);
        return convert_uchar4(sum, true);
    }
}
};

void ccLaplaceFilterLFCKernel(hls::stream<ap_uint<32> > &Output, hls::stream<ap_uint<32> > &Input, int IS_width, int IS_height) {
    struct ccLaplaceFilterLFCKernelKernel kernel;
    processVECT<HIPACC_II_TARGET,HIPACC_MAX_WIDTH,HIPACC_MAX_HEIGHT,3,3,HIPACC_PPT,uchar4 >(Input, Output, IS_width, IS_height, kernel, BorderPadding::BORDER_CLAMP);
}

#endif //_CCLAPLACEFILTERLFC_CC_

