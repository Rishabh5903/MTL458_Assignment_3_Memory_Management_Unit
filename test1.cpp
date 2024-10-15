#include <iostream>
#include <unordered_map>
#include <climits>
#include <limits>
using namespace std;

// Custom implementation of singly linked list node for FIFO
struct Node {
    unsigned int page;
    Node* next;
    Node(unsigned int p) : page(p), next(nullptr) {}
};

// Custom implementation of doubly linked list node for LRU and LIFO
struct DoubleNode {
    unsigned int page;
    DoubleNode *prev, *next;
    DoubleNode(unsigned int p) : page(p), prev(nullptr), next(nullptr) {}
};

// Queue implementation for tracking future page accesses in OPT algorithm
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
    int front_element() { return (front != -1) ? arr[front] : -1; }
};

// Max heap implementation for OPT algorithm to track future page references
class MaxHeap {
    pair<int, unsigned int>* arr;  // pair of {next_access_time, page_number}
    int size, capacity;

    void heapify(int i) {
        int largest = i;
        int left = 2 * i + 1;
        int right = 2 * i + 2;

        if (left < size && arr[left].first > arr[largest].first)
            largest = left;
        if (right < size && arr[right].first > arr[largest].first)
            largest = right;

        if (largest != i) {
            swap(arr[i], arr[largest]);
            heapify(largest);
        }
    }

public:
    MaxHeap(int cap) : size(0), capacity(cap) {
        arr = new pair<int, unsigned int>[cap];
    }
    ~MaxHeap() { delete[] arr; }

    void push(pair<int, unsigned int> x) {
        if (size == capacity) return;
        
        int i = size;
        arr[size++] = x;

        // Maintain heap property
        while (i > 0 && arr[(i - 1) / 2].first < arr[i].first) {
            swap(arr[i], arr[(i - 1) / 2]);
            i = (i - 1) / 2;
        }
    }

    pair<int, unsigned int> pop() {
        if (size <= 0) return {-1, 0};
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

    bool empty() const { return size == 0; }

    pair<int, unsigned int> top() const {
        if (size > 0) return arr[0];
        return {-1, 0};
    }
};

// Base TLB class with common functionality
class TLB {
protected:
    int capacity;
    int size;
    unsigned int page_size_kb;

    // Simplified VPN calculation
    unsigned int getVPN(unsigned int address) {
        return address / (1024 * page_size_kb);
    }


public:
    TLB(int cap, int page_size_kb) : capacity(cap), size(0), page_size_kb(page_size_kb) {}
    virtual ~TLB() {}
    virtual bool access(unsigned int address) = 0;
};

// First-In-First-Out (FIFO) TLB implementation
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

    // Returns true if page is in TLB (hit), false otherwise (miss)
    bool access(unsigned int address) override {
        unsigned int vpn = getVPN(address);
        if (page_map.find(vpn) != page_map.end()) return true;

        // If TLB is full, remove oldest page (from head)
        if (size == capacity) {
            Node* temp = head;
            head = head->next;
            page_map.erase(temp->page);
            delete temp;
            size--;
        }

        // Add new page to tail
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

// Last-In-First-Out (LIFO) TLB implementation
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
        if (page_map.find(vpn) != page_map.end()) return true;

        // If TLB is full, remove most recently added page
        if (size == capacity) {
            DoubleNode* temp = head;
            head = head->next;
            if (head) head->prev = nullptr;
            page_map.erase(temp->page);
            delete temp;
            size--;
        }

        // Add new page to head
        DoubleNode* new_node = new DoubleNode(vpn);
        new_node->next = head;
        if (head) head->prev = new_node;
        head = new_node;
        page_map[vpn] = new_node;
        size++;
        return false;
    }
};

// Least Recently Used (LRU) TLB implementation
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
            // Move accessed page to front (most recently used)
            DoubleNode* node = page_map[vpn];
            if (node != head) {
                if (node->prev) node->prev->next = node->next;
                if (node->next) node->next->prev = node->prev;
                if (node == tail) tail = node->prev;
                node->next = head;
                node->prev = nullptr;
                head->prev = node;
                head = node;
            }
            return true;
        }

        // If TLB is full, remove least recently used page (from tail)
        if (size == capacity) {
            DoubleNode* temp = tail;
            tail = tail->prev;
            if (tail) tail->next = nullptr;
            else head = nullptr;
            page_map.erase(temp->page);
            delete temp;
            size--;
        }

        // Add new page to head
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

