#include "ogg_loader.h"
#include "stb_vorbis.h"

#include "../utils/al_error.h"
#include "../sound.h"

namespace kglt {
namespace loaders {

class StreamWrapper {
public:
    typedef std::shared_ptr<StreamWrapper> ptr;
    StreamWrapper(stb_vorbis* vorbis):
        vorbis_(vorbis) {}

    virtual ~StreamWrapper() {
        stb_vorbis_close(vorbis_);
    }

    stb_vorbis* get() { return vorbis_; }
private:
    stb_vorbis* vorbis_;
};

int32_t queue_buffer(Sound* self, StreamWrapper::ptr stream, ALuint buffer) {
    std::vector<ALshort> pcm;
    int s = self->buffer_size();
    pcm.resize(s);

    int size = 0;
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
    source.set_stream_func(std::bind(&queue_buffer, self, stream, std::placeholders::_1));
}


void OGGLoader::into(Loadable& resource, const LoaderOptions& options) {
    Loadable* res_ptr = &resource;
    Sound* sound = dynamic_cast<Sound*>(res_ptr);
    assert(sound && "You passed a Resource that is not a Sound to the OGG loader");

    std::ifstream t(filename_.encode().c_str(), std::ios::binary);
    std::vector<uint8_t> data;

    t.seekg(0, std::ios::end);
    data.reserve(t.tellg());
    t.seekg(0, std::ios::beg);

    data.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    StreamWrapper stream(stb_vorbis_open_memory(&data[0], data.size(),nullptr, nullptr));

    if(!stream.get()) {
        throw IOError("Unable to load the OGG file");
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
