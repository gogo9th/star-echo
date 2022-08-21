
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
#include "utils.h"
#include "q2cathedral.h"
#include "threaded.h"
#include "mediaProcess.h"

namespace po = boost::program_options;

static const std::array<std::string, 9> fileExts_ { ".aac", ".gsm", ".wav", ".wavpack", ".ass", ".tta",".flac", ".wma", ".mp3" };


static std::filesystem::path outputFilePath(const std::filesystem::path & inputPath, const std::filesystem::path & outputOptional)
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
            outputPath = outputOptional;
        }
        return outputPath;
    }
    return {};
}


bool isInDirectory(const std::filesystem::path & child, const std::filesystem::path & root)
{
    std::filesystem::path normRoot = root;//std::filesystem::canonical(root);
    std::filesystem::path normChild = child;//std::filesystem::canonical(child);
    auto itr = std::search(normChild.begin(), normChild.end(),
                           normRoot.begin(), normRoot.end());
    return itr == normChild.begin();
}

#if defined(_WIN32)

#include <Windows.h>
using ustring = std::wstring;
#define U(s) L##s

template<typename T>
po::typed_value<T, wchar_t> * uvalue(T * v)
{
    return new po::typed_value<T, wchar_t>(v);
}

int wmain(int argc, wchar_t ** argv)

#else

using ustring = std::string;
#define U(s) s

template<typename T>
po::typed_value<T, char> * uvalue(T * v)
{
    return new po::typed_value<T, char>(v);
}

int main(int argc, char ** argv)

#endif

{
#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    setlocale(LC_ALL, "en_US.UTF-8");
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::vector<ustring> input;
    ustring output;
    std::vector<std::string> filters;
    int threads = 0;
    bool keepFormat = false;
    bool overwrite = false;
    bool normalize = false;


    po::variables_map opts_map;
    po::options_description options("Options");
    options.add_options()
        ("help,h", "Display the help message.")
        ("input,i", uvalue(&input)->composing(), "Input file(s)/directory.\n- [Default: the current directory]")
        ("output,o", uvalue(&output), "If the input is a directory, then output should be a directory.\nIf the input is a file, then the output should be a filename.\nIf the inputs are multiple files, then this option is ignored.\n- [Default: './FINAL' directory]")
        ("threads,t", po::value(&threads), "The number of CPU threads to run.\n- [Default: the processor's available total cores]")
        ("keepFormat,k", po::bool_switch(&keepFormat), "Keep each output file's format the same as its source file's.\n- [Default: the output format is .flac]")
        ("overwrite,w", po::bool_switch(&overwrite), "overwrite output file if it exists [Default: false]")
        ("normalize,n", po::bool_switch(&normalize), "normalize the sound to avoid rips [Default: false]")
        ("filter,f", po::value(&filters), "\
Filter(s) to be applied:\n\
 CH[,roomSize[,gain]] - Cathedral,\n\
   Default is 'CH,10,9' if parameters omitted\n\
 EQ,b1,b2,b3,b4,b5,b6,b7 - Equalizer,\n\
   0<=b<=24, b=12 is '0 gain'\n\
 3D,strength,reverb,delay - 3D effect\n\
   0 <= parameters <= 9\n\
 BE,level,cutoff - Bass enhancement\n\
   1 <= parameters <= 15\n\
Predefined filters:\n\
 studio,\n\
 rock,\n\
 classical,\n\
 jazz,\n\
 dance,\n\
 ballad,\n\
 club,\n\
 RnB,\n\
 cafe,\n\
 livecafe,\n\
 concert,\n\
 church,\n\
 up\n\
")
;
    po::positional_options_description posd;
    posd.add("input", -1);
    try
    {
        #if defined(_WIN32)
        po::store(po::wcommand_line_parser(argc, argv).options(options).positional(posd).run(), opts_map);
        #else
        po::store(po::command_line_parser(argc, argv).options(options).positional(posd).run(), opts_map, false);
        #endif
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

    if (input.empty())
    {
        input.push_back(U("."));
    }

    for (const auto & i : input)
    {
        std::filesystem::path inputPath(std::filesystem::absolute((i)));
        if (std::filesystem::is_directory(inputPath))
        {
            if (output.empty())
            {
                output = U("./FINAL");
            }
            std::filesystem::path outputPath = std::filesystem::absolute(output);

            for (auto const & dir_entry : std::filesystem::recursive_directory_iterator { inputPath })
            {
                if (dir_entry.is_regular_file() && dir_entry.path().has_extension()
                    && !isInDirectory(dir_entry.path(), outputPath)) // skip files in the output directory
                {
                    std::filesystem::path inputFile = dir_entry.path();

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
                        if (overwrite || !std::filesystem::exists(outputFile))    // if the file was already converted in the past, skip
                        {
                            inputFiles.push_back({ inputFile, outputFile, normalize });
                        }
                    }
                }
            }
        }
        else if (std::filesystem::is_regular_file(inputPath))
        {
            auto outputFile = outputFilePath(inputPath, std::filesystem::absolute(output));
            if (!outputFile.empty())
            {
                if (!keepFormat)
                {
                    outputFile.replace_extension(".flac");
                }
                if (overwrite || !std::filesystem::exists(outputFile))    // if the file was already converted in the past, skip
                {
                    inputFiles.push_back({ inputPath, outputFile, normalize });
                }
            }
        }
        else
        {
            err() << inputPath.string() << " is not a directory or file, exiting";
            return -1;
        }
    }

    if (inputFiles.empty())
    {
        msg() << "No new music files are found for processing";
        return 0;
    }

    if (filters.empty())
    {
        filters.push_back("ch");
    }

    FilterFabric fab(!normalize);
    for (const auto & desc : filters)
    {
        auto r = fab.addDesc(stringToWstring(desc));

        if (!r)
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
