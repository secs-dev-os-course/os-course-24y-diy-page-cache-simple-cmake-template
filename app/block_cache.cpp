#include "../include/block_cache.hpp"
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

using namespace std;

BlockCache::BlockCache(size_t block_size, size_t max_cache_size)
    : block_size_(block_size), max_cache_size_(max_cache_size) {}

int BlockCache::open(const char *path)
{
    int fd = ::open(path, O_RDWR);
    if (fd == -1)
    {
        return -1;
    }
    // remove default cache on MacOS
    if (fcntl(fd, F_NOCACHE, 1) == -1)
    {
        ::close(fd);
        return -1;
    }
    file_descriptors_.emplace(fd);
    fd_offsets_[fd] = 0;
    return fd;
}

int BlockCache::close(int fd)
{
    if (file_descriptors_.find(fd) == file_descriptors_.end())
    {
        cerr << "Invalid file descriptor!\n";
        return -1;
    }
    if (fsync(fd) == -1)
    {
        cerr << "Error during fsync for file descriptor " << fd << ": " << strerror(errno) << endl;
        return -1;
    }
    file_descriptors_.erase(fd);
    fd_offsets_.erase(fd);
    if (::close(fd) == -1)
    {
        cerr << "Error closing file descriptor " << fd << ": " << strerror(errno) << endl;
        return -1;
    }
    return 0;
}

ssize_t BlockCache::read(int fd, void* buf, size_t count) {
    if (file_descriptors_.find(fd) == file_descriptors_.end()) {
        std::cerr << "Invalid file descriptor!\n";
        return -1;
    }
    off_t offset = fd_offsets_[fd];
    ssize_t bytes_read = 0;
    while (count > 0) {
        off_t block_offset = offset / block_size_;
        off_t block_start = block_offset * block_size_;
        off_t block_end = block_start + block_size_;
        auto it = cache_.find(block_start);
        evictPage();
        if (it == cache_.end()) {
            cache_[block_start].offset = block_start;
            cache_[block_start].data.resize(block_size_);
            cache_[block_start].modified = false;
            ssize_t bytes_read = ::pread(fd, cache_[block_start].data.data(), block_size_, block_start);
            if (bytes_read == -1) {
                std::cerr << "Error reading from file\n";
                return -1;
            }
            // no need to update cache if empty
            if (bytes_read == 0){
                return bytes_read;
            }
        }
        touchPage(block_start);
        off_t block_offset_end = (offset + count - 1) / block_size_;
        off_t block_start_end = block_offset_end * block_size_;
        off_t block_end_end = block_start_end + block_size_;
        off_t bytes_to_read = min(block_end_end - offset, (off_t) count);
        memcpy(buf, cache_[block_start].data.data() + offset - block_start, bytes_to_read);
        buf = static_cast<char*>(buf) + bytes_to_read;
        count -= bytes_to_read;
        offset += bytes_to_read;
        bytes_read += bytes_to_read;
    }
    fd_offsets_[fd] = offset;
    return bytes_read;
}

ssize_t BlockCache::write(int fd, const void *buf, size_t count)
{
    if (file_descriptors_.find(fd) == file_descriptors_.end())
    {
        cerr << "Invalid file descriptor!\n";
        return -1;
    }
    off_t offset = fd_offsets_[fd];
    ssize_t bytes_written = 0;
    while (count > 0)
    {
        off_t block_offset = offset / block_size_;
        off_t block_start = block_offset * block_size_;
        auto it = cache_.find(block_start);
        evictPage();
        if (it == cache_.end())
        {
            cache_[block_start].fd = fd;
            cache_[block_start].offset = block_start;
            cache_[block_start].data.resize(block_size_);
            cache_[block_start].modified = false;
            if (lseek(fd, block_start, SEEK_SET) == -1)
            {
                return -1;
            }
            ssize_t bytes_read = ::read(fd, cache_[block_start].data.data(), block_size_);
            if (bytes_read == -1)
            {
                return -1;
            }
        }
        touchPage(block_start);
        off_t block_offset_end = (offset + count - 1) / block_size_;
        off_t block_start_end = block_offset_end * block_size_;
        off_t block_end_end = block_start_end + block_size_;
        off_t bytes_to_write = min(block_end_end - offset, (off_t)count);
        memcpy(cache_[block_start].data.data() + offset - block_start, buf, bytes_to_write);
        cache_[block_start].modified = true;
        buf = static_cast<const char *>(buf) + bytes_to_write;
        count -= bytes_to_write;
        offset += bytes_to_write;
        bytes_written += bytes_to_write;
    }
    fd_offsets_[fd] = offset;
    return bytes_written;
    return 1;
}

off_t BlockCache::lseek(int fd, off_t offset, int whence)
{
    if (file_descriptors_.find(fd) == file_descriptors_.end())
    {
        cerr << "Invalid file descriptor!\n";
        return -1;
    }
    off_t new_offset = ::lseek(fd, offset, whence);
    if (new_offset == -1)
    {
        return -1;
    }
    fd_offsets_[fd] = new_offset;
    return new_offset;
}

int BlockCache::fsync(int fd)
{
    if (file_descriptors_.find(fd) == file_descriptors_.end())
    {
        cerr << "Invalid file descriptor!\n";
        return -1;
    }
    for (auto &entry : cache_)
    {
        CachePage &page = entry.second;
        if (page.modified)
        {
            if (::pwrite(fd, page.data.data(), block_size_, page.offset) == -1)
            {
                cerr << "Error writing back modified page during fsync\n";
                return -1;
            }
            page.modified = false;
        }
    }
    return 0;
}

void BlockCache::touchPage(off_t offset)
{
    if (lru_map_.find(offset) != lru_map_.end())
    {
        lru_list_.erase(lru_map_[offset]);
    }
    lru_list_.push_front(offset);
    lru_map_[offset] = lru_list_.begin();
}

void BlockCache::evictPage()
{
    if (cache_.size() <= max_cache_size_)
    {
        return;
    }

    off_t lru_offset = lru_list_.back();
    lru_list_.pop_back();
    lru_map_.erase(lru_offset);

    CachePage &page = cache_[lru_offset];
    if (page.modified)
    {
        if (lseek(page.fd, page.offset, SEEK_SET) == -1)
        {
            cerr << "Error seeking to offset for write-back\n";
            return;
        }
        if (::write(page.fd, page.data.data(), block_size_) == -1)
        {
            cerr << "Error writing back modified page\n";
            return;
        }
    }

    cache_.erase(lru_offset);
}