#include <cstdio>
#include <vector>

#include "adx_loader.h"

#define ADXTRACT_IMPLEMENTATION
#include "adxtract.h"

#include "../logging.h"
#include "../sound.h"

namespace smlt {
namespace loaders {

typedef std::shared_ptr<const std::vector<uint8_t>> ADXData;

/* Owns the decoder for a single playing sound. When playing from memory this
 * also keeps the (shared) file data alive, as adxtract doesn't copy it */
class ADXStream {
public:
    typedef std::shared_ptr<ADXStream> ptr;

    ADXStream(adxtract_stream_t* stream, ADXData data = ADXData()):
        stream_(stream),
        data_(data) {}

    ~ADXStream() {
        adx_stream_close(stream_);
        stream_ = nullptr;
    }

    adxtract_stream_t* get() { return stream_; }

    /* Reused for each buffer so we're not allocating while playing */
    std::vector<int16_t> pcm;

private:
    adxtract_stream_t* stream_;
    ADXData data_;
};

static int32_t queue_buffer(std::weak_ptr<Sound> sound, ADXStream::ptr stream, AudioBufferID buffer) {
    std::shared_ptr<Sound> self = sound.lock();
    if(!self) {
        // The sound was destroyed, we can't stream without a sound so just bail
        return -1;
    }

    const std::size_t channels = self->channels();
    const std::size_t max_samples = self->buffer_size() / (channels * sizeof(int16_t));

    stream->pcm.resize(max_samples * channels);

    // 'result' is the total number of samples per channel
    std::size_t result = adx_stream_read_samples(
        stream->get(), true, max_samples, &stream->pcm[0]
    );

    if(!result) {
        return 0;
    }

    int32_t byte_size = result * channels * sizeof(int16_t);
    self->_driver()->upload_buffer_data(
        buffer, self->format(),
        (uint8_t*) &stream->pcm[0], byte_size, self->sample_rate()
    );

    return byte_size;
}

static void init_source(std::weak_ptr<Sound> sound, Path path, ADXData data, PlayingSound& source) {
    /* Set the stream func to nothing - this will give any captured smart pointers
     * the opportunity to unload before we do anything else */
    source.set_stream_func(StreamFunc());

    adxtract_stream_t* adx = nullptr;

    if(data) {
        adx = adx_read_buffer(&(*data)[0], data->size());
    } else {
        /* Each playing sound gets its own file handle, because the decoder
         * tracks the file position and closes the file when it's done */
        FILE* file = fopen(path.str().c_str(), "rb");
        if(!file) {
            S_ERROR("Unable to open ADX file for streaming: {0}", path.str());
            return;
        }

        adx = adx_load_stream(file);
        if(!adx) {
            fclose(file);
        }
    }

    if(!adx) {
        S_ERROR("Unable to start decoding ADX file: {0}", path.str());
        return;
    }

    auto stream = std::make_shared<ADXStream>(adx, data);

    /* Using a weak_ptr is important, otherwise the sound would never be
     * destroyed while it is playing */
    source.set_stream_func(std::bind(&queue_buffer, sound, stream, std::placeholders::_1));
}

bool ADXLoader::into(Loadable& resource, const LoaderOptions& options) {
    /* Stream unless someone passed stream == false */
    bool stream = !(options.count("stream") && any_cast<bool>(options.at("stream")) == false);

    Loadable* res_ptr = &resource;
    Sound* sound = dynamic_cast<Sound*>(res_ptr);
    assert(sound && "You passed a Resource that is not a Sound to the ADX loader");

    /* We can only stream from a file on disk, anything else is read into memory */
    if(stream && !std::dynamic_pointer_cast<FileIfstream>(data_)) {
        stream = false;
    }

    /* The id, then the offset to the audio data (less these 4 bytes) */
    uint8_t prefix[4];
    data_->read((char*) prefix, sizeof(prefix));
    if(!data_->good() || prefix[0] != 0x80 || prefix[1] != 0x00) {
        S_ERROR("Unexpected start to ADX file");
        return false;
    }

    const std::size_t header_size = ((prefix[2] << 8) | prefix[3]) + sizeof(prefix);

    /* When streaming we only need the header to find out the format,
     * otherwise we keep the whole (still compressed) file */
    std::size_t size = header_size;
    if(!stream) {
        data_->seekg(0, std::ios_base::end);
        auto end = data_->tellg();
        size = (end < 0) ? 0 : (std::size_t) end;
        data_->seekg(sizeof(prefix), std::ios_base::beg);
    }

    if(size < header_size) {
        S_ERROR("Truncated ADX file");
        return false;
    }

    auto data = std::make_shared<std::vector<uint8_t>>(size);
    std::copy(prefix, prefix + sizeof(prefix), data->begin());
    data_->read((char*) &(*data)[sizeof(prefix)], size - sizeof(prefix));
    if(std::size_t(data_->gcount()) != size - sizeof(prefix)) {
        S_ERROR("Truncated ADX file");
        return false;
    }

    auto adx = adx_read_buffer(&(*data)[0], data->size());
    if(!adx) {
        S_ERROR("Unsupported ADX file");
        return false;
    }

    const std::size_t channels = adx_stream_channel_count(adx);

    sound->set_sample_rate(adx_stream_sample_rate(adx));
    sound->set_channels(channels);
    sound->set_format((channels == 2) ? AUDIO_DATA_FORMAT_STEREO16 : AUDIO_DATA_FORMAT_MONO16);

    adx_stream_close(adx);

    std::weak_ptr<Sound> wptr = sound->shared_from_this();
    sound->set_playing_sound_init_function(std::bind(
        &init_source, wptr, filename_, (stream) ? ADXData() : ADXData(data),
        std::placeholders::_1
    ));

    return true;
}

}
}
