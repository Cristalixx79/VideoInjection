#include "usb_cameras.h"

std::mutex mtx;

std::string CreateJSONPackage(gchar *camera_id, guint frame_id, const gchar *timestamp)
{
    JsonBuilder *builder = json_builder_new();

    json_builder_begin_object(builder);

    json_builder_set_member_name(builder, "camera_id");
    json_builder_add_string_value(builder, camera_id);

    json_builder_set_member_name(builder, "frame_id");
    json_builder_add_int_value(builder, frame_id);

    json_builder_set_member_name(builder, "timestamp");
    json_builder_add_string_value(builder, timestamp);

    json_builder_end_object(builder);

    JsonGenerator *generator = json_generator_new();
    JsonNode *root = json_builder_get_root(builder);
    json_generator_set_root(generator, root);

    gchar *json_str = json_generator_to_data(generator, NULL);

    g_object_unref(generator);
    json_node_free(root);
    g_object_unref(builder);

    std::string json_packet = std::string(json_str);

    return json_packet;
}

static GstFlowReturn NewSampleHandler(GstElement *sink, Usb::CameraData *data)
{
    GstSample *sample = NULL;
    GstBuffer *buffer = NULL;
    g_signal_emit_by_name(sink, "pull-sample", &sample);

    if (sample)
    {
        buffer = gst_sample_get_buffer(sample);

        if (buffer)
        {
            data->frameCount++;

            gchar *timestamp = Utils::GetCurrentTimestamp();
            std::string json_packet = CreateJSONPackage(data->cameraID, data->frameCount, timestamp);

            std::cout << json_packet << '\n';
            g_free(timestamp);
        }
        gst_sample_unref(sample);
    }
    return GST_FLOW_OK;
}

int Usb::HandleUsbCamera(const std::string &usbCamera, const Usb::FormatInfo &config)
{
    {
        std::lock_guard<std::mutex> lockG(mtx);
        std::cout << "\nThread ID: " << std::this_thread::get_id() << '\t' << "Device: " << usbCamera << '\t'
                  << "Info: " << config.width << ' ' << config.height << ' ' << config.fps << '\n';
    }

    Usb::CameraData data = {};
    data.config = config;
    data.cameraID = const_cast<char *>(usbCamera.c_str());
    data.frameCount = 0;

    data.pipeline = gst_pipeline_new("v4l2-pipeline");
    data.source = gst_element_factory_make("v4l2src", "source");
    data.tee = gst_element_factory_make("tee", "tee");
    data.videoqueue = gst_element_factory_make("queue", "videoqueue");
    data.capsfilter = gst_element_factory_make("capsfilter", "caps");
    data.videoconvert = gst_element_factory_make("videoconvert", "convert");
    data.videosink = gst_element_factory_make("autovideosink", "videosink");
    data.appqueue = gst_element_factory_make("queue", "appqueue");
    data.appsink = gst_element_factory_make("appsink", "sink");

    if (!data.pipeline || !data.source || !data.tee || !data.videoqueue || !data.capsfilter ||
        !data.videoconvert || !data.videosink || !data.appqueue || !data.appsink)
    {
        std::cerr << "Cannot create all elements\n";
        return -1;
    }

    gst_bin_add_many(GST_BIN(data.pipeline),
                     data.source, data.tee, data.videoqueue, data.capsfilter,
                     data.videoconvert, data.videosink, data.appqueue, data.appsink, NULL);

    if (!gst_element_link_many(data.source, data.capsfilter, data.tee, NULL) ||
        !gst_element_link_many(data.videoqueue, data.videoconvert, data.videosink, NULL) ||
        !gst_element_link_many(data.appqueue, data.appsink, NULL))
    {
        std::cerr << "Cannot link elements\n";
        gst_object_unref(data.pipeline);
        gst_caps_unref(data.caps);
        return -2;
    }
    g_object_set(data.source, "device", usbCamera.c_str(), NULL);
    g_object_set(G_OBJECT(data.appsink), "emit-signals", TRUE, "sync", FALSE, "max-buffers", 1, "drop", TRUE, NULL);
    g_signal_connect(data.appsink, "new-sample", G_CALLBACK(NewSampleHandler), &data);

    GstPad *teeVideoPad = gst_element_request_pad_simple(data.tee, "src_%u");
    GstPad *teeAppPad = gst_element_request_pad_simple(data.tee, "src_%u");
    GstPad *videoPad = gst_element_get_static_pad(data.videoqueue, "sink");
    GstPad *appPad = gst_element_get_static_pad(data.appqueue, "sink");

    if (!teeVideoPad || !teeAppPad || !videoPad || !appPad)
    {
        std::cerr << "Cannot get pads\n";
        gst_object_unref(data.pipeline);
        gst_caps_unref(data.caps);
        return -2;
    }

    if (gst_pad_link(teeVideoPad, videoPad) != GST_PAD_LINK_OK ||
        gst_pad_link(teeAppPad, appPad) != GST_PAD_LINK_OK)
    {
        std::cerr << "Cannot link tee elements\n";
        gst_object_unref(data.pipeline);
        gst_caps_unref(data.caps);
        return -2;
    }

    gst_object_unref(videoPad);
    gst_object_unref(appPad);

    data.caps = gst_caps_new_simple("video/x-raw",
                                        "width", G_TYPE_INT, data.config.width,
                                        "height", G_TYPE_INT, data.config.height,
                                        "framerate", GST_TYPE_FRACTION, data.config.fps, 1, NULL);
    
    g_object_set(data.capsfilter, "caps", data.caps, NULL);
    GstStructure *structure = gst_caps_get_structure(data.caps, 0);
    gchar *caps_str = gst_structure_to_string(structure);
    std::cout << " -- Using format for " << usbCamera << ": " << caps_str << "\n";
    g_free(caps_str);

    GstStateChangeReturn ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        std::cerr << "Failed to start pipeline\n";
        gst_object_unref(data.pipeline);
        gst_caps_unref(data.caps);
        return -6;
    }

    data.bus = gst_element_get_bus(data.pipeline);
    int result = ParseMessage(data);

    gst_element_set_state(data.pipeline, GST_STATE_NULL);

    if (data.msg)
        gst_message_unref(data.msg);
    if (data.bus)
        gst_object_unref(data.bus);
    if (data.caps)
        gst_caps_unref(data.caps);
    gst_object_unref(data.pipeline);

    return result;
}

