#include <algorithm>
#include <map>
#include "wav_loader.h"
#include "../sound.h"
#include "../streams/stream_view.h"

namespace smlt {
namespace loaders {

static bool is_big_endian() {
    int a = 1;
    return !((char*)&a)[0];
}

static int convert_to_int(char* buffer, int len) {
    int i = 0;
    int a = 0;
    if (!is_big_endian())
        for (; i<len; i++)
            ((char*)&a)[i] = buffer[i];
    else
        for (; i<len; i++)
            ((char*)&a)[3 - i] = buffer[i];
    return a;
}

/* The format tags we understand. Yamaha ADPCM is the 4-bit ADPCM that the
 * Dreamcast's AICA plays, and there are two tags in use for it:
 *
 *  - 0x14 is what KallistiOS' wav2adpcm writes by default. Stereo data is
 *    not interleaved: all of the left channel is followed by all of the right.
 *  - 0x20 is written by `wav2adpcm -i` and by ffmpeg's adpcm_yamaha encoder.
 *    Stereo data has a frame per byte, but the two tools disagree about which
 *    half of the byte is which channel (see normalize_adpcm_stereo).
 *
 * Mono data is the same in all of them: two samples per byte, the first in
 * the low 4 bits. */
enum WavFormatTag {
    WAV_FORMAT_PCM = 0x01,
    WAV_FORMAT_YAMAHA_ADPCM_SPLIT = 0x14,
    WAV_FORMAT_YAMAHA_ADPCM = 0x20
};

struct WavInfo {
    int format_tag = WAV_FORMAT_PCM;
    std::size_t fmt_size = 0;
    bool adpcm = false;
};

/* The most frames a driver that plays ADPCM itself can take in one buffer
 * (the AICA's limit on the length of a sample) */
static const std::size_t ADPCM_MAX_NATIVE_FRAMES = 65534;

/* Decodes Yamaha ADPCM a code at a time. This matches what ALdc does on
 * the platforms where it has to decode, so a sound is the same whichever
 * of us does it. */
struct YamahaDecoder {
    int signal = 0;
    int step = 0x7F;

