#include <gtest/gtest.h>

#include "Sorter.h"

#include <fstream>
#include <vector>

class SorterTestFixture : public ::testing::Test {
public:
    static void SetUpTestSuite() {
        std::ofstream config_file("config");
        if (config_file.is_open()) {
            config_file << "read_delay: 0\n";
            config_file << "write_delay: 0\n";
            config_file << "move_delay: 0\n";
            config_file << "rewind_delay: 0\n";
            config_file.close();
        } else {
            throw std::runtime_error("Failed to create config file");
        }
    }

    static void TearDownTestSuite() {
        std::remove("config");
    }
};


TEST_F(SorterTestFixture, SimpleSorting) {
    std::string input_file = "TestsInputFile.txt";
    std::string output_file = "TestsOutputFile.txt";
    size_t ram_bytes = 2000;

    std::ofstream input(input_file);
    input << "3\n1\n2\n";
    input.close();

    Sorter sorter(input_file, output_file, ram_bytes);
    sorter.sortTapes();

    std::ifstream output(output_file);
    int value;
    std::vector<int> sorted_data;

    while (output >> value) {
        sorted_data.push_back(value);
    }
    output.close();

    std::vector<int> expected_data = {1, 2, 3};

    ASSERT_EQ(sorted_data.size(), expected_data.size());
    for (size_t i = 0; i < sorted_data.size(); ++i) {
        ASSERT_EQ(sorted_data[i], expected_data[i]);
    }

    std::remove(input_file.c_str());
    std::remove(output_file.c_str());
}

TEST_F(SorterTestFixture, SortingWithLargeData) {
    std::string input_file = "LargeTestsInputFile.txt";
    std::string output_file = "LargeTestsOutputFile.txt";
    size_t ram_bytes = 2000;

    std::ofstream input(input_file);
    for (int i = 10000; i > 0; --i) {
        input << i << "\n";
    }
    input.close();

    Sorter sorter(input_file, output_file, ram_bytes);
    sorter.sortTapes();

    std::ifstream output(output_file);
    int value;
    std::vector<int> sorted_data;

    while (output >> value) {
        sorted_data.push_back(value);
    }
    output.close();

    for (size_t i = 1; i < sorted_data.size(); ++i) {
        ASSERT_LE(sorted_data[i - 1], sorted_data[i]);
    }

    std::remove(input_file.c_str());
    std::remove(output_file.c_str());
}

TEST_F(SorterTestFixture, CheckFileContents) {
    std::string input_file = "TestsInputFile.txt";
    std::string output_file = "TestsOutputFile.txt";
    size_t ram_bytes = 2000;

    std::ofstream input(input_file);
    input << "5\n3\n1\n4\n2";
    input.close();

    Sorter sorter(input_file, output_file, ram_bytes);
    sorter.sortTapes();

    std::ifstream output(output_file);
    int value;
    std::vector<int> sorted_data;

    while (output >> value) {
        sorted_data.push_back(value);
    }
    output.close();

    std::vector<int> expected_data = {1, 2, 3, 4, 5};

    ASSERT_EQ(sorted_data.size(), expected_data.size());
    for (size_t i = 0; i < sorted_data.size(); ++i) {
        ASSERT_EQ(sorted_data[i], expected_data[i]);
    }

    std::remove(input_file.c_str());
    std::remove(output_file.c_str());
}

TEST_F(SorterTestFixture, SortingOneMillionNumbers) {
    std::string input_file = "MillionInputFile.txt";
    std::string output_file = "MillionOutputFile.txt";
    size_t ram_bytes = 10000;

    std::ofstream input(input_file);
    srand(time(nullptr));
    for (int i = 0; i < 1000000; ++i) {
        input << rand() % 1000000 << "\n";
    }
    input.close();

    Sorter sorter(input_file, output_file, ram_bytes);
    sorter.sortTapes();

    std::ifstream output(output_file);
    int value;
    std::vector<int> sorted_data;

    while (output >> value) {
        sorted_data.push_back(value);
    }
    output.close();

    for (size_t i = 1; i < sorted_data.size(); ++i) {
        ASSERT_LE(sorted_data[i - 1], sorted_data[i]);
    }

    std::remove(input_file.c_str());
    std::remove(output_file.c_str());
}

TEST_F(SorterTestFixture, LowMemory) {
    std::string input_file = "InputFile.txt";
    std::string output_file = "OutputFile.txt";
    size_t ram_bytes = 20;

    std::ofstream input(input_file);
    srand(time(nullptr));
    for (int i = 0; i < 10000; ++i) {
        input << rand() % 10000 << "\n";
    }
    input.close();

    ASSERT_THROW({
        Sorter sorter(input_file, output_file, ram_bytes);
        sorter.sortTapes();
    }, std::runtime_error);

    std::remove(input_file.c_str());
    std::remove(output_file.c_str());
}

TEST_F(SorterTestFixture, ConfigNotExist) {
    std::remove("config");

    std::string input_file = "InputFile.txt";
    std::string output_file = "OutputFile.txt";

    std::ofstream input(input_file);
    input << "5\n10\n20\n30\n40\n50\n";
    input.close();

    std::ofstream output(output_file);
    output.close();

    ASSERT_THROW({
        Sorter sorter(input_file, output_file, 20);
        sorter.sortTapes();
    }, std::runtime_error);

    std::remove(input_file.c_str());
    std::remove(output_file.c_str());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}