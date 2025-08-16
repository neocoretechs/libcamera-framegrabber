#include <map>
#include <iostream>
#include <mutex>
#ifdef _WIN32
    // Windows-specific includes
    #include <windows.h>
#else
    // POSIX/Linux includes
    #include <sys/mman.h>
    #include <unistd.h>
#endif

class BufferMapper {
    private:
    struct MappingInfo {
        void *ptr;
        size_t length;
    };
    std::map<int, MappingInfo> mapped_;
    void logTrace(int fd, void *ptr, size_t length) {
        std::cout << "[BufferMapper] Mapped fd=" << fd
                  << " to ptr=" << ptr << " (" << length << " bytes)\n";
    }
public:
    BufferMapper(){}
    ~BufferMapper() {
        cleanup();
    }
    std::mutex mtx_;
    void* map(int fd, size_t length) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (mapped_.count(fd))
            return mapped_[fd].ptr;
#ifdef _WIN32
    std::cerr << "mmap not supported on Windows\n";
    return nullptr;
#else
    void* ptr = mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap failed");
    } else {
        mapped_[fd] = { ptr, length };
    }
    logTrace(fd, ptr, length);
    return (ptr == MAP_FAILED) ? nullptr : ptr;
#endif
    }

    void* get(int fd) {
        std::unique_lock<std::mutex> lock(mtx_);
        auto it = mapped_.find(fd);
        return it != mapped_.end() ? it->second.ptr : nullptr;
    }

    void cleanup() {
#ifdef _WIN32
    std::cerr << "mmap not supported on Windows\n";

#else
        std::unique_lock<std::mutex> lock(mtx_);
        for (const auto &[fd, info] : mapped_) {
            munmap(info.ptr, info.length);
        }
        mapped_.clear();
#endif
    }


};