#include <iostream>
#include <vector>

using namespace std;

/**
 * 比特位图标记
 * 常用于大数据量的查找
 * 如海量数据(整数数据)中查找某个数, 由于内存有限,不可能将全部整数load into memory
 * 但是采用一个比特位来表示一个整数的话,即使40亿个int整数,也只不过需要大约500M的比特图
 */
class Bitmap {
public:
    Bitmap(size_t range) : _size(0) {
        //全部比特位初始化为0
        bitArray.resize((range >> 5) + 1);
    }

    /**
     * 添加data在bitmap中的标记
     * @data 待添加的数据
     * @return 是否添加成功
     */
    bool set(size_t data) {
        // data / 32, 映射到 vector的某个区块 index
        size_t index = data / 32;
        // data % 32, 确认 data 在区块index内部的偏移
        size_t offset = data % 32;
        // !(bitArray[index] & (1 << offset) 
        // 表示该比特位未使用
        if(!(bitArray[index] & (1 << offset))) {
            // 把该比特位置为1
            bitArray[index] |= (1 << offset);
            ++_size;
            return true;
        }
        return false;
    }

    /**
     * 重置data在bitmap中的标记
     * @param 移除data
     * @return 是否移除成功
     */
    bool reset(size_t data) {
        size_t index = data >> 5;
        size_t offset = data % 32;

        // bitArray[index] & (1 << offset)
        // 表示该比特位是 1， 即已经存在data
        if(bitArray[index] & (1 << offset)) {
            // 把该比特位置为 0 
            // ~(1 << offset) 取反, 得到offet位为0,
            // 其他位都是1
            // 然后 & 一下, 即对offset该比特位取反
            bitArray[index] &= (~(1 << offset));
            --_size;
            return true;
        }
        return false;
    }

    /**
     * 测验data是否已标记在bitmap中
     * @param 待测验的数据
     * @return 测验结果
     */
    bool find(size_t data) {
        size_t index = data >> 5;
        size_t offset = data % 32;
        return bitArray[index] & (1 << offset);
    }

    size_t size() {
        return _size;
    }

private:
    size_t _size;
    vector<size_t> bitArray;    ///< 以4字节为一块区块
};