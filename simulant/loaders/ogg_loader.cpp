//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "ogg_loader.h"
#include "stb_vorbis.h"

#include "../deps/kazlog/kazlog.h"
#include "../utils/al_error.h"
#include "../sound.h"

namespace smlt {
namespace loaders {

class StreamWrapper {
public:
    typedef std::shared_ptr<StreamWrapper> ptr;
    StreamWrapper(stb_vorbis* vorbis):
        vorbis_(vorbis) {}

    virtual ~StreamWrapper() {
        stb_vorbis_close(vorbis_);
        vorbis_ = nullptr;
    }

    stb_vorbis* get() { return vorbis_; }
private:
    stb_vorbis* vorbis_;
};

int32_t queue_buffer(std::weak_ptr<Sound> sound, StreamWrapper::ptr stream, ALuint buffer) {
    auto self = sound.lock();
    if(!self) {
        // The sound was destroyed, we can't stream without a sound so just bail
        return 0;
    }

    std::vector<ALshort> pcm;
    int s = self->buffer_size();
    pcm.resize(s);

    size_t size = 0;
    int result = 0;

    while(size < self->buffer_size()) {
        result = stb_vorbis_get_samples_short_interleaved(stream->get(), self->channels(), &pcm[0] + size, self->buffer_size() - size);
        if(result > 0)  {
            size += result * self->channels();
        } else {
            break;
        }
    }

    if(size == 0) {
        return 0;
    }

    ALCheck(alBufferData, buffer, self->format(), &pcm[0], size * sizeof(ALshort), self->sample_rate());
    return size;
}

void init_source(Sound* self, SourceInstance& source) {
    /*
     *  This is either smart or crazy and I haven't worked out which yet...
     *
     *  Create a new stream from the supplied sound, wrap it in a smart pointer and bind it to the Source's stream function.
     *
     *  This means the source knows nothing about the sound or the stream, and we don't need to store any stb_vorbis specific
     *  data on the Source, well, not explicitly.
     */

    StreamWrapper::ptr stream(new StreamWrapper(stb_vorbis_open_memory(&self->data()[0], self->data().size(), nullptr, nullptr)));
    source.set_stream_func(std::bind(&queue_buffer, self->shared_from_this(), stream, std::placeholders::_1));
}


void OGGLoader::into(Loadable& resource, const LoaderOptions& options) {
    Loadable* res_ptr = &resource;
    Sound* sound = dynamic_cast<Sound*>(res_ptr);
    assert(sound && "You passed a Resource that is not a Sound to the OGG loader");

    auto str = this->data_->str();

    assert(!str.empty());

    L_DEBUG(_F("Stream size: {0}").format(str.length()));
    std::vector<uint8_t> data(str.begin(), str.end());

    StreamWrapper stream(stb_vorbis_open_memory(&data[0], data.size(),nullptr, nullptr));

    if(!stream.get()) {
        throw std::runtime_error("Unable to load the OGG file");
    }

    stb_vorbis_info info = stb_vorbis_get_info(stream.get());

    sound->set_sample_rate(info.sample_rate);
    sound->set_buffer_size(4096 * 8);
    sound->set_data(data);
    sound->set_channels(info.channels);
    sound->set_format((info.channels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16);
    sound->set_source_init_function(std::bind(&init_source, sound, std::placeholders::_1));
}


}
}
