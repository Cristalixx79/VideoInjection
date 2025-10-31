#pragma once

#include "main.h"

namespace Usb
{
    struct FormatInfo
    {
        int width;
        int height;
        int fps;

        bool operator<(const FormatInfo &other) const
        {
            if (width != other.width)
                return width < other.width;
            if (height != other.height)
                return height < other.height;
            return fps < other.fps;
        }
    };

    struct CameraData
    {
        GstElement *pipeline;
        GstElement *tee;
        GstElement *source;
        GstElement *videosink, *appsink;
        GstElement *capsfilter;
        GstElement *videoconvert;

        GstElement *videoqueue, *appqueue;

        GstBus *bus;
        GstMessage *msg;

        GstCaps *caps;
        FormatInfo config;

        guint frameCount;
        gchar *cameraID;
    };

    int HandleUsbCamera(const std::string &usbCamera, const FormatInfo &config);
    int ParseMessage(CameraData &data);
}
