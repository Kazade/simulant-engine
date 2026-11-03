#pragma once

#include <cmath>
#include <cstring>
#include <fstream>

#include "simulant/simulant.h"
#include "simulant/sound/drivers/null_sound_driver.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

/* Captures everything the loader uploads, and can pretend to be a driver
 * that plays Yamaha ADPCM itself (as the Dreamcast's does) */
class ADPCMRecordingDriver : public NullSoundDriver {
public:
    ADPCMRecordingDriver(Window* window, bool native_adpcm=false):
        NullSoundDriver(window),
        native_adpcm(native_adpcm) {}

    bool supports_format(AudioDataFormat format) const override {
        if(format == AUDIO_DATA_FORMAT_MONO_ADPCM || format == AUDIO_DATA_FORMAT_STEREO_ADPCM) {
            return native_adpcm;
        }

        return NullSoundDriver::supports_format(format);
    }

    void upload_buffer_data(AudioBufferID, AudioDataFormat format, const uint8_t* data, std::size_t size, uint32_t frequency) override {
        last_format = format;
        last_frequency = frequency;
        largest_upload = std::max(largest_upload, size);
        ++uploads;

        bytes.insert(bytes.end(), data, data + size);
    }

    /* What was uploaded, as 16-bit samples */
    std::vector<int16_t> samples() const {
        std::vector<int16_t> ret(bytes.size() / sizeof(int16_t));
        std::memcpy(&ret[0], &bytes[0], ret.size() * sizeof(int16_t));
        return ret;
    }

    bool native_adpcm = false;

    std::vector<uint8_t> bytes;
    std::size_t largest_upload = 0;
    std::size_t uploads = 0;
    AudioDataFormat last_format = AUDIO_DATA_FORMAT_MONO8;
    uint32_t last_frequency = 0;
};

class WAVADPCMLoaderTests : public test::SimulantTestCase {
public:
    /* These were all encoded from the two _pcm files, except for LONG which
     * is assets/sounds/simulant.wav. The wav2adpcm ones are the output of
     * KallistiOS' utils/wav2adpcm:
     *
     *   wav2adpcm -t mono_pcm.wav mono_wav2adpcm.wav
     *   wav2adpcm -t stereo_pcm.wav stereo_wav2adpcm.wav
     *   wav2adpcm -i -t stereo_pcm.wav stereo_wav2adpcm_interleaved.wav
     *   wav2adpcm -t ../simulant.wav long_wav2adpcm.wav
     *   ffmpeg -i stereo_pcm.wav -acodec adpcm_yamaha stereo_ffmpeg.wav
     */
    const char* MONO_PCM = "assets/sounds/adpcm/mono_pcm.wav";
    const char* STEREO_PCM = "assets/sounds/adpcm/stereo_pcm.wav";
    const char* LONG_PCM = "assets/sounds/simulant.wav";

    const char* MONO = "assets/sounds/adpcm/mono_wav2adpcm.wav";
    const char* STEREO_SPLIT = "assets/sounds/adpcm/stereo_wav2adpcm.wav";
    const char* STEREO_INTERLEAVED = "assets/sounds/adpcm/stereo_wav2adpcm_interleaved.wav";
    const char* STEREO_FFMPEG = "assets/sounds/adpcm/stereo_ffmpeg.wav";
    const char* LONG = "assets/sounds/adpcm/long_wav2adpcm.wav";

    void set_up() {
        SimulantTestCase::set_up();
        source_ = scene->create_child<AudioSource>();
    }

    void tear_down() {
        source_->destroy();
        SimulantTestCase::tear_down();
    }

    SoundPtr load(const char* path, bool stream=true) {
        SoundFlags flags;
        flags.stream_audio = stream;

        /* Otherwise we'd get back whichever variant was loaded first */
        flags.use_asset_cache = false;

        auto sound = scene->assets->load_sound(path, flags);
        assert_true(sound);
        return sound;
    }

    /* Plays the sound into the recorder, pulling buffers until the loader
     * says there is nothing left */
    void play_into(SoundPtr sound, ADPCMRecordingDriver& recorder) {
        assert_true(sound);

        auto original = sound->driver_;
        sound->driver_ = &recorder;

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
    }

    std::vector<int16_t> decode(const char* path, bool stream=true) {
        ADPCMRecordingDriver recorder(window);
        play_into(load(path, stream), recorder);
        return recorder.samples();
    }

