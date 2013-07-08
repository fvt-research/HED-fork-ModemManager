/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details:
 *
 * Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
 */

#include <ModemManager.h>
#define _LIBMM_INSIDE_MM
#include <libmm-glib.h>

#include "mm-iface-modem.h"
#include "mm-iface-modem-signal.h"
#include "mm-log.h"

#define REFRESH_CONTEXT_TAG "signal-refresh-context-tag"

static GQuark refresh_context_quark;

/*****************************************************************************/

void
mm_iface_modem_signal_bind_simple_status (MMIfaceModemSignal *self,
                                          MMSimpleStatus *status)
{
}

/*****************************************************************************/

typedef struct {
    guint rate;
    guint timeout_source;
} RefreshContext;

static void
refresh_context_free (RefreshContext *ctx)
{
    if (ctx->timeout_source)
        g_source_remove (ctx->timeout_source);
    g_slice_free (RefreshContext, ctx);
}

static void
clear_values (MMIfaceModemSignal *self)
{
    MmGdbusModemSignal *skeleton;

    g_object_get (self,
                  MM_IFACE_MODEM_SIGNAL_DBUS_SKELETON, &skeleton,
                  NULL);
    if (!skeleton)
        return;

    mm_gdbus_modem_signal_set_cdma_rssi (skeleton, g_variant_new ("(bd)", FALSE, 0.0));
    mm_gdbus_modem_signal_set_cdma_ecio (skeleton, g_variant_new ("(bd)", FALSE, 0.0));
    mm_gdbus_modem_signal_set_evdo_rssi (skeleton, g_variant_new ("(bd)", FALSE, 0.0));
    mm_gdbus_modem_signal_set_evdo_ecio (skeleton, g_variant_new ("(bd)", FALSE, 0.0));
    mm_gdbus_modem_signal_set_evdo_sinr (skeleton, g_variant_new ("(bd)", FALSE, 0.0));
    mm_gdbus_modem_signal_set_evdo_io   (skeleton, g_variant_new ("(bd)", FALSE, 0.0));
    mm_gdbus_modem_signal_set_gsm_rssi  (skeleton, g_variant_new ("(bd)", FALSE, 0.0));
    mm_gdbus_modem_signal_set_umts_rssi (skeleton, g_variant_new ("(bd)", FALSE, 0.0));
    mm_gdbus_modem_signal_set_umts_ecio (skeleton, g_variant_new ("(bd)", FALSE, 0.0));
    mm_gdbus_modem_signal_set_lte_rssi  (skeleton, g_variant_new ("(bd)", FALSE, 0.0));
    mm_gdbus_modem_signal_set_lte_rsrq  (skeleton, g_variant_new ("(bd)", FALSE, 0.0));
    mm_gdbus_modem_signal_set_lte_rsrp  (skeleton, g_variant_new ("(bd)", FALSE, 0.0));
    mm_gdbus_modem_signal_set_lte_snr   (skeleton, g_variant_new ("(bd)", FALSE, 0.0));
    g_object_unref (skeleton);
}

static void
load_values_ready (MMIfaceModemSignal *self,
                   GAsyncResult *res)
{
    GError *error = NULL;
    gboolean cdma_available;
    gdouble cdma_rssi;
    gdouble cdma_ecio;
    gboolean evdo_available;
    gdouble evdo_rssi;
    gdouble evdo_ecio;
    gdouble evdo_sinr;
    gdouble evdo_io;
    gboolean gsm_available;
    gdouble gsm_rssi;
    gboolean umts_available;
    gdouble umts_rssi;
    gdouble umts_ecio;
    gboolean lte_available;
    gdouble lte_rssi;
    gdouble lte_rsrq;
    gdouble lte_rsrp;
    gdouble lte_snr;
    MmGdbusModemSignal *skeleton;

    if (!MM_IFACE_MODEM_SIGNAL_GET_INTERFACE (self)->load_values_finish (
            self,
            res,
            &cdma_available,
            &cdma_rssi,
            &cdma_ecio,
            &evdo_available,
            &evdo_rssi,
            &evdo_ecio,
            &evdo_sinr,
            &evdo_io,
            &gsm_available,
            &gsm_rssi,
            &umts_available,
            &umts_rssi,
            &umts_ecio,
            &lte_available,
            &lte_rssi,
            &lte_rsrq,
            &lte_rsrp,
            &lte_snr,
            &error)) {
        mm_warn ("Couldn't load extended signal information: %s", error->message);
        g_error_free (error);
        clear_values (self);
        return;
    }

    g_object_get (self,
                  MM_IFACE_MODEM_SIGNAL_DBUS_SKELETON, &skeleton,
                  NULL);
    if (!skeleton) {
        mm_warn ("Cannot update extended signal information: "
                 "Couldn't get interface skeleton");
        return;
    }

    mm_gdbus_modem_signal_set_cdma_rssi (skeleton, g_variant_new ("(bd)", cdma_available, cdma_rssi));
    mm_gdbus_modem_signal_set_cdma_ecio (skeleton, g_variant_new ("(bd)", cdma_available, cdma_ecio));
    mm_gdbus_modem_signal_set_evdo_rssi (skeleton, g_variant_new ("(bd)", evdo_available, evdo_rssi));
    mm_gdbus_modem_signal_set_evdo_ecio (skeleton, g_variant_new ("(bd)", evdo_available, evdo_ecio));
    mm_gdbus_modem_signal_set_evdo_sinr (skeleton, g_variant_new ("(bd)", evdo_available, evdo_sinr));
    mm_gdbus_modem_signal_set_evdo_io   (skeleton, g_variant_new ("(bd)", evdo_available, evdo_io));
    mm_gdbus_modem_signal_set_gsm_rssi  (skeleton, g_variant_new ("(bd)", gsm_available, gsm_rssi));
    mm_gdbus_modem_signal_set_umts_rssi (skeleton, g_variant_new ("(bd)", umts_available, umts_rssi));
    mm_gdbus_modem_signal_set_umts_ecio (skeleton, g_variant_new ("(bd)", umts_available, umts_ecio));
    mm_gdbus_modem_signal_set_lte_rssi  (skeleton, g_variant_new ("(bd)", lte_available, lte_rssi));
    mm_gdbus_modem_signal_set_lte_rsrq  (skeleton, g_variant_new ("(bd)", lte_available, lte_rsrq));
    mm_gdbus_modem_signal_set_lte_rsrp  (skeleton, g_variant_new ("(bd)", lte_available,lte_rsrp));
    mm_gdbus_modem_signal_set_lte_snr   (skeleton, g_variant_new ("(bd)", lte_available, lte_snr));
    /* Flush right away */
    g_dbus_interface_skeleton_flush (G_DBUS_INTERFACE_SKELETON (skeleton));

    g_object_unref (skeleton);
}

static gboolean
refresh_context_cb (MMIfaceModemSignal *self)
{
    MM_IFACE_MODEM_SIGNAL_GET_INTERFACE (self)->load_values (
        self,
        NULL,
        (GAsyncReadyCallback)load_values_ready,
        NULL);
    return TRUE;
}

static void
teardown_refresh_context (MMIfaceModemSignal *self)
{
    mm_dbg ("Extended signal information reporting disabled");
    clear_values (self);
    if (G_UNLIKELY (!refresh_context_quark))
        refresh_context_quark  = g_quark_from_static_string (REFRESH_CONTEXT_TAG);
    g_object_set_qdata (G_OBJECT (self), refresh_context_quark, NULL);
}

static gboolean
setup_refresh_context (MMIfaceModemSignal *self,
                       gboolean update_rate,
                       guint new_rate,
                       GError **error)
{
    MmGdbusModemSignal *skeleton;
    RefreshContext *ctx;
    MMModemState modem_state;

    if (G_UNLIKELY (!refresh_context_quark))
        refresh_context_quark  = g_quark_from_static_string (REFRESH_CONTEXT_TAG);

    g_object_get (self,
                  MM_IFACE_MODEM_SIGNAL_DBUS_SKELETON, &skeleton,
                  MM_IFACE_MODEM_STATE, &modem_state,
                  NULL);
    if (!skeleton) {
        g_set_error (error,
                     MM_CORE_ERROR,
                     MM_CORE_ERROR_FAILED,
                     "Couldn't get interface skeleton");
        return FALSE;
    }

    if (update_rate)
        mm_gdbus_modem_signal_set_rate (skeleton, new_rate);
    else
        new_rate = mm_gdbus_modem_signal_get_rate (skeleton);
    g_object_unref (skeleton);

    /* User disabling? */
    if (new_rate == 0) {
        mm_dbg ("Extended signal information reporting disabled (rate: 0 seconds)");
        clear_values (self);
        g_object_set_qdata (G_OBJECT (self), refresh_context_quark, NULL);
        return TRUE;
    }

    if (modem_state < MM_MODEM_STATE_ENABLING) {
        mm_dbg ("Extended signal information reporting disabled (modem not yet enabled)");
        return TRUE;
    }

    /* Setup refresh context */
    ctx = g_object_get_qdata (G_OBJECT (self), refresh_context_quark);
    if (!ctx) {
        ctx = g_slice_new0 (RefreshContext);
        g_object_set_qdata_full (G_OBJECT (self),
                                 refresh_context_quark,
                                 ctx,
                                 (GDestroyNotify)refresh_context_free);
    }

    /* We're enabling, compare to old rate */
    if (ctx->rate == new_rate) {
        /* Already there */
        return TRUE;
    }

    /* Update refresh context */
    mm_dbg ("Extended signal information reporting enabled (rate: %u seconds)", new_rate);
    ctx->rate = new_rate;
    if (ctx->timeout_source)
        g_source_remove (ctx->timeout_source);
    ctx->timeout_source = g_timeout_add_seconds (ctx->rate, (GSourceFunc) refresh_context_cb, self);

    /* Also launch right away */
    refresh_context_cb (self);

    return TRUE;
}

/*****************************************************************************/

typedef struct {
    GDBusMethodInvocation *invocation;
    MmGdbusModemSignal *skeleton;
    MMIfaceModemSignal *self;
    guint rate;
} HandleSetupContext;

static void
handle_setup_context_free (HandleSetupContext *ctx)
{
    g_object_unref (ctx->invocation);
    g_object_unref (ctx->skeleton);
    g_object_unref (ctx->self);
    g_slice_free (HandleSetupContext, ctx);
}

static void
handle_setup_auth_ready (MMBaseModem *self,
                         GAsyncResult *res,
                         HandleSetupContext *ctx)
{
    GError *error = NULL;

    if (!mm_base_modem_authorize_finish (self, res, &error))
        g_dbus_method_invocation_take_error (ctx->invocation, error);
    else if (!setup_refresh_context (ctx->self, TRUE, ctx->rate, &error))
        g_dbus_method_invocation_take_error (ctx->invocation, error);
    else
        mm_gdbus_modem_signal_complete_setup (ctx->skeleton, ctx->invocation);
    handle_setup_context_free (ctx);
}

static gboolean
handle_setup (MmGdbusModemSignal *skeleton,
              GDBusMethodInvocation *invocation,
              guint rate,
              MMIfaceModemSignal *self)
{
    HandleSetupContext *ctx;

    ctx = g_slice_new (HandleSetupContext);
    ctx->invocation = g_object_ref (invocation);
    ctx->skeleton = g_object_ref (skeleton);
    ctx->self = g_object_ref (self);
    ctx->rate = rate;

    mm_base_modem_authorize (MM_BASE_MODEM (self),
                             invocation,
                             MM_AUTHORIZATION_DEVICE_CONTROL,
                             (GAsyncReadyCallback)handle_setup_auth_ready,
                             ctx);
    return TRUE;
}

/*****************************************************************************/

gboolean
mm_iface_modem_signal_disable_finish (MMIfaceModemSignal *self,
                                      GAsyncResult *res,
                                      GError **error)
{
    return !g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error);
}

void
mm_iface_modem_signal_disable (MMIfaceModemSignal *self,
                               GAsyncReadyCallback callback,
                               gpointer user_data)
{
    GSimpleAsyncResult *result;

    result = g_simple_async_result_new (G_OBJECT (self),
                                        callback,
                                        user_data,
                                        mm_iface_modem_signal_disable);

    teardown_refresh_context (self);

    g_simple_async_result_set_op_res_gboolean (result, TRUE);
    g_simple_async_result_complete_in_idle (result);
    g_object_unref (result);
}

/*****************************************************************************/

gboolean
mm_iface_modem_signal_enable_finish (MMIfaceModemSignal *self,
                                     GAsyncResult *res,
                                     GError **error)
{
    return !g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error);
}

