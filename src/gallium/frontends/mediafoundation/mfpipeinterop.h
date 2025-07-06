/*
 * Copyright © Microsoft Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#pragma once
#include "util/u_video.h"
#include "vl/vl_defines.h"
#include "vl/vl_video_buffer.h"
#include "vl/vl_winsys.h"
#include "pipe_headers.h"

#include <codecapi.h>
#include <mfobjects.h>

enum pipe_video_profile
ConvertAVEncVProfileToPipeVideoProfile( struct vl_screen *vlScreen, UINT32 profile, D3D12_VIDEO_ENCODER_CODEC codec );
enum pipe_video_chroma_format
ConvertAVEncVProfileToPipeVideoChromaFormat( UINT32 profile, D3D12_VIDEO_ENCODER_CODEC codec );

// H264
enum eAVEncH264PictureType
ConvertPictureTypeToAVEncH264PictureType( enum pipe_h2645_enc_picture_type picType );

uint32_t
ConvertPipeProfileToSpecProfile( pipe_video_profile profile );
enum pipe_format
ConvertFourCCToPipeFormat( DWORD dwFourCC );
UINT32
AdjustStrideForPipeFormatAndWidth( enum pipe_format pipeFormat, UINT32 width );

UINT32
GetChromaFormatIdc( enum pipe_format pipeFormat );

enum pipe_format
ConvertProfileToFormat( enum pipe_video_profile profile );
GUID ConvertProfileToSubtype( enum pipe_video_profile );

HRESULT
ConvertErrnoRetToHR( int ret );

const char *
ConvertPipeH2645FrameTypeToString( pipe_h2645_enc_picture_type picType );
