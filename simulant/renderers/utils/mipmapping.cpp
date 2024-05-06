#include "mipmapping.h"
#include <initializer_list>

std::size_t generate_mipmap_level_rgb565(int lw, int lh, const uint8_t* src,
                                         uint8_t* dest) {
    int w = lw >> 1;
    int h = lh >> 1;

    const int stride = 2;
    const int src_row_stride = lw * stride;
    const int dest_row_stride = w * stride;

    // S_DEBUG("[MIP] SRS: {0}, DRS: {1}", src_row_stride, dest_row_stride);

    for(int y = 0; y < lh; y += 2) {
        for(int x = 0; x < lw; x += 2) {
            int source_idx = ((y * src_row_stride) + (x * stride));
            int dest_idx = ((y / 2) * dest_row_stride) + ((x / 2) * stride);

            auto src_t = (src + source_idx);
            auto dest_t = (dest + dest_idx);

            auto t0 = src_t;
            auto t1 = src_t + stride;
            auto t2 = src_t + src_row_stride;
            auto t3 = src_t + src_row_stride + stride;

            int r = 0, g = 0, b = 0;
            for(const auto& tp: {t0, t1, t2, t3}) {
                uint16_t t = *((uint16_t*)tp);

                r += (t >> 11) & 0x1F;
                g += ((t >> 5) & 0x3F);
                b += (t & 0x1F);
            }

            r /= 4;
            g /= 4;
            b /= 4;

            *((uint16_t*)dest_t) = (r << 11) | (g << 5) | (b);
        }
    }

    return dest_row_stride * h;
}

std::size_t generate_mipmap_level_rgba4444(int lw, int lh, const uint8_t* src,
                                           uint8_t* dest) {
    int w = lw >> 1;
    int h = lh >> 1;

    const int stride = 2;
    const int src_row_stride = lw * stride;
    const int dest_row_stride = w * stride;

    for(int y = 0; y < lh; y += 2) {
        for(int x = 0; x < lw; x += 2) {
            int source_idx = ((y * src_row_stride) + (x * stride));
            int dest_idx = ((y / 2) * dest_row_stride) + ((x / 2) * stride);

            auto src_t = (src + source_idx);
            auto dest_t = (dest + dest_idx);

            auto t0 = src_t;
            auto t1 = src_t + stride;
            auto t2 = src_t + src_row_stride;
            auto t3 = src_t + src_row_stride + stride;

            int r = 0, g = 0, b = 0, a = 0;
            for(const auto& tp: {t0, t1, t2, t3}) {
                uint16_t t = *((uint16_t*)tp);

                r += (t >> 12);
                g += ((t >> 8) & 0xF);
                b += ((t >> 4) & 0xF);
                a += (t & 0xF);
            }

            r /= 4;
            g /= 4;
            b /= 4;
            a /= 4;

            *((uint16_t*)dest_t) = (r << 12) | (g << 8) | (b << 4) | (a);
        }
    }

    return dest_row_stride * h;
}

std::size_t generate_mipmap_level_rgba5551(int lw, int lh, const uint8_t* src,
                                           uint8_t* dest) {
    int w = lw >> 1;
    int h = lh >> 1;

    const int stride = 2;
    const int src_row_stride = lw * stride;
    const int dest_row_stride = w * stride;

    for(int y = 0; y < lh; y += 2) {
        for(int x = 0; x < lw; x += 2) {
            int source_idx = ((y * src_row_stride) + (x * stride));
            int dest_idx = ((y / 2) * dest_row_stride) + ((x / 2) * stride);

            auto src_t = (src + source_idx);
            auto dest_t = (dest + dest_idx);

            auto t0 = src_t;
            auto t1 = src_t + stride;
            auto t2 = src_t + src_row_stride;
            auto t3 = src_t + src_row_stride + stride;

            int r = 0, g = 0, b = 0, a = 0;
            for(const auto& tp: {t0, t1, t2, t3}) {
                uint16_t t = *((uint16_t*)tp);

                r += ((t >> 11) & 0x1F);
                g += ((t >> 6) & 0x1F);
                b += ((t >> 1) & 0x1F);
                a += (t & 0x1);
            }

            r /= 4;
            g /= 4;
            b /= 4;
            a /= 4;

            *((uint16_t*)dest_t) = (r << 11) | (g << 6) | (b << 1) | (a & 0x1);
        }
    }

    return dest_row_stride * h;
}

std::size_t generate_mipmap_level_rgba8888(int lw, int lh, const uint8_t* src,
                                           uint8_t* dest) {
    int w = lw >> 1;
    int h = lh >> 1;

    const int stride = 4;
    const int src_row_stride = lw * stride;
    const int dest_row_stride = w * stride;

    for(int y = 0; y < lh; y += 2) {
        for(int x = 0; x < lw; x += 2) {
            int source_idx = ((y * src_row_stride) + (x * stride));
            int dest_idx = ((y / 2) * dest_row_stride) + ((x / 2) * stride);

            auto src_t = (src + source_idx);
            auto dest_t = (dest + dest_idx);

            auto t0 = src_t;
            auto t1 = src_t + stride;
            auto t2 = src_t + src_row_stride;
            auto t3 = src_t + src_row_stride + stride;

            int r = 0, g = 0, b = 0, a = 0;
            for(const auto& tp: {t0, t1, t2, t3}) {
                r += tp[0];
                g += tp[1];
                b += tp[2];
                a += tp[3];
            }

            r /= 4;
            g /= 4;
            b /= 4;
            a /= 4;

            dest_t[0] = r;
            dest_t[1] = g;
            dest_t[2] = b;
            dest_t[3] = a;
        }
    }

    return dest_row_stride * h;
}
