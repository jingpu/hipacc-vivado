//
// Copyright (c) 2012, University of Erlangen-Nuremberg
// Copyright (c) 2012, Siemens AG
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
//#define SIZE_X 3
//#define SIZE_Y 3
//#define WIDTH 4096
//#define HEIGHT 4096
#define CONST_MASK
#define USE_LAMBDA
//#define RUN_UNDEF

using namespace hipacc;
using namespace hipacc::math;


// get time in milliseconds
double time_ms () {
    struct timeval tv;
    gettimeofday (&tv, NULL);

    return ((double)(tv.tv_sec) * 1e+3 + (double)(tv.tv_usec) * 1e-3);
}


// Laplace filter reference
void laplace_filter(uchar *in, uchar *out, int *filter, int size, int width, int
        height) {
    const int size_x = size;
    const int size_y = size;
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
            int sum = 0;

            for (int yf = -anchor_y; yf<=anchor_y; yf++) {
                for (int xf = -anchor_x; xf<=anchor_x; xf++) {
                    sum += filter[(yf+anchor_y)*size_x + xf+anchor_x] *
                           in[(y+yf)*width + x + xf];
                }
            }

            sum = min(sum, 255);
            sum = max(sum, 0);
            out[y*width + x] = sum;
        }
    }
}


// Laplace filter in Hipacc
class LaplaceFilter : public Kernel<uchar> {
    private:
        Accessor<uchar> &input;
        Domain &dom;
        Mask<int> &mask;
        const int size;

    public:
        LaplaceFilter(IterationSpace<uchar> &iter, Accessor<uchar> &input,
                Domain &dom, Mask<int> &mask, const int size) :
            Kernel(iter),
            input(input),
            dom(dom),
            mask(mask),
            size(size)
        { add_accessor(&input); }

