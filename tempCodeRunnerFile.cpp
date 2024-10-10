#include <iostream>
#include <vector>
using namespace std;

vector<int> chooseFleets(const vector<int>& wheels) {
    // Result array to store the number of ways for each element in wheels
    vector<int> result;
    
    // Loop through each number of wheels
    for (int w : wheels) {
        int count = 0;
        
        // Try combinations of 2-wheeled and 4-wheeled vehicles
        for (int twoWheeler = 0; twoWheeler * 2 <= w; ++twoWheeler) {
            int remainingWheels = w - twoWheeler * 2;
            // Check if the remaining wheels can be filled with 4-wheeled vehicles
            if (remainingWheels % 4 == 0) {
                count++;
            }
        }

        // Store the result for this particular wheel count
        result.push_back(count);
    }

    return result;
}

int main() {
    int n;
    cin >> n; // Read the number of elements in wheels
    vector<int> wheels(n);

    // Read the number of wheels
    for (int i = 0; i < n; i++) {
        cin >> wheels[i];
    }

    // Get the number of ways to choose fleets for each wheel count
    vector<int> result = chooseFleets(wheels);

    // Print the results
    for (int res : result) {
        cout << res << endl;
    }

    return 0;
}