int Usb::ParseMessage(Usb::CameraData &data)
{
    data.bus = gst_element_get_bus(data.pipeline);
    while (true)
    {
        data.msg = gst_bus_timed_pop_filtered(data.bus, GST_CLOCK_TIME_NONE,
                                              GstMessageType(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

        if (data.msg != NULL)
        {
            GError *err;
            gchar *debug_info;
            std::string errstr;

            switch (GST_MESSAGE_TYPE(data.msg))
            {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(data.msg, &err, &debug_info);
                std::cout << " -- Error from " << GST_OBJECT_NAME(data.msg->src) << ": " << err->message << '\n';
                std::cout << " -- Debugging info: " << debug_info << '\n';

                errstr = std::string(debug_info);
                if (errstr.find("not-negotiated") != std::string::npos) {
                    goto exit_not_negotiated;
                }

                g_clear_error(&err);
                g_free(debug_info);
                goto exit_error;

            case GST_MESSAGE_EOS:
                std::cout << " -- End-Of-Stream reached\n";
                goto exit_ok;

            case GST_MESSAGE_STATE_CHANGED:
                if (GST_MESSAGE_SRC(data.msg) == GST_OBJECT(data.pipeline))
                {
                    GstState old_state, new_state, pending_state;
                    gst_message_parse_state_changed(data.msg, &old_state, &new_state, &pending_state);

                    std::cout << "Pipeline state changed from "
                              << gst_element_state_get_name(old_state) << " to "
                              << gst_element_state_get_name(new_state) << std::endl;
                }
                break;
            default:
                std::cout << " -- Unknown message\n";
                goto exit_unknown;
            }
        }
    }
exit_ok:
    return 0;
exit_error:
    return -3;
exit_not_negotiated:
    return -4;
exit_unknown:
    return -5;
}