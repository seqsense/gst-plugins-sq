/* GStreamer
 * Copyright (C) <2007> Wim Taymans <wim.taymans@gmail.com>
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

#ifndef __GST_SRTP_PT_DEMUX_H__
#define __GST_SRTP_PT_DEMUX_H__

#include <gst/gst.h>

#define GST_TYPE_SRTP_PT_DEMUX (gst_srtp_pt_demux_get_type())
#define GST_SRTP_PT_DEMUX(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SRTP_PT_DEMUX, GstSrtpPtDemux))
#define GST_SRTP_PT_DEMUX_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SRTP_PT_DEMUX, GstSrtpPtDemuxClass))
#define GST_IS_SRTP_PT_DEMUX(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SRTP_PT_DEMUX))
#define GST_IS_SRTP_PT_DEMUX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SRTP_PT_DEMUX))

typedef struct _GstSrtpPtDemux GstSrtpPtDemux;
typedef struct _GstSrtpPtDemuxClass GstSrtpPtDemuxClass;
typedef struct _GstSrtpPtDemuxPad GstSrtpPtDemuxPad;

struct _GstSrtpPtDemux
{
  GstElement parent; /**< parent class */

  GstPad* sink;       /**< the sink pad */
  guint16 last_pt;    /**< pt of the last packet 0xFFFF if none */
  GSList* srcpads;    /**< a linked list of GstSrtpPtDemuxPad objects */
  GValue ignored_pts; /**< a GstValueArray of payload types that will not have pads created for */
};

struct _GstSrtpPtDemuxClass
{
  GstElementClass parent_class;

  /* get the caps for pt */
  GstCaps* (*request_pt_map)(GstSrtpPtDemux* demux, guint pt);

  /* signal emmited when a new PT is found from the incoming stream */
  void (*new_payload_type)(GstSrtpPtDemux* demux, guint pt, GstPad* pad);

  /* signal emitted when the payload type changes */
  void (*payload_type_change)(GstSrtpPtDemux* demux, guint pt);

  void (*clear_pt_map)(GstSrtpPtDemux* demux);
};

GType gst_srtp_pt_demux_get_type(void);

#endif /* __GST_SRTP_PT_DEMUX_H__ */
