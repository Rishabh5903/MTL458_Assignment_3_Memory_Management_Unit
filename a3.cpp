#include <iostream>
using namespace std;

template<typename T>
class Vector {
private:
    T* arr;
    int capacity;
    int current;

public:
    Vector() {
        arr = new T[1];
        capacity = 1;
        current = 0;
    }

    Vector(int size, T default_value) {
        capacity = size;
        arr = new T[capacity];
        current = size;
        for (int i = 0; i < size; i++) {
            arr[i] = default_value;
        }
    }

    ~Vector() {
        delete[] arr;
    }

    void push_back(T data) {
        if (current == capacity) {
            T* temp = new T[2 * capacity];
            for (int i = 0; i < capacity; i++) {
                temp[i] = arr[i];
            }
            delete[] arr;
            capacity *= 2;
            arr = temp;
        }
        arr[current] = data;
        current++;
    }

    T& operator[](int index) {
        return arr[index];
    }

    const T& operator[](int index) const {
        return arr[index];
    }

    void pop_back() {
        if (current > 0) current--;
    }

    int size() const {
        return current;
    }

    void clear() {
        current = 0;
    }
};

template<typename T>
class Queue {
private:
    struct Node {
        T data;
        Node* next;
        Node(T value) : data(value), next(nullptr) {}
    };
    Node* front;
    Node* rear;
    int size;

public:
    Queue() : front(nullptr), rear(nullptr), size(0) {}

    ~Queue() {
        while (!isEmpty()) {
            dequeue();
        }
    }

    void enqueue(T value) {
        Node* newNode = new Node(value);
        if (isEmpty()) {
            front = rear = newNode;
        } else {
            rear->next = newNode;
            rear = newNode;
        }
        size++;
    }

    T dequeue() {
        if (isEmpty()) {
            return T();
        }
        Node* temp = front;
        T value = temp->data;
        front = front->next;
        delete temp;
        size--;
        if (isEmpty()) {
            rear = nullptr;
        }
        return value;
    }

    T peek() const {
        if (isEmpty()) {
            return T();
        }
        return front->data;
    }

    bool isEmpty() const {
        return size == 0;
    }

    int getSize() const {
        return size;
    }
};


template<typename K, typename V>
class UnorderedMap {
private:
    static const int TABLE_SIZE = 10007;
    struct Node {
        K key;
        V value;
        Node* next;
        Node(K k, V v) : key(k), value(v), next(nullptr) {}
    };
    Vector<Node*> table;
    int count;

public:
    UnorderedMap() : count(0) {
        for (int i = 0; i < TABLE_SIZE; i++) {
            table.push_back(nullptr);
        }
    }

    void insert(K key, V value) {
        int index = key % TABLE_SIZE;
        Node* current = table[index];
        while (current != nullptr) {
            if (current->key == key) {
                current->value = value;
                return;
            }
            current = current->next;
        }
        Node* newNode = new Node(key, value);
        newNode->next = table[index];
        table[index] = newNode;
        count++;
    }

    bool contains(K key) const {
        int index = key % TABLE_SIZE;
        Node* current = table[index];
        while (current != nullptr) {
            if (current->key == key) return true;
            current = current->next;
        }
        return false;
    }

    V get(K key) const {
        int index = key % TABLE_SIZE;
        Node* current = table[index];
        while (current != nullptr) {
            if (current->key == key) return current->value;
            current = current->next;
        }
        return V();
    }

    void erase(K key) {
        int index = key % TABLE_SIZE;
        Node* current = table[index];
        Node* prev = nullptr;
        while (current != nullptr) {
            if (current->key == key) {
                if (prev == nullptr) {
                    table[index] = current->next;
                } else {
                    prev->next = current->next;
                }
                delete current;
                count--;
                return;
            }
            prev = current;
            current = current->next;
        }
    }

    int size() const {
        return count;
    }

    void clear() {
        for (int i = 0; i < TABLE_SIZE; i++) {
            Node* current = table[i];
            while (current != nullptr) {
                Node* temp = current;
                current = current->next;
                delete temp;
            }
            table[i] = nullptr;
        }
        count = 0;
    }
};

class TLB {
protected:
    int size;
    UnorderedMap<unsigned int, unsigned int> entries;

public:
    TLB(int s) : size(s) {}
    virtual ~TLB() {}
    virtual bool access(unsigned int page_number, unsigned int frame_number) = 0;
    bool contains(unsigned int page_number) const { return entries.contains(page_number); }
    virtual void clear() { entries.clear(); }
};

class FIFO_TLB : public TLB {
private:
    Queue<unsigned int> order;

public:
    FIFO_TLB(int s) : TLB(s) {}

