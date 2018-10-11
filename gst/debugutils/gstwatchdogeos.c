/* GStreamer
 * Copyright (C) 2013 Rdio <ingestions@rdio.com>
 * Copyright (C) 2013 David Schleef <ds@schleef.org>
 * Copyright (C) 2018 Atsushi Watanabe <atsushi.w@ieee.org>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstwatchdogeos
 * @title: watchdogeos
 *
 * The watchdogeos element watches buffers and events flowing through
 * a pipeline.  If no buffers are seen for a configurable amount of
 * time, a error message is sent to the bus.
 *
 * To use this element, insert it into a pipeline as you would an
 * identity element.  Once activated, any pause in the flow of
 * buffers through the element will cause an element error.  The
 * maximum allowed pause is determined by the timeout property.
 *
 * This element is currently intended for transcoding pipelines,
 * although may be useful in other contexts.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v fakesrc ! watchdogeos ! fakesink
 * ]|
 *
 */

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include "gstwatchdogeos.h"

GST_DEBUG_CATEGORY_STATIC(gst_watchdogeos_debug_category);
#define GST_CAT_DEFAULT gst_watchdogeos_debug_category

/* prototypes */

static void gst_watchdogeos_set_property(
    GObject *object,
    guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_watchdogeos_get_property(
    GObject *object,
    guint property_id, GValue *value, GParamSpec *pspec);

static gboolean gst_watchdogeos_start(
    GstBaseTransform *trans);
static gboolean gst_watchdogeos_stop(
    GstBaseTransform *trans);
static gboolean gst_watchdogeos_sink_event(
    GstBaseTransform *trans,
    GstEvent *event);
static gboolean gst_watchdogeos_src_event(
    GstBaseTransform *trans,
    GstEvent *event);
static GstFlowReturn gst_watchdogeos_transform_ip(
    GstBaseTransform *trans,
    GstBuffer *buf);
static void gst_watchdogeos_feed(
    GstWatchdogeos *watchdogeos, gpointer mini_object,
    gboolean force);

static GstStateChangeReturn
gst_watchdogeos_change_state(GstElement *element, GstStateChange transition);

enum
{
  PROP_0,
  PROP_TIMEOUT
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(
    GstWatchdogeos, gst_watchdogeos, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_watchdogeos_debug_category, "watchdogeos", 0,
                            "debug category for watchdogeos element"));

static void
gst_watchdogeos_class_init(GstWatchdogeosClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS(klass);

  GstElementClass *gstelement_klass = (GstElementClass *)klass;

  gst_element_class_add_pad_template(
      GST_ELEMENT_CLASS(klass),
      gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                           gst_caps_new_any()));
  gst_element_class_add_pad_template(
      GST_ELEMENT_CLASS(klass),
      gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                           gst_caps_new_any()));

  gst_element_class_set_static_metadata(
      GST_ELEMENT_CLASS(klass),
      "Watchdogeos", "Generic", "Watches for pauses in stream buffers",
      "David Schleef <ds@schleef.org>");

  gstelement_klass->change_state =
      GST_DEBUG_FUNCPTR(gst_watchdogeos_change_state);
  gobject_class->set_property = gst_watchdogeos_set_property;
  gobject_class->get_property = gst_watchdogeos_get_property;
  base_transform_class->start = GST_DEBUG_FUNCPTR(gst_watchdogeos_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_watchdogeos_stop);
  base_transform_class->sink_event =
      GST_DEBUG_FUNCPTR(gst_watchdogeos_sink_event);
  base_transform_class->src_event = GST_DEBUG_FUNCPTR(gst_watchdogeos_src_event);
  base_transform_class->transform_ip =
      GST_DEBUG_FUNCPTR(gst_watchdogeos_transform_ip);

  g_object_class_install_property(
      gobject_class, PROP_TIMEOUT,
      g_param_spec_int(
          "timeout", "Timeout", "Timeout (in ms) after "
                                "which an element error is sent to the bus if no buffers are "
                                "received. 0 means disabled.",
          0, G_MAXINT, 1000,
          G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_watchdogeos_init(GstWatchdogeos *watchdogeos)
{
}

static void
gst_watchdogeos_set_property(
    GObject *object, guint property_id,
    const GValue *value, GParamSpec *pspec)
{
  GstWatchdogeos *watchdogeos = GST_WATCHDOGEOS(object);

  GST_DEBUG_OBJECT(watchdogeos, "set_property");

  switch (property_id)
  {
    case PROP_TIMEOUT:
      GST_OBJECT_LOCK(watchdogeos);
      watchdogeos->timeout = g_value_get_int(value);
      gst_watchdogeos_feed(watchdogeos, NULL, FALSE);
      GST_OBJECT_UNLOCK(watchdogeos);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static void
gst_watchdogeos_get_property(
    GObject *object, guint property_id,
    GValue *value, GParamSpec *pspec)
{
  GstWatchdogeos *watchdogeos = GST_WATCHDOGEOS(object);

  GST_DEBUG_OBJECT(watchdogeos, "get_property");

  switch (property_id)
  {
    case PROP_TIMEOUT:
      g_value_set_int(value, watchdogeos->timeout);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static gpointer
gst_watchdogeos_thread(gpointer user_data)
{
  GstWatchdogeos *watchdogeos = GST_WATCHDOGEOS(user_data);

  GST_DEBUG_OBJECT(watchdogeos, "thread starting");

  g_main_loop_run(watchdogeos->main_loop);

  GST_DEBUG_OBJECT(watchdogeos, "thread exiting");

  return NULL;
}

static gboolean
gst_watchdogeos_trigger(gpointer ptr)
{
  GstWatchdogeos *watchdogeos = GST_WATCHDOGEOS(ptr);

  GST_DEBUG_OBJECT(watchdogeos, "watchdogeos triggered");

  gst_element_send_event(GST_ELEMENT(watchdogeos), gst_event_new_eos());

  return FALSE;
}

static gboolean
gst_watchdogeos_quit_mainloop(gpointer ptr)
{
  GstWatchdogeos *watchdogeos = GST_WATCHDOGEOS(ptr);

  GST_DEBUG_OBJECT(watchdogeos, "watchdogeos quit");

  g_main_loop_quit(watchdogeos->main_loop);

  return FALSE;
}

/*  Call with OBJECT_LOCK taken */
static void
gst_watchdogeos_feed(GstWatchdogeos *watchdogeos, gpointer mini_object, gboolean force)
{
  if (watchdogeos->source)
  {
    if (watchdogeos->waiting_for_flush_start)
    {
      if (mini_object && GST_IS_EVENT(mini_object) &&
          GST_EVENT_TYPE(mini_object) == GST_EVENT_FLUSH_START)
      {
        watchdogeos->waiting_for_flush_start = FALSE;
        watchdogeos->waiting_for_flush_stop = TRUE;
      }

      force = TRUE;
    }
    else if (watchdogeos->waiting_for_flush_stop)
    {
      if (mini_object && GST_IS_EVENT(mini_object) &&
          GST_EVENT_TYPE(mini_object) == GST_EVENT_FLUSH_STOP)
      {
        watchdogeos->waiting_for_flush_stop = FALSE;
        watchdogeos->waiting_for_a_buffer = TRUE;
      }

      force = TRUE;
    }
    else if (watchdogeos->waiting_for_a_buffer)
    {
      if (mini_object && GST_IS_BUFFER(mini_object))
      {
        watchdogeos->waiting_for_a_buffer = FALSE;
        GST_DEBUG_OBJECT(watchdogeos, "Got a buffer \\o/");
      }
      else
      {
        GST_DEBUG_OBJECT(watchdogeos, "Waiting for a buffer and did not get it,"
                                      " keep trying even in PAUSED state");
        force = TRUE;
      }
    }
    g_source_destroy(watchdogeos->source);
    g_source_unref(watchdogeos->source);
    watchdogeos->source = NULL;
  }

  if (watchdogeos->timeout == 0)
  {
    GST_LOG_OBJECT(watchdogeos, "Timeout is 0 => nothing to do");
  }
  else if (watchdogeos->main_context == NULL)
  {
    GST_LOG_OBJECT(watchdogeos, "No maincontext => nothing to do");
  }
  else if ((GST_STATE(watchdogeos) != GST_STATE_PLAYING) && force == FALSE)
  {
    GST_LOG_OBJECT(watchdogeos,
                   "Not in playing and force is FALSE => Nothing to do");
  }
  else
  {
    watchdogeos->source = g_timeout_source_new(watchdogeos->timeout);
    g_source_set_callback(
        watchdogeos->source, gst_watchdogeos_trigger,
        gst_object_ref(watchdogeos), gst_object_unref);
    g_source_attach(watchdogeos->source, watchdogeos->main_context);
  }
}

static gboolean
gst_watchdogeos_start(GstBaseTransform *trans)
{
  GstWatchdogeos *watchdogeos = GST_WATCHDOGEOS(trans);

  GST_DEBUG_OBJECT(watchdogeos, "start");
  GST_OBJECT_LOCK(watchdogeos);

  watchdogeos->main_context = g_main_context_new();
  watchdogeos->main_loop = g_main_loop_new(watchdogeos->main_context, TRUE);
  watchdogeos->thread = g_thread_new("watchdogeos", gst_watchdogeos_thread, watchdogeos);

  GST_OBJECT_UNLOCK(watchdogeos);
  return TRUE;
}

static gboolean
gst_watchdogeos_stop(GstBaseTransform *trans)
{
  GstWatchdogeos *watchdogeos = GST_WATCHDOGEOS(trans);
  GSource *quit_source;

  GST_DEBUG_OBJECT(watchdogeos, "stop");
  GST_OBJECT_LOCK(watchdogeos);

  if (watchdogeos->source)
  {
    g_source_destroy(watchdogeos->source);
    g_source_unref(watchdogeos->source);
    watchdogeos->source = NULL;
  }

  /* dispatch an idle event that trigger g_main_loop_quit to avoid race
   * between g_main_loop_run and g_main_loop_quit */
  quit_source = g_idle_source_new();
  g_source_set_callback(
      quit_source, gst_watchdogeos_quit_mainloop, watchdogeos,
      NULL);
  g_source_attach(quit_source, watchdogeos->main_context);
  g_source_unref(quit_source);

  g_thread_join(watchdogeos->thread);
  watchdogeos->thread = NULL;

  g_main_loop_unref(watchdogeos->main_loop);
  watchdogeos->main_loop = NULL;

  g_main_context_unref(watchdogeos->main_context);
  watchdogeos->main_context = NULL;

  GST_OBJECT_UNLOCK(watchdogeos);
  return TRUE;
}

static gboolean
gst_watchdogeos_sink_event(GstBaseTransform *trans, GstEvent *event)
{
  GstWatchdogeos *watchdogeos = GST_WATCHDOGEOS(trans);

  GST_DEBUG_OBJECT(watchdogeos, "sink_event");

  GST_OBJECT_LOCK(watchdogeos);
  gst_watchdogeos_feed(watchdogeos, event, FALSE);
  GST_OBJECT_UNLOCK(watchdogeos);

  return GST_BASE_TRANSFORM_CLASS(gst_watchdogeos_parent_class)->sink_event(trans, event);
}

static gboolean
gst_watchdogeos_src_event(GstBaseTransform *trans, GstEvent *event)
{
  gboolean force = FALSE;
  GstWatchdogeos *watchdogeos = GST_WATCHDOGEOS(trans);

  GST_DEBUG_OBJECT(watchdogeos, "src_event");

  GST_OBJECT_LOCK(watchdogeos);
  if (GST_EVENT_TYPE(event) == GST_EVENT_SEEK)
  {
    GstSeekFlags flags;

    gst_event_parse_seek(event, NULL, NULL, &flags, NULL, NULL, NULL, NULL);

    if (flags & GST_SEEK_FLAG_FLUSH)
    {
      force = TRUE;
      GST_DEBUG_OBJECT(watchdogeos, "Got a FLUSHING seek, we need a buffer now!");
      watchdogeos->waiting_for_flush_start = TRUE;
    }
  }

  gst_watchdogeos_feed(watchdogeos, event, force);
  GST_OBJECT_UNLOCK(watchdogeos);

  return GST_BASE_TRANSFORM_CLASS(gst_watchdogeos_parent_class)->src_event(trans, event);
}

static GstFlowReturn
gst_watchdogeos_transform_ip(GstBaseTransform *trans, GstBuffer *buf)
{
  GstWatchdogeos *watchdogeos = GST_WATCHDOGEOS(trans);

  GST_DEBUG_OBJECT(watchdogeos, "transform_ip");

  GST_OBJECT_LOCK(watchdogeos);
  gst_watchdogeos_feed(watchdogeos, buf, FALSE);
  GST_OBJECT_UNLOCK(watchdogeos);

  return GST_FLOW_OK;
}

/*
 * Change state handler for the element.
 */
static GstStateChangeReturn
gst_watchdogeos_change_state(GstElement *element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstWatchdogeos *watchdogeos = GST_WATCHDOGEOS(element);

  GST_DEBUG_OBJECT(watchdogeos, "gst_watchdogeos_change_state");

  switch (transition)
  {
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      /* Activate timer */
      GST_OBJECT_LOCK(watchdogeos);
      gst_watchdogeos_feed(watchdogeos, NULL, FALSE);
      GST_OBJECT_UNLOCK(watchdogeos);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS(gst_watchdogeos_parent_class)->change_state(element, transition);

  switch (transition)
  {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      GST_OBJECT_LOCK(watchdogeos);
      watchdogeos->waiting_for_a_buffer = TRUE;
      gst_watchdogeos_feed(watchdogeos, NULL, TRUE);
      GST_OBJECT_UNLOCK(watchdogeos);
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      /* Disable the timer */
      GST_OBJECT_LOCK(watchdogeos);
      if (watchdogeos->source)
      {
        g_source_destroy(watchdogeos->source);
        g_source_unref(watchdogeos->source);
        watchdogeos->source = NULL;
      }
      GST_OBJECT_UNLOCK(watchdogeos);
      break;
    default:
      break;
  }

  return ret;
}
