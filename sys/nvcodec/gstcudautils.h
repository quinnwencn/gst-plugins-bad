/* GStreamer
 * Copyright (C) <2018-2019> Seungha Yang <seungha.yang@navercorp.com>
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

#ifndef __GST_CUDA_UTILS_H__
#define __GST_CUDA_UTILS_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include "gstcudaloader.h"
#include "gstcudacontext.h"

G_BEGIN_DECLS

#ifndef GST_DISABLE_GST_DEBUG
static inline gboolean
_gst_cuda_debug(CUresult result, GstDebugCategory * category,
    const gchar * file, const gchar * function, gint line)
{
  const gchar *_error_name, *_error_text;
  if (result != CUDA_SUCCESS) {
    CuGetErrorName (result, &_error_name);
    CuGetErrorString (result, &_error_text);
    gst_debug_log (category, GST_LEVEL_WARNING, file, function, line,
        NULL, "CUDA call failed: %s, %s", _error_name, _error_text);

    return FALSE;
  }

  return TRUE;
}

/**
 * gst_cuda_result:
 * @result: CUDA device API return code #CUresult
 *
 * Returns: %TRUE if CUDA device API call result is CUDA_SUCCESS
 */
#define gst_cuda_result(result) \
  _gst_cuda_debug(result, GST_CAT_DEFAULT, __FILE__, GST_FUNCTION, __LINE__)
#else

static inline gboolean
_gst_cuda_debug(CUresult result, GstDebugCategory * category,
    const gchar * file, const gchar * function, gint line)
{
  return result == CUDA_SUCCESS;
}

/**
 * gst_cuda_result:
 * @result: CUDA device API return code #CUresult
 *
 * Returns: %TRUE if CUDA device API call result is CUDA_SUCCESS
 */
#define gst_cuda_result(result) \
  _gst_cuda_debug(result, NULL, __FILE__, GST_FUNCTION, __LINE__)
#endif

typedef enum
{
  GST_CUDA_QUARK_GRAPHICS_RESOURCE = 0,

  /* end of quark list */
  GST_CUDA_QUARK_MAX = 1
} GstCudaQuarkId;

typedef enum
{
  GST_CUDA_GRAPHICS_RESOURCE_NONE = 0,
  GST_CUDA_GRAPHICS_RESOURCE_GL_BUFFER = 1,
  GST_CUDA_GRAPHICS_RESOURCE_D3D11_RESOURCE = 2,
} GstCudaGraphicsResourceType;

typedef struct _GstCudaGraphicsResource
{
  GstCudaContext *cuda_context;
  /* GL context or D3D11 device */
  GstObject *graphics_context;

  GstCudaGraphicsResourceType type;
  CUgraphicsResource resource;
  CUgraphicsRegisterFlags flags;

  gboolean registered;
  gboolean mapped;
} GstCudaGraphicsResource;

gboolean        gst_cuda_ensure_element_context (GstElement * element,
                                                 gint device_id,
                                                 GstCudaContext ** cuda_ctx);

gboolean        gst_cuda_handle_set_context     (GstElement * element,
                                                 GstContext * context,
                                                 gint device_id,
                                                 GstCudaContext ** cuda_ctx);

gboolean        gst_cuda_handle_context_query   (GstElement * element,
                                                 GstQuery * query,
                                                 GstCudaContext * cuda_ctx);

GstContext *    gst_context_new_cuda_context    (GstCudaContext * context);

GQuark          gst_cuda_quark_from_id          (GstCudaQuarkId id);

GstCudaGraphicsResource * gst_cuda_graphics_resource_new  (GstCudaContext * context,
                                                           GstObject * graphics_context,
                                                           GstCudaGraphicsResourceType type);

gboolean        gst_cuda_graphics_resource_register_gl_buffer (GstCudaGraphicsResource * resource,
                                                               guint buffer,
                                                               CUgraphicsRegisterFlags flags);

gboolean        gst_cuda_graphics_resource_register_d3d11_resource (GstCudaGraphicsResource * resource,
                                                                    gpointer d3d11_resource,
                                                                    CUgraphicsRegisterFlags flags);

void            gst_cuda_graphics_resource_unregister (GstCudaGraphicsResource * resource);

CUgraphicsResource gst_cuda_graphics_resource_map (GstCudaGraphicsResource * resource,
                                                   CUstream stream,
                                                   CUgraphicsMapResourceFlags flags);

void            gst_cuda_graphics_resource_unmap (GstCudaGraphicsResource * resource,
                                                  CUstream stream);

void            gst_cuda_graphics_resource_free (GstCudaGraphicsResource * resource);

typedef enum
{
  GST_CUDA_BUFFER_COPY_SYSTEM,
  GST_CUDA_BUFFER_COPY_CUDA,
  GST_CUDA_BUFFER_COPY_GL,
  GST_CUDA_BUFFER_COPY_D3D11,
  GST_CUDA_BUFFER_COPY_NVMM,
} GstCudaBufferCopyType;

const gchar * gst_cuda_buffery_copy_type_to_string (GstCudaBufferCopyType type);

gboolean      gst_cuda_buffer_copy (GstBuffer * dst,
                                    GstCudaBufferCopyType dst_type,
                                    const GstVideoInfo * dst_info,
                                    GstBuffer * src,
                                    GstCudaBufferCopyType src_type,
                                    const GstVideoInfo * src_info,
                                    GstCudaContext * context,
                                    CUstream stream);

G_END_DECLS

#endif /* __GST_CUDA_UTILS_H__ */
