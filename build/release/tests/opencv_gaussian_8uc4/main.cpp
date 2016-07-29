//
// Copyright (c) 2013, University of Erlangen-Nuremberg
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include <sys/time.h>

//#define CPU
#ifdef OpenCV
#include <opencv2/opencv.hpp>
#ifndef CPU
#include <opencv2/gpu/gpu.hpp>
#endif
#endif

#include "hipacc.hpp"

// variables set by Makefile
//#define SIZE_X 5
//#define SIZE_Y 5
//#define WIDTH 4096
//#define HEIGHT 4096
#define CONST_MASK
#define USE_LAMBDA
//#define RUN_UNDEF
//#define NO_SEP

using namespace hipacc;


// get time in milliseconds
double time_ms () {
    struct timeval tv;
    gettimeofday (&tv, NULL);

    return ((double)(tv.tv_sec) * 1e+3 + (double)(tv.tv_usec) * 1e-3);
}


// Gaussian blur filter reference
void gaussian_filter(uchar4 *in, uchar4 *out, float *filter, int size_x, int
        size_y, int width, int height) {
    int anchor_x = size_x >> 1;
    int anchor_y = size_y >> 1;
    #ifdef OpenCV
    int upper_x = width-size_x+anchor_x;
    int upper_y = height-size_y+anchor_y;
    #else
    int upper_x = width-anchor_x;
    int upper_y = height-anchor_y;
    #endif

    for (int y=anchor_y; y<upper_y; ++y) {
        for (int x=anchor_x; x<upper_x; ++x) {
            float4 sum = { 0.5f, 0.5f, 0.5f, 0.5f };

            for (int yf = -anchor_y; yf<=anchor_y; yf++) {
                for (int xf = -anchor_x; xf<=anchor_x; xf++) {
                    sum += filter[(yf+anchor_y)*size_x + xf+anchor_x] *
                           convert_float4(in[(y+yf)*width + x + xf]);
                }
            }
            out[y*width + x] = convert_uchar4(sum);
        }
    }
}
void gaussian_filter_row(uchar4 *in, float4 *out, float *filter, int size_x, int
        width, int height) {
    int anchor_x = size_x >> 1;
    #ifdef OpenCV
    int upper_x = width-size_x+anchor_x;
    #else
    int upper_x = width-anchor_x;
    #endif

    for (int y=0; y<height; ++y) {
        for (int x=anchor_x; x<upper_x; ++x) {
            float4 sum = { 0.0f, 0.0f, 0.0f, 0.0f };

            for (int xf = -anchor_x; xf<=anchor_x; xf++) {
                sum += filter[xf+anchor_x] *
                       convert_float4(in[(y)*width + x + xf]);
            }
            out[y*width + x] = sum;
        }
    }
}
void gaussian_filter_column(float4 *in, uchar4 *out, float *filter, int size_y,
        int width, int height) {
    int anchor_y = size_y >> 1;
    #ifdef OpenCV
    int upper_y = height-size_y+anchor_y;
    #else
    int upper_y = height-anchor_y;
    #endif

    for (int y=anchor_y; y<upper_y; ++y) {
        for (int x=0; x<width; ++x) {
            float4 sum = { 0.5f, 0.5f, 0.5f, 0.5f };

            for (int yf = -anchor_y; yf<=anchor_y; yf++) {
                sum += filter[yf + anchor_y] * in[(y + yf)*width + x];
            }
            out[y*width + x] = convert_uchar4(sum);
        }
    }
}


// Gaussian blur filter in Hipacc
#ifdef NO_SEP
class GaussianBlurFilterMask : public Kernel<uchar4> {
    private:
        Accessor<uchar4> &input;
        Mask<float> &mask;
        const int size_x, size_y;

    public:
        GaussianBlurFilterMask(IterationSpace<uchar4> &iter, Accessor<uchar4>
                &input, Mask<float> &mask, const int size_x, const int size_y) :
            Kernel(iter),
            input(input),
            mask(mask),
            size_x(size_x),
            size_y(size_y)
        { add_accessor(&input); }

