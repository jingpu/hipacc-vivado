//
// Copyright (c) 2013, University of Erlangen-Nuremberg
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#include <iostream>
#include <vector>
#include <numeric>

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "hipacc.hpp"


//#define TEST


// variables set by Makefile
#ifdef TEST
#define WIDTH  24
#define HEIGHT 24
#else
#define WIDTH  1920
#define HEIGHT 1080
#endif


#define blockSize  3
#define Ksize      3
#define k          0.04f
#define threshold  100

using namespace hipacc;


// Harris Corner filter in HIPAcc
class Sobel : public Kernel<short> {
  private:
    Accessor<uchar> &Input;
    Mask<char> &cMask;
    Domain &dom;

  public:
    Sobel(IterationSpace<short> &IS,
            Accessor<uchar> &Input, Mask<char> &cMask, Domain &dom)
          : Kernel(IS),
            Input(Input),
            cMask(cMask),
            dom(dom) {
      add_accessor(&Input);
    }

    void kernel() {
      short sum = 0;
      sum += reduce(dom, Reduce::SUM, [&] () -> short {
          return Input(dom) * cMask(dom);
      });
      output() = sum;
    }
};

class Square1 : public Kernel<int> {
  private:
    Accessor<short> &Input;

  public:
    Square1(IterationSpace<int> &IS,
            Accessor<short> &Input)
          : Kernel(IS),
            Input(Input) {
      add_accessor(&Input);
    }

    void kernel() {
      short in = Input();
      output() = in * in;
    }
};
class Square2 : public Kernel<int> {
  private:
    Accessor<short> &Input1;
    Accessor<short> &Input2;

  public:
    Square2(IterationSpace<int> &IS,
            Accessor<short> &Input1,
            Accessor<short> &Input2)
          : Kernel(IS),
            Input1(Input1),
            Input2(Input2) {
      add_accessor(&Input1);
      add_accessor(&Input2);
    }

    void kernel() {
      output() = Input1() * Input2();
    }
};

class Gauss : public Kernel<int> {
  private:
    Accessor<int> &Input;
    Mask<uchar> &cMask;

  public:
    Gauss(IterationSpace<int> &IS,
            Accessor<int> &Input, Mask<uchar> &cMask)
          : Kernel(IS),
            Input(Input),
            cMask(cMask) {
      add_accessor(&Input);
    }

    void kernel() {
      int sum = 0;
      sum += convolve(cMask, Reduce::SUM, [&] () -> int {
          return Input(cMask) * cMask();
      });
      output() = sum;
    }
};

class HarrisCorner : public Kernel<float> {
  private:
    Accessor<int> &Dx;
    Accessor<int> &Dy;
    Accessor<int> &Dxy;

  public:
    HarrisCorner(IterationSpace<float> &IS,
            Accessor<int> &Dx, Accessor<int> &Dy, Accessor<int> &Dxy)
          : Kernel(IS),
            Dx(Dx),
            Dy(Dy),
            Dxy(Dxy) {
      add_accessor(&Dx);
      add_accessor(&Dy);
      add_accessor(&Dxy);
    }

    void kernel() {
      int x = Dx();
      int y = Dy();
      int xy = Dxy();
      int scale = (1 << (Ksize-1)) * blockSize;
      float fx = (x / scale / scale);
      float fy = (y / scale / scale);
      float fxy = (xy / scale / scale);
      float R = ((fx * fy) - (fxy * fxy)) /* det   */
        - (k * (fx + fy) * (fx + fy)); /* trace */
      output() = R;
    }
};

class NonMaxSuppression : public Kernel<uchar> {
  private:
    Accessor<float> &input;
    Domain &dom;

  public:
    NonMaxSuppression(IterationSpace<uchar> &IS,
                      Accessor<float> &input, Domain &dom)
          : Kernel(IS),
            input(input),
            dom(dom) {
      add_accessor(&input);
    }

    void kernel() {
      float z = input();
      bool is_max = true;
      iterate(dom, [&] () {
          if (dom.x() != 0 || dom.y() != 0)
            is_max = is_max && (input(dom) < z);
        });
      uchar out = 0;
      if (is_max && (input(0, 0) > threshold))
        out = 255;
      output() = out;
    }
};


/*************************************************************************
 * Main function                                                         *
 *************************************************************************/
