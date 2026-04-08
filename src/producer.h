/**
 * @file producer.h
 * @author Nir Jacobson
 * @date 2026-04-07
 */
#ifndef PRODUCER_H
#define PRODUCER_H

template <typename T>
class Consumer;

/// @brief The Producer interface
/// @see \ref Producer/Consumer.
/// @tparam T The data type produced
template <typename T>
class Producer {
    public:
        /// @brief When a Producer has multiple Consumers, each Consumer
        /// must call this overloaded version of next().
        /// @param [in] consumer the Consumer for which to provide the next datum 
        /// @return the next datum
        T next(const Consumer<T>* const consumer);

        /// @brief When a producer has a single Consumer, that Consumer
        /// may simply call this version of next() to retrieve the next datum.
        /// When a Producer has multiple Consumers, this method must take
        /// into account the _current_consumer.
        /// @return the next datum
        virtual T next() = 0;

    private:
        /// @brief The Consumer that will receive the datum returned by next()
        Consumer<T>* _current_consumer = nullptr;

        /// @brief Use this method to maintain state associated with a particular Consumer.
        /// The state can be used to retrieve the right datum for the Consumer.
        /// @param [in] consumer the Consumer to register
        void register_consumer(const Consumer<T>* const consumer) {};

    protected:
        Consumer<T>* current_consumer() const;
};

template <typename T>
T Producer<T>::next(const Consumer<T>* const consumer) {
    _current_consumer = consumer;
    return next();
}

template <typename T>
Consumer<T>* Producer<T>::current_consumer() const {
    return _current_consumer;
}

#endif // PRODUCER_H
