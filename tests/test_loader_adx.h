#pragma once

#include <cmath>
#include <cstring>
#include <fstream>

#include "simulant/simulant.h"
#include "simulant/sound/drivers/null_sound_driver.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

/* Captures everything the loader uploads so that we can check what it decoded */
class RecordingSoundDriver : public NullSoundDriver {
public:
    RecordingSoundDriver(Window* window):
        NullSoundDriver(window) {}

    void upload_buffer_data(AudioBufferID, AudioDataFormat format, const uint8_t* data, std::size_t bytes, uint32_t frequency) override {
        last_format = format;
        last_frequency = frequency;
        largest_upload = std::max(largest_upload, bytes);

        std::size_t count = bytes / sizeof(int16_t);
        std::size_t offset = samples.size();
        samples.resize(offset + count);
        std::memcpy(&samples[offset], data, count * sizeof(int16_t));
    }

    std::vector<int16_t> samples;
    std::size_t largest_upload = 0;
    AudioDataFormat last_format = AUDIO_DATA_FORMAT_MONO8;
    uint32_t last_frequency = 0;
};

class ADXLoaderTests : public test::SimulantTestCase {
public:
    const char* ADX_PATH = "assets/sounds/simulant.adx";
    const char* WAV_PATH = "assets/sounds/simulant.wav";

    void set_up() {
        SimulantTestCase::set_up();
        source_ = scene->create_child<AudioSource>();
    }

    void tear_down() {
        source_->destroy();
        SimulantTestCase::tear_down();
    }

    SoundPtr load(bool stream) {
        SoundFlags flags;
        flags.stream_audio = stream;

        /* Otherwise we'd get back whichever variant was loaded first */
        flags.use_asset_cache = false;

        return scene->assets->load_sound(ADX_PATH, flags);
    }

    /* Plays the sound into a recording driver, and pulls buffers until
     * the loader says there is nothing left */
    std::vector<int16_t> decode(SoundPtr sound, RecordingSoundDriver* recorder = nullptr) {
        RecordingSoundDriver local(window);
        if(!recorder) {
            recorder = &local;
        }

        auto original = sound->driver_;
        sound->driver_ = recorder;

        auto playing = source_->play_sound(sound);
        assert_true(playing);
        assert_true(bool(playing->stream_func_));

        // Normally bounded by the length of the file, this just stops a
        // broken loader hanging the tests
        for(int i = 0; i < 10000; ++i) {
            if(playing->stream_func_(0) <= 0) {
                break;
            }
        }

        assert_equal(playing->stream_func_(0), 0);

        sound->driver_ = original;
        source_->stop_sound(playing->id());

        return recorder->samples;
    }

    /* The samples from the .wav that the .adx was encoded from */
    std::vector<int16_t> read_wav_samples() {
        auto path = application->vfs->locate_file(WAV_PATH);
        assert_true(path.has_value());

        std::ifstream in(path.value().str().c_str(), std::ios::binary);
        assert_true(in.good());

        in.seekg(12); // RIFF, size, WAVE

        while(in.good()) {
            char id[4];
            uint8_t len[4];
            in.read(id, 4);
            in.read((char*) len, 4);

            uint32_t size = len[0] | (len[1] << 8) | (len[2] << 16) | (uint32_t(len[3]) << 24);
            if(std::memcmp(id, "data", 4) == 0) {
                std::vector<uint8_t> bytes(size);
                in.read((char*) &bytes[0], size);

                std::vector<int16_t> ret(size / 2);
                for(std::size_t i = 0; i < ret.size(); ++i) {
                    ret[i] = (int16_t) (bytes[i * 2] | (bytes[i * 2 + 1] << 8));
                }
                return ret;
            }

            in.seekg(size + (size & 1), std::ios_base::cur);
        }

        return {};
    }

    Path write_temp_file(const std::string& name, const std::vector<uint8_t>& data) {
        auto path = Path::system_temp_dir().append(name);

        std::ofstream out(path.str().c_str(), std::ios::binary);
        out.write((const char*) &data[0], data.size());
        out.close();

        return path;
    }

    void test_loader_is_registered() {
        assert_true(application->loader_type("adx"));
        assert_true(application->loader_for(ADX_PATH));
    }

    void test_streaming_is_the_default() {
        auto sound = scene->assets->load_sound(ADX_PATH);
        assert_true(sound);

        assert_equal(sound->sample_rate(), 44100u);
        assert_equal(sound->channels(), 1);
        assert_equal(sound->format(), AUDIO_DATA_FORMAT_MONO16);

        auto streamed = decode(sound);
        assert_false(streamed.empty());
        assert_true(streamed == decode(load(true)));
    }