        #ifdef USE_LAMBDA
        void kernel() {
            output() = convert_uchar4(convolve(mask, Reduce::SUM, [&] () -> float4 {
                    return mask() * convert_float4(input(mask));
                    }) + 0.5f);
        }
        #else
        void kernel() {
            const int anchor_x = size_x >> 1;
            const int anchor_y = size_y >> 1;
            float4 sum = { 0.5f, 0.5f, 0.5f, 0.5f };

            for (int yf = -anchor_y; yf<=anchor_y; yf++) {
                for (int xf = -anchor_x; xf<=anchor_x; xf++) {
                    sum += mask(xf, yf) * convert_float4(input(xf, yf));
                }
            }

            output() = convert_uchar4(sum);
        }
        #endif
};
#else
class GaussianBlurFilterMaskRow : public Kernel<float4> {
    private:
        Accessor<uchar4> &input;
        Mask<float> &mask;
        const int size;

    public:
        GaussianBlurFilterMaskRow(IterationSpace<float4> &iter, Accessor<uchar4>
                &input, Mask<float> &mask, const int size) :
            Kernel(iter),
            input(input),
            mask(mask),
            size(size)
        { add_accessor(&input); }

        #ifdef USE_LAMBDA
        void kernel() {
            output() = convolve(mask, Reduce::SUM, [&] () -> float4 {
                    return mask() * convert_float4(input(mask));
                    });
        }
        #else
        void kernel() {
            const int anchor = size >> 1;
            float4 sum = { 0.0f, 0.0f, 0.0f, 0.0f };

            for (int xf = -anchor; xf<=anchor; xf++) {
                sum += mask(xf, 0) * convert_float4(input(xf, 0));
            }

            output() = sum;
        }
        #endif
};
class GaussianBlurFilterMaskColumn : public Kernel<uchar4> {
    private:
        Accessor<float4> &input;
        Mask<float> &mask;
        const int size;

    public:
        GaussianBlurFilterMaskColumn(IterationSpace<uchar4> &iter,
                Accessor<float4> &input, Mask<float> &mask, const int size) :
            Kernel(iter),
            input(input),
            mask(mask),
            size(size)
        { add_accessor(&input); }

        #ifdef USE_LAMBDA
        void kernel() {
            output() = convert_uchar4(convolve(mask, Reduce::SUM, [&] () -> float4 {
                    return mask() * convert_float4(input(mask));
                    }) + 0.5f);
        }
        #else
        void kernel() {
            const int anchor = size >> 1;
            float4 sum = { 0.5f, 0.5f, 0.5f, 0.5f };

            for (int yf = -anchor; yf<=anchor; yf++) {
                sum += mask(0, yf) * convert_float4(input(0, yf));
            }

            output() = convert_uchar4(sum);
        }
        #endif
};
#endif


/*************************************************************************
 * Main function                                                         *
 *************************************************************************/
