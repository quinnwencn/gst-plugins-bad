/* GStreamer
 * Copyright (C) 2019 Seungha Yang <seungha.yang@navercorp.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * NOTE: some of implementations are copied/modified from Chromium code
 *
 * Copyright 2015 The Chromium Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gstd3d11decoder.h"
#include "gstd3d11converter.h"
#include "gstd3d11pluginutils.h"
#include <string.h>
#include <string>

#ifdef HAVE_WINMM
#include <mmsystem.h>
#endif

GST_DEBUG_CATEGORY_EXTERN (gst_d3d11_decoder_debug);
#define GST_CAT_DEFAULT gst_d3d11_decoder_debug

/* GUID might not be defined in MinGW header */
DEFINE_GUID (GST_GUID_D3D11_DECODER_PROFILE_H264_IDCT_FGT, 0x1b81be67, 0xa0c7,
    0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID (GST_GUID_D3D11_DECODER_PROFILE_H264_VLD_NOFGT, 0x1b81be68, 0xa0c7,
    0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID (GST_GUID_D3D11_DECODER_PROFILE_H264_VLD_FGT, 0x1b81be69, 0xa0c7,
    0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID (GST_GUID_D3D11_DECODER_PROFILE_HEVC_VLD_MAIN,
    0x5b11d51b, 0x2f4c, 0x4452, 0xbc, 0xc3, 0x09, 0xf2, 0xa1, 0x16, 0x0c, 0xc0);
DEFINE_GUID (GST_GUID_D3D11_DECODER_PROFILE_HEVC_VLD_MAIN10,
    0x107af0e0, 0xef1a, 0x4d19, 0xab, 0xa8, 0x67, 0xa1, 0x63, 0x07, 0x3d, 0x13);
DEFINE_GUID (GST_GUID_D3D11_DECODER_PROFILE_VP8_VLD,
    0x90b899ea, 0x3a62, 0x4705, 0x88, 0xb3, 0x8d, 0xf0, 0x4b, 0x27, 0x44, 0xe7);
DEFINE_GUID (GST_GUID_D3D11_DECODER_PROFILE_VP9_VLD_PROFILE0,
    0x463707f8, 0xa1d0, 0x4585, 0x87, 0x6d, 0x83, 0xaa, 0x6d, 0x60, 0xb8, 0x9e);
DEFINE_GUID (GST_GUID_D3D11_DECODER_PROFILE_VP9_VLD_10BIT_PROFILE2,
    0xa4c749ef, 0x6ecf, 0x48aa, 0x84, 0x48, 0x50, 0xa7, 0xa1, 0x16, 0x5f, 0xf7);
DEFINE_GUID (GST_GUID_D3D11_DECODER_PROFILE_MPEG2_VLD, 0xee27417f, 0x5e28,
    0x4e65, 0xbe, 0xea, 0x1d, 0x26, 0xb5, 0x08, 0xad, 0xc9);
DEFINE_GUID (GST_GUID_D3D11_DECODER_PROFILE_MPEG2and1_VLD, 0x86695f12, 0x340e,
    0x4f04, 0x9f, 0xd3, 0x92, 0x53, 0xdd, 0x32, 0x74, 0x60);
DEFINE_GUID (GST_GUID_D3D11_DECODER_PROFILE_AV1_VLD_PROFILE0, 0xb8be4ccb,
    0xcf53, 0x46ba, 0x8d, 0x59, 0xd6, 0xb8, 0xa6, 0xda, 0x5d, 0x2a);

static const GUID *profile_h264_list[] = {
  &GST_GUID_D3D11_DECODER_PROFILE_H264_IDCT_FGT,
  &GST_GUID_D3D11_DECODER_PROFILE_H264_VLD_NOFGT,
  &GST_GUID_D3D11_DECODER_PROFILE_H264_VLD_FGT,
};

static const GUID *profile_hevc_list[] = {
  &GST_GUID_D3D11_DECODER_PROFILE_HEVC_VLD_MAIN,
};

static const GUID *profile_hevc_10_list[] = {
  &GST_GUID_D3D11_DECODER_PROFILE_HEVC_VLD_MAIN10,
};

static const GUID *profile_vp8_list[] = {
  &GST_GUID_D3D11_DECODER_PROFILE_VP8_VLD,
};

static const GUID *profile_vp9_list[] = {
  &GST_GUID_D3D11_DECODER_PROFILE_VP9_VLD_PROFILE0,
};

static const GUID *profile_vp9_10_list[] = {
  &GST_GUID_D3D11_DECODER_PROFILE_VP9_VLD_10BIT_PROFILE2,
};

static const GUID *profile_mpeg2_list[] = {
  &GST_GUID_D3D11_DECODER_PROFILE_MPEG2_VLD,
  &GST_GUID_D3D11_DECODER_PROFILE_MPEG2and1_VLD
};

static const GUID *profile_av1_list[] = {
  &GST_GUID_D3D11_DECODER_PROFILE_AV1_VLD_PROFILE0,
  /* TODO: add more profile */
};

enum
{
  PROP_0,
  PROP_DEVICE,
};

struct _GstD3D11Decoder
{
  GstObject parent;

  gboolean configured;
  gboolean opened;

  GstD3D11Device *device;

  ID3D11VideoDevice *video_device;
  ID3D11VideoContext *video_context;

  ID3D11VideoDecoder *decoder_handle;

  GstVideoInfo info;
  GstVideoInfo output_info;
  GstDXVACodec codec;
  gint coded_width;
  gint coded_height;
  DXGI_FORMAT decoder_format;
  gboolean downstream_supports_d3d11;

  GstVideoCodecState *input_state;
  GstVideoCodecState *output_state;

  /* Protect internal pool */
  GMutex internal_pool_lock;

  GstBufferPool *internal_pool;
  /* Internal pool params */
  gint aligned_width;
  gint aligned_height;
  gboolean use_array_of_texture;
  guint dpb_size;
  guint downstream_min_buffers;
  gboolean wait_on_pool_full;

  /* Used for array-of-texture */
  guint8 next_view_id;

  /* for staging */
  ID3D11Texture2D *staging;
  gsize staging_texture_offset[GST_VIDEO_MAX_PLANES];
  gint stating_texture_stride[GST_VIDEO_MAX_PLANES];

  GUID decoder_profile;

  /* For device specific workaround */
  gboolean can_direct_rendering;

  /* For high precision clock */
  guint timer_resolution;
};

static void gst_d3d11_decoder_constructed (GObject * object);
static void gst_d3d11_decoder_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_d3d11_decoder_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_d3d11_decoder_dispose (GObject * obj);
static void gst_d3d11_decoder_finalize (GObject * obj);
static gboolean gst_d3d11_decoder_can_direct_render (GstD3D11Decoder * decoder,
    GstVideoDecoder * videodec, GstBuffer * view_buffer,
    gint display_width, gint display_height);

#define parent_class gst_d3d11_decoder_parent_class
G_DEFINE_TYPE (GstD3D11Decoder, gst_d3d11_decoder, GST_TYPE_OBJECT);

static void
gst_d3d11_decoder_class_init (GstD3D11DecoderClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = gst_d3d11_decoder_constructed;
  gobject_class->set_property = gst_d3d11_decoder_set_property;
  gobject_class->get_property = gst_d3d11_decoder_get_property;
  gobject_class->dispose = gst_d3d11_decoder_dispose;
  gobject_class->finalize = gst_d3d11_decoder_finalize;

  g_object_class_install_property (gobject_class, PROP_DEVICE,
      g_param_spec_object ("device", "Device",
          "D3D11 Devicd to use", GST_TYPE_D3D11_DEVICE,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
              G_PARAM_STATIC_STRINGS)));
}

static void
gst_d3d11_decoder_init (GstD3D11Decoder * self)
{
  g_mutex_init (&self->internal_pool_lock);
}

static void
gst_d3d11_decoder_constructed (GObject * object)
{
  GstD3D11Decoder *self = GST_D3D11_DECODER (object);
  ID3D11VideoDevice *video_device;
  ID3D11VideoContext *video_context;

  if (!self->device) {
    GST_ERROR_OBJECT (self, "No D3D11Device available");
    return;
  }

  video_device = gst_d3d11_device_get_video_device_handle (self->device);
  if (!video_device) {
    GST_WARNING_OBJECT (self, "ID3D11VideoDevice is not available");
    return;
  }

  video_context = gst_d3d11_device_get_video_context_handle (self->device);
  if (!video_context) {
    GST_WARNING_OBJECT (self, "ID3D11VideoContext is not available");
    return;
  }

  self->video_device = video_device;
  video_device->AddRef ();

  self->video_context = video_context;
  video_context->AddRef ();

  return;
}

static void
gst_d3d11_decoder_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstD3D11Decoder *self = GST_D3D11_DECODER (object);

  switch (prop_id) {
    case PROP_DEVICE:
      self->device = (GstD3D11Device *) g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_d3d11_decoder_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstD3D11Decoder *self = GST_D3D11_DECODER (object);

  switch (prop_id) {
    case PROP_DEVICE:
      g_value_set_object (value, self->device);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_d3d11_decoder_clear_resource (GstD3D11Decoder * self)
{
  g_mutex_lock (&self->internal_pool_lock);
  if (self->internal_pool) {
    gst_buffer_pool_set_active (self->internal_pool, FALSE);
    gst_clear_object (&self->internal_pool);
  }
  g_mutex_unlock (&self->internal_pool_lock);

  GST_D3D11_CLEAR_COM (self->decoder_handle);
  GST_D3D11_CLEAR_COM (self->staging);

  memset (self->staging_texture_offset,
      0, sizeof (self->staging_texture_offset));
  memset (self->stating_texture_stride,
      0, sizeof (self->stating_texture_stride));
}

static void
gst_d3d11_decoder_reset (GstD3D11Decoder * self)
{
  gst_d3d11_decoder_clear_resource (self);

  self->dpb_size = 0;
  self->downstream_min_buffers = 0;

  self->configured = FALSE;
  self->opened = FALSE;

  self->use_array_of_texture = FALSE;
  self->downstream_supports_d3d11 = FALSE;

  g_clear_pointer (&self->output_state, gst_video_codec_state_unref);
  g_clear_pointer (&self->input_state, gst_video_codec_state_unref);
}

static void
gst_d3d11_decoder_dispose (GObject * obj)
{
  GstD3D11Decoder *self = GST_D3D11_DECODER (obj);

  gst_d3d11_decoder_reset (self);

  GST_D3D11_CLEAR_COM (self->video_device);
  GST_D3D11_CLEAR_COM (self->video_context);

  gst_clear_object (&self->device);

  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
gst_d3d11_decoder_finalize (GObject * obj)
{
  GstD3D11Decoder *self = GST_D3D11_DECODER (obj);

#if HAVE_WINMM
  /* Restore clock precision */
  if (self->timer_resolution)
    timeEndPeriod (self->timer_resolution);
#endif

  g_mutex_clear (&self->internal_pool_lock);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

GstD3D11Decoder *
gst_d3d11_decoder_new (GstD3D11Device * device, GstDXVACodec codec)
{
  GstD3D11Decoder *self;

  g_return_val_if_fail (GST_IS_D3D11_DEVICE (device), nullptr);
  g_return_val_if_fail (codec > GST_DXVA_CODEC_NONE, nullptr);
  g_return_val_if_fail (codec < GST_DXVA_CODEC_LAST, nullptr);

  self = (GstD3D11Decoder *)
      g_object_new (GST_TYPE_D3D11_DECODER, "device", device, NULL);

  if (!self->video_device || !self->video_context) {
    gst_object_unref (self);
    return NULL;
  }

  self->codec = codec;

  gst_object_ref_sink (self);

  return self;
}

gboolean
gst_d3d11_decoder_is_configured (GstD3D11Decoder * decoder)
{
  g_return_val_if_fail (GST_IS_D3D11_DECODER (decoder), FALSE);

  return decoder->configured;
}

static GQuark
gst_d3d11_decoder_view_id_quark (void)
{
  static gsize id_quark = 0;

  if (g_once_init_enter (&id_quark)) {
    GQuark quark = g_quark_from_string ("GstD3D11DecoderViewId");
    g_once_init_leave (&id_quark, quark);
  }

  return (GQuark) id_quark;
}

static gboolean
gst_d3d11_decoder_ensure_output_view (GstD3D11Decoder * self,
    GstBuffer * buffer)
{
  GstD3D11Memory *mem;
  gpointer val = NULL;

  mem = (GstD3D11Memory *) gst_buffer_peek_memory (buffer, 0);
  if (!gst_d3d11_memory_get_decoder_output_view (mem, self->video_device,
          &self->decoder_profile)) {
    GST_ERROR_OBJECT (self, "Decoder output view is unavailable");
    return FALSE;
  }

  if (!self->use_array_of_texture)
    return TRUE;

  val = gst_mini_object_get_qdata (GST_MINI_OBJECT (mem),
      gst_d3d11_decoder_view_id_quark ());
  if (!val) {
    g_assert (self->next_view_id < 128);
    g_assert (self->next_view_id > 0);

    gst_mini_object_set_qdata (GST_MINI_OBJECT (mem),
        gst_d3d11_decoder_view_id_quark (),
        GUINT_TO_POINTER (self->next_view_id), NULL);

    self->next_view_id++;
    /* valid view range is [0, 126], but 0 is not used to here
     * (it's NULL as well) */
    self->next_view_id %= 128;
    if (self->next_view_id == 0)
      self->next_view_id = 1;
  }


  return TRUE;
}

static gboolean
gst_d3d11_decoder_prepare_output_view_pool (GstD3D11Decoder * self)
{
  GstD3D11AllocationParams *alloc_params = NULL;
  GstBufferPool *pool = NULL;
  GstCaps *caps = NULL;
  GstVideoAlignment align;
  GstD3D11AllocationFlags alloc_flags = (GstD3D11AllocationFlags) 0;
  gint bind_flags = D3D11_BIND_DECODER;
  GstVideoInfo *info = &self->info;
  guint pool_size;

  g_mutex_lock (&self->internal_pool_lock);
  if (self->internal_pool) {
    gst_buffer_pool_set_active (self->internal_pool, FALSE);
    gst_clear_object (&self->internal_pool);
  }
  g_mutex_unlock (&self->internal_pool_lock);

  if (!self->use_array_of_texture) {
    alloc_flags = GST_D3D11_ALLOCATION_FLAG_TEXTURE_ARRAY;
  } else {
    /* array of texture can have shader resource view */
    bind_flags |= D3D11_BIND_SHADER_RESOURCE;
  }

  alloc_params = gst_d3d11_allocation_params_new (self->device, info,
      alloc_flags, bind_flags);

  if (!alloc_params) {
    GST_ERROR_OBJECT (self, "Failed to create allocation param");
    goto error;
  }

  pool_size = self->dpb_size + self->downstream_min_buffers;
  GST_DEBUG_OBJECT (self,
      "Configuring internal pool with size %d "
      "(dpb size: %d, downstream min buffers: %d)", pool_size, self->dpb_size,
      self->downstream_min_buffers);

  if (!self->use_array_of_texture) {
    alloc_params->desc[0].ArraySize = pool_size;
  } else {
    /* Valid view id is [0, 126], but we will use [1, 127] range so that
     * it can be used by qdata, because zero is equal to null */
    self->next_view_id = 1;

    /* our pool size can be increased as much as possbile */
    pool_size = 0;
  }

  gst_video_alignment_reset (&align);

  align.padding_right = self->aligned_width - GST_VIDEO_INFO_WIDTH (info);
  align.padding_bottom = self->aligned_height - GST_VIDEO_INFO_HEIGHT (info);
  if (!gst_d3d11_allocation_params_alignment (alloc_params, &align)) {
    GST_ERROR_OBJECT (self, "Cannot set alignment");
    goto error;
  }

  caps = gst_video_info_to_caps (info);
  if (!caps) {
    GST_ERROR_OBJECT (self, "Couldn't convert video info to caps");
    goto error;
  }

  pool = gst_d3d11_buffer_pool_new_with_options (self->device,
      caps, alloc_params, 0, pool_size);
  gst_clear_caps (&caps);
  g_clear_pointer (&alloc_params, gst_d3d11_allocation_params_free);

  if (!pool) {
    GST_ERROR_OBJECT (self, "Failed to create buffer pool");
    goto error;
  }

  if (!gst_buffer_pool_set_active (pool, TRUE)) {
    GST_ERROR_OBJECT (self, "Couldn't activate pool");
    goto error;
  }

  g_mutex_lock (&self->internal_pool_lock);
  self->internal_pool = pool;
  g_mutex_unlock (&self->internal_pool_lock);

  return TRUE;

error:
  if (alloc_params)
    gst_d3d11_allocation_params_free (alloc_params);
  if (pool)
    gst_object_unref (pool);
  if (caps)
    gst_caps_unref (caps);

  return FALSE;
}

static const gchar *
gst_dxva_codec_to_string (GstDXVACodec codec)
{
  switch (codec) {
    case GST_DXVA_CODEC_NONE:
      return "none";
    case GST_DXVA_CODEC_H264:
      return "H.264";
    case GST_DXVA_CODEC_VP9:
      return "VP9";
    case GST_DXVA_CODEC_H265:
      return "H.265";
    case GST_DXVA_CODEC_VP8:
      return "VP8";
    case GST_DXVA_CODEC_MPEG2:
      return "MPEG2";
    case GST_DXVA_CODEC_AV1:
      return "AV1";
    default:
      g_assert_not_reached ();
      break;
  }

  return "Unknown";
}

gboolean
gst_d3d11_decoder_get_supported_decoder_profile (GstD3D11Device * device,
    GstDXVACodec codec, GstVideoFormat format, const GUID ** selected_profile)
{
  GUID *guid_list = nullptr;
  const GUID *profile = nullptr;
  guint available_profile_count;
  guint i, j;
  HRESULT hr;
  ID3D11VideoDevice *video_device;
  const GUID **profile_list = nullptr;
  guint profile_size = 0;

  g_return_val_if_fail (GST_IS_D3D11_DEVICE (device), FALSE);
  g_return_val_if_fail (selected_profile != nullptr, FALSE);

  video_device = gst_d3d11_device_get_video_device_handle (device);
  if (!video_device)
    return FALSE;

  switch (codec) {
    case GST_DXVA_CODEC_H264:
      if (format == GST_VIDEO_FORMAT_NV12) {
        profile_list = profile_h264_list;
        profile_size = G_N_ELEMENTS (profile_h264_list);
      }
      break;
    case GST_DXVA_CODEC_H265:
      if (format == GST_VIDEO_FORMAT_NV12) {
        profile_list = profile_hevc_list;
        profile_size = G_N_ELEMENTS (profile_hevc_list);
      } else if (format == GST_VIDEO_FORMAT_P010_10LE) {
        profile_list = profile_hevc_10_list;
        profile_size = G_N_ELEMENTS (profile_hevc_10_list);
      }
      break;
    case GST_DXVA_CODEC_VP8:
      if (format == GST_VIDEO_FORMAT_NV12) {
        profile_list = profile_vp8_list;
        profile_size = G_N_ELEMENTS (profile_vp8_list);
      }
      break;
    case GST_DXVA_CODEC_VP9:
      if (format == GST_VIDEO_FORMAT_NV12) {
        profile_list = profile_vp9_list;
        profile_size = G_N_ELEMENTS (profile_vp9_list);
      } else if (format == GST_VIDEO_FORMAT_P010_10LE) {
        profile_list = profile_vp9_10_list;
        profile_size = G_N_ELEMENTS (profile_vp9_10_list);
      }
      break;
    case GST_DXVA_CODEC_MPEG2:
      if (format == GST_VIDEO_FORMAT_NV12) {
        profile_list = profile_mpeg2_list;
        profile_size = G_N_ELEMENTS (profile_mpeg2_list);
      }
      break;
    case GST_DXVA_CODEC_AV1:
      profile_list = profile_av1_list;
      profile_size = G_N_ELEMENTS (profile_av1_list);
      break;
    default:
      break;
  }

  if (!profile_list) {
    GST_ERROR_OBJECT (device,
        "Not supported codec (%d) and format (%s) configuration", codec,
        gst_video_format_to_string (format));
    return FALSE;
  }

  available_profile_count = video_device->GetVideoDecoderProfileCount ();

  if (available_profile_count == 0) {
    GST_INFO_OBJECT (device, "No available decoder profile");
    return FALSE;
  }

  GST_DEBUG_OBJECT (device,
      "Have %u available decoder profiles", available_profile_count);
  guid_list = (GUID *) g_alloca (sizeof (GUID) * available_profile_count);

  for (i = 0; i < available_profile_count; i++) {
    hr = video_device->GetVideoDecoderProfile (i, &guid_list[i]);
    if (!gst_d3d11_result (hr, device)) {
      GST_WARNING_OBJECT (device, "Failed to get %d th decoder profile", i);
      return FALSE;
    }
  }

#ifndef GST_DISABLE_GST_DEBUG
  GST_LOG_OBJECT (device, "Supported decoder GUID");
  for (i = 0; i < available_profile_count; i++) {
    const GUID *guid = &guid_list[i];

    GST_LOG_OBJECT (device,
        "\t { %8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x }",
        (guint) guid->Data1, (guint) guid->Data2, (guint) guid->Data3,
        guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
        guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
  }

  GST_LOG_OBJECT (device, "Requested decoder GUID");
  for (i = 0; i < profile_size; i++) {
    const GUID *guid = profile_list[i];

    GST_LOG_OBJECT (device,
        "\t { %8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x }",
        (guint) guid->Data1, (guint) guid->Data2, (guint) guid->Data3,
        guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
        guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
  }
#endif

  for (i = 0; i < profile_size; i++) {
    for (j = 0; j < available_profile_count; j++) {
      if (IsEqualGUID (*profile_list[i], guid_list[j])) {
        profile = profile_list[i];
        break;
      }
    }
  }

  if (!profile) {
    GST_INFO_OBJECT (device, "No supported decoder profile for %s codec",
        gst_dxva_codec_to_string (codec));
    return FALSE;
  }

  *selected_profile = profile;

  GST_DEBUG_OBJECT (device,
      "Selected guid "
      "{ %8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x }",
      (guint) profile->Data1, (guint) profile->Data2, (guint) profile->Data3,
      profile->Data4[0], profile->Data4[1], profile->Data4[2],
      profile->Data4[3], profile->Data4[4], profile->Data4[5],
      profile->Data4[6], profile->Data4[7]);

  return TRUE;
}


gboolean
gst_d3d11_decoder_configure (GstD3D11Decoder * decoder,
    GstVideoCodecState * input_state, GstVideoInfo * info, gint coded_width,
    gint coded_height, guint dpb_size)
{
  GstD3D11Format d3d11_format;

  g_return_val_if_fail (GST_IS_D3D11_DECODER (decoder), FALSE);
  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (input_state != NULL, FALSE);
  g_return_val_if_fail (coded_width >= GST_VIDEO_INFO_WIDTH (info), FALSE);
  g_return_val_if_fail (coded_height >= GST_VIDEO_INFO_HEIGHT (info), FALSE);
  g_return_val_if_fail (dpb_size > 0, FALSE);

  gst_d3d11_decoder_reset (decoder);

  if (!gst_d3d11_device_get_format (decoder->device,
          GST_VIDEO_INFO_FORMAT (info), &d3d11_format) ||
      d3d11_format.dxgi_format == DXGI_FORMAT_UNKNOWN) {
    GST_ERROR_OBJECT (decoder, "Could not determine dxgi format from %s",
        gst_video_format_to_string (GST_VIDEO_INFO_FORMAT (info)));
    return FALSE;
  }

  /* Additional 4 frames to help zero-copying */
  dpb_size += 4;

  decoder->input_state = gst_video_codec_state_ref (input_state);
  decoder->info = decoder->output_info = *info;
  decoder->coded_width = coded_width;
  decoder->coded_height = coded_height;
  decoder->dpb_size = dpb_size;
  decoder->decoder_format = d3d11_format.dxgi_format;

  decoder->configured = TRUE;

  return TRUE;
}

static gboolean
gst_d3d11_decoder_ensure_staging_texture (GstD3D11Decoder * self)
{
  ID3D11Device *device_handle;
  D3D11_TEXTURE2D_DESC desc = { 0, };
  HRESULT hr;

  if (self->staging)
    return TRUE;

  device_handle = gst_d3d11_device_get_device_handle (self->device);

  /* create stage texture to copy out */
  desc.Width = self->aligned_width;
  desc.Height = self->aligned_height;
  desc.MipLevels = 1;
  desc.Format = self->decoder_format;
  desc.SampleDesc.Count = 1;
  desc.ArraySize = 1;
  desc.Usage = D3D11_USAGE_STAGING;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

  hr = device_handle->CreateTexture2D (&desc, NULL, &self->staging);
  if (!gst_d3d11_result (hr, self->device)) {
    GST_ERROR_OBJECT (self, "Couldn't create staging texture");
    return FALSE;
  }

  return TRUE;
}

static void
gst_d3d11_decoder_enable_high_precision_timer (GstD3D11Decoder * self)
{
#if HAVE_WINMM
  GstD3D11DeviceVendor vendor;

  if (self->timer_resolution)
    return;

  vendor = gst_d3d11_get_device_vendor (self->device);
  /* Do this only for NVIDIA at the moment, other vendors doesn't seem to be
   * requiring retry for BeginFrame() */
  if (vendor == GST_D3D11_DEVICE_VENDOR_NVIDIA) {
    TIMECAPS time_caps;
    if (timeGetDevCaps (&time_caps, sizeof (TIMECAPS)) == TIMERR_NOERROR) {
      guint resolution;
      MMRESULT ret;

      resolution = MIN (MAX (time_caps.wPeriodMin, 1), time_caps.wPeriodMax);

      ret = timeBeginPeriod (resolution);
      if (ret == TIMERR_NOERROR) {
        self->timer_resolution = resolution;
        GST_INFO_OBJECT (self, "Updated timer resolution to %d", resolution);
      }
    }
  }
#endif
}

static gboolean
gst_d3d11_decoder_open (GstD3D11Decoder * self)
{
  HRESULT hr;
  BOOL can_support = FALSE;
  guint config_count;
  D3D11_VIDEO_DECODER_CONFIG *config_list;
  D3D11_VIDEO_DECODER_CONFIG *best_config = NULL;
  D3D11_VIDEO_DECODER_DESC decoder_desc = { 0, };
  const GUID *selected_profile = NULL;
  guint i;
  gint aligned_width, aligned_height;
  guint alignment;
  GstD3D11DeviceVendor vendor;
  ID3D11VideoDevice *video_device;
  GstVideoInfo *info = &self->info;

  if (self->opened)
    return TRUE;

  if (!self->configured) {
    GST_ERROR_OBJECT (self, "Should configure first");
    return FALSE;
  }

  video_device = self->video_device;

  gst_d3d11_device_lock (self->device);
  if (!gst_d3d11_decoder_get_supported_decoder_profile (self->device,
          self->codec, GST_VIDEO_INFO_FORMAT (info), &selected_profile)) {
    goto error;
  }

  hr = video_device->CheckVideoDecoderFormat (selected_profile,
      self->decoder_format, &can_support);
  if (!gst_d3d11_result (hr, self->device) || !can_support) {
    GST_ERROR_OBJECT (self,
        "VideoDevice could not support dxgi format %d, hr: 0x%x",
        self->decoder_format, (guint) hr);
    goto error;
  }

  gst_d3d11_decoder_clear_resource (self);
  self->can_direct_rendering = TRUE;

  vendor = gst_d3d11_get_device_vendor (self->device);
  switch (vendor) {
    case GST_D3D11_DEVICE_VENDOR_XBOX:
      /* FIXME: Need to figure out Xbox device's behavior
       * https://gitlab.freedesktop.org/gstreamer/gst-plugins-bad/-/issues/1312
       */
      self->can_direct_rendering = FALSE;
      break;
    default:
      break;
  }

  /* NOTE: other dxva implementations (ffmpeg and vlc) do this
   * and they say the required alignment were mentioned by dxva spec.
   * See ff_dxva2_common_frame_params() in dxva.c of ffmpeg and
   * directx_va_Setup() in directx_va.c of vlc.
   * But... where it is? */
  switch (self->codec) {
    case GST_DXVA_CODEC_H265:
    case GST_DXVA_CODEC_AV1:
      /* See directx_va_Setup() impl. in vlc */
      if (vendor != GST_D3D11_DEVICE_VENDOR_XBOX)
        alignment = 128;
      else
        alignment = 16;
      break;
    case GST_DXVA_CODEC_MPEG2:
      /* XXX: ffmpeg does this */
      alignment = 32;
      break;
    default:
      alignment = 16;
      break;
  }

  aligned_width = GST_ROUND_UP_N (self->coded_width, alignment);
  aligned_height = GST_ROUND_UP_N (self->coded_height, alignment);
  if (aligned_width != self->coded_width ||
      aligned_height != self->coded_height) {
    GST_DEBUG_OBJECT (self,
        "coded resolution %dx%d is not aligned to %d, adjust to %dx%d",
        self->coded_width, self->coded_height, alignment, aligned_width,
        aligned_height);
  }

  self->aligned_width = aligned_width;
  self->aligned_height = aligned_height;

  decoder_desc.SampleWidth = aligned_width;
  decoder_desc.SampleHeight = aligned_height;
  decoder_desc.OutputFormat = self->decoder_format;
  decoder_desc.Guid = *selected_profile;

  hr = video_device->GetVideoDecoderConfigCount (&decoder_desc, &config_count);
  if (!gst_d3d11_result (hr, self->device) || config_count == 0) {
    GST_ERROR_OBJECT (self, "Could not get decoder config count, hr: 0x%x",
        (guint) hr);
    goto error;
  }

  GST_DEBUG_OBJECT (self, "Total %d config available", config_count);

  config_list = (D3D11_VIDEO_DECODER_CONFIG *)
      g_alloca (sizeof (D3D11_VIDEO_DECODER_CONFIG) * config_count);

  for (i = 0; i < config_count; i++) {
    hr = video_device->GetVideoDecoderConfig (&decoder_desc, i,
        &config_list[i]);
    if (!gst_d3d11_result (hr, self->device)) {
      GST_ERROR_OBJECT (self, "Could not get decoder %dth config, hr: 0x%x",
          i, (guint) hr);
      goto error;
    }

    /* FIXME: need support DXVA_Slice_H264_Long ?? */
    /* this config uses DXVA_Slice_H264_Short */
    switch (self->codec) {
      case GST_DXVA_CODEC_H264:
        if (config_list[i].ConfigBitstreamRaw == 2)
          best_config = &config_list[i];
        break;
      case GST_DXVA_CODEC_H265:
      case GST_DXVA_CODEC_VP9:
      case GST_DXVA_CODEC_VP8:
      case GST_DXVA_CODEC_MPEG2:
      case GST_DXVA_CODEC_AV1:
        if (config_list[i].ConfigBitstreamRaw == 1)
          best_config = &config_list[i];
        break;
      default:
        g_assert_not_reached ();
        goto error;
    }

    if (best_config)
      break;
  }

  if (best_config == NULL) {
    GST_ERROR_OBJECT (self, "Could not determine decoder config");
    goto error;
  }

  GST_DEBUG_OBJECT (self, "ConfigDecoderSpecific 0x%x",
      best_config->ConfigDecoderSpecific);

  /* bit 14 is equal to 1b means this config support array of texture and
   * it's recommended type as per DXVA spec */
  if ((best_config->ConfigDecoderSpecific & 0x4000) == 0x4000) {
    GST_DEBUG_OBJECT (self, "Config support array of texture");
    self->use_array_of_texture = TRUE;
  }

  hr = video_device->CreateVideoDecoder (&decoder_desc,
      best_config, &self->decoder_handle);
  if (!gst_d3d11_result (hr, self->device) || !self->decoder_handle) {
    GST_ERROR_OBJECT (self,
        "Could not create decoder object, hr: 0x%x", (guint) hr);
    goto error;
  }

  GST_DEBUG_OBJECT (self, "Decoder object %p created", self->decoder_handle);

  if (!self->downstream_supports_d3d11 &&
      !gst_d3d11_decoder_ensure_staging_texture (self)) {
    GST_ERROR_OBJECT (self, "Couldn't prepare staging texture");
    goto error;
  }

  self->decoder_profile = *selected_profile;

  /* Store pool related information here, then we will setup internal pool
   * later once the number of min buffer size required by downstream is known.
   * Actual buffer pool size will be "dpb_size + downstream_min_buffers"
   */
  self->downstream_min_buffers = 0;
  self->wait_on_pool_full = FALSE;

  self->opened = TRUE;
  gst_d3d11_device_unlock (self->device);

  gst_d3d11_decoder_enable_high_precision_timer (self);

  return TRUE;

error:
  gst_d3d11_decoder_reset (self);
  gst_d3d11_device_unlock (self->device);

  return FALSE;
}

static gboolean
gst_d3d11_decoder_begin_frame (GstD3D11Decoder * decoder,
    ID3D11VideoDecoderOutputView * output_view, guint content_key_size,
    gconstpointer content_key)
{
  ID3D11VideoContext *video_context;
  guint retry_count = 0;
  HRESULT hr;
  guint retry_threshold = 100;

  /* if we have high resolution timer, do more retry */
  if (decoder->timer_resolution)
    retry_threshold = 500;

  video_context = decoder->video_context;

  do {
    GST_LOG_OBJECT (decoder, "Try begin frame, retry count %d", retry_count);
    hr = video_context->DecoderBeginFrame (decoder->decoder_handle,
        output_view, content_key_size, content_key);

    /* HACK: Do retry with 1ms sleep per failure, since DXVA/D3D11
     * doesn't provide API for "GPU-IS-READY-TO-DECODE" like signal.
     */
    if (hr == E_PENDING && retry_count < retry_threshold) {
      GST_LOG_OBJECT (decoder, "GPU is busy, try again. Retry count %d",
          retry_count);
      g_usleep (1000);
    } else {
      if (gst_d3d11_result (hr, decoder->device))
        GST_LOG_OBJECT (decoder, "Succeeded with retry count %d", retry_count);
      break;
    }

    retry_count++;
  } while (TRUE);

  if (!gst_d3d11_result (hr, decoder->device)) {
    GST_ERROR_OBJECT (decoder, "Failed to begin frame, hr: 0x%x", (guint) hr);
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_d3d11_decoder_end_frame (GstD3D11Decoder * decoder)
{
  HRESULT hr;
  ID3D11VideoContext *video_context;

  video_context = decoder->video_context;
  hr = video_context->DecoderEndFrame (decoder->decoder_handle);

  if (!gst_d3d11_result (hr, decoder->device)) {
    GST_WARNING_OBJECT (decoder, "EndFrame failed, hr: 0x%x", (guint) hr);
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_d3d11_decoder_get_decoder_buffer (GstD3D11Decoder * decoder,
    D3D11_VIDEO_DECODER_BUFFER_TYPE type, guint * buffer_size,
    gpointer * buffer)
{
  UINT size;
  void *decoder_buffer;
  HRESULT hr;
  ID3D11VideoContext *video_context;

  video_context = decoder->video_context;
  hr = video_context->GetDecoderBuffer (decoder->decoder_handle,
      type, &size, &decoder_buffer);

  if (!gst_d3d11_result (hr, decoder->device)) {
    GST_WARNING_OBJECT (decoder, "Getting buffer type %d error, hr: 0x%x",
        type, (guint) hr);
    return FALSE;
  }

  *buffer_size = size;
  *buffer = decoder_buffer;

  return TRUE;
}

static gboolean
gst_d3d11_decoder_release_decoder_buffer (GstD3D11Decoder * decoder,
    D3D11_VIDEO_DECODER_BUFFER_TYPE type)
{
  HRESULT hr;
  ID3D11VideoContext *video_context;

  video_context = decoder->video_context;
  hr = video_context->ReleaseDecoderBuffer (decoder->decoder_handle, type);

  if (!gst_d3d11_result (hr, decoder->device)) {
    GST_WARNING_OBJECT (decoder, "ReleaseDecoderBuffer failed, hr: 0x%x",
        (guint) hr);
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_d3d11_decoder_submit_decoder_buffers (GstD3D11Decoder * decoder,
    guint buffer_count, const D3D11_VIDEO_DECODER_BUFFER_DESC * buffers)
{
  HRESULT hr;
  ID3D11VideoContext *video_context;

  video_context = decoder->video_context;
  hr = video_context->SubmitDecoderBuffers (decoder->decoder_handle,
      buffer_count, buffers);
  if (!gst_d3d11_result (hr, decoder->device)) {
    GST_WARNING_OBJECT (decoder, "SubmitDecoderBuffers failed, hr: 0x%x",
        (guint) hr);
    return FALSE;
  }

  return TRUE;
}

gboolean
gst_d3d11_decoder_decode_frame (GstD3D11Decoder * decoder,
    ID3D11VideoDecoderOutputView * output_view,
    GstD3D11DecodeInputStreamArgs * input_args)
{
  guint d3d11_buffer_size;
  gpointer d3d11_buffer;
  D3D11_VIDEO_DECODER_BUFFER_DESC buffer_desc[4];
  guint buffer_desc_size;

  g_return_val_if_fail (GST_IS_D3D11_DECODER (decoder), FALSE);
  g_return_val_if_fail (output_view != nullptr, FALSE);
  g_return_val_if_fail (input_args != nullptr, FALSE);

  memset (buffer_desc, 0, sizeof (buffer_desc));

  buffer_desc[0].BufferType = D3D11_VIDEO_DECODER_BUFFER_PICTURE_PARAMETERS;
  buffer_desc[0].DataSize = input_args->picture_params_size;

  buffer_desc[1].BufferType = D3D11_VIDEO_DECODER_BUFFER_SLICE_CONTROL;
  buffer_desc[1].DataSize = input_args->slice_control_size;

  buffer_desc[2].BufferType = D3D11_VIDEO_DECODER_BUFFER_BITSTREAM;
  buffer_desc[2].DataOffset = 0;
  buffer_desc[2].DataSize = input_args->bitstream_size;

  buffer_desc_size = 3;
  if (input_args->inverse_quantization_matrix &&
      input_args->inverse_quantization_matrix_size > 0) {
    buffer_desc[3].BufferType =
        D3D11_VIDEO_DECODER_BUFFER_INVERSE_QUANTIZATION_MATRIX;
    buffer_desc[3].DataSize = input_args->inverse_quantization_matrix_size;
    buffer_desc_size++;
  }

  gst_d3d11_device_lock (decoder->device);
  if (!gst_d3d11_decoder_begin_frame (decoder, output_view, 0, nullptr)) {
    gst_d3d11_device_unlock (decoder->device);

    return FALSE;
  }

  if (!gst_d3d11_decoder_get_decoder_buffer (decoder,
          D3D11_VIDEO_DECODER_BUFFER_PICTURE_PARAMETERS, &d3d11_buffer_size,
          &d3d11_buffer)) {
    GST_ERROR_OBJECT (decoder,
        "Failed to get decoder buffer for picture parameters");
    goto error;
  }

  if (d3d11_buffer_size < input_args->picture_params_size) {
    GST_ERROR_OBJECT (decoder,
        "Too small picture param buffer size %d", d3d11_buffer_size);

    gst_d3d11_decoder_release_decoder_buffer (decoder,
        D3D11_VIDEO_DECODER_BUFFER_PICTURE_PARAMETERS);
    goto error;
  }

  memcpy (d3d11_buffer, input_args->picture_params,
      input_args->picture_params_size);

  if (!gst_d3d11_decoder_release_decoder_buffer (decoder,
          D3D11_VIDEO_DECODER_BUFFER_PICTURE_PARAMETERS)) {
    GST_ERROR_OBJECT (decoder, "Failed to release picture param buffer");
    goto error;
  }

  if (!gst_d3d11_decoder_get_decoder_buffer (decoder,
          D3D11_VIDEO_DECODER_BUFFER_SLICE_CONTROL, &d3d11_buffer_size,
          &d3d11_buffer)) {
    GST_ERROR_OBJECT (decoder, "Failed to get slice control buffer");
    goto error;
  }

  if (d3d11_buffer_size < input_args->slice_control_size) {
    GST_ERROR_OBJECT (decoder,
        "Too small slice control buffer size %d", d3d11_buffer_size);

    gst_d3d11_decoder_release_decoder_buffer (decoder,
        D3D11_VIDEO_DECODER_BUFFER_SLICE_CONTROL);
    goto error;
  }

  memcpy (d3d11_buffer,
      input_args->slice_control, input_args->slice_control_size);

  if (!gst_d3d11_decoder_release_decoder_buffer (decoder,
          D3D11_VIDEO_DECODER_BUFFER_SLICE_CONTROL)) {
    GST_ERROR_OBJECT (decoder, "Failed to release slice control buffer");
    goto error;
  }

  if (!gst_d3d11_decoder_get_decoder_buffer (decoder,
          D3D11_VIDEO_DECODER_BUFFER_BITSTREAM, &d3d11_buffer_size,
          &d3d11_buffer)) {
    GST_ERROR_OBJECT (decoder, "Failed to get bitstream buffer");
    goto error;
  }

  if (d3d11_buffer_size < input_args->bitstream_size) {
    GST_ERROR_OBJECT (decoder, "Too small bitstream buffer size %d",
        d3d11_buffer_size);

    gst_d3d11_decoder_release_decoder_buffer (decoder,
        D3D11_VIDEO_DECODER_BUFFER_BITSTREAM);
    goto error;
  }

  memcpy (d3d11_buffer, input_args->bitstream, input_args->bitstream_size);

  if (!gst_d3d11_decoder_release_decoder_buffer (decoder,
          D3D11_VIDEO_DECODER_BUFFER_BITSTREAM)) {
    GST_ERROR_OBJECT (decoder, "Failed to release bitstream buffer");
    goto error;
  }

  if (input_args->inverse_quantization_matrix_size > 0) {
    if (!gst_d3d11_decoder_get_decoder_buffer (decoder,
            D3D11_VIDEO_DECODER_BUFFER_INVERSE_QUANTIZATION_MATRIX,
            &d3d11_buffer_size, &d3d11_buffer)) {
      GST_ERROR_OBJECT (decoder,
          "Failed to get inverse quantization matrix buffer");
      goto error;
    }

    if (d3d11_buffer_size < input_args->inverse_quantization_matrix_size) {
      GST_ERROR_OBJECT (decoder,
          "Too small inverse quantization matrix buffer buffer %d",
          d3d11_buffer_size);

      gst_d3d11_decoder_release_decoder_buffer (decoder,
          D3D11_VIDEO_DECODER_BUFFER_INVERSE_QUANTIZATION_MATRIX);
      goto error;
    }

    memcpy (d3d11_buffer, input_args->inverse_quantization_matrix,
        input_args->inverse_quantization_matrix_size);

    if (!gst_d3d11_decoder_release_decoder_buffer (decoder,
            D3D11_VIDEO_DECODER_BUFFER_INVERSE_QUANTIZATION_MATRIX)) {
      GST_ERROR_OBJECT (decoder,
          "Failed to release inverse quantization matrix buffer");
      goto error;
    }
  }

  if (!gst_d3d11_decoder_submit_decoder_buffers (decoder,
          buffer_desc_size, buffer_desc)) {
    GST_ERROR_OBJECT (decoder, "Failed to submit decoder buffers");
    goto error;
  }

  if (!gst_d3d11_decoder_end_frame (decoder)) {
    gst_d3d11_device_unlock (decoder->device);
    return FALSE;
  }

  gst_d3d11_device_unlock (decoder->device);

  return TRUE;

error:
  gst_d3d11_decoder_end_frame (decoder);
  gst_d3d11_device_unlock (decoder->device);
  return FALSE;
}

GstBuffer *
gst_d3d11_decoder_get_output_view_buffer (GstD3D11Decoder * decoder,
    GstVideoDecoder * videodec)
{
  GstBuffer *buf = NULL;
  GstFlowReturn ret;

  g_return_val_if_fail (GST_IS_D3D11_DECODER (decoder), FALSE);

  if (!decoder->internal_pool) {
    /* Try negotiate again whatever the previous negotiation result was.
     * There could be updated field(s) in sinkpad caps after we negotiated with
     * downstream on new_sequence() call. For example, h264/h265 parse
     * will be able to update HDR10 related caps field after parsing
     * corresponding SEI messages which are usually placed after the essential
     * headers */
    gst_video_decoder_negotiate (videodec);

    if (!gst_d3d11_decoder_prepare_output_view_pool (decoder)) {
      GST_ERROR_OBJECT (videodec, "Failed to setup internal pool");
      return NULL;
    }
  } else if (!gst_buffer_pool_set_active (decoder->internal_pool, TRUE)) {
    GST_ERROR_OBJECT (videodec, "Couldn't set active internal pool");
    return NULL;
  }

  ret = gst_buffer_pool_acquire_buffer (decoder->internal_pool, &buf, NULL);

  if (ret != GST_FLOW_OK || !buf) {
    if (ret != GST_FLOW_FLUSHING) {
      GST_ERROR_OBJECT (videodec, "Couldn't get buffer from pool, ret %s",
          gst_flow_get_name (ret));
    } else {
      GST_DEBUG_OBJECT (videodec, "We are flusing");
    }

    return NULL;
  }

  if (!gst_d3d11_decoder_ensure_output_view (decoder, buf)) {
    GST_ERROR_OBJECT (videodec, "Output view unavailable");
    gst_buffer_unref (buf);

    return NULL;
  }

  return buf;
}

ID3D11VideoDecoderOutputView *
gst_d3d11_decoder_get_output_view_from_buffer (GstD3D11Decoder * decoder,
    GstBuffer * buffer, guint8 * index)
{
  GstMemory *mem;
  GstD3D11Memory *dmem;
  ID3D11VideoDecoderOutputView *view;

  g_return_val_if_fail (GST_IS_D3D11_DECODER (decoder), NULL);
  g_return_val_if_fail (GST_IS_BUFFER (buffer), NULL);

  mem = gst_buffer_peek_memory (buffer, 0);
  if (!gst_is_d3d11_memory (mem)) {
    GST_WARNING_OBJECT (decoder, "Not a d3d11 memory");
    return NULL;
  }

  dmem = (GstD3D11Memory *) mem;
  view = gst_d3d11_memory_get_decoder_output_view (dmem, decoder->video_device,
      &decoder->decoder_profile);

  if (!view) {
    GST_ERROR_OBJECT (decoder, "Decoder output view is unavailable");
    return NULL;
  }

  if (index) {
    if (decoder->use_array_of_texture) {
      guint8 id;
      gpointer val = gst_mini_object_get_qdata (GST_MINI_OBJECT (mem),
          gst_d3d11_decoder_view_id_quark ());
      if (!val) {
        GST_ERROR_OBJECT (decoder, "memory has no qdata");
        return NULL;
      }

      id = (guint8) GPOINTER_TO_UINT (val);
      g_assert (id < 128);

      *index = (id - 1);
    } else {
      *index = gst_d3d11_memory_get_subresource_index (dmem);
    }
  }

  return view;
}

gboolean
gst_d3d11_decoder_process_output (GstD3D11Decoder * decoder,
    GstVideoDecoder * videodec, gint display_width, gint display_height,
    GstBuffer * decoder_buffer, GstBuffer ** output)
{
  g_return_val_if_fail (GST_IS_D3D11_DECODER (decoder), FALSE);
  g_return_val_if_fail (GST_IS_VIDEO_DECODER (videodec), FALSE);
  g_return_val_if_fail (GST_IS_BUFFER (decoder_buffer), FALSE);
  g_return_val_if_fail (output != NULL, FALSE);

  if (display_width != GST_VIDEO_INFO_WIDTH (&decoder->output_info) ||
      display_height != GST_VIDEO_INFO_HEIGHT (&decoder->output_info)) {
    GST_INFO_OBJECT (videodec, "Frame size changed, do renegotiate");

    gst_video_info_set_format (&decoder->output_info,
        GST_VIDEO_INFO_FORMAT (&decoder->info), display_width, display_height);
    GST_VIDEO_INFO_INTERLACE_MODE (&decoder->output_info) =
        GST_VIDEO_INFO_INTERLACE_MODE (&decoder->info);

    if (!gst_video_decoder_negotiate (videodec)) {
      GST_ERROR_OBJECT (videodec, "Failed to re-negotiate with new frame size");
      return FALSE;
    }
  }

  if (gst_d3d11_decoder_can_direct_render (decoder, videodec, decoder_buffer,
          display_width, display_height)) {
    GstMemory *mem;

    mem = gst_buffer_peek_memory (decoder_buffer, 0);
    GST_MINI_OBJECT_FLAG_SET (mem, GST_D3D11_MEMORY_TRANSFER_NEED_DOWNLOAD);

    *output = gst_buffer_ref (decoder_buffer);

    return TRUE;
  }

  *output = gst_video_decoder_allocate_output_buffer (videodec);
  if (*output == NULL) {
    GST_ERROR_OBJECT (videodec, "Couldn't allocate output buffer");

    return FALSE;
  }

  return gst_d3d11_buffer_copy_into (*output,
      decoder_buffer, &decoder->output_info);
}

gboolean
gst_d3d11_decoder_negotiate (GstD3D11Decoder * decoder,
    GstVideoDecoder * videodec)
{
  GstVideoInfo *info;
  GstCaps *peer_caps;
  GstVideoCodecState *state = NULL;
  gboolean alternate_interlaced;
  gboolean alternate_supported = FALSE;
  gboolean d3d11_supported = FALSE;
  GstVideoCodecState *input_state;
  GstStructure *s;
  const gchar *str;

  g_return_val_if_fail (GST_IS_D3D11_DECODER (decoder), FALSE);
  g_return_val_if_fail (GST_IS_VIDEO_DECODER (videodec), FALSE);

  info = &decoder->output_info;
  input_state = decoder->input_state;

  alternate_interlaced =
      (GST_VIDEO_INFO_INTERLACE_MODE (info) ==
      GST_VIDEO_INTERLACE_MODE_ALTERNATE);

  peer_caps = gst_pad_get_allowed_caps (GST_VIDEO_DECODER_SRC_PAD (videodec));
  GST_DEBUG_OBJECT (videodec, "Allowed caps %" GST_PTR_FORMAT, peer_caps);

  if (!peer_caps || gst_caps_is_any (peer_caps)) {
    GST_DEBUG_OBJECT (videodec,
        "cannot determine output format, use system memory");
  } else {
    GstCapsFeatures *features;
    guint size = gst_caps_get_size (peer_caps);
    guint i;

    for (i = 0; i < size; i++) {
      features = gst_caps_get_features (peer_caps, i);

      if (!features)
        continue;

      if (gst_caps_features_contains (features,
              GST_CAPS_FEATURE_MEMORY_D3D11_MEMORY)) {
        d3d11_supported = TRUE;
      }

      /* FIXME: software deinterlace element will not return interlaced caps
       * feature... We should fix it */
      if (gst_caps_features_contains (features,
              GST_CAPS_FEATURE_FORMAT_INTERLACED)) {
        alternate_supported = TRUE;
      }
    }
  }
  gst_clear_caps (&peer_caps);

  GST_DEBUG_OBJECT (videodec,
      "Downstream feature support, D3D11 memory: %d, interlaced format %d",
      d3d11_supported, alternate_supported);

  if (alternate_interlaced) {
    /* FIXME: D3D11 cannot support alternating interlaced stream yet */
    GST_FIXME_OBJECT (videodec,
        "Implement alternating interlaced stream for D3D11");

    if (alternate_supported) {
      gint height = GST_VIDEO_INFO_HEIGHT (info);

      /* Set caps resolution with display size, that's how we designed
       * for alternating interlaced stream */
      height = 2 * height;
      state = gst_video_decoder_set_interlaced_output_state (videodec,
          GST_VIDEO_INFO_FORMAT (info), GST_VIDEO_INFO_INTERLACE_MODE (info),
          GST_VIDEO_INFO_WIDTH (info), height, input_state);
    } else {
      GST_WARNING_OBJECT (videodec,
          "Downstream doesn't support alternating interlaced stream");

      state = gst_video_decoder_set_output_state (videodec,
          GST_VIDEO_INFO_FORMAT (info), GST_VIDEO_INFO_WIDTH (info),
          GST_VIDEO_INFO_HEIGHT (info), input_state);

      /* XXX: adjust PAR, this would produce output similar to that of
       * "line doubling" (so called bob deinterlacing) processing.
       * apart from missing anchor line (top-field or bottom-field) information.
       * Potentially flickering could happen. So this might not be correct.
       * But it would be better than negotiation error of half-height squeezed
       * image */
      state->info.par_d *= 2;
      state->info.fps_n *= 2;
    }
  } else {
    state = gst_video_decoder_set_interlaced_output_state (videodec,
        GST_VIDEO_INFO_FORMAT (info), GST_VIDEO_INFO_INTERLACE_MODE (info),
        GST_VIDEO_INFO_WIDTH (info), GST_VIDEO_INFO_HEIGHT (info), input_state);
  }

  if (!state) {
    GST_ERROR_OBJECT (decoder, "Couldn't set output state");
    return FALSE;
  }

  state->caps = gst_video_info_to_caps (&state->info);

  s = gst_caps_get_structure (input_state->caps, 0);
  str = gst_structure_get_string (s, "mastering-display-info");
  if (str) {
    gst_caps_set_simple (state->caps,
        "mastering-display-info", G_TYPE_STRING, str, nullptr);
  }

  str = gst_structure_get_string (s, "content-light-level");
  if (str) {
    gst_caps_set_simple (state->caps,
        "content-light-level", G_TYPE_STRING, str, nullptr);
  }

  g_clear_pointer (&decoder->output_state, gst_video_codec_state_unref);
  decoder->output_state = state;

  if (d3d11_supported) {
    gst_caps_set_features (state->caps, 0,
        gst_caps_features_new (GST_CAPS_FEATURE_MEMORY_D3D11_MEMORY, NULL));
  }

  decoder->downstream_supports_d3d11 = d3d11_supported;

  return gst_d3d11_decoder_open (decoder);
}

gboolean
gst_d3d11_decoder_decide_allocation (GstD3D11Decoder * decoder,
    GstVideoDecoder * videodec, GstQuery * query)
{
  GstCaps *outcaps;
  GstBufferPool *pool = NULL;
  guint n, size, min = 0, max = 0;
  GstVideoInfo vinfo = { 0, };
  GstStructure *config;
  GstD3D11AllocationParams *d3d11_params;
  gboolean use_d3d11_pool;
  gboolean has_videometa;

  g_return_val_if_fail (GST_IS_D3D11_DECODER (decoder), FALSE);
  g_return_val_if_fail (GST_IS_VIDEO_DECODER (videodec), FALSE);
  g_return_val_if_fail (query != NULL, FALSE);

  if (!decoder->opened) {
    GST_ERROR_OBJECT (videodec, "Should open decoder first");
    return FALSE;
  }

  gst_query_parse_allocation (query, &outcaps, NULL);

  if (!outcaps) {
    GST_DEBUG_OBJECT (decoder, "No output caps");
    return FALSE;
  }

  has_videometa = gst_query_find_allocation_meta (query,
      GST_VIDEO_META_API_TYPE, nullptr);

  use_d3d11_pool = decoder->downstream_supports_d3d11;

  gst_video_info_from_caps (&vinfo, outcaps);
  n = gst_query_get_n_allocation_pools (query);
  if (n > 0)
    gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);

  /* create our own pool */
  if (pool) {
    if (use_d3d11_pool) {
      if (!GST_IS_D3D11_BUFFER_POOL (pool)) {
        GST_DEBUG_OBJECT (videodec,
            "Downstream pool is not d3d11, will create new one");
        gst_clear_object (&pool);
      } else {
        GstD3D11BufferPool *dpool = GST_D3D11_BUFFER_POOL (pool);
        if (dpool->device != decoder->device) {
          GST_DEBUG_OBJECT (videodec, "Different device, will create new one");
          gst_clear_object (&pool);
        }
      }
    } else if (has_videometa) {
      /* We will use d3d11 staging buffer pool */
      gst_clear_object (&pool);
    }
  }

  if (!pool) {
    if (use_d3d11_pool)
      pool = gst_d3d11_buffer_pool_new (decoder->device);
    else if (has_videometa)
      pool = gst_d3d11_staging_buffer_pool_new (decoder->device);
    else
      pool = gst_video_buffer_pool_new ();

    size = (guint) vinfo.size;
  }

  config = gst_buffer_pool_get_config (pool);
  gst_buffer_pool_config_set_params (config, outcaps, size, min, max);
  gst_buffer_pool_config_add_option (config, GST_BUFFER_POOL_OPTION_VIDEO_META);

  if (use_d3d11_pool) {
    GstVideoAlignment align;
    gint width, height;

    gst_video_alignment_reset (&align);

    d3d11_params = gst_buffer_pool_config_get_d3d11_allocation_params (config);
    if (!d3d11_params)
      d3d11_params = gst_d3d11_allocation_params_new (decoder->device, &vinfo,
          (GstD3D11AllocationFlags) 0, 0);

    width = GST_VIDEO_INFO_WIDTH (&vinfo);
    height = GST_VIDEO_INFO_HEIGHT (&vinfo);

    /* need alignment to copy decoder output texture to downstream texture */
    align.padding_right = GST_ROUND_UP_16 (width) - width;
    align.padding_bottom = GST_ROUND_UP_16 (height) - height;
    if (!gst_d3d11_allocation_params_alignment (d3d11_params, &align)) {
      GST_ERROR_OBJECT (videodec, "Cannot set alignment");
      return FALSE;
    }

    /* Needs render target bind flag so that it can be used for
     * output of shader pipeline if internal resizing is required.
     * Also, downstream can keep using video processor even if we copy
     * some decoded textures into downstream buffer */
    d3d11_params->desc[0].BindFlags |= D3D11_BIND_RENDER_TARGET;

    gst_buffer_pool_config_set_d3d11_allocation_params (config, d3d11_params);
    gst_d3d11_allocation_params_free (d3d11_params);

    /* Store min buffer size. We need to take account of the amount of buffers
     * which might be held by downstream in case of zero-copy playback */
    if (!decoder->internal_pool) {
      if (n > 0) {
        GST_DEBUG_OBJECT (videodec, "Downstream proposed pool");
        decoder->wait_on_pool_full = TRUE;
        /* XXX: hardcoded bound 16, to avoid too large pool size */
        decoder->downstream_min_buffers = MIN (min, 16);
      } else {
        GST_DEBUG_OBJECT (videodec, "Downstream didn't propose pool");
        decoder->wait_on_pool_full = FALSE;
        /* don't know how many buffers would be queued by downstream */
        decoder->downstream_min_buffers = 4;
      }
    } else {
      /* We configured our DPB pool already, let's check if our margin can
       * cover min size */
      decoder->wait_on_pool_full = FALSE;

      if (n > 0) {
        if (decoder->downstream_min_buffers >= min)
          decoder->wait_on_pool_full = TRUE;

        GST_DEBUG_OBJECT (videodec,
            "Pre-allocated margin %d can%s cover downstream min size %d",
            decoder->downstream_min_buffers,
            decoder->wait_on_pool_full ? "" : "not", min);
      } else {
        GST_DEBUG_OBJECT (videodec, "Downstream min size is unknown");
      }
    }

    GST_DEBUG_OBJECT (videodec, "Downstream min buffres: %d", min);

    /* We will not use downstream pool for decoding, and therefore preallocation
     * is unnecessary. So, Non-zero min buffer will be a waste of GPU memory */
    min = 0;
  }

  gst_buffer_pool_set_config (pool, config);
  /* d3d11 buffer pool will update buffer size based on allocated texture,
   * get size from config again */
  config = gst_buffer_pool_get_config (pool);
  gst_buffer_pool_config_get_params (config, nullptr, &size, nullptr, nullptr);
  gst_structure_free (config);

  if (n > 0)
    gst_query_set_nth_allocation_pool (query, 0, pool, size, min, max);
  else
    gst_query_add_allocation_pool (query, pool, size, min, max);
  gst_object_unref (pool);

  return TRUE;
}

gboolean
gst_d3d11_decoder_set_flushing (GstD3D11Decoder * decoder,
    GstVideoDecoder * videodec, gboolean flushing)
{
  g_return_val_if_fail (GST_IS_D3D11_DECODER (decoder), FALSE);

  g_mutex_lock (&decoder->internal_pool_lock);
  if (decoder->internal_pool)
    gst_buffer_pool_set_flushing (decoder->internal_pool, flushing);
  g_mutex_unlock (&decoder->internal_pool_lock);

  return TRUE;
}

static gboolean
gst_d3d11_decoder_can_direct_render (GstD3D11Decoder * decoder,
    GstVideoDecoder * videodec, GstBuffer * view_buffer,
    gint display_width, gint display_height)
{
  GstMemory *mem;
  GstD3D11PoolAllocator *alloc;
  guint max_size = 0, outstanding_size = 0;

  /* We don't support direct render for reverse playback */
  if (videodec->input_segment.rate < 0)
    return FALSE;

  if (!decoder->can_direct_rendering || !decoder->downstream_supports_d3d11)
    return FALSE;

  /* different size, need copy */
  /* TODO: crop meta */
  if (display_width != GST_VIDEO_INFO_WIDTH (&decoder->info) ||
      display_height != GST_VIDEO_INFO_HEIGHT (&decoder->info))
    return FALSE;

  /* we can do direct render in this case, since there is no DPB pool size
   * limit */
  if (decoder->use_array_of_texture)
    return TRUE;

  /* Let's believe downstream info */
  if (decoder->wait_on_pool_full)
    return TRUE;

  /* Check if we are about to full */
  mem = gst_buffer_peek_memory (view_buffer, 0);

  /* something went wrong */
  if (!gst_is_d3d11_memory (mem)) {
    GST_ERROR_OBJECT (decoder, "Not a D3D11 memory");
    return FALSE;
  }

  alloc = GST_D3D11_POOL_ALLOCATOR (mem->allocator);
  if (!gst_d3d11_pool_allocator_get_pool_size (alloc, &max_size,
          &outstanding_size)) {
    GST_ERROR_OBJECT (decoder, "Couldn't query pool size");
    return FALSE;
  }

  /* 2 buffer margin */
  if (max_size <= outstanding_size + 1) {
    GST_DEBUG_OBJECT (decoder, "memory pool is about to full (%u/%u)",
        outstanding_size, max_size);
    return FALSE;
  }

  GST_LOG_OBJECT (decoder, "Can do direct rendering");

  return TRUE;
}

/* Keep sync with chromium and keep in sorted order.
 * See supported_profile_helpers.cc in chromium */
static const guint legacy_amd_list[] = {
  0x130f, 0x6700, 0x6701, 0x6702, 0x6703, 0x6704, 0x6705, 0x6706, 0x6707,
  0x6708, 0x6709, 0x6718, 0x6719, 0x671c, 0x671d, 0x671f, 0x6720, 0x6721,
  0x6722, 0x6723, 0x6724, 0x6725, 0x6726, 0x6727, 0x6728, 0x6729, 0x6738,
  0x6739, 0x673e, 0x6740, 0x6741, 0x6742, 0x6743, 0x6744, 0x6745, 0x6746,
  0x6747, 0x6748, 0x6749, 0x674a, 0x6750, 0x6751, 0x6758, 0x6759, 0x675b,
  0x675d, 0x675f, 0x6760, 0x6761, 0x6762, 0x6763, 0x6764, 0x6765, 0x6766,
  0x6767, 0x6768, 0x6770, 0x6771, 0x6772, 0x6778, 0x6779, 0x677b, 0x6798,
  0x67b1, 0x6821, 0x683d, 0x6840, 0x6841, 0x6842, 0x6843, 0x6849, 0x6850,
  0x6858, 0x6859, 0x6880, 0x6888, 0x6889, 0x688a, 0x688c, 0x688d, 0x6898,
  0x6899, 0x689b, 0x689c, 0x689d, 0x689e, 0x68a0, 0x68a1, 0x68a8, 0x68a9,
  0x68b0, 0x68b8, 0x68b9, 0x68ba, 0x68be, 0x68bf, 0x68c0, 0x68c1, 0x68c7,
  0x68c8, 0x68c9, 0x68d8, 0x68d9, 0x68da, 0x68de, 0x68e0, 0x68e1, 0x68e4,
  0x68e5, 0x68e8, 0x68e9, 0x68f1, 0x68f2, 0x68f8, 0x68f9, 0x68fa, 0x68fe,
  0x9400, 0x9401, 0x9402, 0x9403, 0x9405, 0x940a, 0x940b, 0x940f, 0x9440,
  0x9441, 0x9442, 0x9443, 0x9444, 0x9446, 0x944a, 0x944b, 0x944c, 0x944e,
  0x9450, 0x9452, 0x9456, 0x945a, 0x945b, 0x945e, 0x9460, 0x9462, 0x946a,
  0x946b, 0x947a, 0x947b, 0x9480, 0x9487, 0x9488, 0x9489, 0x948a, 0x948f,
  0x9490, 0x9491, 0x9495, 0x9498, 0x949c, 0x949e, 0x949f, 0x94a0, 0x94a1,
  0x94a3, 0x94b1, 0x94b3, 0x94b4, 0x94b5, 0x94b9, 0x94c0, 0x94c1, 0x94c3,
  0x94c4, 0x94c5, 0x94c6, 0x94c7, 0x94c8, 0x94c9, 0x94cb, 0x94cc, 0x94cd,
  0x9500, 0x9501, 0x9504, 0x9505, 0x9506, 0x9507, 0x9508, 0x9509, 0x950f,
  0x9511, 0x9515, 0x9517, 0x9519, 0x9540, 0x9541, 0x9542, 0x954e, 0x954f,
  0x9552, 0x9553, 0x9555, 0x9557, 0x955f, 0x9580, 0x9581, 0x9583, 0x9586,
  0x9587, 0x9588, 0x9589, 0x958a, 0x958b, 0x958c, 0x958d, 0x958e, 0x958f,
  0x9590, 0x9591, 0x9593, 0x9595, 0x9596, 0x9597, 0x9598, 0x9599, 0x959b,
  0x95c0, 0x95c2, 0x95c4, 0x95c5, 0x95c6, 0x95c7, 0x95c9, 0x95cc, 0x95cd,
  0x95ce, 0x95cf, 0x9610, 0x9611, 0x9612, 0x9613, 0x9614, 0x9615, 0x9616,
  0x9640, 0x9641, 0x9642, 0x9643, 0x9644, 0x9645, 0x9647, 0x9648, 0x9649,
  0x964a, 0x964b, 0x964c, 0x964e, 0x964f, 0x9710, 0x9711, 0x9712, 0x9713,
  0x9714, 0x9715, 0x9802, 0x9803, 0x9804, 0x9805, 0x9806, 0x9807, 0x9808,
  0x9809, 0x980a, 0x9830, 0x983d, 0x9850, 0x9851, 0x9874, 0x9900, 0x9901,
  0x9903, 0x9904, 0x9905, 0x9906, 0x9907, 0x9908, 0x9909, 0x990a, 0x990b,
  0x990c, 0x990d, 0x990e, 0x990f, 0x9910, 0x9913, 0x9917, 0x9918, 0x9919,
  0x9990, 0x9991, 0x9992, 0x9993, 0x9994, 0x9995, 0x9996, 0x9997, 0x9998,
  0x9999, 0x999a, 0x999b, 0x999c, 0x999d, 0x99a0, 0x99a2, 0x99a4
};

static const guint legacy_intel_list[] = {
  0x102, 0x106, 0x116, 0x126, 0x152, 0x156, 0x166,
  0x402, 0x406, 0x416, 0x41e, 0xa06, 0xa16, 0xf31,
};

static gint
binary_search_compare (const guint * a, const guint * b)
{
  return *a - *b;
}

/* Certain AMD GPU drivers like R600, R700, Evergreen and Cayman and some second
 * generation Intel GPU drivers crash if we create a video device with a
 * resolution higher then 1920 x 1088. This function checks if the GPU is in
 * this list and if yes returns true. */
gboolean
gst_d3d11_decoder_util_is_legacy_device (GstD3D11Device * device)
{
  const guint amd_id[] = { 0x1002, 0x1022 };
  const guint intel_id = 0x8086;
  guint device_id = 0;
  guint vendor_id = 0;
  guint *match = NULL;

  g_return_val_if_fail (GST_IS_D3D11_DEVICE (device), FALSE);

  g_object_get (device, "device-id", &device_id, "vendor-id", &vendor_id, NULL);

  if (vendor_id == amd_id[0] || vendor_id == amd_id[1]) {
    match =
        (guint *) gst_util_array_binary_search ((gpointer) legacy_amd_list,
        G_N_ELEMENTS (legacy_amd_list), sizeof (guint),
        (GCompareDataFunc) binary_search_compare,
        GST_SEARCH_MODE_EXACT, &device_id, NULL);
  } else if (vendor_id == intel_id) {
    match =
        (guint *) gst_util_array_binary_search ((gpointer) legacy_intel_list,
        G_N_ELEMENTS (legacy_intel_list), sizeof (guint),
        (GCompareDataFunc) binary_search_compare,
        GST_SEARCH_MODE_EXACT, &device_id, NULL);
  }

  if (match) {
    GST_DEBUG_OBJECT (device, "it's legacy device");
    return TRUE;
  }

  return FALSE;
}

gboolean
gst_d3d11_decoder_supports_format (GstD3D11Device * device,
    const GUID * decoder_profile, DXGI_FORMAT format)
{
  HRESULT hr;
  BOOL can_support = FALSE;
  ID3D11VideoDevice *video_device;

  g_return_val_if_fail (GST_IS_D3D11_DEVICE (device), FALSE);
  g_return_val_if_fail (decoder_profile != NULL, FALSE);
  g_return_val_if_fail (format != DXGI_FORMAT_UNKNOWN, FALSE);

  video_device = gst_d3d11_device_get_video_device_handle (device);
  if (!video_device)
    return FALSE;

  hr = video_device->CheckVideoDecoderFormat (decoder_profile, format,
      &can_support);
  if (!gst_d3d11_result (hr, device) || !can_support) {
    GST_DEBUG_OBJECT (device,
        "VideoDevice could not support dxgi format %d, hr: 0x%x",
        format, (guint) hr);

    return FALSE;
  }

  return TRUE;
}

/* Don't call this method with legacy device */
gboolean
gst_d3d11_decoder_supports_resolution (GstD3D11Device * device,
    const GUID * decoder_profile, DXGI_FORMAT format, guint width, guint height)
{
  D3D11_VIDEO_DECODER_DESC desc;
  HRESULT hr;
  UINT config_count;
  ID3D11VideoDevice *video_device;

  g_return_val_if_fail (GST_IS_D3D11_DEVICE (device), FALSE);
  g_return_val_if_fail (decoder_profile != NULL, FALSE);
  g_return_val_if_fail (format != DXGI_FORMAT_UNKNOWN, FALSE);

  video_device = gst_d3d11_device_get_video_device_handle (device);
  if (!video_device)
    return FALSE;

  desc.SampleWidth = width;
  desc.SampleHeight = height;
  desc.OutputFormat = format;
  desc.Guid = *decoder_profile;

  hr = video_device->GetVideoDecoderConfigCount (&desc, &config_count);
  if (!gst_d3d11_result (hr, device) || config_count == 0) {
    GST_DEBUG_OBJECT (device, "Could not get decoder config count, hr: 0x%x",
        (guint) hr);
    return FALSE;
  }

  return TRUE;
}

enum
{
  PROP_DECODER_ADAPTER_LUID = 1,
  PROP_DECODER_DEVICE_ID,
  PROP_DECODER_VENDOR_ID,
};

struct _GstD3D11DecoderClassData
{
  GstD3D11DecoderSubClassData subclass_data;
  GstCaps *sink_caps;
  GstCaps *src_caps;
  gchar *description;
};

/**
 * gst_d3d11_decoder_class_data_new:
 * @device: (transfer none): a #GstD3D11Device
 * @sink_caps: (transfer full): a #GstCaps
 * @src_caps: (transfer full): a #GstCaps
 * @max_resolution: maximum supported resolution
 *
 * Create new #GstD3D11DecoderClassData
 *
 * Returns: (transfer full): the new #GstD3D11DecoderClassData
 */
GstD3D11DecoderClassData *
gst_d3d11_decoder_class_data_new (GstD3D11Device * device, GstDXVACodec codec,
    GstCaps * sink_caps, GstCaps * src_caps, guint max_resolution)
{
  GstD3D11DecoderClassData *ret;
  guint min_width = 1;
  guint min_height = 1;

  g_return_val_if_fail (GST_IS_D3D11_DEVICE (device), NULL);
  g_return_val_if_fail (sink_caps != NULL, NULL);
  g_return_val_if_fail (src_caps != NULL, NULL);

  /* FIXME: D3D11/DXVA does not have an API for querying minimum resolution
   * capability. Might need to find a nice way for testing minimum resolution.
   *
   * Below hardcoded values were checked on RTX 2080/3060 GPUs via NVDEC API
   * (VP8 decoding is not supported by those GPUs via D3D11/DXVA) */
  if (gst_d3d11_get_device_vendor (device) == GST_D3D11_DEVICE_VENDOR_NVIDIA) {
    switch (codec) {
      case GST_DXVA_CODEC_MPEG2:
      case GST_DXVA_CODEC_H264:
      case GST_DXVA_CODEC_VP8:
        min_width = 48;
        min_height = 16;
        break;
      case GST_DXVA_CODEC_H265:
        min_width = min_height = 144;
        break;
      case GST_DXVA_CODEC_VP9:
      case GST_DXVA_CODEC_AV1:
        min_width = min_height = 128;
        break;
      default:
        g_assert_not_reached ();
        return nullptr;
    }
  }

  ret = g_new0 (GstD3D11DecoderClassData, 1);

  gst_caps_set_simple (sink_caps,
      "width", GST_TYPE_INT_RANGE, min_width, max_resolution,
      "height", GST_TYPE_INT_RANGE, min_height, max_resolution, nullptr);
  gst_caps_set_simple (src_caps,
      "width", GST_TYPE_INT_RANGE, min_width, max_resolution,
      "height", GST_TYPE_INT_RANGE, min_height, max_resolution, nullptr);

  /* class data will be leaked if the element never gets instantiated */
  GST_MINI_OBJECT_FLAG_SET (sink_caps, GST_MINI_OBJECT_FLAG_MAY_BE_LEAKED);
  GST_MINI_OBJECT_FLAG_SET (src_caps, GST_MINI_OBJECT_FLAG_MAY_BE_LEAKED);

  ret->subclass_data.codec = codec;
  g_object_get (device, "adapter-luid", &ret->subclass_data.adapter_luid,
      "device-id", &ret->subclass_data.device_id,
      "vendor-id", &ret->subclass_data.vendor_id,
      "description", &ret->description, nullptr);
  ret->sink_caps = sink_caps;
  ret->src_caps = src_caps;

  return ret;
}

void
gst_d3d11_decoder_class_data_fill_subclass_data (GstD3D11DecoderClassData *
    data, GstD3D11DecoderSubClassData * subclass_data)
{
  g_return_if_fail (data != nullptr);
  g_return_if_fail (subclass_data != nullptr);

  *subclass_data = data->subclass_data;
}

static void
gst_d3d11_decoder_class_data_free (GstD3D11DecoderClassData * data)
{
  if (!data)
    return;

  gst_clear_caps (&data->sink_caps);
  gst_clear_caps (&data->src_caps);
  g_free (data->description);
  g_free (data);
}

void
gst_d3d11_decoder_proxy_class_init (GstElementClass * klass,
    GstD3D11DecoderClassData * data, const gchar * author)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstD3D11DecoderSubClassData *cdata = &data->subclass_data;
  std::string long_name;
  std::string description;
  const gchar *codec_name;

  g_object_class_install_property (gobject_class, PROP_DECODER_ADAPTER_LUID,
      g_param_spec_int64 ("adapter-luid", "Adapter LUID",
          "DXGI Adapter LUID (Locally Unique Identifier) of created device",
          G_MININT64, G_MAXINT64, cdata->adapter_luid,
          (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_DECODER_DEVICE_ID,
      g_param_spec_uint ("device-id", "Device Id",
          "DXGI Device ID", 0, G_MAXUINT32, 0,
          (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_DECODER_VENDOR_ID,
      g_param_spec_uint ("vendor-id", "Vendor Id",
          "DXGI Vendor ID", 0, G_MAXUINT32, 0,
          (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

  codec_name = gst_dxva_codec_to_string (cdata->codec);
  long_name = "Direct3D11/DXVA " + std::string (codec_name) + " " +
      std::string (data->description) + " Decoder";
  description = "Direct3D11/DXVA based " + std::string (codec_name) +
      " video decoder";

  gst_element_class_set_metadata (klass, long_name.c_str (),
      "Codec/Decoder/Video/Hardware", description.c_str (), author);

  gst_element_class_add_pad_template (klass,
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          data->sink_caps));
  gst_element_class_add_pad_template (klass,
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          data->src_caps));

  gst_d3d11_decoder_class_data_free (data);
}

void
gst_d3d11_decoder_proxy_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec,
    GstD3D11DecoderSubClassData * subclass_data)
{
  switch (prop_id) {
    case PROP_DECODER_ADAPTER_LUID:
      g_value_set_int64 (value, subclass_data->adapter_luid);
      break;
    case PROP_DECODER_DEVICE_ID:
      g_value_set_uint (value, subclass_data->device_id);
      break;
    case PROP_DECODER_VENDOR_ID:
      g_value_set_uint (value, subclass_data->vendor_id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

gboolean
gst_d3d11_decoder_proxy_open (GstVideoDecoder * videodec,
    GstD3D11DecoderSubClassData * subclass_data, GstD3D11Device ** device,
    GstD3D11Decoder ** decoder)
{
  GstElement *elem = GST_ELEMENT (videodec);

  if (!gst_d3d11_ensure_element_data_for_adapter_luid (elem,
          subclass_data->adapter_luid, device)) {
    GST_ERROR_OBJECT (elem, "Cannot create d3d11device");
    return FALSE;
  }

  *decoder = gst_d3d11_decoder_new (*device, subclass_data->codec);

  if (*decoder == nullptr) {
    GST_ERROR_OBJECT (elem, "Cannot create d3d11 decoder");
    gst_clear_object (device);
    return FALSE;
  }

  return TRUE;
}
