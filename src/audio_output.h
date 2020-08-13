#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

#include <iostream>
#include <portaudio.h>
#include <stdint.h>

#include "consumer.h"

template <typename T>
class AudioOutput : public Consumer<T> {
    public:
        static AudioOutput* instance();

        static void init();
        static void destroy();

        static void start();
        static void stop();
        static void restart();

        static bool isDefault();

        static constexpr int SAMPLE_RATE = 44100;

    private:
        static AudioOutput* _instance;

        static PaError _paError;
        static PaStream* _paStream;

        static int paCallback(const void* inputBuffer,
                              void* outputBuffer,
                              unsigned long framesPerBuffer,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void* userData);
};

template <typename T>
AudioOutput<T>* AudioOutput<T>::_instance = nullptr;
template <typename T>
PaError AudioOutput<T>::_paError;
template <typename T>
PaStream* AudioOutput<T>::_paStream;

template <typename T>
AudioOutput<T>* AudioOutput<T>::instance() {
    if (AudioOutput::_instance == nullptr) {
        AudioOutput::_instance = new AudioOutput;
    }

    return _instance;
}

template <typename T>
void AudioOutput<T>::init() {
    _paError = Pa_Initialize();
    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    }

    PaSampleFormat format;
    if constexpr (std::is_same_v<T, int16_t>)
        format = paInt16;
    else if constexpr (std::is_same_v<T, float>)
        format = paFloat32;

    /* Open an audio I/O stream. */
    _paError = Pa_OpenDefaultStream( &_paStream,
                                     0,          /* no input channels */
                                     2,          /* stereo output */
                                     format,
                                     SAMPLE_RATE,
                                     256,        /* frames per buffer, i.e. the number
                                                  of sample frames that PortAudio will
                                                  request from the callback. Many apps
                                                  may want to use
                                                  paFramesPerBufferUnspecified, which
                                                  tells PortAudio to pick the best,
                                                  possibly changing, buffer size.*/
                                     &paCallback, /* this is your callback function */
                                     nullptr); /*This is a pointer that will be passed to
                                                  your callback*/
    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    }
}

template <typename T>
void AudioOutput<T>::destroy() {
    _paError = Pa_CloseStream( _paStream );
    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    }

    _paError = Pa_Terminate();
    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    }

    delete _instance;
}

template <typename T>
void AudioOutput<T>::start() {
    _paError = Pa_StartStream( _paStream );
    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    }
}

template <typename T>
void AudioOutput<T>::stop() {
    _paError = Pa_StopStream( _paStream );
    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    }
}

template <typename T>
void AudioOutput<T>::restart() {
    _paError = Pa_CloseStream( _paStream );
    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    }

    _paError = Pa_Terminate();
    if( _paError != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _paError ) << std::endl;
        return;
    }

    init();
}

template <typename T>
bool AudioOutput<T>::isDefault() {
    return Pa_GetDefaultOutputDevice() == 0;
}

template <typename T>
int AudioOutput<T>::paCallback(const void* inputBuffer,
                               void* outputBuffer,
                               unsigned long framesPerBuffer,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void* userData) {
    T* out = (T*)outputBuffer;
    unsigned int i;
    (void) inputBuffer; /* Prevent unused variable warning. */
    (void) userData;

    for( i=0; i<framesPerBuffer; i++ )
    {
        *out++ = instance()->consume(); // left
        *out++ = instance()->consume(); // right
    }

    return 0;
}

#endif // AUDIO_OUTPUT_H