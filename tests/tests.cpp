#include <gtest/gtest.h>

#include <cstring>
#include <fstream>

#include "FilesInterface.h"

#define CACHE_SIZE 256 * 1024 * 1024  // Bytes

class FilesManagerTest : public ::testing::Test {
   protected:
    void SetUp() override {
        std::ofstream outfile("test.txt");
        if (!outfile) {
            std::cerr << "Ошибка при открытии файла для записи!" << std::endl;
            FAIL();
        }
        outfile.close();
    }

    void TearDown() override {
        std::remove("test.txt");
    }
};

TEST_F(FilesManagerTest, OpenFileSuccess) {
    FilesManager fm(CACHE_SIZE);
    int fd = fm.f_open("test.txt");
    EXPECT_GE(fd, 0);
    fm.f_close(fd);
}

TEST_F(FilesManagerTest, TestRead) {
    FilesManager fm(CACHE_SIZE);

    std::ofstream outFile;
    outFile.open("test.txt");
    outFile << "Hello, world!";
    outFile.close();

    int fd = fm.f_open("test.txt");

    char buffer[32] = {0};
    ssize_t bytes_read = fm.f_read(fd, buffer, sizeof(buffer) - 1);

    EXPECT_GT(bytes_read, 0);
    EXPECT_STREQ(buffer, "Hello, world!");
    fm.f_close(fd);
}

TEST_F(FilesManagerTest, TestLongRead) {
    FilesManager fm(CACHE_SIZE);

    std::ofstream outFile;
    outFile.open("test.txt");

    std::string first_page(4096, 'a');
    std::string second_page(4096, 'b');
    std::string third_page(4096, 'c');

    outFile << first_page;
    outFile << second_page;
    outFile << third_page;
    outFile.close();

    int fd = fm.f_open("test.txt");

    char buffer[4096 * 3] = {0};
    ssize_t bytes_read = fm.f_read(fd, buffer, sizeof(buffer));

    EXPECT_GE(bytes_read, 4096 * 3);
    std::string res = first_page + second_page + third_page;
    EXPECT_STREQ(buffer, res.c_str());
    fm.f_close(fd);
}

TEST_F(FilesManagerTest, TestLongReadWithRest) {
    FilesManager fm(CACHE_SIZE);

    std::ofstream outFile;
    outFile.open("test.txt");

    std::string first_page(4096, 'a');
    std::string second_page(4096, 'b');
    std::string third_page(4096, 'c');
    std::string fourth_page(50, 'd');

    outFile << first_page;
    outFile << second_page;
    outFile << third_page;
    outFile << fourth_page;
    outFile.close();

    int fd = fm.f_open("test.txt");

    char buffer[4096 * 3 + 50] = {0};
    ssize_t bytes_read = fm.f_read(fd, buffer, sizeof(buffer));

    EXPECT_GE(bytes_read, 4096 * 3 + 50);
    std::string res = first_page + second_page + third_page + fourth_page;
    EXPECT_EQ(strncmp(buffer, res.c_str(), 4096 * 3 + 50), 0);
    fm.f_close(fd);
}

TEST_F(FilesManagerTest, TestWrite) {
    FilesManager fm(CACHE_SIZE);
    int fd = fm.f_open("test.txt");

    const char* data = "New data!";
    ssize_t bytes_written = fm.f_write(fd, data, strlen(data));
    EXPECT_GE(bytes_written, strlen(data));

    fm.f_lseek(fd, 0, SEEK_SET);
    char buffer[32] = {0};
    fm.f_read(fd, buffer, sizeof(buffer));

    EXPECT_STREQ(buffer, "New data!");
    fm.f_close(fd);
}