    /* The 16-bit samples of an uncompressed .wav */
    std::vector<int16_t> read_pcm(const char* asset) {
        auto path = application->vfs->locate_file(asset);
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

    /* One channel of interleaved samples */
    static std::vector<int16_t> channel(const std::vector<int16_t>& samples, int which, int channels) {
        std::vector<int16_t> ret;
        for(std::size_t i = which; i < samples.size(); i += channels) {
            ret.push_back(samples[i]);
        }
        return ret;
    }

    /* How well actual matches expected, in dB, over the length of expected */
    static double snr(const std::vector<int16_t>& expected, const std::vector<int16_t>& actual) {
        double signal = 0, noise = 0;
        for(std::size_t i = 0; i < expected.size() && i < actual.size(); ++i) {
            double diff = double(actual[i]) - double(expected[i]);
            signal += double(expected[i]) * double(expected[i]);
            noise += diff * diff;
        }

        return 10.0 * std::log10(signal / std::max(noise, 1.0));
    }

    /* A straightforward decoder for the layout that sound drivers are given,
     * so we can check what's uploaded to one that plays ADPCM itself */
    static std::vector<int16_t> reference_decode(const std::vector<uint8_t>& data, int channels) {
        static const int DIFF[16] = {1, 3, 5, 7, 9, 11, 13, 15, -1, -3, -5, -7, -9, -11, -13, -15};
        static const int SCALE[8] = {230, 230, 230, 230, 307, 409, 512, 614};

        int signal[2] = {0, 0};
        int step[2] = {127, 127};

        auto next = [&](int c, int code) -> int16_t {
            signal[c] = std::max(-32768, std::min(32767, signal[c] + ((step[c] * DIFF[code]) / 8)));
            step[c] = std::max(127, std::min(24576, (step[c] * SCALE[code & 7]) >> 8));
            return (int16_t) signal[c];
        };

        std::vector<int16_t> ret;
        for(auto byte: data) {
            // Mono: two samples, low half first. Stereo: left low, right high
            ret.push_back(next(0, byte & 0xF));
            ret.push_back(next((channels == 2) ? 1 : 0, byte >> 4));
        }
        return ret;
    }

    /* ADPCM is lossy and only has 4 bits a sample, but a correct decode is
     * comfortably above this and a wrong one (the channels swapped, the
     * halves of each byte in the wrong order...) is nowhere near it */
    const double MIN_SNR = 15.0;

    void test_mono_properties() {
        auto sound = load(MONO);

        /* The sound reports what it decodes to */
        assert_equal(sound->sample_rate(), 22050u);
        assert_equal(sound->channels(), 1);
        assert_equal(sound->format(), AUDIO_DATA_FORMAT_MONO16);
    }

    void test_stereo_properties() {
        for(auto path: {STEREO_SPLIT, STEREO_INTERLEAVED, STEREO_FFMPEG}) {
            auto sound = load(path);

            assert_equal(sound->sample_rate(), 22050u);
            assert_equal(sound->channels(), 2);
            assert_equal(sound->format(), AUDIO_DATA_FORMAT_STEREO16);
        }
    }

    void test_mono_matches_source() {
        auto expected = read_pcm(MONO_PCM);
        assert_false(expected.empty());

        for(bool stream: {true, false}) {
            auto actual = decode(MONO, stream);

            assert_equal(actual.size(), expected.size());
            assert_true(snr(expected, actual) > MIN_SNR);
        }
    }

    void test_stereo_matches_source() {
        auto expected = read_pcm(STEREO_PCM);
        assert_false(expected.empty());

        auto left = channel(expected, 0, 2);
        auto right = channel(expected, 1, 2);

        for(auto path: {STEREO_SPLIT, STEREO_INTERLEAVED, STEREO_FFMPEG}) {
            for(bool stream: {true, false}) {
                auto actual = decode(path, stream);

                /* ffmpeg pads the end out to a whole block */
                assert_true(actual.size() >= expected.size());

                auto actual_left = channel(actual, 0, 2);
                auto actual_right = channel(actual, 1, 2);

                /* Each channel must come out of the side it went in */
                assert_true(snr(left, actual_left) > MIN_SNR);
                assert_true(snr(right, actual_right) > MIN_SNR);
                assert_true(snr(left, actual_right) < 3.0);
                assert_true(snr(right, actual_left) < 3.0);
            }
        }
    }

    void test_wav2adpcm_stereo_layouts_decode_identically() {
        /* The same encoder made both, they're just arranged differently */
        assert_true(decode(STEREO_SPLIT) == decode(STEREO_INTERLEAVED));
    }

    void test_long_sound_is_continuous_across_buffers() {
        auto sound = load(LONG);
        assert_equal(sound->sample_rate(), 44100u);

        ADPCMRecordingDriver recorder(window);
        play_into(sound, recorder);

        /* It doesn't fit in one buffer... */
        assert_true(recorder.uploads > 1);
        assert_true(recorder.largest_upload <= sound->buffer_size());
        assert_equal(recorder.last_format, AUDIO_DATA_FORMAT_MONO16);
        assert_equal(recorder.last_frequency, 44100u);

        auto actual = recorder.samples();
        auto expected = read_pcm(LONG_PCM);
        assert_equal(actual.size(), expected.size());
        assert_true(snr(expected, actual) > MIN_SNR);

        /* ...but the decoder mustn't start again for each one, the result
         * has to be exactly what decoding it in one go gives */
        auto path = application->vfs->locate_file(LONG);
        assert_true(path.has_value());
        std::ifstream in(path.value().str().c_str(), std::ios::binary);
        std::vector<uint8_t> file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        // wav2adpcm's header is always 44 bytes
        std::vector<uint8_t> data(file.begin() + 44, file.end());
        assert_true(actual == reference_decode(data, 1));
    }

    void test_native_driver_gets_compressed_mono() {
        auto sound = load(MONO);

        ADPCMRecordingDriver recorder(window, true);
        play_into(sound, recorder);

        /* The whole sound, still compressed, in a single buffer */
        assert_equal(recorder.uploads, 1u);
        assert_equal(recorder.last_format, AUDIO_DATA_FORMAT_MONO_ADPCM);
        assert_equal(recorder.last_frequency, 22050u);

        auto expected = read_pcm(MONO_PCM);
        assert_equal(recorder.bytes.size() * 2, expected.size());
        assert_true(snr(expected, reference_decode(recorder.bytes, 1)) > MIN_SNR);

        /* And it's the same audio we'd have decoded ourselves */
        assert_true(reference_decode(recorder.bytes, 1) == decode(MONO));
    }

    void test_native_driver_gets_compressed_stereo() {
        auto expected = read_pcm(STEREO_PCM);
        auto left = channel(expected, 0, 2);
        auto right = channel(expected, 1, 2);

        for(auto path: {STEREO_SPLIT, STEREO_INTERLEAVED, STEREO_FFMPEG}) {
            for(bool stream: {true, false}) {
                auto sound = load(path, stream);

                ADPCMRecordingDriver recorder(window, true);
                play_into(sound, recorder);

                assert_equal(recorder.uploads, 1u);
                assert_equal(recorder.last_format, AUDIO_DATA_FORMAT_STEREO_ADPCM);

                /* Whatever the file's layout, the driver gets a frame per
                 * byte with the left channel in the low half */
                auto decoded = reference_decode(recorder.bytes, 2);
                assert_true(snr(left, channel(decoded, 0, 2)) > MIN_SNR);
                assert_true(snr(right, channel(decoded, 1, 2)) > MIN_SNR);
            }
        }
    }

    void test_native_driver_gets_pcm_if_sound_is_too_long() {
        auto sound = load(LONG);

        ADPCMRecordingDriver recorder(window, true);
        play_into(sound, recorder);

        /* More frames than fit in a single compressed buffer, and several
         * would click where they join, so it's decoded instead */
        assert_true(recorder.uploads > 1);
        assert_equal(recorder.last_format, AUDIO_DATA_FORMAT_MONO16);
        assert_true(recorder.samples() == decode(LONG));
    }

    void test_sound_can_be_replayed() {
        for(auto path: {MONO, STEREO_SPLIT}) {
            auto sound = load(path);

            ADPCMRecordingDriver first(window), second(window);
            play_into(sound, first);
            play_into(sound, second);

            assert_false(first.bytes.empty());
            assert_true(first.bytes == second.bytes);
        }
    }

    void test_playback_finishes() {
        for(auto path: {MONO, STEREO_SPLIT, STEREO_INTERLEAVED, STEREO_FFMPEG}) {
            auto sound = load(path);

            assert_false(source_->playing_sound_count());
            source_->play_sound(sound);
            assert_true(source_->playing_sound_count());

            while(source_->playing_sound_count()) {
                application->run_frame();
            }
        }
    }

    void test_pcm_wav_keeps_all_of_its_data() {
        /* The loader used to drop the last 8 bytes of the data chunk */
        auto expected = read_pcm(MONO_PCM);

        ADPCMRecordingDriver recorder(window);
        play_into(load(MONO_PCM), recorder);

        assert_equal(recorder.last_format, AUDIO_DATA_FORMAT_MONO16);
        assert_true(recorder.samples() == expected);
    }

private:
    AudioSource* source_ = nullptr;
};

}
