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

        static PaError _pa_error;
        static PaStream* _pa_stream;

        static int pa_callback(const void* input_buffer,
                              void* output_buffer,
                              unsigned long frames_per_buffer,
                              const PaStreamCallbackTimeInfo* time_info,
                              PaStreamCallbackFlags status_flags,
                              void* user_data);
};

template <typename T>
AudioOutput<T>* AudioOutput<T>::_instance = nullptr;
template <typename T>
PaError AudioOutput<T>::_pa_error;
template <typename T>
PaStream* AudioOutput<T>::_pa_stream;

template <typename T>
AudioOutput<T>* AudioOutput<T>::instance() {
    if (AudioOutput::_instance == nullptr) {
        AudioOutput::_instance = new AudioOutput;
    }

    return _instance;
}

template <typename T>
void AudioOutput<T>::init() {
    _pa_error = Pa_Initialize();
    if( _pa_error != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _pa_error ) << std::endl;
        return;
    }

    PaSampleFormat format;
    if constexpr (std::is_same_v<T, int16_t>)
        format = paInt16;
    else if constexpr (std::is_same_v<T, float>)
        format = paFloat32;

    /* Open an audio I/O stream. */
    _pa_error = Pa_OpenDefaultStream( &_pa_stream,
                                     0,          /* no input channels */
                                     2,          /* stereo output */
                                     format,
                                     SAMPLE_RATE,
                                     256,        /* frames per buffer, i.e. the number
                                                  of sample frames that PortAudio will
                                                  request from the callback. Many apps
                                                  may want to use
                                                  paframes_per_bufferUnspecified, which
                                                  tells PortAudio to pick the best,
                                                  possibly changing, buffer size.*/
                                     &pa_callback, /* this is your callback function */
                                     nullptr); /*This is a pointer that will be passed to
                                                  your callback*/
    if( _pa_error != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _pa_error ) << std::endl;
        return;
    }
}

template <typename T>
void AudioOutput<T>::destroy() {
    _pa_error = Pa_CloseStream( _pa_stream );
    if( _pa_error != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _pa_error ) << std::endl;
        return;
    }

    _pa_error = Pa_Terminate();
    if( _pa_error != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _pa_error ) << std::endl;
        return;
    }

    delete _instance;
}

template <typename T>
void AudioOutput<T>::start() {
    _pa_error = Pa_StartStream( _pa_stream );
    if( _pa_error != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _pa_error ) << std::endl;
        return;
    }
}

template <typename T>
void AudioOutput<T>::stop() {
    _pa_error = Pa_StopStream( _pa_stream );
    if( _pa_error != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _pa_error ) << std::endl;
        return;
    }
}

template <typename T>
void AudioOutput<T>::restart() {
    _pa_error = Pa_CloseStream( _pa_stream );
    if( _pa_error != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _pa_error ) << std::endl;
        return;
    }

    _pa_error = Pa_Terminate();
    if( _pa_error != paNoError ) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText( _pa_error ) << std::endl;
        return;
    }

    init();
}

template <typename T>
bool AudioOutput<T>::isDefault() {
    return Pa_GetDefaultOutputDevice() == 0;
}

template <typename T>
int AudioOutput<T>::pa_callback(const void* input_buffer,
                               void* output_buffer,
                               unsigned long frames_per_buffer,
                               const PaStreamCallbackTimeInfo* time_info,
                               PaStreamCallbackFlags status_flags,
                               void* user_data) {
    T* out = (T*)output_buffer;
    unsigned int i;
    (void) input_buffer; /* Prevent unused variable warning. */
    (void) user_data;

    for( i=0; i<frames_per_buffer; i++ )
    {
        *out++ = instance()->consume(); // left
        *out++ = instance()->consume(); // right
    }

    return 0;
}

#endif // AUDIO_OUTPUT_H