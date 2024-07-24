#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>

#include <opencv2/opencv.hpp>

using namespace std;

bool read_image(const string &filename, vector<unsigned char> &image, int &width, int &height, int &channels)
{
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    if (data == nullptr)
    {
        cerr << "Failed to read image: " << filename << endl;
        return false;
    }
    image.resize(width * height * channels);
    memcpy(image.data(), data, image.size());
    stbi_image_free(data);
    return true;
}

bool save_image(const string &filename, const vector<unsigned char> &image, int width, int height, int channels)
{
    if (stbi_write_png(filename.c_str(), width, height, channels, image.data(), width * channels) == 0)
    {
        cerr << "Failed to save image: " << filename << endl;
        return false;
    }
    return true;
}

bool save_compressed_image(const string &filename, const vector<unsigned char> &compressed_image)
{
    ofstream ofs(filename, ios::binary);
    if (!ofs)
    {
        cerr << "Failed to save compressed image: " << filename << endl;
        return false;
    }
    ofs.write(reinterpret_cast<const char *>(compressed_image.data()), compressed_image.size());
    return true;
}

bool read_compressed_image(const string &filename, vector<unsigned char> &compressed_image)
{
    ifstream ifs(filename, ios::binary);
    if (!ifs)
    {
        cerr << "Failed to read compressed image: " << filename << endl;
        return false;
    }
    compressed_image.assign((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());
    return true;
}

vector<unsigned char> rle_compress(const vector<unsigned char> &image, int width, int height, int channels)
{
    cout << "Compressing image..." << endl;
    vector<unsigned char> compressed_image;
    for (int i = 0; i < image.size(); i += channels)
    {
        int count = 1;
        while (i + count * channels < image.size() && memcmp(&image[i], &image[i + count * channels], channels) == 0 && count < 255)
        {
            count++;
        }
        compressed_image.push_back(count);
        for (int j = 0; j < channels; j++)
        {
            compressed_image.push_back(image[i + j]);
        }
        i += (count - 1) * channels;
    }
    cout << "Done." << endl;
    return compressed_image;
}

vector<unsigned char> rle_decompress(const vector<unsigned char> &compressed_image, int width, int height, int channels)
{
    vector<unsigned char> image;
    for (size_t i = 0; i < compressed_image.size(); i += (channels + 1))
    {
        int count = compressed_image[i];
        for (int j = 0; j < count; j++)
        {
            for (int k = 0; k < channels; k++)
            {
                image.push_back(compressed_image[i + 1 + k]);
            }
        }
    }
    return image;
}

double calculate_entropy(const vector<unsigned char> &image, int width, int height, int channels)
{
    vector<int> histogram(256, 0);
    for (int i = 0; i < width * height * channels; i++)
    {
        histogram[image[i]]++;
    }
    double entropy = 0;
    for (int i = 0; i < 256; i++)
    {
        if (histogram[i] == 0)
        {
            continue;
        }
        double p = static_cast<double>(histogram[i]) / (width * height * channels);
        entropy -= p * log2(p);
    }
    return entropy;
}

double calculate_average_code_length(const vector<unsigned char> &image, int width, int height, int channels)
{
    vector<int> histogram(256, 0);
    for (int i = 0; i < width * height * channels; i++)
    {
        histogram[image[i]]++;
    }
    double average_code_length = 0;
    for (int i = 0; i < 256; i++)
    {
        if (histogram[i] == 0)
        {
            continue;
        }
        double p = static_cast<double>(histogram[i]) / (width * height * channels);
        average_code_length += p * (1 + log2(1 / p));
    }
    return average_code_length;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <filename>" << endl;
        return -1;
    }

    string filename = argv[1];
    vector<unsigned char> image;
    int width, height, channels;
    if (!read_image(filename, image, width, height, channels))
    {
        return -1;
    }
    vector<unsigned char> compressed_image = rle_compress(image, width, height, channels);
    if (!save_compressed_image("compressed_" + filename, compressed_image))
    {
        return -1;
    }
    vector<unsigned char> decompressed_image = rle_decompress(compressed_image, width, height, channels);
    if (!save_image("decompressed_" + filename, decompressed_image, width, height, channels))
    {
        return -1;
    }
    double entropy = calculate_entropy(image, width, height, channels);
    double average_code_length = calculate_average_code_length(image, width, height, channels);
    cout << "Entropy: " << entropy << endl;
    cout << "Average code length: " << average_code_length << endl;
    cout << "Coding efficiency: " << entropy / average_code_length << endl;

    cv::Mat img1 = cv::imread(filename);
    cv::Mat img2 = cv::imread("decompressed_" + filename);

    if (img1.empty() || img2.empty())
    {
        cerr << "Failed to read image: " << filename << endl;
        return -1;
    }

    cv::imshow("Original Image", img1);
    cv::imshow("Decompressed Image", img2);
    cv::waitKey(0);

    return 0;
}
