﻿#include "videostream.h"


VideoStream::VideoStream():
    videoFPS(30)
  , frameCount(0)
{
}

VideoStream::~VideoStream()
{

}

AVStream* VideoStream::addVideoStream(AVFormatContext *oc, enum AVCodecID codec_id)//init AVFormatContext struct for output
{
    AVStream *st;
    AVCodec *codec;

    const int dst_width = 1280;
    const int dst_height = 800;

    st = avformat_new_stream(oc, NULL);
    if (!st)
    {
        qDebug("Could not alloc stream\n");
        exit(1);
    }
    codec = avcodec_find_encoder(codec_id);//find mjpeg decoders
    if (!codec)
    {
        qDebug("codec not found\n");
        exit(1);
    }
    avcodec_get_context_defaults3(st->codec, codec);

    st->codec->bit_rate = 64000;//bit
    st->codec->width = dst_width;
    st->codec->height = dst_height;
    st->codec->time_base.den = videoFPS;//fps
    st->codec->time_base.num = 1;

    st->codec->pix_fmt = /*AV_PIX_FMT_YUV420P;*/codec->pix_fmts[0];//format
    st->codec->codec_tag = 0;
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        st->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    return st;
}

bool VideoStream::avWriterInit(const char* out_filename, int fps)
{

    videoFPS = fps;
    int ret;

    av_register_all();//init decode
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);//init AVFormatContext struct for output

    if (!ofmt_ctx)
    {
        qDebug("Could not create output context\n");
        return false;
    }

    AVStream *outStream = addVideoStream(ofmt_ctx, AV_CODEC_ID_MJPEG);//create output video stream
    //    av_dump_format(ofmt_ctx, 0, out_filename, 1); //display the video info

    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE))//open the output file
    {
        ret = avio_open2(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE, nullptr, nullptr);
        if (ret < 0) {
            qDebug("Could not open output file '%s'", out_filename);
            return false;
        }
    }
    AVDictionary *opt = 0;
    av_dict_set_int(&opt, "video_track_timescale", videoFPS, 0); //set the fps!!!
    if (avformat_write_header(ofmt_ctx, &opt) < 0)//Write file header
    {
        printf("Error occurred when opening output file\n");
        return false;
    }

    av_init_packet(&pkt);
    pkt.flags |= AV_PKT_FLAG_KEY;
    pkt.stream_index = outStream->index;
    return true;
}

void VideoStream::avWriter(std::vector<uchar> buffer)
{
    pkt.size = buffer.size();
    pkt.data = &*buffer.begin();
    pkt.pts = pkt.dts = frameCount * (ofmt_ctx->streams[0]->time_base.den) /ofmt_ctx->streams[0]->time_base.num / videoFPS;
    if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) //write to video file
    {
        qDebug("Error muxing packet\n");
    }
//    qDebug("Write %8d frames to output file\n", frameCount);
    frameCount++;
}

bool VideoStream::avWriterRelease()
{
    frameCount = 0;
    av_free_packet(&pkt);
    av_write_trailer(ofmt_ctx);//Write file trailer
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx->pb);//close video file
    avformat_free_context(ofmt_ctx);//release struct
    return true;
}




