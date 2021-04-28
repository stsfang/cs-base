#include <iostream>
#include <vector>
#include <string>
#include <limits>

using namespace std;

class Bitmap {
public:
    Bitmap(size_t range) : _size(0) {  
        _bitmap1.resize((range >> 5) + 1);
        _bitmap2.resize((range >> 5) + 1);
    }

    bool set(size_t data) {
        size_t index = data / 32;
        size_t offset = data % 32;

        if(!(_bitmap1[index] & (1 << offset))) {
            _bitmap1[index] |= (1 << offset);
            ++_size;
            return true;
        }
        else if((_bitmap1[index] & (1 << offset)) && !( _bitmap2[index] & (1 << offset)) ) {
            _bitmap2[index] |= (1 << offset);
            return true;
        }
        else if((_bitmap1[index] & (1 << offset)) && (_bitmap2[index] & (1 << offset))) {
            return true;
        } 
        return false;
    }

    bool find(size_t data) {
        size_t index = data >> 5;
        size_t offset = data % 32;
        return _bitmap1[index] & (1 << offset);
    }

    bool duplicate(size_t data) {
        size_t index = data >> 5;
        size_t offset = data % 32;
        return (_bitmap1[index] & (1 << offset)) && (_bitmap2[index] & (1 << offset));
    }

    size_t size() {
        return _size;
    }

private:
    size_t _size;               ///< non-duplicated
    vector<size_t> _bitmap1;    ///< 以4字节为一块区块
    vector<size_t> _bitmap2;
};

vector<int> readBatchFromFile(string&& filename, int batch_size) {
    return {};
}

int main() {
    int batch_size = 100000;
    int total = 10000000000;
    int maxNum = numeric_limits<int>::min();
    Bitmap bmUtil(total);
    
    {
        //while read file != EOF
        vector<int> nums(readBatchFromFile(string("bigdata_file.txt"), batch_size));
        for(auto& data : nums) {
            maxNum = max(maxNum, data);
            bmUtil.set(data);
        }
        //end while
    }

    cout << "non-duplicated: " << endl;
    for(int i = 0; i <= maxNum; i++) {
        if(!bmUtil.duplicate(i)) {
            cout << i << " ";
        }
    }
    cout << endl;

    return 0;
}