        #ifdef USE_LAMBDA
        void kernel() {
            int sum = reduce(dom, Reduce::SUM, [&] () -> int {
                    return mask(dom) * input(dom);
                    });
            sum = min(sum, 255);
            sum = max(sum, 0);
            output() = (uchar) (sum);
        }
        #else
        void kernel() {
            const int anchor = size >> 1;
            int sum = 0;

            for (int yf = -anchor; yf<=anchor; yf++) {
                for (int xf = -anchor; xf<=anchor; xf++) {
                    sum += mask(xf, yf)*input(xf, yf);
                }
            }

            sum = min(sum, 255);
            sum = max(sum, 0);
            output() = (uchar) (sum);
        }
        #endif
};


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

    // only filter kernel sizes 3x3 and 5x5 supported
    if (size_x != size_y || !(size_x == 3 || size_x == 5)) {
        std::cerr << "Wrong filter kernel size. Currently supported values: 3x3 and 5x5!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // convolution filter mask
    #ifdef CONST_MASK
    const
    #endif
    #if SIZE_X == 1
    #define SIZE 3
    #elif SIZE_X == 3
    #define SIZE 3
    #else
    #define SIZE 5
    #endif
    int mask[SIZE][SIZE] = {
        #if SIZE_X==1
        { 0,  1,  0 },
        { 1, -4,  1 },
        { 0,  1,  0 }
        #endif
        #if SIZE_X==3
        { 2,  0,  2 },
        { 0, -8,  0 },
        { 2,  0,  2 }
        #endif
        #if SIZE_X==5
        { 1,   1,   1,   1,   1 },
        { 1,   1,   1,   1,   1 },
        { 1,   1, -24,   1,   1 },
        { 1,   1,   1,   1,   1 },
        { 1,   1,   1,   1,   1 }
        #endif
    };

    // host memory for image of width x height pixels
    uchar *input = new uchar[width*height];
    uchar *reference_in = new uchar[width*height];
    uchar *reference_out = new uchar[width*height];

    // initialize data
    for (int y=0; y<height; ++y) {
        for (int x=0; x<width; ++x) {
            input[y*width + x] = (uchar)(y*width + x) % 256;
            reference_in[y*width + x] = (uchar)(y*width + x) % 256;
            reference_out[y*width + x] = 0;
        }
    }


    // input and output image of width x height pixels
    Image<uchar> IN(width, height, input);
    Image<uchar> OUT(width, height);

    // filter mask
    Mask<int> M(mask);

    // filter domain
    Domain D(M);

    IterationSpace<uchar> IsOut(OUT);


    #ifndef OpenCV
    std::cerr << "Calculating Hipacc Laplace filter ..." << std::endl;
    float timing = 0.0f;

    // UNDEFINED
    #ifdef RUN_UNDEF
    BoundaryCondition<uchar> BcInUndef(IN, M, Boundary::UNDEFINED);
    Accessor<uchar> AccInUndef(BcInUndef);
    LaplaceFilter LFU(IsOut, AccInUndef, D, M, size_x);

    LFU.execute();
    timing = hipacc_last_kernel_timing();
    #endif
    timings.push_back(timing);
    std::cerr << "Hipacc (UNDEFINED): " << timing << " ms, " << (width*height/timing)/1000 << " Mpixel/s" << std::endl;


    // CLAMP
    BoundaryCondition<uchar> BcInClamp(IN, M, Boundary::CLAMP);
    Accessor<uchar> AccInClamp(BcInClamp);
    LaplaceFilter LFC(IsOut, AccInClamp, D, M, size_x);

    LFC.execute();
    timing = hipacc_last_kernel_timing();
    timings.push_back(timing);
    std::cerr << "Hipacc (CLAMP): " << timing << " ms, " << (width*height/timing)/1000 << " Mpixel/s" << std::endl;


    // REPEAT
    BoundaryCondition<uchar> BcInRepeat(IN, M, Boundary::REPEAT);
    Accessor<uchar> AccInRepeat(BcInRepeat);
    LaplaceFilter LFR(IsOut, AccInRepeat, D, M, size_x);

    LFR.execute();
    timing = hipacc_last_kernel_timing();
    timings.push_back(timing);
    std::cerr << "Hipacc (REPEAT): " << timing << " ms, " << (width*height/timing)/1000 << " Mpixel/s" << std::endl;


    // MIRROR
    BoundaryCondition<uchar> BcInMirror(IN, M, Boundary::MIRROR);
    Accessor<uchar> AccInMirror(BcInMirror);
    LaplaceFilter LFM(IsOut, AccInMirror, D, M, size_x);

    LFM.execute();
    timing = hipacc_last_kernel_timing();
    timings.push_back(timing);
    std::cerr << "Hipacc (MIRROR): " << timing << " ms, " << (width*height/timing)/1000 << " Mpixel/s" << std::endl;


    // CONSTANT
    BoundaryCondition<uchar> BcInConst(IN, M, Boundary::CONSTANT, '1');
    Accessor<uchar> AccInConst(BcInConst);
    LaplaceFilter LFConst(IsOut, AccInConst, D, M, size_x);

    LFConst.execute();
    timing = hipacc_last_kernel_timing();
    timings.push_back(timing);
    std::cerr << "Hipacc (CONSTANT): " << timing << " ms, " << (width*height/timing)/1000 << " Mpixel/s" << std::endl;


    // get pointer to result data
    uchar *output = OUT.data();
    #endif



    #ifdef OpenCV
    #ifdef CPU
    std::cerr << std::endl << "Calculating OpenCV Laplace filter on the CPU ..." << std::endl;
    #else
    std::cerr << std::endl << "Calculating OpenCV Laplace filter on the GPU ..." << std::endl;
    #endif


    cv::Mat cv_data_in(height, width, CV_8UC1, input);
    cv::Mat cv_data_out(height, width, CV_8UC1, cv::Scalar(0));
    int ddepth = CV_8U;
    double scale = 1.0f;
    double delta = 0.0f;

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

            cv::Laplacian(cv_data_in, cv_data_out, ddepth, size_x, scale, delta, brd_type);

            time1 = time_ms();
            dt = time1 - time0;
            if (dt < min_dt) min_dt = dt;
        }
        #else
        #if SIZE_X==5
        #error "OpenCV supports only 1x1 and 3x3 Laplace filters on the GPU!"
        #endif
        cv::gpu::GpuMat gpu_in, gpu_out;
        gpu_in.upload(cv_data_in);

        min_dt = DBL_MAX;
        for (int nt=0; nt<10; nt++) {
            time0 = time_ms();

            cv::gpu::Laplacian(gpu_in, gpu_out, -1, size_x, scale, brd_type);

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
    uchar *output = (uchar *)cv_data_out.data;
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
        laplace_filter(reference_in, reference_out, (int *)mask, size_x, width, height);

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
            if (reference_out[y*width + x] != output[y*width + x]) {
                std::cerr << "Test FAILED, at (" << x << "," << y << "): "
                          << (int)reference_out[y*width + x] << " vs. "
                          << (int)output[y*width + x] << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    }
    std::cerr << "Test PASSED" << std::endl;

    // memory cleanup
    delete[] input;
    delete[] reference_in;
    delete[] reference_out;

    return EXIT_SUCCESS;
}