int main(int argc, const char **argv) {

    const int width = WIDTH;
    const int height = HEIGHT;

    double timing = 0.0;

    // host memory for image of of width x height pixels
    uchar host_in[WIDTH*HEIGHT] = { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
             0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
             0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
             0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
             0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
             0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
             0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
             0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
            255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255,
             0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
             0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
             0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
             0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
             0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
             0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
             0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
             0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,255,255,255,255,255,255,255,255, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 };
    uchar *host_out = (uchar *)malloc(sizeof(uchar)*width*height);

    // initialize data
    //for (int y=0; y<height; ++y) {
    //  for (int x=0; x<width; ++x) {
    //    host_out[y*width + x] = 0;
    //  }
    //}

    // convolution filter mask
    const uchar filter_xy[3][3] = {
        { 1, 1, 1 },
        { 1, 1, 1 },
        { 1, 1, 1 }
    };

    const  char mask_x[3][3] = { {-1,  0,  1},//{-0.166666667f,          0.0f,  0.166666667f},
                                 {-2,  0,  2},//{-0.166666667f,          0.0f,  0.166666667f},
                                 {-1,  0,  1} };//{-0.166666667f,          0.0f,  0.166666667f} };
    const  char mask_y[3][3] = { {-1, -2, -1},//{-0.166666667f, -0.166666667f, -0.166666667f},
                                 { 0,  0,  0},//{         0.0f,          0.0f,          0.0f},
                                 { 1,  2,  1} };//{ 0.166666667f,  0.166666667f,  0.166666667f} };

    Mask<uchar> G(filter_xy);
    Mask<char> MX(mask_x);
    Mask<char> MY(mask_y);
    Domain DomX(MX);
    Domain DomY(MY);

    // input and output image of width x height pixels
    Image<uchar> IN(WIDTH, HEIGHT);
    Image<uchar> OUT(WIDTH, HEIGHT);
    Image<short> DX(WIDTH, HEIGHT);
    Image<short> DY(WIDTH, HEIGHT);
    Image<int> DXX(WIDTH, HEIGHT);
    Image<int> DYY(WIDTH, HEIGHT);
    Image<int> DXY(WIDTH, HEIGHT);
    Image<int> GX(WIDTH, HEIGHT);
    Image<int> GY(WIDTH, HEIGHT);
    Image<int> GXY(WIDTH, HEIGHT);
    Image<float> CIM(WIDTH, HEIGHT);

    IterationSpace<uchar> IsOut(OUT);
    IterationSpace<short> IsDx(DX);
    IterationSpace<short> IsDy(DY);
    IterationSpace<int> IsDxx(DXX);
    IterationSpace<int> IsDyy(DYY);
    IterationSpace<int> IsDxy(DXY);
    IterationSpace<int> IsGx(GX);
    IterationSpace<int> IsGy(GY);
    IterationSpace<int> IsGxy(GXY);
    IterationSpace<float> IsCim(CIM);

    BoundaryCondition<uchar> BcInClamp(IN, MX, Boundary::CLAMP);
    Accessor<uchar> AccInClamp(BcInClamp);

    Accessor<short> AccDx(DX);
    Accessor<short> AccDy(DY);

    BoundaryCondition<int> BcInClampDxx(DXX, G, Boundary::CLAMP);
    Accessor<int> AccInClampDxx(BcInClampDxx);
    BoundaryCondition<int> BcInClampDyy(DYY, G, Boundary::CLAMP);
    Accessor<int> AccInClampDyy(BcInClampDyy);
    BoundaryCondition<int> BcInClampDxy(DXY, G, Boundary::CLAMP);
    Accessor<int> AccInClampDxy(BcInClampDxy);

    Accessor<int> AccGx(GX);
    Accessor<int> AccGy(GY);
    Accessor<int> AccGxy(GXY);
    Domain dom(3, 3);
    BoundaryCondition<float> BcInClampCim(CIM, G, Boundary::CLAMP);
    Accessor<float> AccInClampCim(BcInClampCim);

    IN = host_in;
    OUT = host_out;

    Sobel DerivX(IsDx, AccInClamp, MX, DomX);
    DerivX.execute();
    timing += hipacc_last_kernel_timing();

    Sobel DerivY(IsDy, AccInClamp, MY, DomY);
    DerivY.execute();
    timing += hipacc_last_kernel_timing();

    Square1 SquareX(IsDxx, AccDx);
    SquareX.execute();
    timing += hipacc_last_kernel_timing();

    Square1 SquareY(IsDyy, AccDy);
    SquareY.execute();
    timing += hipacc_last_kernel_timing();

    Square2 SquareXY(IsDxy, AccDx, AccDy);
    SquareXY.execute();
    timing += hipacc_last_kernel_timing();

    Gauss GaussX(IsGx, AccInClampDxx, G);
    GaussX.execute();
    timing += hipacc_last_kernel_timing();

    Gauss GaussY(IsGy, AccInClampDyy, G);
    GaussY.execute();
    timing += hipacc_last_kernel_timing();

    Gauss GaussXY(IsGxy, AccInClampDxy, G);
    GaussXY.execute();
    timing += hipacc_last_kernel_timing();

    HarrisCorner HC(IsCim, AccGx, AccGy, AccGxy);
    HC.execute();
    timing += hipacc_last_kernel_timing();

    NonMaxSuppression NMS(IsOut, AccInClampCim, dom);
    NMS.execute();
    timing += hipacc_last_kernel_timing();

    // get results
    host_out = OUT.data();
    fprintf(stdout,"<HIPACC:> Overall time: %fms\n", timing);

#ifdef TEST
    int i,j;
    for(i = 0; i < HEIGHT; i++) {
        for(j = 0; j < WIDTH; j++) {
            //fprintf(stdout,"%d ", host_out[i*WIDTH+j]);
            if (host_out[i*WIDTH+j] == 1) {
                fprintf(stdout,"X ");
            } else {
                fprintf(stdout,"- ");
            }
        }
        fprintf(stdout,"\n");
    }
#endif

    //free(host_in);
    free(host_out);

    return EXIT_SUCCESS;
}

