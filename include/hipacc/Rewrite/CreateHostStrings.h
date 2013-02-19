//
// Copyright (c) 2012, University of Erlangen-Nuremberg
// Copyright (c) 2012, Siemens AG
// Copyright (c) 2010, ARM Limited
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

//===--- CreateHostStrings.h - OpenCL/CUDA helper for the Rewriter --------===//
//
// This file implements functionality for printing OpenCL/CUDA host code to
// strings.
//
//===----------------------------------------------------------------------===//

#ifndef _CREATE_HOST_STRINGS_H_
#define _CREATE_HOST_STRINGS_H_

#include "hipacc/Config/CompilerOptions.h"
#include "hipacc/Device/TargetDescription.h"
#include "hipacc/DSL/ClassRepresentation.h"


namespace clang {
namespace hipacc {
class CreateHostStrings {
  private:
    CompilerOptions &options;
    unsigned int literalCountGridBock;
    int num_indent, cur_indent;
    std::string indent;

    void inc_indent() {
      cur_indent += num_indent;
      indent = std::string(cur_indent, ' ');
    }
    void dec_indent() {
      cur_indent -= num_indent;
      if (cur_indent < 0) cur_indent = 0;
      indent = std::string(cur_indent, ' ');
    }

  public:
    CreateHostStrings(CompilerOptions &options) :
      options(options),
      literalCountGridBock(0),
      num_indent(4),
      cur_indent(num_indent),
      indent(cur_indent, ' ')
    {}

    std::string getIndent() { return indent; }
    void writeHeaders(std::string &resultStr);
    void writeInitialization(std::string &resultStr);
    void writeKernelCompilation(std::string kernelName, std::string &resultStr,
        std::string suffix="");
    void addReductionArgument(HipaccGlobalReduction *GR, std::string
        device_name, std::string host_name, std::string &resultStr, bool bind);
    void writeReductionCompilation(HipaccGlobalReduction *GR, std::string
        &resultStr);
    void writeReductionDeclaration(HipaccGlobalReduction *GR, std::string
        &resultStr);
    void writeMemoryAllocation(std::string memName, std::string type,
        std::string width, std::string height, std::string &pitchStr,
        std::string &resultStr, HipaccDevice &targetDevice);
    void writeMemoryAllocationConstant(std::string memName, std::string type,
        std::string width, std::string height, std::string &pitchStr,
        std::string &resultStr);
    void writeMemoryTransfer(HipaccImage *Img, std::string mem,
        MemoryTransferDirection direction, std::string &resultStr);
    void writeMemoryTransferRegion(HipaccImage *SrcImg, HipaccImage *DstImg,
        std::string src_ox, std::string src_oy, std::string dst_ox, std::string
        dst_oy, std::string roi_width, std::string roi_height, std::string
        &resultStr);
    void writeMemoryTransferSymbol(HipaccMask *Mask, std::string mem,
        MemoryTransferDirection direction, std::string &resultStr);
    void writeKernelCall(std::string kernelName, HipaccKernelClass *KC,
        HipaccKernel *K, std::string &resultStr);
    void writeGlobalReductionCall(HipaccGlobalReduction *GR, std::string
        &resultStr);
    void writeInterpolationDefinition(HipaccKernel *K, HipaccAccessor *Acc,
        std::string function_name, std::string type_suffix, InterpolationMode
        ip_mode, BoundaryMode bh_mode, std::string &resultStr);
};
} // end namespace hipacc
} // end namespace clang

#endif  // _CREATE_HOST_STRINGS_H_

// vim: set ts=2 sw=2 sts=2 et ai:

