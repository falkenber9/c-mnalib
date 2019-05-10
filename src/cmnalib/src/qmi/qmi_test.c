/*
 *
 *
 *
 *
 *   Copyright (C) 2017 Robert Falkenberg <robert.falkenberg@tu-dortmund.de>
 */

#include <stdio.h>
#include <stdlib.h>

#include <glib.h>
#include <gio/gio.h>

#include <libqmi-glib/libqmi-glib.h>

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

#define DEVICEPATH "/dev/cdc-wdm1"

/* Globals */
static GMainLoop *loop;
static GCancellable *cancellable;
static QmiDevice *device;               // abstraction of cdc-wdmX device
static QmiClient *client;               // client representation accessing a specific service
static QmiService service;
static gboolean operation_status;

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientDms *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;


static void close_ready (QmiDevice *dev, GAsyncResult *res) {
    GError *error = NULL;

    if (!qmi_device_close_finish (dev, res, &error)) {
        printf ("error: couldn't close: %s\n", error->message);
        g_error_free (error);
    } else
        printf ("Closed\n");

    g_main_loop_quit (loop);
}

static void release_client_ready (QmiDevice *dev, GAsyncResult *res) {
    GError *error = NULL;

    if (!qmi_device_release_client_finish (dev, res, &error)) {
        printf ("error: couldn't release client: %s\n", error->message);
        g_error_free (error);
    } else
        printf ("Client released\n");

    qmi_device_close_async (dev, 10, NULL, (GAsyncReadyCallback)close_ready, NULL);
}


void qmicli_async_operation_done (gboolean reported_operation_status, gboolean skip_cid_release) {
    QmiDeviceReleaseClientFlags flags = QMI_DEVICE_RELEASE_CLIENT_FLAGS_NONE;

    /* Keep the result of the operation */
    operation_status = reported_operation_status;

    /* Cleanup cancellation */
    g_clear_object (&cancellable);

    /* If no client was allocated (e.g. generic action), just quit */
    if (!client) {
        g_main_loop_quit (loop);
        return;
    }

    if (skip_cid_release)
        printf("Skipped CID release");
    else //if (!client_no_release_cid_flag)
        flags |= QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID;
/*    else
        g_print ("[%s] Client ID not released:\n"
                 "\tService: '%s'\n"
                 "\t    CID: '%u'\n",
                 qmi_device_get_path_display (device),
                 qmi_service_get_string (service),
                 qmi_client_get_cid (client));
*/

    flags |= QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID;

    qmi_device_release_client (device,
                               client,
                               flags,
                               10,
                               NULL,
                               (GAsyncReadyCallback)release_client_ready,
                               NULL);
}



static void context_free (Context *context) {
    if (!context)
        return;

    if (context->cancellable)
        g_object_unref (context->cancellable);
    if (context->device)
        g_object_unref (context->device);
    if (context->client)
        g_object_unref (context->client);
    g_slice_free (Context, context);
}



static void operation_shutdown (gboolean operation_status) {
    /* Cleanup context and finish async operation */
    context_free (ctx);
    qmicli_async_operation_done (operation_status, FALSE);
}



