/* GStreamer
 * Copyright 2010 ST-Ericsson SA
 *  @author: Benjamin Gaignard <benjamin.gaignard@stericsson.com>
 * Copyright 2023 Igalia S.L.
 *  @author: Thibault Saunier <tsaunier@igalia.com>
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

#ifndef __GST_AUTO_VIDEO_CONVERT_H__
#define __GST_AUTO_VIDEO_CONVERT_H__

#include <gst/gst.h>
#include <gst/gstbin.h>
#include "gstautoconvert.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(GstAutoVideoConvert, gst_auto_video_convert, GST, AUTO_VIDEO_CONVERT, GstBaseAutoConvert);
GST_ELEMENT_REGISTER_DECLARE (autovideoconvert);

typedef struct
{
  const gchar *first_elements[4];
  const gchar *colorspace_converters[4];
  const gchar *last_elements[4];

  const gchar *possible_filters[4];
  GstRank rank;
} GstAutoVideoConvertFilterBinsGenerator;

void gst_auto_video_register_well_known_bins (GstAutoVideoConvert *self, GstAutoVideoConvertFilterBinsGenerator *gen);

G_END_DECLS
#endif /* __GST_AUTO_VIDEO_CONVERT_H__ */
