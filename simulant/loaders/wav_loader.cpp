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

static bool read_fmt(const std::shared_ptr<std::istream>& stream,
                     Sound* sound, std::size_t len, bool force_copy) {
    _S_UNUSED(len);
    _S_UNUSED(force_copy);

    char buffer[4];

    AudioDataFormat format;
    std::size_t freq;
    uint8_t channels;

    stream->read(buffer, 2 * sizeof(char));      //1 == pcm
    stream->read(buffer, 2 * sizeof(char));      //channels

    channels = convert_to_int(buffer, 2);
    stream->read(buffer, 4 * sizeof(char)); // freq
    freq = convert_to_int(buffer, 4);
    stream->read(buffer, 4 * sizeof(char)); // byte rate
    stream->read(buffer, 2 * sizeof(char)); //
    stream->read(buffer, 2 * sizeof(char));
    int bps = convert_to_int(buffer, 2);

    if(channels == 1) {
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
                      std::size_t, bool) {
    return true;
}

static bool read_data(const std::shared_ptr<std::istream>& stream,
                      Sound* sound, std::size_t len, bool force_copy) {
    auto format = sound->format();

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
                            Sound* sound, std::size_t len, bool force_copy)>
    ChunkFunc;

static ChunkFunc get_chunk_func(const std::string& name) {
    if(name == std::string("data")) return read_data;
    else if(name == std::string("fmt ")) return read_fmt;
    return read_junk;
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

    while(!data_->eof()) {
        char buffer[4];

        data_->read(buffer, 4 * sizeof(char));
        std::string chunk_id(buffer, buffer + 4);

        data_->read(buffer, 4 * sizeof(char));
        uint32_t size = convert_to_int(buffer, 4);
        uint32_t offset = data_->tellg();

        auto func = get_chunk_func(chunk_id);
        if(!func(data_, sound, size - 8, !stream_audio)) {
            S_ERROR("Unsupported .wav format");
            return false;
        }

        seen_chunks.insert(chunk_id);

        if(seen_chunks.count("data") && seen_chunks.count("fmt ")) {
            break;
        }

        data_->seekg(offset + size);
    }

    std::weak_ptr<Sound> wptr = sound->shared_from_this();

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
