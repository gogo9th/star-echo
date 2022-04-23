
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#ifdef __cplusplus  
extern "C" {
#endif
#include <libavutil/log.h>
#ifdef __cplusplus  
}
#endif

#include <filesystem>

#include "log.hpp"
#include "q2effect.h"
#include "threaded.h"
#include "mediaProcess.h"
#include "DNSE_CH.h"

namespace po = boost::program_options;

static const std::array<std::string, 4> fileExts_{ ".wav", ".flac", ".wma", ".mp3" };


static std::filesystem::path outputFilePath(const std::filesystem::path & inputPath, std::string outputOptional)
{
    if (inputPath.has_extension()
        && std::find_if(fileExts_.begin(), fileExts_.end(),
                        [ext = inputPath.extension().string()](const std::string & r)
                        {
                            return boost::iequals(ext, r);
                        }) != fileExts_.end())
    {
        std::filesystem::path outputPath;
        if (outputOptional.empty())
        {
            outputPath = inputPath.parent_path() / inputPath.stem();
            outputPath += " [Q2]";
            outputPath += inputPath.extension();
        }
        else
        {
            outputPath = std::filesystem::absolute(outputOptional);
        }
        return outputPath;
    }
    return {};
}

bool isInDirectory(const std::filesystem::path &child, const std::filesystem::path &root) {
    std::filesystem::path normRoot = root;//std::filesystem::canonical(root);
    std::filesystem::path normChild = child;//std::filesystem::canonical(child);
    auto itr = std::search(normChild.begin(), normChild.end(), 
                           normRoot.begin(), normRoot.end());
    return itr == normChild.begin();
}

int main(int argc, char ** argv)
{
#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    setlocale(LC_ALL, "en_US.UTF-8");

    std::vector<std::string> input;
    std::string output;
    std::vector<std::string> filters;
    int threads = 0;
    bool keepFormat = false;


    po::variables_map opts_map;
    po::options_description options("Options");
    options.add_options()
        ("help,h", "Display the help message.")
        ("input,i", po::value(&input)->composing(), "Input file(s)/directory.\n- [Default: the current directory]")
        ("output,o", po::value(&output), "If the input is a directory, then output should be a directory.\nIf the input is a file, then the output should be a filename.\nIf the inputs are multiple files, then this option is ignored.\n- [Default: './FINAL' directory]")
        ("threads,t", po::value(&threads)->composing(), "The number of CPU threads to run.\n- [Default: the processor's available total cores]")
        ("filter,f", po::value(&filters)->composing(), "Filter(s) to be applied:\n CH[,roomSize[,gain]].\n- [Default: CH[10,9]] (where 'CH' is Cathedral)")
        ("keepFormat,k", po::bool_switch(&keepFormat)->default_value(false), "Keep each output file's format to each the same as its source file's.\n- [Default: the output format is .flac]")
        ;
    po::positional_options_description posd;
    posd.add("input", -1);

    try
    {
        po::store(po::command_line_parser(argc, argv).options(options).positional(posd).run(), opts_map);
        po::notify(opts_map);
    }
    catch (po::error & e)
    {
        err() << "ERROR: " << e.what();
        return -1;
    }


    if (opts_map.find("help") != opts_map.end())
    {
        msg() << options;
        return 0;
    }

    // nothing:  ./
    // directory
    // file1 [file2 file3]

    std::vector<FileItem> inputFiles;

    if (input.size() <= 1)
    { 
        std::filesystem::path inputPath = input.empty() ? std::filesystem::absolute(".") : std::filesystem::absolute(input[0]);

        if (std::filesystem::is_directory(inputPath))
        {   
	        if (output.empty())
            {
                output = "./FINAL";
            }
            std::filesystem::path outputPath = std::filesystem::absolute(output);
            
            for (auto const & dir_entry : std::filesystem::recursive_directory_iterator{ inputPath })
            {   if (dir_entry.is_regular_file() && dir_entry.path().has_extension() 
                    && !isInDirectory(dir_entry.path(), outputPath)) // skip files in the output directory
                {
                    std::filesystem::path inputFile = dir_entry.path();
                    std::filesystem::path inputFileRelative = std::filesystem::relative(inputFile, std::filesystem::absolute("."));
                    std::filesystem::path destinationAbsolute = outputPath / inputFileRelative;
                    if (!keepFormat)
                        destinationAbsolute.replace_extension(".flac");
                    if (std::filesystem::exists(destinationAbsolute)) // if the file was already converted in the past, skip
                        continue;
                    
                    // filter by file extension
                    if (std::find_if(fileExts_.begin(), fileExts_.end(), [ext = inputFile.extension().string()](const std::string & r)
                    {
                        return boost::iequals(ext, r);
                    }) != fileExts_.end())
                    {
                        auto relParent = inputFile.lexically_relative(inputPath);
                        auto outputFile = (outputPath / relParent);

                        if (!keepFormat)
                        {
                            outputFile.replace_extension(".flac");
                        }

                        inputFiles.push_back({ inputFile, outputFile });
                    }
                }
            }
        }
        else if (std::filesystem::is_regular_file(inputPath))
        {
            auto outputFile = outputFilePath(inputPath, output);
            if (!outputFile.empty())
            {
                if (!keepFormat)
                {
                    outputFile.replace_extension(".flac");
                }

                inputFiles.push_back({ inputPath, outputFile });
            }
        }
        else
        {
            err() << inputPath.string() << " is not a directory or file, exiting";
            return -1;
        }
    }
    else
    {
        for (auto & i : input)
        {
            std::filesystem::path inputPath = std::filesystem::absolute(i);

            auto outputFile = outputFilePath(inputPath, {});
            if (!outputFile.empty())
            {
                if (!keepFormat)
                {
                    outputFile.replace_extension(".flac");
                }

                inputFiles.push_back({ inputPath, outputFile });
            }
        }
    }

    if (inputFiles.empty())
    {
        msg() << "No new music files are found for processing";
        return 0;
    }

    FilterFab fab;
    if (opts_map.find("input-file") != opts_map.end())
    {
        opts_map["input-file"].as<std::vector<std::string>>();
    }
    if (filters.empty())
    {
        filters.push_back("ch");
    }
    for (auto & f : filters)
    {
        if (!fab.addDesc(f))
        {
            return -1;
        }
    }
#if defined(_DEBUG)
    av_log_set_level(AV_LOG_WARNING);
    //av_log_set_level(99);
#else
    av_log_set_level(AV_LOG_FATAL);
#endif

    auto processor = std::make_unique<MediaProcess>(fab);
    //msg() << processor->operator()(inputFiles[0]);
    ThreadedWorker<FileItem, MediaProcess> worker(inputFiles, processor, threads);
    worker.waitForDone();

    return 0;
}