// Optimal (OPT) TLB implementation
class OPT : public TLB {
private:
    unordered_map<unsigned int, Queue*> future_map;
    MaxHeap* heap;
    unordered_map<unsigned int, bool> page_map;
    int current_index;
    int sequence_size;

public:
    OPT(int cap, int page_size, unsigned int* sequence, int seq_size) 
        : TLB(cap, page_size), current_index(0), sequence_size(seq_size) {
        heap = new MaxHeap(seq_size);
        
        // Initialize future access information for each page
        for (int i = 0; i < seq_size; i++) {
            unsigned int vpn = getVPN(sequence[i]);
            if (future_map.find(vpn) == future_map.end()) {
                future_map[vpn] = new Queue(seq_size);
            }
            future_map[vpn]->push(i);
        }
    }

    ~OPT() {
        delete heap;
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

        if (page_map.size() == capacity) {
            // The top of heap will always be a valid page to evict
            pair<int, unsigned int> top = heap->pop();
            page_map.erase(top.second);
        }

        page_map[vpn] = true;
        updatePage(vpn);
        current_index++;
        return false;
    }

private:
    // Update future access information for a page
    void updatePage(unsigned int vpn) {
        Queue* future_queue = future_map[vpn];
        
        while (!future_queue->empty() && future_queue->front_element() <= current_index) {
            future_queue->pop();
        }

        int next_access = future_queue->empty() ? INT_MAX : future_queue->front_element();
        heap->push({next_access, vpn});
    }
};

// Main simulation function to test different TLB replacement policies
void simulate(unsigned int* addresses, int N, int address_space_size, int page_size, int tlb_size, int* hits) {
    FIFO fifo_tlb(tlb_size, page_size);
    LIFO lifo_tlb(tlb_size, page_size);
    LRU lru_tlb(tlb_size, page_size);
    OPT opt_tlb(tlb_size, page_size, addresses, N);
    
    for (int i = 0; i < 4; i++) hits[i] = 0;  // Initialize hits for each algorithm
    
    // Process each address through all TLB implementations
    for (int i = 0; i < N; i++) {
        if (fifo_tlb.access(addresses[i])) hits[0]++;
        if (lifo_tlb.access(addresses[i])) hits[1]++;
        if (lru_tlb.access(addresses[i])) hits[2]++;
        if (opt_tlb.access(addresses[i])) hits[3]++;
    }
}

#include <fstream>
#include <chrono>
#include <iomanip>

using namespace std::chrono;

int main() {
    ifstream input_file("input.txt");
    if (!input_file.is_open()) {
        cerr << "Error opening input file" << endl;
        return 1;
    }

    int T;
    input_file >> T;

    for (int t = 0; t < T; t++) {
        int address_space_size, page_size, tlb_size, N;
        input_file >> address_space_size >> page_size >> tlb_size >> N;

        unsigned int* addresses = new unsigned int[N];
        for (int i = 0; i < N; i++) {
            input_file >> hex >> addresses[i];
        }
        input_file >> dec;

        int hits[4] = {0};

        // Measure time for FIFO
        auto start = high_resolution_clock::now();
        FIFO fifo_tlb(tlb_size, page_size);
        for (int i = 0; i < N; i++) {
            if (fifo_tlb.access(addresses[i])) hits[0]++;
        }
        auto stop = high_resolution_clock::now();
        auto duration_fifo = duration_cast<microseconds>(stop - start);

        // Measure time for LIFO
        start = high_resolution_clock::now();
        LIFO lifo_tlb(tlb_size, page_size);
        for (int i = 0; i < N; i++) {
            if (lifo_tlb.access(addresses[i])) hits[1]++;
        }
        stop = high_resolution_clock::now();
        auto duration_lifo = duration_cast<microseconds>(stop - start);

        // Measure time for LRU
        start = high_resolution_clock::now();
        LRU lru_tlb(tlb_size, page_size);
        for (int i = 0; i < N; i++) {
            if (lru_tlb.access(addresses[i])) hits[2]++;
        }
        stop = high_resolution_clock::now();
        auto duration_lru = duration_cast<microseconds>(stop - start);

        // Measure time for OPT
        start = high_resolution_clock::now();
        OPT opt_tlb(tlb_size, page_size, addresses, N);
        for (int i = 0; i < N; i++) {
            if (opt_tlb.access(addresses[i])) hits[3]++;
        }
        stop = high_resolution_clock::now();
        auto duration_opt = duration_cast<microseconds>(stop - start);

        // Print results
        cout << "Test case " << t + 1 << ":" << endl;
        cout << "FIFO: " << hits[0] << " hits, Time: " << duration_fifo.count() << " microseconds" << endl;
        cout << "LIFO: " << hits[1] << " hits, Time: " << duration_lifo.count() << " microseconds" << endl;
        cout << "LRU:  " << hits[2] << " hits, Time: " << duration_lru.count() << " microseconds" << endl;
        cout << "OPT:  " << hits[3] << " hits, Time: " << duration_opt.count() << " microseconds" << endl;
        cout << endl;

        delete[] addresses;
    }

    input_file.close();
    return 0;
}