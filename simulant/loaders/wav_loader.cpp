#include <map>
#include "wav_loader.h"
#include "../sound.h"

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

static bool read_riff(std::istream* stream, Sound* sound, std::size_t len) {
    char buffer[4];

    AudioDataFormat format;
    std::size_t freq;
    uint8_t channels;

    stream->read(buffer, 4 * sizeof(char));      //WAVE
    stream->read(buffer, 4 * sizeof(char));      //fmt
    stream->read(buffer, 4 * sizeof(char));      //16
    stream->read(buffer, 2 * sizeof(char));      //1 == pcm
    stream->read(buffer, 2 * sizeof(char));      //channels

    channels = convert_to_int(buffer, 2);
    stream->read(buffer, 4 * sizeof(char)); // freq
    freq = convert_to_int(buffer, 4);
    stream->read(buffer, 4 * sizeof(char)); // byte rate
    stream->read(buffer, 2 * sizeof(char)); //
    stream->read(buffer, 2 * sizeof(char));
    int bps = convert_to_int(buffer, 2);
    stream->read(buffer, 4 * sizeof(char));      //data
    stream->read(buffer, 4 * sizeof(char));

    if(channels == 1) {
        format = (bps == 8) ? AUDIO_DATA_FORMAT_MONO8 : (bps == 16) ? AUDIO_DATA_FORMAT_MONO16 : AUDIO_DATA_FORMAT_MONO24;
    } else {
        format = (bps == 8) ? AUDIO_DATA_FORMAT_STEREO8 : (bps == 16) ? AUDIO_DATA_FORMAT_STEREO16 : AUDIO_DATA_FORMAT_STEREO24;
    }

    sound->set_sample_rate(freq);
    sound->set_buffer_size(4096 * 8);
    sound->set_channels(channels);
    sound->set_format(format);
    return true;
}

static bool read_junk(std::istream* stream, Sound* sound, std::size_t len) {
    return true;
}

static bool read_data(std::istream* stream, Sound* sound, std::size_t len) {
    std::vector<uint8_t> data;

    data.resize(len);
    stream->read((char*) &data[0], len);
    sound->set_data(data);
    return true;
}

const static std::map<std::string, std::function<bool (std::istream* stream, Sound* sound, std::size_t len)>> CHUNK_MAP = {
    {"RIFF", read_riff},
    {"junk", read_junk},
    {"data", read_data}
};

void WAVLoader::into(Loadable& resource, const LoaderOptions &options) {
    Loadable* res_ptr = &resource;
    Sound* sound = dynamic_cast<Sound*>(res_ptr);
    assert(sound && "You passed a Resource that is not a Sound to the OGG loader");

    while(!data_->eof()) {
        char buffer[4];

        data_->read(buffer, 4 * sizeof(char));
        std::string chunk_id(buffer, buffer + 4);

        data_->read(buffer, 4 * sizeof(char));
        uint32_t size = convert_to_int(buffer, 4);
        uint32_t offset = data_->tellg();

        if(chunk_id == "RIFF") {
            // RIFF is the top-level chunk and its size is
            // the size of the file, not the header, so we force
            // it to match here (header is 36 bytes, minus the 8 we just read)
            size = 36 - 8;
        }

        auto it = CHUNK_MAP.find(chunk_id);
        if(it != CHUNK_MAP.end()) {
            if(!it->second(data_.get(), sound, size)) {
                L_ERROR("Unsupported .wav format");
                return;
            }
        } else {
            L_ERROR(_F("Invalid chunk type {0}").format(chunk_id));
            return;
        }

        if(chunk_id == "data") {
            break;
        }
        data_->seekg(offset + size);
    }

    auto& data = sound->data();
    auto data_size = data.size();
    auto format = sound->format();

    if(format == AUDIO_DATA_FORMAT_MONO24 || format == AUDIO_DATA_FORMAT_STEREO24) {
        // Downsample to 16 bit
        std::vector<uint8_t> new_data;
        new_data.reserve((data_size / 3) * 2);

        for(uint32_t i = 0; i < data_size; i += 3) {
            uint8_t* sample = &data[i];

            sample++; // Skip least significant
            new_data.push_back(*(sample++));
            new_data.push_back(*(sample++));
        }
        sound->set_data(new_data);
        sound->set_format((format == AUDIO_DATA_FORMAT_MONO24) ? AUDIO_DATA_FORMAT_MONO16 : AUDIO_DATA_FORMAT_STEREO16);
    }

    std::weak_ptr<Sound> wptr = sound->shared_from_this();

    sound->set_source_init_function([wptr](SourceInstance& source) {
        struct SourcePlayState {
            int offset = 0;
        };

        auto state = std::make_shared<SourcePlayState>();

        // This is important, we increase the ref-count of the sound
        // and store the smart pointer in the callback (by passing by value
        // to the lambda). This prevents the sound from being destroyed while
        // the data is being streamed
        auto sound = wptr.lock();
        source.set_stream_func([state, sound](AudioBufferID id) -> int32_t {
            if(sound) {
                auto& data = sound->data();
                const uint32_t buffer_size = sound->buffer_size();
                const std::size_t sample_size = audio_data_format_byte_size(sound->format());

                const uint32_t remaining_in_bytes = data.size() - state->offset;

                assert(buffer_size % sample_size == 0);

                std::vector<uint8_t> buffer;
                if(remaining_in_bytes == 0) {
                    return 0;
                } else if(remaining_in_bytes < buffer_size) {
                    buffer.assign(data.begin() + state->offset, data.end());
                    state->offset += buffer.size();
                } else {
                    auto it = data.begin() + state->offset;
                    buffer.assign(it, it + buffer_size);
                    state->offset += buffer_size;
                }

                sound->_driver()->upload_buffer_data(
                    id, sound->format(), &buffer[0], buffer.size(), sound->sample_rate()
                );

                return buffer.size();
            }

            return 0;
        });
    });
}

}
}
