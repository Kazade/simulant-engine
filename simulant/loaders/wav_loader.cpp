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
    _S_UNUSED(len);

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

static bool read_junk(std::istream*, Sound*, std::size_t) {
    return true;
}

static bool read_data(std::istream* stream, Sound* sound, std::size_t len) {
    std::vector<uint8_t> data;

    data.resize(len);
    stream->read((char*) &data[0], len);

    auto ss = std::make_shared<std::stringstream>();
    ss->write((char*) &data[0], len);
    ss->seekg(0);

    sound->set_input_stream(ss);
    return true;
}

typedef std::function<bool (std::istream* stream, Sound* sound, std::size_t len)> ChunkFunc;

static ChunkFunc get_chunk_func(const std::string& name) {
    if(name == std::string("RIFF")) return read_riff;
    else if(name == std::string("data")) return read_data;
    return read_junk;
}

void WAVLoader::into(Loadable& resource, const LoaderOptions &options) {
    _S_UNUSED(options);

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

        auto func = get_chunk_func(chunk_id);
        if(!func(data_.get(), sound, size)) {
            L_ERROR("Unsupported .wav format");
            return;
        }

        if(chunk_id == "data") {
            break;
        }
        data_->seekg(offset + size);
    }

    auto format = sound->format();

    if(format == AUDIO_DATA_FORMAT_MONO24 || format == AUDIO_DATA_FORMAT_STEREO24) {
        // Downsample to 16 bit. We create a new stringstream to replace the old one.
        std::vector<uint8_t> new_data;
        auto stream = sound->input_stream();
        stream->seekg(0, std::ios_base::beg);

        for(auto i = 0u; i < sound->stream_length() / 3; ++i) {
            uint8_t sample;
            stream->read((char*) &sample, 1); // Skip least significant
            stream->read((char*) &sample, 1);
            new_data.push_back(sample);

            stream->read((char*) &sample, 1);
            new_data.push_back(sample);
        }

        auto new_ss = std::make_shared<std::stringstream>();
        new_ss->write((char*) &new_data[0], new_data.size());
        new_ss->seekg(0, std::ios_base::beg);

        sound->set_input_stream(new_ss);
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

        source.set_stream_func([state, wptr](AudioBufferID id) -> int32_t {
            auto sound = wptr.lock();

            if(sound) {
                auto& stream = sound->input_stream();
                const uint32_t buffer_size = sound->buffer_size();
                const uint32_t remaining_in_bytes = sound->stream_length() - state->offset;

                assert((buffer_size % audio_data_format_byte_size(sound->format())) == 0);

                std::vector<uint8_t> buffer;
                if(remaining_in_bytes == 0) {
                    return 0;
                } else if(remaining_in_bytes < buffer_size) {
                    buffer.resize(remaining_in_bytes);
                    stream->read((char*) &buffer[0], remaining_in_bytes);
                    state->offset += buffer.size();
                } else {
                    buffer.resize(buffer_size);
                    stream->read((char*) &buffer[0], buffer_size);
                    state->offset += buffer_size;
                }

                sound->_driver()->upload_buffer_data(
                    id, sound->format(), &buffer[0], buffer.size(), sound->sample_rate()
                );

                return buffer.size();
            } else {
                return -1;
            }
        });
    });
}

}
}
