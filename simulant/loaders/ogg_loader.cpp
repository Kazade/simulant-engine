//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "ogg_loader.h"
#include "stb_vorbis.h"

#include "../logging.h"
#include "../sound.h"
#include "../generic/raii.h"

namespace smlt {
namespace loaders {

/* FIXME: This could be smart pointer with
 * a custom deleter */
class StreamWrapper {
public:
    typedef std::shared_ptr<StreamWrapper> ptr;
    StreamWrapper(stb_vorbis* vorbis):
        vorbis_(vorbis) {}

    StreamWrapper(stb_vorbis* vorbis, std::shared_ptr<std::vector<uint8_t>> data):
        vorbis_(vorbis),
        data_(data) {}

    virtual ~StreamWrapper() {
        stb_vorbis_close(vorbis_);
        vorbis_ = nullptr;
    }

    stb_vorbis* get() { return vorbis_; }

private:
    stb_vorbis* vorbis_;
    std::shared_ptr<std::vector<uint8_t>> data_;
};

int32_t queue_buffer(std::weak_ptr<Sound> sound, StreamWrapper::ptr stream, AudioBufferID buffer) {
    std::shared_ptr<Sound> self = sound.lock();
    if(!self) {
        // The sound was destroyed, we can't stream without a sound so just bail
        return -1;
    }

    auto buffer_size = self->buffer_size();
    std::vector<int16_t> pcm(buffer_size, 0);

    int shorts_required = buffer_size / 2;

    // 'result' is the total number of samples per channel
    int result = stb_vorbis_get_samples_short_interleaved(
        stream->get(),
        self->channels(),
        &pcm[0],
        shorts_required
    );

    if(result > 0)  {
        // samples-per-channel * channels * size of each sample
        // so this is the total bytes we just read
        int byte_size = result * self->channels() * sizeof(int16_t);
        self->_driver()->upload_buffer_data(
            buffer, self->format(),
            (uint8_t*) &pcm[0], byte_size, self->sample_rate()
        );

        return byte_size;
    } else {
        S_INFO("No more samples available");
        return 0;
    }
}

static void init_source(Sound* self, PlayingSound& source) {
    /*
     *  This is either smart or crazy and I haven't worked out which yet...
     *
     *  Create a new stream from the supplied sound, wrap it in a smart pointer and bind it to the Source's stream function.
     *
     *  This means the source knows nothing about the sound or the stream, and we don't need to store any stb_vorbis specific
     *  data on the Source, well, not explicitly.
     */

    /* Set the stream func to nothing - this will give any captured smart pointers
     * the opportunity to unload before we do anything else */
    source.set_stream_func(StreamFunc());

    auto fstream = std::dynamic_pointer_cast<FileIfstream>(self->input_stream());
    fstream->seekg(0);

    auto stb = stb_vorbis_open_file(fstream->file(), 0, nullptr, nullptr);
    StreamWrapper::ptr stream(new StreamWrapper(stb));

    /* Using a weak_ptr is important, otherwise the shared_ptr will be bound to the function
     * object even though the argument is a weak_ptr */
    std::weak_ptr<Sound> wptr = self->shared_from_this();
    source.set_stream_func(std::bind(&queue_buffer, wptr, stream, std::placeholders::_1));
}

static void init_source_memory(Sound* self, PlayingSound& source) {
    auto fstream = std::dynamic_pointer_cast<FileIfstream>(self->input_stream());
    fstream->seekg(0);

    std::shared_ptr<std::vector<uint8_t>> data;
    data.reset(new std::vector<uint8_t>(
        std::istreambuf_iterator<char>(*fstream), {}
    ));

    auto stb = stb_vorbis_open_memory(&(*data)[0], data->size(), nullptr, nullptr);
    StreamWrapper::ptr stream(new StreamWrapper(stb, data));

    /* Using a weak_ptr is important, otherwise the shared_ptr will be bound to the function
     * object even though the argument is a weak_ptr */
    std::weak_ptr<Sound> wptr = self->shared_from_this();
    source.set_stream_func(std::bind(&queue_buffer, wptr, stream, std::placeholders::_1));
}

void OGGLoader::into(Loadable& resource, const LoaderOptions& options) {
    /* Stream unless someone passed stream == false */
    bool stream = !(options.count("stream") && any_cast<bool>(options.at("stream")) == false);

    Loadable* res_ptr = &resource;
    Sound* sound = dynamic_cast<Sound*>(res_ptr);
    assert(sound && "You passed a Resource that is not a Sound to the OGG loader");

    auto fstream = std::dynamic_pointer_cast<FileIfstream>(
        data_
    );

    int error = 0;

    auto stb_stream = stb_vorbis_open_file(
        fstream->file(),
        0,
        &error, nullptr
    );

    if(!stb_stream) {
        S_ERROR("Unable to load the OGG file");
        throw std::runtime_error("Unable to load the OGG file");
    }

    stb_vorbis_info info = stb_vorbis_get_info(stb_stream);

    raii::Finally finally([&]() {
        stb_vorbis_close(stb_stream);
    });

    _S_UNUSED(finally);

    // Rewind
    fstream->seekg(0);

    sound->set_sample_rate(info.sample_rate);
    sound->set_input_stream(fstream);
    sound->set_channels(info.channels);
    sound->set_format((info.channels == 2) ? AUDIO_DATA_FORMAT_STEREO16 : AUDIO_DATA_FORMAT_MONO16);

    if(stream) {
        sound->set_playing_sound_init_function(std::bind(&init_source, sound, std::placeholders::_1));
    } else {
        sound->set_playing_sound_init_function(std::bind(&init_source_memory, sound, std::placeholders::_1));
    }
}


}
}
