#pragma once

template <typename T>
inline bool Queue<T>::empty()
{
    return head == nullptr;
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
    head = new Node<T>{el, head};
    size++;
    if(size == 1)
        tail = head;
}

template <typename T>
inline void Queue<T>::dequeue()
{
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
    Node<T> popped = *head;
    dequeue();
    return popped.data;
}

template <typename T>
inline const T& Queue<T>::front() const
{
    return head->data;
}
