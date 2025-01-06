#include <gtest/gtest.h>

#include <cstring>
#include <fstream>

#include "FilesInterface.h"

#define CACHE_SIZE 256 * 1024 * 1024  // Bytes

class FilesManagerTest : public ::testing::Test {
   protected:
    std::string test_file = "test_file.txt";
};

TEST_F(FilesManagerTest, OpenFileSuccess) {
    FilesManager fm(CACHE_SIZE);
    int fd = fm.f_open(test_file.c_str());
    EXPECT_GE(fd, 0);
    fm.f_close(fd);
}

// TEST_F(FilesManagerTest, ReadFile) {
//     FilesManager fm(1024);
//     int fd = fm.f_open(test_file.c_str());

//     char buffer[32] = {0};
//     ssize_t bytes_read = fm.f_read(fd, buffer, sizeof(buffer) - 1);

//     EXPECT_GT(bytes_read, 0);
//     EXPECT_STREQ(buffer, "Hello, FilesManager!");
//     fm.f_close(fd);
// }

// TEST_F(FilesManagerTest, WriteFile) {
//     FilesManager fm(1024);
//     int fd = fm.f_open(test_file.c_str());

//     const char* data = "New data!";
//     ssize_t bytes_written = fm.f_write(fd, data, strlen(data));
//     EXPECT_EQ(bytes_written, strlen(data));

//     // Перемотка и проверка записи
//     fm.f_lseek(fd, 0, SEEK_SET);
//     char buffer[32] = {0};
//     fm.f_read(fd, buffer, sizeof(buffer) - 1);

//     EXPECT_STREQ(buffer, "New data!esManager!");
//     fm.f_close(fd);
// }

// TEST_F(FilesManagerTest, SeekFile) {
//     FilesManager fm(1024);
//     int fd = fm.f_open(test_file.c_str());

//     off_t new_offset = fm.f_lseek(fd, 7, SEEK_SET);
//     EXPECT_EQ(new_offset, 7);

//     char buffer[16] = {0};
//     ssize_t bytes_read = fm.f_read(fd, buffer, sizeof(buffer) - 1);

//     EXPECT_STREQ(buffer, "FilesManager!");
//     fm.f_close(fd);
// }

// TEST_F(FilesManagerTest, SyncFile) {
//     FilesManager fm(1024);
//     int fd = fm.f_open(test_file.c_str());

//     const char* data = "New sync data!";
//     fm.f_write(fd, data, strlen(data));

//     int sync_result = fm.fsync(fd);
//     EXPECT_EQ(sync_result, 0);

//     fm.f_close(fd);
// }

// TEST_F(FilesManagerTest, CloseFile) {
//     FilesManager fm(1024);
//     int fd = fm.f_open(test_file.c_str());
//     int close_result = fm.f_close(fd);

//     EXPECT_EQ(close_result, 0);
// }

// TEST_F(FilesManagerTest, OpenNonExistentFile) {
//     FilesManager fm(1024);
//     EXPECT_THROW(fm.f_open("non_existent_file.txt"), std::runtime_error);
// }
