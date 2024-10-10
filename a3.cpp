#include <iostream>
#include <unordered_map>
#include <climits>
#include <limits>
using namespace std;

// Custom implementation of singly linked list node
struct Node {
    unsigned int page;
    Node* next;
    Node(unsigned int p) : page(p), next(nullptr) {}
};

// Custom implementation of doubly linked list node
struct DoubleNode {
    unsigned int page;
    DoubleNode *prev, *next;
    DoubleNode(unsigned int p) : page(p), prev(nullptr), next(nullptr) {}
};

// Custom implementation of queue for future accesses in OPT
class Queue {
    int* arr;
    int front, rear, capacity;
public:
    Queue(int size) : front(-1), rear(-1), capacity(size) {
        arr = new int[size];
    }
    ~Queue() { delete[] arr; }
    void push(int x) {
        if (rear == capacity - 1) return;
        if (front == -1) front = 0;
        arr[++rear] = x;
    }
    int pop() {
        if (front == -1) return -1;
        int x = arr[front];
        if (front == rear) front = rear = -1;
        else front++;
        return x;
    }
    bool empty() { return front == -1; }
};

// Custom implementation of min heap for OPT
class MinHeap {
    pair<int, unsigned int>* arr;
    int size, capacity;

    void heapify(int i) {
        int smallest = i;
        int left = 2 * i + 1;
        int right = 2 * i + 2;

        if (left < size && arr[left].first < arr[smallest].first)
            smallest = left;
        if (right < size && arr[right].first < arr[smallest].first)
            smallest = right;

        if (smallest != i) {
            swap(arr[i], arr[smallest]);
            heapify(smallest);
        }
    }

public:
    MinHeap(int cap) : size(0), capacity(cap) {
        arr = new pair<int, unsigned int>[cap];
    }
    ~MinHeap() { delete[] arr; }

    void push(pair<int, unsigned int> x) {
        if (size == capacity) return;
        int i = size;
        arr[size++] = x;

        while (i > 0 && arr[(i - 1) / 2].first > arr[i].first) {
            swap(arr[i], arr[(i - 1) / 2]);
            i = (i - 1) / 2;
        }
    }

    pair<int, unsigned int> pop() {
        if (size <= 0) return {INT_MAX, 0};
        if (size == 1) {
            size--;
            return arr[0];
        }

        pair<int, unsigned int> root = arr[0];
        arr[0] = arr[size - 1];
        size--;
        heapify(0);

        return root;
    }

    void update(unsigned int page, int new_time) {
        for (int i = 0; i < size; i++) {
            if (arr[i].second == page) {
                arr[i].first = new_time;
                while (i > 0 && arr[(i - 1) / 2].first > arr[i].first) {
                    swap(arr[i], arr[(i - 1) / 2]);
                    i = (i - 1) / 2;
                }
                heapify(i);
                break;
            }
        }
    }
};

class TLB {
protected:
    int capacity;
    int size;
    unsigned int page_size_bits;

    unsigned int getVPN(unsigned int address) {
        return address >> page_size_bits;
    }

public:
    TLB(int cap, int page_size) : capacity(cap), size(0) {
        page_size_bits = 0;
        while (page_size > 1) {
            page_size >>= 1;
            page_size_bits++;
        }
    }
    virtual ~TLB() {}
    virtual bool access(unsigned int address) = 0;
};

class FIFO : public TLB {
private:
    Node *head, *tail;
    unordered_map<unsigned int, Node*> page_map;

public:
    FIFO(int cap, int page_size) : TLB(cap, page_size), head(nullptr), tail(nullptr) {}

    ~FIFO() {
        while (head) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
    }

    bool access(unsigned int address) override {
        unsigned int vpn = getVPN(address);
        if (page_map.find(vpn) != page_map.end()) return true;

        if (size == capacity) {
            Node* temp = head;
            head = head->next;
            page_map.erase(temp->page);
            delete temp;
            size--;
        }

        Node* new_node = new Node(vpn);
        if (!head) head = tail = new_node;
        else {
            tail->next = new_node;
            tail = new_node;
        }
        page_map[vpn] = new_node;
        size++;
        return false;
    }
};

class LIFO : public TLB {
private:
    DoubleNode *head;
    unordered_map<unsigned int, DoubleNode*> page_map;

public:
    LIFO(int cap, int page_size) : TLB(cap, page_size), head(nullptr) {}

    ~LIFO() {
        while (head) {
            DoubleNode* temp = head;
            head = head->next;
            delete temp;
        }
    }

    bool access(unsigned int address) override {
        unsigned int vpn = getVPN(address);
        if (page_map.find(vpn) != page_map.end()) {
            DoubleNode* node = page_map[vpn];
            if (node != head) {
                // Remove node from its current position
                if (node->prev) node->prev->next = node->next;
                if (node->next) node->next->prev = node->prev;
                // Move node to head
                node->next = head;
                node->prev = nullptr;
                head->prev = node;
                head = node;
            }
            return true;
        }

        if (size == capacity) {
            DoubleNode* temp = head;
            head = head->next;
            if (head) head->prev = nullptr;
            page_map.erase(temp->page);
            delete temp;
            size--;
        }

        DoubleNode* new_node = new DoubleNode(vpn);
        new_node->next = head;
        if (head) head->prev = new_node;
        head = new_node;
        page_map[vpn] = new_node;
        size++;
        return false;
    }
};

