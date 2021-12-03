#pragma once
#include <cstddef>

template <typename T>
struct Node
{   
    T data{};
    Node *next{};

    Node(const T& data_ , Node<T>* next_):data(data_) , next(next_){}
};

template <typename T>
class Queue
{
    private:
    Node<T> *head{};
    Node<T> *tail{};
    size_t size = 0;

    void copy(const Queue<T> &other);

    public:
    Queue() = default;

    Queue(const Queue<T>&);

    Queue& operator=(const Queue<T>);

    ~Queue();

    bool empty();

    void enqueue(const T&);

    void dequeue();

    const size_t& get_size() const;

    const T& front() const;

    T pop();

};

#include "queue.inl"