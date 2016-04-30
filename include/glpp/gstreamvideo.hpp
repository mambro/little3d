/**
 * GStreamer Source
 * Emanuele Ruffaldi SSSA 2016 V2
 *
 * TODO:
 * verify presence of appsink
 * extract image
 * do not use polling
 * seek
 * use YUV decoding
 * 
 *
 * Reference: https://www.linuxtv.org/wiki/index.php/GStreamer#Record_raw_video_only

 * Example: basic V4L
 * v4l2src device=%s ! video/x-raw,format=RGB, width=%d,height=%d ! appsink name=sink
 *
 * Example: C920 Logitech h264
 * v4l2src name=src ! queue ! video/x-h264, width=%d, height=%d, framerate=30/1 ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=%s, width=%d,height=%d ! appsink name=sink
 *
 * Output: xvimagesink osxvideosink
 * Split: 
 */
#include "glpp/draw.hpp"
#include <gst/gst.h> 
#include <gst/app/gstappsink.h>
#include <glib.h>
#include <iostream>
#include <sstream>

class GStreamerVideo
{
public:
    ~GStreamerVideo() { close(); }
    void init();
    void onConfig();
    bool onUpdate();
    void close();

    /// playback a file automatically decoded
    void setPlayFile(const char * name);

    /// return suffix to be appended to every pipeline
    const char * pipelineSuffix() const;

    glpp::GLSize size_;
    std::string pipelinetext_;
    std::vector<uint8_t> buffer_;

private:
    bool valid_ = false;
    int camera_id_ = 0;

    //bool needs_load_ = true;
    //bool display_ = false;
    //std::unique_ptr<cv::VideoCapture> cap_;
    GstElement *pipeline_ = 0;
    GstElement * sink_ = 0;
    GstAppSink * appsink_ = 0;
    GstMessage * msg_ = 0;
    GstBus * bus_ = 0;
    GError * error_ = 0;
};

inline void GStreamerVideo::setPlayFile(const char * name)
{
    std::string x = "filesrc location=";
    x += name;
    x += " ! decodebin name=dec ! queue  ! videoconvert ! video/x-raw,format=RGB ! appsink name=sink ";
    // dec. ! queue ! audioconvert ! audioresample ! autoaudiosink";
    pipelinetext_ =x;    
}

inline void GStreamerVideo::init()
{
    // TODO mark triggered 
    // TODO mark output port as latched
}

inline const char * GStreamerVideo::pipelineSuffix() const
{
    return "! videoconvert ! video/x-raw,format=RGB ! appsink name=sink";
}


inline void GStreamerVideo::close()
{
    if(!pipeline_)
        return;
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(sink_);
    gst_object_unref(pipeline_);
    gst_object_unref(bus_);
    pipeline_ = 0;
    sink_ = 0;
    bus_ = 0;
}

inline void GStreamerVideo::onConfig()
{
    if(pipelinetext_.empty())
    {
        if(!size_.width)
        {
            size_.width = 640;
            size_.height = 480;
        }
        char buf[1024];
        sprintf(buf,"videotestsrc ! video/x-raw,format=RGB, width=%d,height=%d ! appsink name=sink", size_.width, size_.height); // or I420
        pipelinetext_ = buf;
    }
    valid_ = false;
    close();
    gst_init(NULL,NULL);
    pipeline_ = gst_parse_launch(pipelinetext_.c_str(), &error_);
    if (!pipeline_)
    {
        std::cout << "PIPELINE FAILED " << pipelinetext_ << std::endl;
        return;
    }

    sink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "sink");

    // TODO check presence of AppSink if not add ...
    appsink_ = (GstAppSink*)(sink_);
    g_signal_connect(pipeline_, "deep-notify", G_CALLBACK(gst_object_default_deep_notify ), NULL);             
    gst_app_sink_set_emit_signals(appsink_, true);
    gst_app_sink_set_drop(appsink_, true);
    gst_app_sink_set_max_buffers(appsink_, 1);    
    gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    bus_ = gst_element_get_bus(pipeline_);
    valid_ = true;   
}

inline bool GStreamerVideo::onUpdate()
{
    if(!valid_)
        return false;

    // polling
    GstSample * sample = gst_app_sink_pull_sample(appsink_);
    GstBuffer * gstImageBuffer= gst_sample_get_buffer(sample); 
    GstCaps * c = gst_sample_get_caps(sample); 
    int xw=0,yw=0;
    const GstStructure *str= gst_caps_get_structure (c, 0);
    //GST_WARNING ("caps are %" GST_PTR_FORMAT, c);
    gst_structure_get_int (str, "width", &xw); 
    gst_structure_get_int (str, "height", &yw);  
    size_.width = xw;
    size_.height=yw;
    int s = size_.width*size_.height*3;
    buffer_.resize(s);
    gst_buffer_extract(gstImageBuffer, 0, &buffer_[0], s);
    
    gst_buffer_unref(gstImageBuffer);    
    return true;
}