#pragma once
#include <iostream>
#include <exception>

template <typename T>
inline bool Queue<T>::empty() const
{
    return size == 0;
}

template <typename T>
inline Queue<T>::Queue(const Queue<T>& other)
{
    Node<T> *current = other->head;
    while(current != nullptr)
    {
        enqueue(current->data);
        current = current->next;
    }
}

template <typename T>
inline Queue<T>& Queue<T>::operator=(Queue<T> other)
{
    swap(head, other.head);
    swap(tail, other.tail);
    size = other.size;
}

template<typename T>
inline Queue<T>::~Queue()
{
    while(head != nullptr)
    {
        Node<T> *toDel = head;
        head = head->next;
        delete toDel;
    }
    head = nullptr;
    tail = head;
}

template <typename T>
inline void Queue<T>::enqueue(const T &el)
{
    if(empty())
    {
        head = new Node<T>{el, nullptr};
        tail = head;
        size++;
        return;
    }

    tail->next = new Node<T>{el, nullptr};
    tail = tail->next;
    size++;
}

template <typename T>
inline void Queue<T>::dequeue()
{
    if(empty())
    {
        throw std::out_of_range("Cannot dequeue: The queue is empty!\n");
    }
    Node<T> *toDel = head;
    head = head->next;
    delete toDel;
    size--;
}

template <typename T>
inline const size_t& Queue<T>::get_size() const
{
    return size;
}

template <typename T>
inline T Queue<T>::pop()
{
    if(empty())
    {
        throw std::out_of_range("Cannot pop: The queue is empty!\n");
    }
    Node<T> popped = *head;
    dequeue();
    return popped.data;
}

template <typename T>
inline const T& Queue<T>::front() const
{
    if(empty())
    {
        throw std::invalid_argument("Cannot take front: The queue is empty!\n");
    }
    return head->data;
}
