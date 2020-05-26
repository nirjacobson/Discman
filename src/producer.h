#ifndef PRODUCER_H
#define PRODUCER_H

template <typename T>
class Producer {
    public:
        virtual T next() = 0;
};

#endif // PRODUCER_H
