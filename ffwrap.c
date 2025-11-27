#include "ffwrap.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

struct ffw_ctx {
    AVFormatContext* fmt;
    AVPacket* pkt;
};

const char* ffw_version(void) {
    return av_version_info(); // e.g. "6.1.1"
}

int ffw_init_network(void) {
    return avformat_network_init();
}
void ffw_deinit_network(void) {
    avformat_network_deinit();
}

int ffw_open(const char* url, ffw_ctx** out_ctx) {
    if (!out_ctx) return AVERROR(EINVAL);
    *out_ctx = NULL;

    ffw_ctx* c = av_mallocz(sizeof(ffw_ctx));
    if (!c) return AVERROR(ENOMEM);

    int ret = avformat_open_input(&c->fmt, url, NULL, NULL);
    if (ret < 0) { av_free(c); return ret; }

    ret = avformat_find_stream_info(c->fmt, NULL);
    if (ret < 0) { avformat_close_input(&c->fmt); av_free(c); return ret; }

    c->pkt = av_packet_alloc();
    if (!c->pkt) { avformat_close_input(&c->fmt); av_free(c); return AVERROR(ENOMEM); }

    *out_ctx = c;
    return 0;
}

int ffw_read_packet(ffw_ctx* ctx, int* out_stream_index,
                    uint8_t* out_buf, int out_buf_size, int* out_data_len,
                    int64_t* out_pts, int64_t* out_dts) {
    if (!ctx || !ctx->pkt) return AVERROR(EINVAL);

    int ret = av_read_frame(ctx->fmt, ctx->pkt);
    if (ret == AVERROR_EOF) return 1; // signal EOF
    if (ret < 0) return ret;

    if (out_stream_index) *out_stream_index = ctx->pkt->stream_index;
    if (out_pts) *out_pts = ctx->pkt->pts;
    if (out_dts) *out_dts = ctx->pkt->dts;

    int copy = ctx->pkt->size;
    if (out_buf && out_buf_size > 0) {
        if (copy > out_buf_size) copy = out_buf_size;
        memcpy(out_buf, ctx->pkt->data, copy);
    }
    if (out_data_len) *out_data_len = copy;

    av_packet_unref(ctx->pkt);
    return 0;
}

int ffw_seek(ffw_ctx* ctx, int64_t timestamp_us) {
    if (!ctx || !ctx->fmt) return AVERROR(EINVAL);
    // convert microseconds to AV_TIME_BASE
    int64_t ts = av_rescale_q(timestamp_us, (AVRational){1,1000000}, AV_TIME_BASE_Q);
    return avformat_seek_file(ctx->fmt, -1, INT64_MIN, ts, INT64_MAX, 0);
}

void ffw_close(ffw_ctx** ctxp) {
    if (!ctxp || !*ctxp) return;
    ffw_ctx* ctx = *ctxp;
    if (ctx->pkt) av_packet_free(&ctx->pkt);
    if (ctx->fmt) avformat_close_input(&ctx->fmt);
    av_free(ctx);
    *ctxp = NULL;
}
