/* GStreamer
 * Copyright (C) 2013 Rdio <ingestions@rdio.com>
 * Copyright (C) 2013 David Schleef <ds@schleef.org>
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

#ifndef _GST_WATCHDOGEOS_H_
#define _GST_WATCHDOGEOS_H_

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_TYPE_WATCHDOGEOS (gst_watchdogeos_get_type())
#define GST_WATCHDOGEOS(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_WATCHDOGEOS, GstWatchdogeos))
#define GST_WATCHDOGEOS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_WATCHDOGEOS, GstWatchdogeosClass))
#define GST_IS_WATCHDOGEOS(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_WATCHDOGEOS))
#define GST_IS_WATCHDOGEOS_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_WATCHDOGEOS))

typedef struct _GstWatchdogeos GstWatchdogeos;
typedef struct _GstWatchdogeosClass GstWatchdogeosClass;

struct _GstWatchdogeos
{
  GstBaseTransform base_watchdogeos;

  /* properties */
  int timeout;

  GMainContext *main_context;
  GMainLoop *main_loop;
  GThread *thread;
  GSource *source;

  gboolean waiting_for_a_buffer;
  gboolean waiting_for_flush_start;
  gboolean waiting_for_flush_stop;
};

struct _GstWatchdogeosClass
{
  GstBaseTransformClass base_watchdogeos_class;
};

GType gst_watchdogeos_get_type(void);

G_END_DECLS

#endif
