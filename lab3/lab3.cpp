#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <fstream>
#include <vector>
#include <openssl/sha.h>

using namespace std;
namespace fs = filesystem;

string computeSHA1(const fs::path& filepath) {
    ifstream file(filepath, ios::binary);
    if (!file) {
        throw runtime_error("Cannot open file: " + filepath.string());
    }
    SHA_CTX context;
    SHA1_Init(&context);
    char buffer[1024];
    while (file.read(buffer, sizeof(buffer))) {
        SHA1_Update(&context, buffer, file.gcount());
    }
    SHA1_Update(&context, buffer, file.gcount());
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1_Final(hash, &context);
    string result;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        char buf[3];
        sprintf(buf, "%02x", hash[i]);
        result += buf;
    }
    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <directory>" << endl; 
        return 1;
    }
    fs::path directory = argv[1];
    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        cout << "Error: " << directory << " is not a valid directory" << endl;
        return 1;
    }
    unordered_map<string, fs::path> hash_to_file;
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (fs::is_regular_file(entry)) {
            string hash = computeSHA1(entry.path());
            if (hash_to_file.find(hash) == hash_to_file.end()) {
                hash_to_file[hash] = entry.path();
                cout << "Original: " << entry.path() << " ["<<hash << "]" << endl;
            }
            else {
                fs::path original_file = hash_to_file[hash];
                fs::path current_file = entry.path();
                fs::remove(current_file);
                fs::create_hard_link(original_file, current_file);
                cout << "Replaced with hard link: " << current_file << " -> " << original_file << endl;
            }
       }
    }
    cout << "Processing completed. " << hash_to_file.size() << " unique files found." << endl;
    return 0;
}