TEST_F(FilesManagerTest, TestLongWrite) {
    FilesManager fm(CACHE_SIZE);
    int fd = fm.f_open("test.txt");

    std::string first_page(4096, 'a');
    std::string second_page(4096, 'b');
    std::string third_page(4096, 'c');

    std::string data = first_page + second_page + third_page;
    char buff[4096 * 3 + 1];

    std::strncpy(buff, data.c_str(), sizeof(buff));
    buff[sizeof(buff) - 1] = '\0';

    ssize_t bytes_written = fm.f_write(fd, buff, strlen(buff));
    EXPECT_GE(bytes_written, strlen(buff));

    fm.f_lseek(fd, 0, SEEK_SET);
    char buffer[4096 * 3] = {0};
    fm.f_read(fd, buffer, sizeof(buffer));

    EXPECT_STREQ(buffer, data.c_str());
    fm.f_close(fd);
}

TEST_F(FilesManagerTest, TestLongWriteWithRest) {
    FilesManager fm(CACHE_SIZE);
    int fd = fm.f_open("test.txt");

    std::string first_page(4096, 'a');
    std::string second_page(4096, 'b');
    std::string third_page(4096, 'c');
    std::string fourth_page(50, 'd');

    std::string data = first_page + second_page + third_page + fourth_page;
    char buff[4096 * 3 + 50 + 1];

    std::strncpy(buff, data.c_str(), sizeof(buff));
    buff[sizeof(buff) - 1] = '\0';

    ssize_t bytes_written = fm.f_write(fd, buff, strlen(buff));
    EXPECT_GE(bytes_written, strlen(buff));

    fm.f_lseek(fd, 0, SEEK_SET);
    char buffer[4096 * 3 + 50] = {0};
    fm.f_read(fd, buffer, sizeof(buffer));

    EXPECT_STREQ(buffer, data.c_str());
    fm.f_close(fd);
}

TEST_F(FilesManagerTest, TestSyncFile) {
    FilesManager fm(CACHE_SIZE);
    int fd = fm.f_open("test.txt");

    std::string first_page(4096, 'a');
    std::string second_page(4096, 'b');
    std::string third_page(4096, 'c');

    std::string data = first_page + second_page + third_page;
    char buff[4096 * 3 + 1];

    std::strncpy(buff, data.c_str(), sizeof(buff));
    buff[sizeof(buff) - 1] = '\0';

    ssize_t bytes_written = fm.f_write(fd, buff, strlen(buff));
    EXPECT_GE(bytes_written, strlen(buff));

    fm.f_lseek(fd, 0, SEEK_SET);
    char buffer[4096 * 3 + 1] = {0};
    fm.f_read(fd, buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, data.c_str());

    std::fstream output;
    output.open("test.txt");

    ASSERT_TRUE(output.peek() == std::ifstream::traits_type::eof());
    output.close();

    fm.f_close(fd);

    output.open("test.txt");
    ASSERT_FALSE(output.peek() == std::ifstream::traits_type::eof());
    output.close();
}

TEST_F(FilesManagerTest, TestCacheCorrectDisplacement) {
    std::string data(4096, 'a');

    for (int i = 0; i < 65536; i++) {
    }
}

TEST_F(FilesManagerTest, TestCacheOverflow) {
    const int num_pages = (CACHE_SIZE / PAGE_SIZE) + 10;
    std::string fileName = "test.txt";
    FilesManager filesManager(CACHE_SIZE);
    int fd = filesManager.f_open(fileName.c_str());

    for (int i = 0; i < num_pages; ++i) {
        std::string data(PAGE_SIZE, 'A' + (i % 26));
        filesManager.f_write(fd, data.c_str(), PAGE_SIZE);
    }

    filesManager.f_close(fd);

    fd = filesManager.f_open(fileName.c_str());

    for (int i = 0; i < num_pages; ++i) {
        char buffer[PAGE_SIZE];
        ssize_t bytesRead = filesManager.f_read(fd, buffer, PAGE_SIZE);

        ASSERT_GE(bytesRead, PAGE_SIZE);

        std::string expectedData(PAGE_SIZE, 'A' + (i % 26));
        ASSERT_EQ(std::strncmp(buffer, expectedData.c_str(), PAGE_SIZE), 0) << "Data mismatch at page " << i;
    }

    filesManager.f_close(fd);
}

TEST_F(FilesManagerTest, OpenNonExistentFile) {
    FilesManager fm(52);
    EXPECT_THROW(fm.f_open("non_existent_file.txt"), std::runtime_error);
}
