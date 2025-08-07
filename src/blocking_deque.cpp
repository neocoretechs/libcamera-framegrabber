#include <deque>
#include <mutex>
#include <condition_variable>
#include <vector>

class BlockingDeque {
public:
    BlockingDeque(size_t capacity) : capacity_(capacity) {}

    void push(std::vector<uint8_t> item) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_not_full_.wait(lock, [&] { return queue_.size() < capacity_; });
        queue_.push_back(item);
        cv_not_empty_.notify_one();
    }

    std::vector<uint8_t> pop() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_not_empty_.wait(lock, [&] { return !queue_.empty(); });
        std::vector<uint8_t> item = queue_.front();
        queue_.pop_front();
        cv_not_full_.notify_one();
        return item;
    }
    
    bool empty() {
        return queue_.empty();
    }
private:
    std::deque<std::vector<uint8_t>> queue_;
    size_t capacity_;
    std::mutex mtx_;
    std::condition_variable cv_not_empty_;
    std::condition_variable cv_not_full_;
};