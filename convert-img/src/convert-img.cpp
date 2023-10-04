#include <iostream>
#include <filesystem>
#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <Magick++.h>
#include <chrono>
#include <mutex>
#include <thread>

#include <queue>
#include <condition_variable>
#include <functional>

using namespace std;



class ThreadPool {
private:
    vector<thread> workers;
    queue<function<void()>> tasks;

    mutex queue_mutex;
    condition_variable condition;
    bool stop = false;

public:
    ThreadPool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; i++) {
            workers.emplace_back([this] {
                while (true) {
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] {
                            return this->stop || !this->tasks.empty();
                            });
                        if (this->stop && this->tasks.empty()) return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
                });
        }
    }

    template<class F>
    void enqueue(F&& f) {
        {
            unique_lock<mutex> lock(queue_mutex);
            if (stop) throw runtime_error("enqueue on stopped ThreadPool");
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            unique_lock<mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (thread& worker : workers) {
            worker.join();
        }
    }
};


// Moved repetitive utility functions here to reduce redundancy
namespace utils {
    string quote(const string& str) {
        return "\"" + str + "\"";
    }

    bool is_file(const string& path) {
        return filesystem::is_regular_file(path);
    }

    bool is_directory(const string& path)
    {
    	return filesystem::is_directory(path);
	}

    string get_extension(const string& path) {
        return filesystem::path(path).extension().string();
    }

    vector<string> get_files(const string& path, const string& ext)
	{
        vector<string> files;
        for (const auto& entry : filesystem::directory_iterator(path)) 
        {
            const string& file_path = entry.path().string();
            if (is_file(file_path) && (ext.empty() || get_extension(file_path) == ext)) 
            {
                files.push_back(file_path);
            }
        }
        return files;
    }
}  // namespace utils

enum class CompressionType {  // Changed to enum class for type-safety
    NoCompression,
    LZWCompression,
    ZipCompression,
    RLECompression,
    JPEGCompression,
    Invalid
};

CompressionType get_compression_type(const string& compression)
{
    string compression_lower = compression;
    transform(compression_lower.begin(), compression_lower.end(), compression_lower.begin(), ::tolower);

    if (compression_lower == "none") return CompressionType::NoCompression;
    if (compression_lower == "lzw") return CompressionType::LZWCompression;
    if (compression_lower == "deflate") return CompressionType::ZipCompression;
    if (compression_lower == "packbits") return CompressionType::RLECompression;
    if (compression_lower == "jpeg") return CompressionType::JPEGCompression;
    return CompressionType::Invalid;
}

void set_quality(Magick::Image& image, const int quality, const string& extension, const CompressionType compression)
{
    if (quality < 1 || quality > 100) throw invalid_argument("Invalid quality: " + to_string(quality));
    if (extension == ".webp" || extension == ".jpeg" || extension == ".jpg" || extension == ".png") 
    {
        image.quality(quality);
        image.compressType(static_cast<Magick::CompressionType>(compression));
    }
    else if (extension == ".tiff" || extension == ".tif") 
    {
        image.compressType(static_cast<Magick::CompressionType>(compression));
    }
    else 
    {
        image.quality(quality);
    }
}

string get_new_path(const string& base_path) {
    string new_path = base_path;
    int count = 1;
    while (filesystem::exists(new_path))
    {
        new_path = base_path.substr(0, base_path.find_last_of(".")) + "_" + to_string(count) + base_path.substr(base_path.find_last_of("."));
        count++;
    }
    return new_path;
}

void convert_image(
    const string& input_path, const string& output_path,
    const int quality, const CompressionType compression,
    const double scale, const bool overwrite)
{
    spdlog::info("Converting image: {} -> {}", utils::quote(input_path), utils::quote(output_path));

    try {
        Magick::Image image(input_path);
        image.scale(Magick::Geometry(image.columns() * scale, image.rows() * scale));
        set_quality(image, quality, utils::get_extension(output_path), compression);

        string output_path_to_use = overwrite ? output_path : get_new_path(output_path);
        image.write(output_path_to_use);
    }
    catch (Magick::Exception& e) {
        throw runtime_error("Magick++ exception: " + string(e.what()));
    }
}