    void test_loading_into_memory() {
        auto sound = load(false);
        assert_true(sound);

        assert_equal(sound->sample_rate(), 44100u);
        assert_equal(sound->channels(), 1);
        assert_equal(sound->format(), AUDIO_DATA_FORMAT_MONO16);

        auto in_memory = decode(sound);
        assert_false(in_memory.empty());

        /* Both paths must produce exactly the same audio */
        assert_true(in_memory == decode(load(true)));
    }

    void test_uploads_match_sound_format() {
        auto sound = load(true);

        RecordingSoundDriver recorder(window);
        decode(sound, &recorder);

        assert_equal(recorder.last_format, AUDIO_DATA_FORMAT_MONO16);
        assert_equal(recorder.last_frequency, 44100u);
        assert_true(recorder.largest_upload > 0);
        assert_true(recorder.largest_upload <= sound->buffer_size());
    }

    void test_decoded_audio_matches_source() {
        auto expected = read_wav_samples();
        assert_false(expected.empty());

        for(bool stream: {true, false}) {
            auto actual = decode(load(stream));

            assert_equal(actual.size(), expected.size());

            /* ADX is lossy, so compare the noise to the signal */
            double signal = 0, noise = 0;
            for(std::size_t i = 0; i < expected.size(); ++i) {
                double diff = double(actual[i]) - double(expected[i]);
                signal += double(expected[i]) * double(expected[i]);
                noise += diff * diff;
            }

            double snr = 10.0 * std::log10(signal / std::max(noise, 1.0));
            assert_true(snr > 40.0);
        }
    }

    void test_sound_can_be_replayed() {
        for(bool stream: {true, false}) {
            auto sound = load(stream);

            auto first = decode(sound);
            auto second = decode(sound);

            assert_false(first.empty());
            assert_true(first == second);
        }
    }

    void test_simultaneous_streams_are_independent() {
        auto sound = load(true);
        auto expected = decode(sound);

        RecordingSoundDriver recorder(window);
        auto original = sound->driver_;
        sound->driver_ = &recorder;

        auto a = source_->play_sound(sound);
        auto b = source_->play_sound(sound);

        /* Each sound has queued its initial buffers, now interleave reads
         * from both and make sure a alone still gets the whole file */
        std::size_t before_a = recorder.samples.size() / 2;
        std::vector<int16_t> from_a(
            recorder.samples.begin(), recorder.samples.begin() + before_a
        );

        for(int i = 0; i < 10000; ++i) {
            std::size_t start = recorder.samples.size();
            int32_t bytes = a->stream_func_(0);
            from_a.insert(from_a.end(), recorder.samples.begin() + start, recorder.samples.end());

            b->stream_func_(0);

            if(bytes <= 0) {
                break;
            }
        }

        sound->driver_ = original;
        source_->stop_sound(a->id());
        source_->stop_sound(b->id());

        assert_true(from_a == expected);
    }

    void test_playback_finishes() {
        for(bool stream: {true, false}) {
            auto sound = load(stream);

            assert_false(source_->playing_sound_count());
            source_->play_sound(sound);
            assert_true(source_->playing_sound_count());

            while(source_->playing_sound_count()) {
                application->run_frame();
            }
        }
    }

    void test_sound_destruction_stops_stream() {
        auto sound = load(true);
        auto playing = source_->play_sound(sound);
        auto func = playing->stream_func_;

        auto id = sound->id();
        scene->assets->destroy_sound(id);
        sound.reset();
        scene->assets->run_garbage_collection();

        assert_false(scene->assets->has_sound(id));

        /* The stream doesn't keep the sound alive */
        assert_equal(func(0), -1);
    }

    void test_invalid_files_are_rejected() {
        std::vector<uint8_t> garbage(256, 0x42);
        assert_false(scene->assets->load_sound(write_temp_file("simulant_garbage.adx", garbage)));

        /* A valid start, but nothing after it */
        std::vector<uint8_t> truncated = {0x80, 0x00, 0x00, 0x20, 0x03, 0x12};
        assert_false(scene->assets->load_sound(write_temp_file("simulant_truncated.adx", truncated)));

        SoundFlags flags;
        flags.stream_audio = false;
        assert_false(scene->assets->load_sound(write_temp_file("simulant_truncated.adx", truncated), flags));
    }

private:
    AudioSource* source_ = nullptr;
};

}