void
mm_iface_modem_signal_enable (MMIfaceModemSignal *self,
                              GCancellable *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer user_data)
{
    GSimpleAsyncResult *result;
    GError *error = NULL;

    result = g_simple_async_result_new (G_OBJECT (self),
                                        callback,
                                        user_data,
                                        mm_iface_modem_signal_enable);

    if (!setup_refresh_context (self, FALSE, 0, &error))
        g_simple_async_result_take_error (result, error);
    else
        g_simple_async_result_set_op_res_gboolean (result, TRUE);

    g_simple_async_result_complete_in_idle (result);
    g_object_unref (result);
}

/*****************************************************************************/

gboolean
mm_iface_modem_signal_initialize_finish (MMIfaceModemSignal *self,
                                         GAsyncResult *res,
                                         GError **error)
{
    return !g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error);
}

void
mm_iface_modem_signal_initialize (MMIfaceModemSignal *self,
                                  GCancellable *cancellable,
                                  GAsyncReadyCallback callback,
                                  gpointer user_data)
{
    GSimpleAsyncResult *result;
    MmGdbusModemSignal *skeleton = NULL;
    gboolean supported;

    result = g_simple_async_result_new (G_OBJECT (self),
                                        callback,
                                        user_data,
                                        mm_iface_modem_signal_initialize);

    supported = (MM_IFACE_MODEM_SIGNAL_GET_INTERFACE (self)->load_values &&
                 MM_IFACE_MODEM_SIGNAL_GET_INTERFACE (self)->load_values_finish);

    /* Did we already create it? */
    g_object_get (self,
                  MM_IFACE_MODEM_SIGNAL_DBUS_SKELETON, &skeleton,
                  NULL);
    if (!skeleton) {
        skeleton = mm_gdbus_modem_signal_skeleton_new ();

        g_object_set (self,
                      MM_IFACE_MODEM_SIGNAL_DBUS_SKELETON, skeleton,
                      NULL);

        if (supported) {
            /* Set initial values */
            clear_values (self);

            /* Handle method invocations */
            g_signal_connect (skeleton,
                              "handle-setup",
                              G_CALLBACK (handle_setup),
                              self);
            /* Finally, export the new interface */
            mm_gdbus_object_skeleton_set_modem_signal (MM_GDBUS_OBJECT_SKELETON (self),
                                                       MM_GDBUS_MODEM_SIGNAL (skeleton));
        }
    }

    if (supported)
        g_simple_async_result_set_op_res_gboolean (result, TRUE);
    else
        g_simple_async_result_set_error (result,
                                         MM_CORE_ERROR,
                                         MM_CORE_ERROR_UNSUPPORTED,
                                         "Extended signal information reporting not supported");
    g_simple_async_result_complete_in_idle (result);
    g_object_unref (result);
}

void
mm_iface_modem_signal_shutdown (MMIfaceModemSignal *self)
{
    /* Unexport DBus interface and remove the skeleton */
    mm_gdbus_object_skeleton_set_modem_signal (MM_GDBUS_OBJECT_SKELETON (self), NULL);
    g_object_set (self,
                  MM_IFACE_MODEM_SIGNAL_DBUS_SKELETON, NULL,
                  NULL);
}

/*****************************************************************************/

static void
iface_modem_signal_init (gpointer g_iface)
{
    static gboolean initialized = FALSE;

    if (initialized)
        return;

    /* Properties */
    g_object_interface_install_property
        (g_iface,
         g_param_spec_object (MM_IFACE_MODEM_SIGNAL_DBUS_SKELETON,
                              "Signal DBus skeleton",
                              "DBus skeleton for the Signal interface",
                              MM_GDBUS_TYPE_MODEM_SIGNAL_SKELETON,
                              G_PARAM_READWRITE));

    initialized = TRUE;
}

GType
mm_iface_modem_signal_get_type (void)
{
    static GType iface_modem_signal_type = 0;

    if (!G_UNLIKELY (iface_modem_signal_type)) {
        static const GTypeInfo info = {
            sizeof (MMIfaceModemSignal), /* class_size */
            iface_modem_signal_init,     /* base_init */
            NULL,                      /* base_finalize */
        };

        iface_modem_signal_type = g_type_register_static (G_TYPE_INTERFACE,
                                                          "MMIfaceModemSignal",
                                                          &info,
                                                          0);

        g_type_interface_add_prerequisite (iface_modem_signal_type, MM_TYPE_IFACE_MODEM);
    }

    return iface_modem_signal_type;
}