    bool access(unsigned int page_number, unsigned int frame_number) override {
        if (contains(page_number)) return true;
        if (entries.size() == size) {
            entries.erase(order.dequeue());
        }
        entries.insert(page_number, frame_number);
        order.enqueue(page_number);
        return false;
    }

    void clear() override {
        TLB::clear();
        while (!order.isEmpty()) {
            order.dequeue();
        }
    }
};

class LIFO_TLB : public TLB {
private:
    Vector<unsigned int> stack;

public:
    LIFO_TLB(int s) : TLB(s) {}

    bool access(unsigned int page_number, unsigned int frame_number) override {
        if (contains(page_number)) return true;
        if (entries.size() == size) {
            entries.erase(stack[stack.size() - 1]);
            stack.pop_back();
        }
        entries.insert(page_number, frame_number);
        stack.push_back(page_number);
        return false;
    }

    void clear() override {
        TLB::clear();
        stack.clear();
    }
};

class LRU_TLB : public TLB {
private:
    Vector<unsigned int> order;

public:
    LRU_TLB(int s) : TLB(s) {}

    bool access(unsigned int page_number, unsigned int frame_number) override {
        for (int i = 0; i < order.size(); i++) {
            if (order[i] == page_number) {
                for (int j = i; j < order.size() - 1; j++) {
                    order[j] = order[j + 1];
                }
                order[order.size() - 1] = page_number;
                return true;
            }
        }
        
        if (entries.size() == size) {
            entries.erase(order[0]);
            for (int i = 0; i < order.size() - 1; i++) {
                order[i] = order[i + 1];
            }
            order[order.size() - 1] = page_number;
        } else {
            order.push_back(page_number);
        }
        
        entries.insert(page_number, frame_number);
        return false;
    }

    void clear() override {
        TLB::clear();
        order.clear();
    }
};

class OPT_TLB : public TLB {
private:
    const Vector<unsigned int>& future_accesses;
    int current_index;

public:
    OPT_TLB(int s, const Vector<unsigned int>& fa) 
        : TLB(s), future_accesses(fa), current_index(0) {}

    bool access(unsigned int page_number, unsigned int frame_number) override {
        if (contains(page_number)) {
            current_index++;
            return true;
        }
        
        if (entries.size() == size) {
            entries.erase(find_victim());
        }
        
        entries.insert(page_number, frame_number);
        current_index++;
        return false;
    }

    unsigned int find_victim() {
        unsigned int victim = 0;
        int max_future = -1;
        
        for (int i = 0; i < future_accesses.size(); i++) {
            if (entries.contains(future_accesses[i])) {
                int future_index = -1;
                for (int j = current_index; j < future_accesses.size(); j++) {
                    if (future_accesses[j] == future_accesses[i]) {
                        future_index = j;
                        break;
                    }
                }
                if (future_index == -1 || future_index > max_future) {
                    max_future = future_index;
                    victim = future_accesses[i];
                }
            }
        }
        return victim;
    }

    void clear() override {
        TLB::clear();
        current_index = 0;
    }
};

void simulate(const Vector<unsigned int>& addresses, int address_space_size, int page_size, int tlb_size) {
    unsigned int p_bytes = page_size * 1024;
    
    FIFO_TLB fifo_tlb(tlb_size);
    LIFO_TLB lifo_tlb(tlb_size);
    LRU_TLB lru_tlb(tlb_size);
    OPT_TLB opt_tlb(tlb_size, addresses);
    
    Vector<int> hits(4, 0);
    
    for (int i = 0; i < addresses.size(); i++) {
        unsigned int page_number = addresses[i] / p_bytes;
        
        if (fifo_tlb.access(page_number, page_number)) hits[0]++;
        if (lifo_tlb.access(page_number, page_number)) hits[1]++;
        if (lru_tlb.access(page_number, page_number)) hits[2]++;
        if (opt_tlb.access(page_number, page_number)) hits[3]++;
    }
    
    cout << hits[0] << " " << hits[1] << " " << hits[2] << " " << hits[3] << endl;
}

int main() {
    int T;
    cin >> T;
    
    for (int t = 0; t < T; t++) {
        int address_space_size, page_size, tlb_size, N;
        cin >> address_space_size >> page_size >> tlb_size >> N;
        
        Vector<unsigned int> addresses;
        for (int i = 0; i < N; i++) {
            unsigned int address;
            cin >> hex >> address;
            addresses.push_back(address);
        }
        
        simulate(addresses, address_space_size, page_size, tlb_size);
    }
    
    return 0;
}