void convert_images(
    const string& input_dir, const string& output_dir,
    const string& input_ext, const string& output_ext,
    const CompressionType compression, const int quality,
    const double scale, const bool overwrite, const int thread_count)
{
    const vector<string> files = utils::get_files(input_dir, input_ext);

    if (files.empty()) {
        spdlog::warn("No files found in input directory: {}", utils::quote(input_dir));
        return;
    }

    ThreadPool pool(thread_count);
    for (const auto& input_path : files) {
        pool.enqueue([&] {
            const filesystem::path input_p(input_path);
            const string input_filename = input_p.filename().string();
            const string output_path = output_dir + input_filename + "." + output_ext;
            convert_image(input_path, output_path, quality, compression, scale, overwrite);
            });
    }
}


int main(int argc, char** argv)
{
    const auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);
    Magick::InitializeMagick(nullptr);  // Moved initialization here

    CLI::App app{ "Image Conversion Tool (Convert, Scale, Resize)" };
    string input_path, output_path, input_ext, output_ext, compression = "lzw";
    int quality = 80;
    double scale = 1.0;
    bool overwrite = false;
    unsigned int num_threads = std::thread::hardware_concurrency(); // Default: number of available CPU cores

    app.add_option("input", input_path, "Input image path")->required();
    app.add_option("output", output_path, "Output image path")->required();
    app.add_option("-q,--quality", quality, "Output image quality (1-100)")->check(CLI::Range(1, 100));
    app.add_option("-c,--compression", compression, "compression methods (none, lzw, deflate, packbits, jpeg)");
    app.add_option("-s,--scale", scale, "Output image scale (0.1-1.0)")->check(CLI::Range(0.1, 1.0));
    app.add_option("-i,--in-ext", input_ext, "Input image extension");
    app.add_option("-o,--out-ext", output_ext, "Output image extension");
    app.add_option("-t,--threads", num_threads, "Number of threads to use");
    app.add_flag("-f,--force", overwrite, "Overwrite existing file");

    CLI11_PARSE(app, argc, argv);

    try 
    {
	    std::chrono::time_point<chrono::steady_clock> start;
        std::chrono::time_point<chrono::steady_clock> end;

        // remove `.` from out extensions if present
        if (output_ext[0] == '.') output_ext.erase(0, 1);
        // add `.` to in extensions if not present
        if (input_ext[0] != '.') input_ext.insert(0, 1, '.');

	    const CompressionType comp_type = get_compression_type(compression);
        if (comp_type == CompressionType::Invalid) throw invalid_argument("Invalid compression type: " + utils::quote(compression));
        if (utils::is_file(input_path)) 
        {
            start = std::chrono::high_resolution_clock::now();
            if (utils::get_extension(output_path) == "tiff" && quality != 95) spdlog::warn("Quality is ignored for tiff files");
            convert_image(input_path, output_path, quality, comp_type, scale, overwrite);
            end = std::chrono::high_resolution_clock::now();
        }
        else 
        {
            if (output_ext == "tiff" && quality != 80) spdlog::warn("Quality is ignored for tiff files");

            // check if input directory exists
            if (!utils::is_directory(input_path))
            {
            	spdlog::error("Directory {} does not exist", utils::quote(input_path));
				return 1;
            }

            // check if output directory exists
            if (!utils::is_directory(output_path))
            {
                cout << "Directory " + utils::quote(output_path) + " does not exist. Create? (y/n): ";
                char input;
                cin >> input;
                if (input == 'y' || input == 'Y')
                {
                    spdlog::info("Creating output directory: {}", output_path);
                    filesystem::create_directory(output_path);
                }
                else
                {
	                spdlog::info("Exiting");
					return 0;
                }
            }
            start = std::chrono::high_resolution_clock::now();
            convert_images(input_path, output_path, input_ext, output_ext, comp_type, quality, scale, overwrite, num_threads);
            end = std::chrono::high_resolution_clock::now();
        }
	    const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        // convert duration to seconds and print
        const double seconds = duration.count() / 1e6;

        spdlog::info("Took {} seconds", seconds);

        spdlog::info("Done");
        return 0;
    }
    catch (exception e) {
        spdlog::error(e.what());
        return 1;
    }
}
