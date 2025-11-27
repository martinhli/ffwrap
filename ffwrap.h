#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to a demuxer context
typedef struct ffw_ctx ffw_ctx;

// Version (for quick sanity checks)
const char* ffw_version(void);

// One-time init if you’ll use network streams
int ffw_init_network(void);   // returns 0 on success
void ffw_deinit_network(void);

// Open a media file/URL for demuxing
int ffw_open(const char* url, ffw_ctx** out_ctx); // 0 on success

// Read next packet; caller provides buffers.
// Returns: 0=ok, >0=EOF, <0=error
int ffw_read_packet(ffw_ctx* ctx, int* out_stream_index,
                    uint8_t* out_buf, int out_buf_size, int* out_data_len,
                    int64_t* out_pts, int64_t* out_dts);

// Seek (optional)
int ffw_seek(ffw_ctx* ctx, int64_t timestamp_us);

// Close and free
void ffw_close(ffw_ctx** ctx);

#ifdef __cplusplus
}
#endif
