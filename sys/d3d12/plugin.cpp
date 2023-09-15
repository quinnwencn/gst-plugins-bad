/* GStreamer
 * Copyright (C) 2023 Seungha Yang <seungha@centricular.com>
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>
#include "gstd3d12device.h"
#include "gstd3d12h264dec.h"
#include "gstd3d12h265dec.h"

#include <wrl.h>

/* *INDENT-OFF* */
using namespace Microsoft::WRL;
/* *INDENT-ON* */

GST_DEBUG_CATEGORY (gst_d3d12_debug);
GST_DEBUG_CATEGORY (gst_d3d12_allocator_debug);
GST_DEBUG_CATEGORY (gst_d3d12_command_list_debug);
GST_DEBUG_CATEGORY (gst_d3d12_decoder_debug);
GST_DEBUG_CATEGORY (gst_d3d12_fence_debug);
GST_DEBUG_CATEGORY (gst_d3d12_format_debug);
GST_DEBUG_CATEGORY (gst_d3d12_utils_debug);

#define GST_CAT_DEFAULT gst_d3d12_debug

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_d3d12_debug, "d3d12", 0, "d3d12");
  GST_DEBUG_CATEGORY_INIT (gst_d3d12_allocator_debug, "d3d12allocator", 0,
      "d3d12allocator");
  GST_DEBUG_CATEGORY_INIT (gst_d3d12_command_list_debug, "d3d12commandlist", 0,
      "d3d12commandlist");
  GST_DEBUG_CATEGORY_INIT (gst_d3d12_decoder_debug, "d3d12decoder", 0,
      "d3d12decoder");
  GST_DEBUG_CATEGORY_INIT (gst_d3d12_fence_debug, "d3d12fence", 0,
      "d3d12fence");
  GST_DEBUG_CATEGORY_INIT (gst_d3d12_format_debug, "d3d12format", 0,
      "d3d12format");
  GST_DEBUG_CATEGORY_INIT (gst_d3d12_utils_debug,
      "d3d12utils", 0, "d3d12utils");

  /* Enumerate devices to register decoders per device and to get the highest
   * feature level */
  /* AMD seems to be supporting up to 12 cards, and 8 for NVIDIA */
  for (guint i = 0; i < 12; i++) {
    GstD3D12Device *device = nullptr;
    ID3D12Device *device_handle;
    ComPtr < ID3D12VideoDevice > video_device;
    HRESULT hr;

    device = gst_d3d12_device_new (i);
    if (!device)
      break;

    device_handle = gst_d3d12_device_get_device_handle (device);
    hr = device_handle->QueryInterface (IID_PPV_ARGS (&video_device));
    if (FAILED (hr)) {
      gst_object_unref (device);
      continue;
    }

    gst_d3d12_h264_dec_register (plugin, device, video_device.Get (),
        GST_RANK_NONE);
    gst_d3d12_h265_dec_register (plugin, device, video_device.Get (),
        GST_RANK_NONE);

    gst_object_unref (device);
  }

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    d3d12,
    "Direct3D12 plugin",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
