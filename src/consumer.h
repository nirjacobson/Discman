/**
 * @file consumer.h
 * @author Nir Jacobson
 * @date 2026-04-07
 */
#ifndef CONSUMER_H
#define CONSUMER_H

#include "producer.h"

/// @brief The Consumer interface
/// @see \ref Producer/Consumer.
/// @tparam T The data type consumed
template <typename T>
class Consumer {
    public:
        Consumer();
        virtual ~Consumer() {};

        /// @brief Sets the producer from which to retrieve data
        /// @param [in] producer The producer from which to retrieve data
        virtual void producer(Producer<T>* const producer);

    protected:
        /// @brief Returns one datum from the producer
        /// @return The datum
        T consume() const;

    private:
        /// @brief The Producer from which to retrieve data
        Producer<T>* _producer;
};

template <typename T>
Consumer<T>::Consumer()
    : _producer(nullptr) { }

template <typename T>
void Consumer<T>::producer(Producer<T>* const producer) {
    _producer = producer;
    _producer->register_consumer(this);
}

template <typename T>
T Consumer<T>::consume() const {
    if (_producer == nullptr)
        throw "no producer set";

    return _producer->next();
}

#endif // CONSUMER_H
