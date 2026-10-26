#pragma once
/**
 * @file LinkedQueue.hpp
 * @brief FIFO queue built on a singly linked list (Lecture 2: Queue ADT + Singly Linked List).
 *
 *   head -> [a | next] -> [b | next] -> [c | null] <- tail
 *   dequeue() removes at the head, enqueue() adds at the tail.
 *
 * Complexity:
 *   enqueue O(1), dequeue O(1), front O(1), size O(1), clear O(n).
 *   (The array queue in the lecture keeps the front at index 0, so dequeue has
 *   to shift every element: O(n). Keeping a tail pointer makes both ends O(1).)
 *
 * Used by:
 *   navigation/Pathfinding.cpp  BFS frontier
 *   fulfilment/SortingRobot     conveyor of items waiting to be sorted
 *
 * Memory: every node is created with `new` and freed with `delete` in clear(),
 * which the destructor calls (Lecture 1b: constructor/destructor). Because the
 * class owns raw pointers it also defines a copy constructor and copy
 * assignment ("rule of three"); otherwise two queues would share, and both
 * delete, the same nodes.
 */
#include <stdexcept>

namespace wh {

template <typename T>
class LinkedQueue {
public:
    LinkedQueue() = default;
    ~LinkedQueue() { clear(); }

    LinkedQueue(const LinkedQueue& other) { copyFrom(other); }
    LinkedQueue& operator=(const LinkedQueue& other)
    {
        if (this != &other) {
            clear();
            copyFrom(other);
        }
        return *this;
    }

    /// Adds `value` at the back. O(1).
    void enqueue(const T& value)
    {
        Node* node = new Node{value, nullptr};
        if (tail_ == nullptr) {
            head_ = node;          // queue was empty: node is both head and tail
        } else {
            tail_->next = node;    // link after the current last node
        }
        tail_ = node;
        ++size_;
    }

    /// Removes and returns the front value. O(1). Throws std::underflow_error if empty.
    T dequeue()
    {
        if (head_ == nullptr) throw std::underflow_error("LinkedQueue::dequeue on an empty queue");
        Node* old = head_;
        T value = old->data;
        head_ = old->next;
        if (head_ == nullptr) tail_ = nullptr;   // removed the last node
        delete old;
        --size_;
        return value;
    }

    /// The value that dequeue() would return. Throws std::underflow_error if empty.
    const T& front() const
    {
        if (head_ == nullptr) throw std::underflow_error("LinkedQueue::front on an empty queue");
        return head_->data;
    }

    bool empty() const { return head_ == nullptr; }
    int size() const { return size_; }

    /// Deletes every node. O(n).
    void clear()
    {
        while (head_ != nullptr) {
            Node* next = head_->next;
            delete head_;
            head_ = next;
        }
        tail_ = nullptr;
        size_ = 0;
    }

    /// Calls visit(value) for every value from front to back (used to display the queue). O(n).
    template <typename Visitor>
    void forEach(Visitor visit) const
    {
        for (Node* n = head_; n != nullptr; n = n->next) visit(n->data);
    }

private:
    struct Node {
        T data;
        Node* next;
    };

    void copyFrom(const LinkedQueue& other)
    {
        for (Node* n = other.head_; n != nullptr; n = n->next) enqueue(n->data);
    }

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    int size_ = 0;
};

} // namespace wh