    int16_t decode(int code) {
        static const int DIFF[16] = {
            1, 3, 5, 7, 9, 11, 13, 15, -1, -3, -5, -7, -9, -11, -13, -15
        };

        static const int STEP_SCALE[8] = {
            0x0E6, 0x0E6, 0x0E6, 0x0E6, 0x133, 0x199, 0x200, 0x266
        };

        signal += (step * DIFF[code & 0xF]) / 8;
        signal = std::max(-32768, std::min(32767, signal));

        step = (step * STEP_SCALE[code & 0x7]) >> 8;
        step = std::max(0x7F, std::min(0x6000, step));

        return (int16_t) signal;
    }
};

/* Rearranges stereo ADPCM into the one layout we use from here on, which is
 * the one sound drivers take: a frame per byte, with the left channel in the
 * low 4 bits and the right channel in the high 4 bits. */
static void normalize_adpcm_stereo(std::vector<uint8_t>& data, const WavInfo& info) {
    if(info.format_tag == WAV_FORMAT_YAMAHA_ADPCM_SPLIT) {
        /* The first half is the left channel, the second half is the right,
         * and each is packed like mono data */
        const std::size_t half = data.size() / 2;
        std::vector<uint8_t> out(half * 2);

        for(std::size_t i = 0; i < out.size(); ++i) {
            const int shift = (i & 1) ? 4 : 0;
            const uint8_t left = (data[i / 2] >> shift) & 0xF;
            const uint8_t right = (data[half + (i / 2)] >> shift) & 0xF;
            out[i] = left | (right << 4);
        }

        std::swap(data, out);
    } else if(info.fmt_size == 16) {
        /* `wav2adpcm -i` puts the left channel in the high 4 bits, which is
         * the other way around to ffmpeg. We can tell them apart because
         * wav2adpcm writes a bare 16 byte fmt chunk, while ffmpeg (correctly,
         * for anything that isn't PCM) writes the extended one. */
        for(auto& byte: data) {
            byte = (uint8_t) ((byte >> 4) | (byte << 4));
        }
    }
}

static bool read_fmt(const std::shared_ptr<std::istream>& stream,
                     Sound* sound, std::size_t len, bool force_copy,
                     WavInfo& info) {
    _S_UNUSED(force_copy);

    char buffer[4];

    AudioDataFormat format;
    std::size_t freq;
    uint8_t channels;

    stream->read(buffer, 2 * sizeof(char));      //1 == pcm
    info.format_tag = convert_to_int(buffer, 2);
    info.fmt_size = len;

    stream->read(buffer, 2 * sizeof(char));      //channels

    channels = convert_to_int(buffer, 2);
    stream->read(buffer, 4 * sizeof(char)); // freq
    freq = convert_to_int(buffer, 4);
    stream->read(buffer, 4 * sizeof(char)); // byte rate
    stream->read(buffer, 2 * sizeof(char)); //
    stream->read(buffer, 2 * sizeof(char));
    int bps = convert_to_int(buffer, 2);

    if(info.format_tag == WAV_FORMAT_YAMAHA_ADPCM ||
       info.format_tag == WAV_FORMAT_YAMAHA_ADPCM_SPLIT) {

        if(bps != 4 || (channels != 1 && channels != 2)) {
            S_ERROR("Unsupported Yamaha ADPCM .wav ({0} bits, {1} channels)", bps, (int) channels);
            return false;
        }

        /* Report what it decodes to. Whether it's uploaded compressed is
         * decided when it's played, as it depends on the driver. */
        info.adpcm = true;
        format = (channels == 1) ? AUDIO_DATA_FORMAT_MONO16 : AUDIO_DATA_FORMAT_STEREO16;
    } else if(channels == 1) {
        format = (bps == 8) ? AUDIO_DATA_FORMAT_MONO8 : (bps == 16) ? AUDIO_DATA_FORMAT_MONO16 : AUDIO_DATA_FORMAT_MONO24;
    } else {
        format = (bps == 8) ? AUDIO_DATA_FORMAT_STEREO8 : (bps == 16) ? AUDIO_DATA_FORMAT_STEREO16 : AUDIO_DATA_FORMAT_STEREO24;
    }

    sound->set_sample_rate(freq);
    sound->set_channels(channels);
    sound->set_format(format);
    return true;
}

static bool read_junk(const std::shared_ptr<std::istream>&, Sound*,
                      std::size_t, bool, WavInfo&) {
    return true;
}

static bool read_data(const std::shared_ptr<std::istream>& stream,
                      Sound* sound, std::size_t len, bool force_copy,
                      WavInfo& info) {
    auto format = sound->format();

    /* Stereo ADPCM that isn't laid out the way we play it is rearranged
     * here, once, so it has to be read into memory - but it's still
     * compressed, so that's a quarter of the size of the audio */
    const bool rearrange = info.adpcm && sound->channels() == 2 && (
        info.format_tag == WAV_FORMAT_YAMAHA_ADPCM_SPLIT || info.fmt_size == 16
    );

    if(rearrange) {
        std::vector<uint8_t> data(len);
        stream->read((char*) &data[0], len);
        data.resize(stream->gcount());

        normalize_adpcm_stereo(data, info);

        auto ss = std::make_shared<std::stringstream>();
        ss->write((char*) &data[0], data.size());
        ss->seekg(0);

        sound->set_input_stream(ss);
        return true;
    }

    /* Streaming path: don't copy the (potentially multi-megabyte) audio data
     * into RAM. Keep the source stream alive in the Sound and remember the
     * chunk's offset/length so playback can read directly from it. This
     * matters a lot on memory constrained platforms (e.g. Dreamcast) where a
     * single large std::vector + stringstream copy can exhaust the heap. */
    if(!force_copy && format != AUDIO_DATA_FORMAT_MONO24 &&
       format != AUDIO_DATA_FORMAT_STEREO24) {
        const std::streamoff offset = stream->tellg();
        sound->set_input_stream(stream,
                                (offset < 0) ? 0 : (std::size_t) offset, len);
        return true;
    }

    std::vector<uint8_t> data;

    data.resize(len);
    stream->read((char*) &data[0], len);

    if(format == AUDIO_DATA_FORMAT_MONO24 || format == AUDIO_DATA_FORMAT_STEREO24) {
        // Downsample to 16 bit
        std::vector<uint8_t> new_data;

        uint8_t* sample = &data[0];

        for(auto i = 0u; i < data.size(); i += 3) {
            ++sample; // Skip least significant
            new_data.push_back(*(sample++));
            new_data.push_back(*(sample++));
        }

        std::swap(data, new_data);
        sound->set_format((format == AUDIO_DATA_FORMAT_MONO24) ? AUDIO_DATA_FORMAT_MONO16 : AUDIO_DATA_FORMAT_STEREO16);
    }

    auto ss = std::make_shared<std::stringstream>();
    ss->write((char*) &data[0], data.size());
    ss->seekg(0);

    sound->set_input_stream(ss);
    return true;
}

typedef std::function<bool (const std::shared_ptr<std::istream>& stream,
                            Sound* sound, std::size_t len, bool force_copy,
                            WavInfo& info)>
    ChunkFunc;

static ChunkFunc get_chunk_func(const std::string& name) {
    if(name == std::string("data")) return read_data;
    else if(name == std::string("fmt ")) return read_fmt;
    return read_junk;
}

/* Sets up playback of Yamaha ADPCM, which sound->input_stream() holds as
 * mono data or as stereo data in the layout normalize_adpcm_stereo produces.
 *
 * A driver that plays ADPCM itself is given the whole sound in one buffer,
 * compressed. It has to be one buffer because each one starts decoding from
 * scratch, so there'd be a click wherever one ended and the next began. If
 * the sound is too long for that, or the driver only takes PCM, we decode
 * it ourselves as it plays and stream 16-bit audio like any other .wav. */
static void init_adpcm_source(std::weak_ptr<Sound> wptr, PlayingSound& source) {
    source.set_stream_func(StreamFunc());

    struct ADPCMPlayState {
        std::size_t offset = 0;
        YamahaDecoder decoders[2];
        std::vector<uint8_t> adpcm;
        std::vector<int16_t> pcm;
        bool native = false;

        // Weak, so that a playing sound doesn't keep the sound alive
        std::weak_ptr<Sound> sound_ptr;
    };

    auto sound_ptr = wptr.lock();
    if(!sound_ptr) {
        S_WARN("Sound was destroyed before playing");
        return;
    }

    const std::size_t channels = sound_ptr->channels();
    const AudioDataFormat adpcm_format = (channels == 2) ?
        AUDIO_DATA_FORMAT_STEREO_ADPCM : AUDIO_DATA_FORMAT_MONO_ADPCM;

    /* A byte is two mono frames, or one stereo frame */
    const std::size_t total_frames = (channels == 2) ?
        sound_ptr->stream_length() : sound_ptr->stream_length() * 2;

    auto state = std::make_shared<ADPCMPlayState>();
    state->sound_ptr = sound_ptr;
    state->native = sound_ptr->_driver()->supports_format(adpcm_format);

    if(state->native && total_frames > ADPCM_MAX_NATIVE_FRAMES) {
        S_INFO(
            "ADPCM sound has {0} frames, which is more than the {1} that can be "
            "played compressed. Decoding it to 16-bit instead.",
            total_frames, ADPCM_MAX_NATIVE_FRAMES
        );
        state->native = false;
    }

    auto stream = std::make_shared<StreamView>(
        sound_ptr->input_stream(), sound_ptr->stream_offset(),
        sound_ptr->stream_length());

    source.set_stream_func([state, stream, channels, adpcm_format](AudioBufferID id) -> int32_t {
        auto sound = state->sound_ptr.lock();
        if(!sound) {
            S_WARN("Sound was destroyed while playing, stopping playback");
            return -1;
        }

        const std::size_t remaining = stream->length() - state->offset;
        if(!remaining) {
            return 0;
        }

        std::size_t bytes = remaining;
        if(!state->native) {
            /* As much as decodes to a buffer's worth of 16-bit audio */
            const std::size_t max_frames = sound->buffer_size() / (channels * sizeof(int16_t));
            const std::size_t max_bytes = (channels == 2) ? max_frames : max_frames / 2;
            bytes = std::min(remaining, std::max(max_bytes, (std::size_t) 1));
        }

        state->adpcm.resize(bytes);
        stream->seekg(state->offset, std::ios_base::beg);
        stream->read((char*) &state->adpcm[0], bytes);
        state->offset += bytes;

        if(state->native) {
            sound->_driver()->upload_buffer_data(
                id, adpcm_format, &state->adpcm[0], bytes, sound->sample_rate()
            );

            return bytes;
        }

        /* The decoders carry on from wherever the last buffer left them */
        state->pcm.resize(bytes * 2);
        if(channels == 2) {
            for(std::size_t i = 0; i < bytes; ++i) {
                state->pcm[i * 2] = state->decoders[0].decode(state->adpcm[i] & 0xF);
                state->pcm[(i * 2) + 1] = state->decoders[1].decode(state->adpcm[i] >> 4);
            }
        } else {
            for(std::size_t i = 0; i < bytes; ++i) {
                state->pcm[i * 2] = state->decoders[0].decode(state->adpcm[i] & 0xF);
                state->pcm[(i * 2) + 1] = state->decoders[0].decode(state->adpcm[i] >> 4);
            }
        }

        const std::size_t pcm_bytes = state->pcm.size() * sizeof(int16_t);
        sound->_driver()->upload_buffer_data(
            id, sound->format(), (const uint8_t*) &state->pcm[0], pcm_bytes, sound->sample_rate()
        );

        return pcm_bytes;
    });
}

bool WAVLoader::into(Loadable& resource, const LoaderOptions& options) {
    /* Default to streaming: the caller (AssetManager::load_sound) passes
     * "stream" = flags.stream_audio, which defaults to true. */
    bool stream_audio = true;
    if(options.count("stream")) {
        stream_audio = any_cast<bool>(options.at("stream"));
    }

    Loadable* res_ptr = &resource;
    Sound* sound = dynamic_cast<Sound*>(res_ptr);
    assert(sound && "You passed a Resource that is not a Sound to the OGG loader");

    char buffer[4];
    data_->read(buffer, 4 * sizeof(char));
    std::string id(buffer, buffer + 4);
    if(id != "RIFF") {
        S_ERROR("Unexpected start to WAV file");
        return false;
    }

    data_->read(buffer, 4 * sizeof(char));
    uint32_t file_length = convert_to_int(buffer, 4);
    _S_UNUSED(file_length);

    data_->read(buffer, 4 * sizeof(char));
    id = std::string(buffer, buffer + 4);

    if(id != "WAVE") {
        S_ERROR("Unsupported WAV file.");
        return false;
    }

    std::set<std::string> seen_chunks;
    WavInfo info;

    while(!data_->eof()) {
        char buffer[4];

        data_->read(buffer, 4 * sizeof(char));
        std::string chunk_id(buffer, buffer + 4);

        data_->read(buffer, 4 * sizeof(char));
        uint32_t size = convert_to_int(buffer, 4);
        uint32_t offset = data_->tellg();

        if(!data_->good()) {
            break;
        }

        /* The size in a chunk's header is that of its contents alone */
        auto func = get_chunk_func(chunk_id);
        if(!func(data_, sound, size, !stream_audio, info)) {
            S_ERROR("Unsupported .wav format");
            return false;
        }

        seen_chunks.insert(chunk_id);

        if(seen_chunks.count("data") && seen_chunks.count("fmt ")) {
            break;
        }

        /* Chunks are padded to an even length */
        data_->seekg(offset + size + (size & 1));
    }

    if(!seen_chunks.count("data") || !seen_chunks.count("fmt ")) {
        S_ERROR("Incomplete .wav file");
        return false;
    }

    std::weak_ptr<Sound> wptr = sound->shared_from_this();

    if(info.adpcm) {
        sound->set_playing_sound_init_function(
            std::bind(&init_adpcm_source, wptr, std::placeholders::_1)
        );

        return true;
    }

    sound->set_playing_sound_init_function([wptr](PlayingSound& source) {
        source.set_stream_func(StreamFunc());

        struct SourcePlayState {
            int offset = 0;

            // This is set in init_source, and released
            // when the sound stops playing
            std::weak_ptr<Sound> sound_ptr;
        };

        auto state = std::make_shared<SourcePlayState>();

        // This is important, we increase the ref-count of the sound
        // and store the smart pointer in the callback (by passing by value
        // to the lambda). This prevents the sound from being destroyed while
        // the data is being streamed

        std::shared_ptr<Sound> sound_ptr;
        state->sound_ptr = sound_ptr = wptr.lock();
        if(!sound_ptr) {
            S_WARN("Sound was destroyed before playing");
            return;
        }

        auto stream = std::make_shared<StreamView>(
            sound_ptr->input_stream(), sound_ptr->stream_offset(),
            sound_ptr->stream_length());

        S_DEBUG("Initialized stream_func for source instance {0}", &source);

        source.set_stream_func([state, stream](AudioBufferID id) -> int32_t {
            auto sound = state->sound_ptr.lock();

            if(sound) {
                const uint32_t buffer_size = sound->buffer_size();
                const uint32_t remaining_in_bytes = stream->length() - state->offset;

                //assert((buffer_size % audio_data_format_byte_size(sound->format())) == 0);

                std::vector<uint8_t> buffer;
                stream->seekg(state->offset, std::ios_base::beg);

                if(remaining_in_bytes == 0) {
                    return 0;
                } else if(remaining_in_bytes < buffer_size) {
                    buffer.resize(remaining_in_bytes);
                    stream->read((char*) &buffer[0], remaining_in_bytes);
                } else {
                    buffer.resize(buffer_size);
                    stream->read((char*) &buffer[0], buffer_size);
                }

                state->offset += buffer.size();

                sound->_driver()->upload_buffer_data(
                    id, sound->format(), &buffer[0], buffer.size(), sound->sample_rate()
                );

                return buffer.size();
            } else {
                S_WARN("Sound was destroyed while playing, stopping playback");
                return -1;
            }
        });
    });

    return true;
}
}
}