int main(int argc, const char **argv) {
    double time0, time1, dt, min_dt;
    const int width = WIDTH;
    const int height = HEIGHT;
    const int size_x = SIZE_X;
    const int size_y = SIZE_Y;
    const int offset_x = size_x >> 1;
    const int offset_y = size_y >> 1;
    std::vector<float> timings;
    const double sigma1 = ((size_x-1)*0.5 - 1)*0.3 + 0.8;
    const double sigma2 = ((size_y-1)*0.5 - 1)*0.3 + 0.8;

    // filter coefficients
    #ifdef CONST_MASK
    // only filter kernel sizes 3x3, 5x5, and 7x7 implemented
    if (size_x != size_y || !(size_x == 3 || size_x == 5 || size_x == 7)) {
        std::cerr << "Wrong filter kernel size. Currently supported values: 3x3, 5x5, and 7x7!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // convolution filter mask
    const float filter_x[1][SIZE_X] = {
        #if SIZE_X == 3
        { 0.238994f, 0.522011f, 0.238994f }
        #endif
        #if SIZE_X == 5
        { 0.070766f, 0.244460f, 0.369546f, 0.244460f, 0.070766f }
        #endif
        #if SIZE_X == 7
        { 0.028995f, 0.103818f, 0.223173f, 0.288026f, 0.223173f, 0.103818f, 0.028995f }
        #endif
    };
    const float filter_y[SIZE_Y][1] = {
        #if SIZE_Y == 3
        { 0.238994f }, { 0.522011f }, { 0.238994f }
        #endif
        #if SIZE_Y == 5
        { 0.070766f }, { 0.244460f }, { 0.369546f }, { 0.244460f }, { 0.070766f }
        #endif
        #if SIZE_Y == 7
        { 0.028995f }, { 0.103818f }, { 0.223173f }, { 0.288026f }, { 0.223173f }, { 0.103818f }, { 0.028995f }
        #endif
    };
    const float filter_xy[SIZE_Y][SIZE_X] = {
        #if SIZE_X == 3
        { 0.057118f, 0.124758f, 0.057118f },
        { 0.124758f, 0.272496f, 0.124758f },
        { 0.057118f, 0.124758f, 0.057118f }
        #endif
        #if SIZE_X == 5
        { 0.005008f, 0.017300f, 0.026151f, 0.017300f, 0.005008f },
        { 0.017300f, 0.059761f, 0.090339f, 0.059761f, 0.017300f },
        { 0.026151f, 0.090339f, 0.136565f, 0.090339f, 0.026151f },
        { 0.017300f, 0.059761f, 0.090339f, 0.059761f, 0.017300f },
        { 0.005008f, 0.017300f, 0.026151f, 0.017300f, 0.005008f }
        #endif
        #if SIZE_X == 7
        { 0.000841, 0.003010, 0.006471, 0.008351, 0.006471, 0.003010, 0.000841 },
        { 0.003010, 0.010778, 0.023169, 0.029902, 0.023169, 0.010778, 0.003010 },
        { 0.006471, 0.023169, 0.049806, 0.064280, 0.049806, 0.023169, 0.006471 },
        { 0.008351, 0.029902, 0.064280, 0.082959, 0.064280, 0.029902, 0.008351 },
        { 0.006471, 0.023169, 0.049806, 0.064280, 0.049806, 0.023169, 0.006471 },
        { 0.003010, 0.010778, 0.023169, 0.029902, 0.023169, 0.010778, 0.003010 },
        { 0.000841, 0.003010, 0.006471, 0.008351, 0.006471, 0.003010, 0.000841 }
        #endif
    };
    #else
    float filter_x[1][SIZE_X];
    float filter_y[SIZE_Y][1];
    float filter_xy[SIZE_Y][SIZE_X];

    double scale2X = -0.5/(sigma1*sigma1);
    double scale2Y = -0.5/(sigma2*sigma2);
    double sum_x = 0.;
    double sum_y = 0.;

    for (int i=0; i < size_x; i++) {
        double x = i - (size_x-1)*0.5;
        double t = exp(scale2X*x*x);

        filter_x[0][i] = (float)t;
        sum_x += filter_x[0][i];
    }
    for (int i=0; i < size_y; i++) {
        double x = i - (size_y-1)*0.5;
        double t = exp(scale2Y*x*x);

        filter_y[i][0] = (float)t;
        sum_y += filter_y[i][0];
    }

    sum_x = 1./sum_x;
    sum_y = 1./sum_y;
    for (int i=0; i < size_x; i++) {
        filter_x[0][i] = (float)(filter_x[0][i]*sum_x);
    }
    for (int i=0; i < size_y; i++) {
        filter_y[i][0] = (float)(filter_y[i][0]*sum_y);
    }

    for (int y=0; y < size_y; y++) {
        for (int x=0; x < size_x; x++) {
            filter_xy[y][x] = filter_x[0][x]*filter_y[y][0];
        }
    }
    #endif

    // host memory for image of width x height pixels
    uchar4 *input = new uchar4[width*height];
    uchar4 *reference_in = new uchar4[width*height];
    uchar4 *reference_out = new uchar4[width*height];
    float4 *reference_tmp = new float4[width*height];

    // initialize data
    for (int y=0; y<height; ++y) {
        for (int x=0; x<width; ++x) {
            uchar4 val;
            val.x = (y*width + x + 1) % 256;
            val.y = (y*width + x + 2) % 256;
            val.z = (y*width + x + 3) % 256;
            val.w = (y*width + x + 4) % 256;
            input[y*width + x] = val;
            reference_in[y*width + x] = val;
            reference_out[y*width + x] = (uchar4){ 0, 0, 0, 0 };
            reference_tmp[y*width + x] = (float4){ 0.0f, 0.0f, 0.0f, 0.0f };
        }
    }


    // input and output image of width x height pixels
    Image<uchar4> IN(width, height, input);
    Image<uchar4> OUT(width, height);
    Image<float4> TMP(width, height);

    // filter mask
    Mask<float> M(filter_xy);
    Mask<float> MX(filter_x);
    Mask<float> MY(filter_y);

    IterationSpace<uchar4> IsOut(OUT);
    IterationSpace<float4> IsTmp(TMP);


    #ifndef OpenCV
    std::cerr << "Calculating Hipacc Gaussian filter ..." << std::endl;
    float timing = 0.0f;

    // UNDEFINED
    #ifdef RUN_UNDEF
    #ifdef NO_SEP
    BoundaryCondition<uchar4> BcInUndef2(IN, M, Boundary::UNDEFINED);
    Accessor<uchar4> AccInUndef2(BcInUndef2);
    GaussianBlurFilterMask GFU(IsOut, AccInUndef2, M, size_x, size_y);

    GFU.execute();
    timing = hipacc_last_kernel_timing();
    #else
    BoundaryCondition<uchar4> BcInUndef(IN, MX, Boundary::UNDEFINED);
    Accessor<uchar4> AccInUndef(BcInUndef);
    GaussianBlurFilterMaskRow GFRU(IsTmp, AccInUndef, MX, size_x);

    BoundaryCondition<float4> BcTmpUndef(TMP, MY, Boundary::UNDEFINED);
    Accessor<float4> AccTmpUndef(BcTmpUndef);
    GaussianBlurFilterMaskColumn GFCU(IsOut, AccTmpUndef, MY, size_y);

    GFRU.execute();
    timing = hipacc_last_kernel_timing();
    GFCU.execute();
    timing += hipacc_last_kernel_timing();
    #endif
    #endif
    timings.push_back(timing);
    std::cerr << "Hipacc (UNDEFINED): " << timing << " ms, " << (width*height/timing)/1000 << " Mpixel/s" << std::endl;


    // CLAMP
    #ifdef NO_SEP
    BoundaryCondition<uchar4> BcInClamp2(IN, M, Boundary::CLAMP);
    Accessor<uchar4> AccInClamp2(BcInClamp2);
    GaussianBlurFilterMask GFC(IsOut, AccInClamp2, M, size_x, size_y);

    GFC.execute();
    timing = hipacc_last_kernel_timing();
    #else
    BoundaryCondition<uchar4> BcInClamp(IN, MX, Boundary::CLAMP);
    Accessor<uchar4> AccInClamp(BcInClamp);
    GaussianBlurFilterMaskRow GFRC(IsTmp, AccInClamp, MX, size_x);

    BoundaryCondition<float4> BcTmpClamp(TMP, MY, Boundary::CLAMP);
    Accessor<float4> AccTmpClamp(BcTmpClamp);
    GaussianBlurFilterMaskColumn GFCC(IsOut, AccTmpClamp, MY, size_y);

    GFRC.execute();
    timing = hipacc_last_kernel_timing();
    GFCC.execute();
    timing += hipacc_last_kernel_timing();
    #endif
    timings.push_back(timing);
    std::cerr << "Hipacc (CLAMP): " << timing << " ms, " << (width*height/timing)/1000 << " Mpixel/s" << std::endl;


    // REPEAT
    #ifdef NO_SEP
    BoundaryCondition<uchar4> BcInRepeat2(IN, M, Boundary::REPEAT);
    Accessor<uchar4> AccInRepeat2(BcInRepeat2);
    GaussianBlurFilterMask GFR(IsOut, AccInRepeat2, M, size_x, size_y);

    GFR.execute();
    timing = hipacc_last_kernel_timing();
    #else
    BoundaryCondition<uchar4> BcInRepeat(IN, MX, Boundary::REPEAT);
    Accessor<uchar4> AccInRepeat(BcInRepeat);
    GaussianBlurFilterMaskRow GFRR(IsTmp, AccInRepeat, MX, size_x);

    BoundaryCondition<float4> BcTmpRepeat(TMP, MY, Boundary::REPEAT);
    Accessor<float4> AccTmpRepeat(BcTmpRepeat);
    GaussianBlurFilterMaskColumn GFCR(IsOut, AccTmpRepeat, MY, size_y);

    GFRR.execute();
    timing = hipacc_last_kernel_timing();
    GFCR.execute();
    timing += hipacc_last_kernel_timing();
    #endif
    timings.push_back(timing);
    std::cerr << "Hipacc (REPEAT): " << timing << " ms, " << (width*height/timing)/1000 << " Mpixel/s" << std::endl;


    // MIRROR
    #ifdef NO_SEP
    BoundaryCondition<uchar4> BcInMirror2(IN, M, Boundary::MIRROR);
    Accessor<uchar4> AccInMirror2(BcInMirror2);
    GaussianBlurFilterMask GFM(IsOut, AccInMirror2, M, size_x, size_y);

    GFM.execute();
    timing = hipacc_last_kernel_timing();
    #else
    BoundaryCondition<uchar4> BcInMirror(IN, MX, Boundary::MIRROR);
    Accessor<uchar4> AccInMirror(BcInMirror);
    GaussianBlurFilterMaskRow GFRM(IsTmp, AccInMirror, MX, size_x);

    BoundaryCondition<float4> BcTmpMirror(TMP, MY, Boundary::MIRROR);
    Accessor<float4> AccTmpMirror(BcTmpMirror);
    GaussianBlurFilterMaskColumn GFCM(IsOut, AccTmpMirror, MY, size_y);

    GFRM.execute();
    timing = hipacc_last_kernel_timing();
    GFCM.execute();
    timing += hipacc_last_kernel_timing();
    #endif
    timings.push_back(timing);
    std::cerr << "Hipacc (MIRROR): " << timing << " ms, " << (width*height/timing)/1000 << " Mpixel/s" << std::endl;


    // CONSTANT
    #ifdef NO_SEP
    BoundaryCondition<uchar4> BcInConst2(IN, M, Boundary::CONSTANT, (uchar4){'1','1','1','1'});
    Accessor<uchar4> AccInConst2(BcInConst2);
    GaussianBlurFilterMask GFConst(IsOut, AccInConst2, M, size_x, size_y);

    GFConst.execute();
    timing = hipacc_last_kernel_timing();
    #else
    BoundaryCondition<uchar4> BcInConst(IN, MX, Boundary::CONSTANT, (uchar4){'1','1','1','1'});
    Accessor<uchar4> AccInConst(BcInConst);
    GaussianBlurFilterMaskRow GFRConst(IsTmp, AccInConst, MX, size_x);

    BoundaryCondition<float4> BcTmpConst(TMP, MY, Boundary::CONSTANT, (float4){1.0f,1.0f,1.0f,1.0f});
    Accessor<float4> AccTmpConst(BcTmpConst);
    GaussianBlurFilterMaskColumn GFCConst(IsOut, AccTmpConst, MY, size_y);

    GFRConst.execute();
    timing = hipacc_last_kernel_timing();
    GFCConst.execute();
    timing += hipacc_last_kernel_timing();
    #endif
    timings.push_back(timing);
    std::cerr << "Hipacc (CONSTANT): " << timing << " ms, " << (width*height/timing)/1000 << " Mpixel/s" << std::endl;


    // get pointer to result data
    uchar4 *output = OUT.data();
    #endif



    #ifdef OpenCV
    #ifdef CPU
    std::cerr << std::endl << "Calculating OpenCV Gaussian filter on the CPU ..." << std::endl;
    #else
    std::cerr << std::endl << "Calculating OpenCV Gaussian filter on the GPU ..." << std::endl;
    #endif


    cv::Mat cv_data_in(height, width, CV_8UC4, input);
    cv::Mat cv_data_out(height, width, CV_8UC4, cv::Scalar(0));
    cv::Size ksize(size_x, size_y);

    for (int brd_type=0; brd_type<5; brd_type++) {
        #ifdef CPU
        if (brd_type==cv::BORDER_WRAP) {
            // BORDER_WRAP is not supported on the CPU by OpenCV
            timings.push_back(0.0f);
            continue;
        }
        min_dt = DBL_MAX;
        for (int nt=0; nt<10; nt++) {
            time0 = time_ms();

            cv::GaussianBlur(cv_data_in, cv_data_out, ksize, sigma1, sigma2, brd_type);

            time1 = time_ms();
            dt = time1 - time0;
            if (dt < min_dt) min_dt = dt;
        }
        #else
        cv::gpu::GpuMat gpu_in, gpu_out;
        gpu_in.upload(cv_data_in);

        min_dt = DBL_MAX;
        for (int nt=0; nt<10; nt++) {
            time0 = time_ms();

            cv::gpu::GaussianBlur(gpu_in, gpu_out, ksize, sigma1, sigma2, brd_type);

            time1 = time_ms();
            dt = time1 - time0;
            if (dt < min_dt) min_dt = dt;
        }

        gpu_out.download(cv_data_out);
        #endif

        std::cerr << "OpenCV (";
        switch (brd_type) {
            case IPL_BORDER_CONSTANT:    std::cerr << "CONSTANT";   break;
            case IPL_BORDER_REPLICATE:   std::cerr << "CLAMP";      break;
            case IPL_BORDER_REFLECT:     std::cerr << "MIRROR";     break;
            case IPL_BORDER_WRAP:        std::cerr << "REPEAT";     break;
            case IPL_BORDER_REFLECT_101: std::cerr << "MIRROR_101"; break;
            default: break;
        }
        std::cerr << "): " << min_dt << " ms, " << (width*height/min_dt)/1000 << " Mpixel/s" << std::endl;
        timings.push_back(min_dt);
    }

    // get pointer to result data
    uchar4 *output = (uchar4 *)cv_data_out.data;
    #endif

    // print statistics
    for (std::vector<float>::const_iterator it = timings.begin(); it != timings.end(); ++it) {
        std::cerr << "\t" << *it;
    }
    std::cerr << std::endl << std::endl;


    std::cerr << "Calculating reference ..." << std::endl;
    min_dt = DBL_MAX;
    for (int nt=0; nt<3; nt++) {
        time0 = time_ms();

        // calculate reference
        #ifdef NO_SEP
        gaussian_filter(reference_in, reference_out, (float *)filter_xy, size_x, size_y, width, height);
        #else
        gaussian_filter_row(reference_in, reference_tmp, (float *)filter_x, size_x, width, height);
        gaussian_filter_column(reference_tmp, reference_out, (float *)filter_y, size_y, width, height);
        #endif

        time1 = time_ms();
        dt = time1 - time0;
        if (dt < min_dt) min_dt = dt;
    }
    std::cerr << "Reference: " << min_dt << " ms, " << (width*height/min_dt)/1000 << " Mpixel/s" << std::endl;

    std::cerr << std::endl << "Comparing results ..." << std::endl;
    #ifdef OpenCV
    int upper_y = height-size_y+offset_y;
    int upper_x = width-size_x+offset_x;
    #else
    int upper_y = height-offset_y;
    int upper_x = width-offset_x;
    #endif
    // compare results
    for (int y=offset_y; y<upper_y; y++) {
        for (int x=offset_x; x<upper_x; x++) {
            if (reference_out[y*width + x].x != output[y*width + x].x ||
                reference_out[y*width + x].y != output[y*width + x].y ||
                reference_out[y*width + x].z != output[y*width + x].z ||
                reference_out[y*width + x].w != output[y*width + x].w) {
                std::cerr << "Test FAILED, at (" << x << "," << y << "): ("
                          << (int)reference_out[y*width + x].x << ","
                          << (int)reference_out[y*width + x].y << ","
                          << (int)reference_out[y*width + x].z << ","
                          << (int)reference_out[y*width + x].w << ") vs. ("
                          << (int)output[y*width + x].x << ","
                          << (int)output[y*width + x].y << ","
                          << (int)output[y*width + x].z << ","
                          << (int)output[y*width + x].w << ")" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    }
    std::cerr << "Test PASSED" << std::endl;

    // memory cleanup
    delete[] input;
    delete[] reference_in;
    delete[] reference_tmp;
    delete[] reference_out;

    return EXIT_SUCCESS;
}
