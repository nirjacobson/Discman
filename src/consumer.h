#ifndef CONSUMER_H
#define CONSUMER_H

#include "producer.h"

template <typename T>
class Consumer {
    public:
        Consumer();
        void producer(Producer<T>* const producer);

    protected:
        T consume() const;

    private:
        Producer<T>* _producer;
};

template <typename T>
Consumer<T>::Consumer()
    : _producer(nullptr) { }

template <typename T>
void Consumer<T>::producer(Producer<T>* const producer) {
    _producer = producer;
}

template <typename T>
T Consumer<T>::consume() const {
    if (_producer == nullptr)
        throw "no producer set";
    
    return _producer->next();
}

#endif // CONSUMER_H
