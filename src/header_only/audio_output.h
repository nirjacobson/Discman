/**
 * @file audio_output.h
 * @author Nir Jacobson
 * @date 2026-04-07
 */

 #ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

#include <iostream>
#include <portaudio.h>
#include <stdint.h>

#include "producer_consumer/consumer.h"

/// @brief Abstracts PortAudio using a given audio sample data type.
/// @see \ref Producer/Consumer.
/// @tparam T The audio sample format. Must be int16_t or float.
template <typename T>
class AudioOutput : public Consumer<T> {
    public:

        /// @brief Factory method to return the AudioOutput.
        /// It may be called one or more times.
        /// Be sure to destroy() the instance in order to avoid leaks.
        /// It is suggested to do so in the teardown of the first caller.
        /// @return The AudioOutput.
        static AudioOutput* instance();

        static void init();    ///< Initializes PortAudio.
        static void destroy(); ///< Tears down PortAudio.
        static void restart(); ///< Restarts PortAudio.

        static void start();   ///< Starts the invokation of the PortAudio callback.
        static void stop();    ///< Stops the invokation of the PortAudio callback.

        /// @brief Returns whether the default audio output device is used.
        /// This indicates that a Bluetooth device is *not* being used.
        /// The default output is typically an audio jack or an HDMI port on the host.
        /// @return Whether the default audio output device is used.
        static bool isDefault();

        /// @brief The AudioOutput sample rate, which is matched to the sample rate
        /// of CD audio.
        static constexpr int SAMPLE_RATE = 44100;

    private:
        static AudioOutput* _instance; ///< The global AudioOutput instance.

        static PaError _pa_error;      ///< The most recent PortAudio error.
        static PaStream* _pa_stream;   ///< The PortAudio stream.

        /// @brief The PortAudio callback.
        /** @see <a href="https://www.portaudio.com/docs/v19-doxydocs/writing_a_callback.html"
         *          target="_blank">PortAudio documentation</a>.
         */
        /// @param [in] input_buffer      Not used. Normally used for audio recording.
        /// @param [in] output_buffer     Pointer to a buffer of audio samples to be filled. Sent to the audio device upon the method's return.
        /// @param [in] frames_per_buffer The number of frames (left + right sample) in the output_buffer.
        /// @param [in] time_info         Not used.
        /// @param [in] status_flags      Not used.
        /// @param [in] user_data         Not used.
        /// @return 0 for the successful filling of the output_buffer.
        static int pa_callback(const void* input_buffer,
                               void* output_buffer,
                               unsigned long frames_per_buffer,
                               const PaStreamCallbackTimeInfo* time_info,
                               PaStreamCallbackFlags status_flags,
                               void* user_data);
};

/// @brief The global AudioOutput instance.
/// @tparam T The data type of an audio sample.
/// Must be either int16_t or float.
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