class LRU : public TLB {
private:
    DoubleNode *head, *tail;
    unordered_map<unsigned int, DoubleNode*> page_map;

public:
    LRU(int cap, int page_size) : TLB(cap, page_size), head(nullptr), tail(nullptr) {}

    ~LRU() {
        while (head) {
            DoubleNode* temp = head;
            head = head->next;
            delete temp;
        }
    }

    bool access(unsigned int address) override {
        unsigned int vpn = getVPN(address);
        if (page_map.find(vpn) != page_map.end()) {
            DoubleNode* node = page_map[vpn];
            if (node != head) {
                // Remove node from its current position
                if (node->prev) node->prev->next = node->next;
                if (node->next) node->next->prev = node->prev;
                if (node == tail) tail = node->prev;
                // Move node to head
                node->next = head;
                node->prev = nullptr;
                head->prev = node;
                head = node;
            }
            return true;
        }

        if (size == capacity) {
            DoubleNode* temp = tail;
            tail = tail->prev;
            if (tail) tail->next = nullptr;
            else head = nullptr;
            page_map.erase(temp->page);
            delete temp;
            size--;
        }

        DoubleNode* new_node = new DoubleNode(vpn);
        new_node->next = head;
        if (head) head->prev = new_node;
        head = new_node;
        if (!tail) tail = new_node;
        page_map[vpn] = new_node;
        size++;
        return false;
    }
};

class OPT : public TLB {
private:
    unordered_map<unsigned int, Queue*> future_map;
    MinHeap* heap;
    int* access_sequence;
    int sequence_size;
    int current_index;
    unordered_map<unsigned int, bool> page_map;

public:
    OPT(int cap, int page_size, unsigned int* sequence, int seq_size) 
        : TLB(cap, page_size), sequence_size(seq_size), current_index(0) {
        heap = new MinHeap(cap);
        access_sequence = new int[seq_size];
        for (int i = 0; i < seq_size; i++) {
            access_sequence[i] = sequence[i];
        }

        for (int i = 0; i < seq_size; i++) {
            unsigned int vpn = getVPN(access_sequence[i]);
            if (future_map.find(vpn) == future_map.end()) {
                future_map[vpn] = new Queue(seq_size);
            }
            future_map[vpn]->push(i);
        }
    }

    ~OPT() {
        delete heap;
        delete[] access_sequence;
        for (auto& pair : future_map) {
            delete pair.second;
        }
    }

    bool access(unsigned int address) override {
        unsigned int vpn = getVPN(address);
        if (page_map.find(vpn) != page_map.end()) {
            updatePage(vpn);
            current_index++;
            return true;
        }

        if (size == capacity) {
            auto victim = heap->pop();
            page_map.erase(victim.second);
            size--;
        }

        page_map[vpn] = true;
        size++;
        updatePage(vpn);
        current_index++;
        return false;
    }

private:
    void updatePage(unsigned int vpn) {
        future_map[vpn]->pop();
        int next_access = future_map[vpn]->empty() ? INT_MAX : future_map[vpn]->pop();
        heap->push({next_access, vpn});
    }
};

void simulate(unsigned int* addresses, int N, int address_space_size, int page_size, int tlb_size) {
    unsigned int p_bytes = page_size * 1024;
    
    FIFO fifo_tlb(tlb_size, page_size);
    LIFO lifo_tlb(tlb_size, page_size);
    LRU lru_tlb(tlb_size, page_size);
    OPT opt_tlb(tlb_size, page_size, addresses, N);
    
    int hits[4] = {0};
    
    for (int i = 0; i < N; i++) {
        if (fifo_tlb.access(addresses[i])) hits[0]++;
        if (lifo_tlb.access(addresses[i])) hits[1]++;
        if (lru_tlb.access(addresses[i])) hits[2]++;
        if (opt_tlb.access(addresses[i])) hits[3]++;
    }
    
    cout << hits[0] << " " << hits[1] << " " << hits[2] << " " << hits[3] << endl;
}

int main() {
    int T;
    cin >> T;
    
    while (T--) {
        int address_space_size_mb, page_size_kb, tlb_size, N;
        cin >> address_space_size_mb >> page_size_kb >> tlb_size >> N;
        
        // Convert address space size from MB to bytes
        unsigned long long address_space_size = static_cast<unsigned long long>(address_space_size_mb) * 1024 * 1024;
        
        // Dynamically allocate memory for addresses
        unsigned int* addresses = new unsigned int[N];  // Change to unsigned int for addresses
        for (int i = 0; i < N; i++) {
            cin >> hex >> addresses[i]; // Read hexadecimal input directly into addresses
        }
        
        // Call the simulate function
        simulate(addresses, N, address_space_size, page_size_kb, tlb_size);
        
        // Free allocated memory
        delete[] addresses;
        
        // Reset input mode to decimal for further input
        cin >> dec; // Ensure further inputs are treated as decimal
    }
    
    return 0;
}