static void get_model_ready (QmiClientDms *client, GAsyncResult *res) {
    const gchar *str = NULL;
    QmiMessageDmsGetModelOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_get_model_finish (client, res, &error);
    if (!output) {
        printf("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_model_output_get_result (output, &error)) {
        printf("error: couldn't get model: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_model_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_model_output_get_model (output, &str, NULL);

    printf("[%s] Device model retrieved:\n"
             "\tModel: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_get_model_output_unref (output);
    operation_shutdown (TRUE);
}


void qmicli_dms_run (QmiDevice *device, QmiClientDms *client, GCancellable *cancellable) {
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);

    /* Request to get manufacturer? */
    /*
    if (get_manufacturer_flag) {
        g_debug ("Asynchronously getting manufacturer...");
        qmi_client_dms_get_manufacturer (ctx->client,
                                         NULL,
                                         10,
                                         ctx->cancellable,
                                         (GAsyncReadyCallback)get_manufacturer_ready,
                                         NULL);
        return;
    }
    */

    /* Request to get model? */
    /*if (get_model_flag) { */
        g_debug ("Asynchronously getting model...");
        qmi_client_dms_get_model (ctx->client,
                                  NULL,
                                  10,
                                  ctx->cancellable,
                                  (GAsyncReadyCallback)get_model_ready,
                                  NULL);
        return;
    /* } */
}


static void allocate_client_ready (QmiDevice *dev, GAsyncResult *res) {
    GError *error = NULL;

    client = qmi_device_allocate_client_finish (dev, res, &error);
    if (!client) {
        g_printerr ("error: couldn't create client for the '%s' service: %s\n",
                    qmi_service_get_string (service),
                    error->message);
        exit (EXIT_FAILURE);
    }

    /* Run the service-specific action */
    switch (service) {
    case QMI_SERVICE_DMS:
        qmicli_dms_run (dev, QMI_CLIENT_DMS (client), cancellable);
        return;
/*
    case QMI_SERVICE_NAS:
        qmicli_nas_run (dev, QMI_CLIENT_NAS (client), cancellable);
        return;
    case QMI_SERVICE_WDS:
        qmicli_wds_run (dev, QMI_CLIENT_WDS (client), cancellable);
        return;
    case QMI_SERVICE_PBM:
        qmicli_pbm_run (dev, QMI_CLIENT_PBM (client), cancellable);
        return;
    case QMI_SERVICE_PDC:
        qmicli_pdc_run (dev, QMI_CLIENT_PDC (client), cancellable);
        return;
    case QMI_SERVICE_UIM:
        qmicli_uim_run (dev, QMI_CLIENT_UIM (client), cancellable);
        return;
    case QMI_SERVICE_WMS:
        qmicli_wms_run (dev, QMI_CLIENT_WMS (client), cancellable);
        return;
    case QMI_SERVICE_WDA:
        qmicli_wda_run (dev, QMI_CLIENT_WDA (client), cancellable);
        return;
    case QMI_SERVICE_VOICE:
        qmicli_voice_run (dev, QMI_CLIENT_VOICE (client), cancellable);
        return;
    default:
        g_assert_not_reached ();
*/
    }

}



static void device_allocate_client (QmiDevice *dev) {
    guint8 cid = QMI_CID_NONE;

    /*
    if (client_cid_str) {
        guint32 cid32;

        cid32 = atoi (client_cid_str);
        if (!cid32 || cid32 > G_MAXUINT8) {
            g_printerr ("error: invalid CID given '%s'\n",
                        client_cid_str);
            exit (EXIT_FAILURE);
        }

        cid = (guint8)cid32;
        g_debug ("Reusing CID '%u'", cid);
    }
    */

    /* As soon as we get the QmiDevice, create a client for the requested
     * service */
    qmi_device_allocate_client (dev,
                                service,
                                cid,
                                10,
                                cancellable,
                                (GAsyncReadyCallback)allocate_client_ready,
                                NULL);
}



static void device_open_ready (QmiDevice *dev, GAsyncResult *res) {
    GError *error = NULL;

    if (!qmi_device_open_finish (dev, res, &error)) {
        printf("error: couldn't open the QmiDevice: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

    printf("QMI Device at '%s' ready",
             qmi_device_get_path_display (dev));
/*
    if (device_set_instance_id_str)
        device_set_instance_id (dev);
    else if (get_service_version_info_flag)
        device_get_service_version_info (dev);
    else if (get_wwan_iface_flag)
        device_get_wwan_iface (dev);
    else if (get_expected_data_format_flag)
        device_get_expected_data_format (dev);
    else if (set_expected_data_format_str)
        device_set_expected_data_format (dev);
    else
*/
        device_allocate_client (dev);
}



static void device_new_ready (GObject *unused, GAsyncResult *res) {
    QmiDeviceOpenFlags open_flags = QMI_DEVICE_OPEN_FLAGS_NONE;
    GError *error = NULL;

    device = qmi_device_new_finish (res, &error);
    if (!device) {
        printf("error: couldn't create QmiDevice: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }
    
    /* Open the device */
    qmi_device_open (device,
                     open_flags,
                     15,
                     cancellable,
                     (GAsyncReadyCallback)device_open_ready,
                     NULL);
}



int main(int argc, char** argv) {
    GError *error = NULL;
    GFile *file;
    //GOptionContext *context;

    service = QMI_SERVICE_DMS;

    printf("Starting qmi Test\n");

    /* Build new GFile from the commandline arg */
    file = g_file_new_for_commandline_arg (DEVICEPATH);

    /* Create requirements for async options */
    cancellable = g_cancellable_new ();
    loop = g_main_loop_new (NULL, FALSE);

    qmi_device_new(file, cancellable, (GAsyncReadyCallback)device_new_ready, NULL);

    g_main_loop_run (loop);

    printf("Cleaning up\n");

    if (cancellable)
        g_object_unref (cancellable);
    if (client)
        g_object_unref (client);
    if (device)
        g_object_unref (device);
    g_main_loop_unref (loop);
    g_object_unref (file);

}
