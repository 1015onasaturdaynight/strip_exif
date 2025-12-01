#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>

namespace fs = std::filesystem;

[[nodiscard]] bool strip_exif_from_file(const fs::path &filepath) {
    // preallocate buffer
    std::error_code ec;
    const auto filesize = fs::file_size(filepath, ec);
    if (ec) {
        std::cerr << "Failed to get file size: " << filepath << " (" << ec.message() << ")\n";
        return false;
    }

    // fs::file_size returns uintmax_t - cast to streamsize (signed) for ifstream::read
    const auto streamsz = static_cast<std::streamsize>(filesize);
    std::vector<unsigned char> buffer(filesize);
    std::ifstream in(filepath, std::ios::binary);
    if (!in.read(reinterpret_cast<char*>(buffer.data()), streamsz)) {
        std::cerr << "Failed to read file: " << filepath << "\n";
        return false;
    }

    // verify jpg soi marker
    if (buffer.size() < 4 || buffer[0] != 0xFF || buffer[1] != 0xD8) {
        std::cerr << "Not a valid JPEG: " << filepath << "\n";
        return false;
    }

    size_t pos = 2;
    std::vector<unsigned char> output;
    output.reserve(buffer.size()); // avoid repeated reallocations
    output = {0xFF, 0xD8};

    while (pos + 4 <= buffer.size() && buffer[pos] == 0xFF) {
        const unsigned char marker = buffer[pos + 1];
        // explicit casts to avoid narrowing - shift unsigned char into uint16_t cleanly
        const auto segment_length =
            static_cast<uint16_t>(static_cast<uint16_t>(buffer[pos + 2]) << 8 |
                                   static_cast<uint16_t>(buffer[pos + 3]));
        const size_t segment_end = pos + 2 + static_cast<size_t>(segment_length);

        if (segment_end > buffer.size()) {
            std::cerr << "Corrupt JPEG segment in: " << filepath << "\n";
            return false;
        }

        // end of image or start of scan -> copy rest of file as-is
        if (marker == 0xDA || marker == 0xD9) {
            output.insert(output.end(),
                          buffer.begin() + static_cast<std::ptrdiff_t>(pos),
                          buffer.end());
            break;
        }

        // skip APP1/EXIF segment (marker 0xE1)
        if (marker == 0xE1) {
            pos = segment_end;
            continue;
        }

        // copy all other segments unchanged
        output.insert(output.end(),
                      buffer.begin() + static_cast<std::ptrdiff_t>(pos),
                      buffer.begin() + static_cast<std::ptrdiff_t>(segment_end));
        pos = segment_end;
    }

    fs::path outpath = filepath;
    outpath.replace_filename(outpath.stem().string() + ".stripped" + outpath.extension().string());
    std::ofstream out(outpath, std::ios::binary);

    if (!out.write(reinterpret_cast<const char *>(output.data()),
                   static_cast<std::streamsize>(output.size()))) {
        std::cerr << "Failed to write " << outpath << "\n";
        return false;
    }

    std::cout << "Stripped EXIF: " << filepath << " -> " << outpath << "\n";

    return true;
}

[[nodiscard]] std::vector<fs::path> find_jpeg_files(const fs::path& input) {
    std::vector<fs::path> files;

    if (fs::is_directory(input)) {
        for (auto& entry : fs::recursive_directory_iterator(input)) {
            if (!entry.is_regular_file()) continue;
            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
            if (ext == ".jpg" || ext == ".jpeg") {
                files.push_back(entry.path());
            }
        }
    } else {
        files.push_back(input);
    }

    return files;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <jpg files or directory>\n";
        return 1;
    }

    std::vector<fs::path> files;
    for (int i = 1; i < argc; ++i) {
        auto found = find_jpeg_files(argv[i]);
        files.insert(files.end(), found.begin(), found.end());
    }

    if (files.empty()) {
        std::cerr << "No .jpg files found.\n";
        return 1;
    }

    bool all_ok = true;
    for (const auto& f : files) {
        if (!strip_exif_from_file(f)) {
            all_ok = false;
        }
    }

    return all_ok ? 0 : 1;
}
