#include "AudioAsset.hpp"
#include <iostream>
#include <filesystem>
#include <cmath>

using namespace std;

AudioAsset::AudioAsset() = default;

AudioAsset::~AudioAsset() {
    unload();
}

bool AudioAsset::load(const string& path) {
    filePath = path;

    if (filesystem::exists(path)) {
        soundData = LoadSound(path.c_str());
        loaded = (soundData.frameCount > 0);
        memorySize = soundData.frameCount * 2 * 2; // Uoc tinh bytes cho stereo 16-bit
        cout << "[AudioAsset] Loaded Sound: " << path << " (" << memorySize / 1024 << " KB)\n";
        return loaded;
    }

    // Neu khong co file vat ly, tao am thanh song Sin (Beep/SFX) mo phong bang Raylib
    Wave wave = { 0 };
    if (IsAudioDeviceReady()) {
        // Tao am thanh ngan 0.1s tan so 520Hz (am thanh tan cong/hit)
        int sampleRate = 22050;
        float duration = 0.1f;
        int totalSamples = static_cast<int>(sampleRate * duration);
        short* samples = new short[totalSamples];
        
        for (int i = 0; i < totalSamples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            float freq = 440.0f + 200.0f * (1.0f - t / duration);
            samples[i] = static_cast<short>(10000 * sinf(2.0f * 3.14159f * freq * t));
        }

        wave.frameCount = totalSamples;
        wave.sampleRate = sampleRate;
        wave.sampleSize = 16;
        wave.channels = 1;
        wave.data = samples;

        soundData = LoadSoundFromWave(wave);
        delete[] samples;
        wave.data = nullptr;

        loaded = true;
        memorySize = 1 * 1024 * 1024; // Mo phong 1 MB RAM cho file audio goc
        cout << "[AudioAsset] Generated Procedural Sound: " << path << " (~1 MB RAM)\n";
        return true;
    }

    loaded = true;
    memorySize = 1024 * 1024;
    return true;
}

void AudioAsset::unload() {
    if (loaded && soundData.frameCount > 0 && IsAudioDeviceReady()) {
        UnloadSound(soundData);
        soundData.frameCount = 0;
        loaded = false;
        cout << "[AudioAsset] Unloaded Audio: " << filePath << "\n";
    }
}

void AudioAsset::render(float posX, float posY, float scale) {
    // Audio khong ve truc tiep len man hinh
}

void AudioAsset::play() {
    if (loaded && soundData.frameCount > 0 && IsAudioDeviceReady()) {
        PlaySound(soundData);
    }
}

bool AudioAsset::isLoaded() const {
    return loaded;
}

size_t AudioAsset::getSizeBytes() const {
    return memorySize;
}

AssetType AudioAsset::getType() const {
    return AssetType::AUDIO_CLIP;
}

string AudioAsset::getPath() const {
    return filePath;
}

shared_ptr<AudioAsset> AudioAsset::createSilentPlaceholder() {
    auto placeholder = make_shared<AudioAsset>();
    placeholder->filePath = "placeholder_silent_audio";
    placeholder->loaded = true;
    placeholder->memorySize = 64; // Hau nhu khong chiem bo nho
    return placeholder;
}
