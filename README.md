
# Samsung Q2-YP MP3 Player's Cathedral Sound Effect Generator


This software generates almost the same sound effect as [Samsung YP-Q2 MP3 player firmware](https://github.com/LemonBoy/Q2-Tools)'s Cathedral effect.


## Software Description

- When the software is executed, all music extension files (`.flac, .mp3, .wav, .wma`) in the current directory are applied the Cathedral sound effect and their converted new music files are saved in the `"FINAL"` folder created in the same directory.

- All sub-directory structures of original music files are preserved and saved in the `"FINAL"` folder. 

- Already converted music files in the past (whose converted version already exists in the `"FINAL"` folder) are skipped. 

- A pre-built Windows software is available:  [`q2cathedral_windows.exe`](https://github.com/gogo9th/yp-q2-cathedral/blob/main/q2cathedral_windows.exe).


## Ubuntu Installation

<b><u>Step 1.</u></b> Install the boost and ffmpeg libraries.
```console
    $ sudo apt install ffmpeg libboost-program-options-dev libavformat-dev libavcodec-dev libavutil-dev libswresample-dev
```
<b><u>Step 2.</u></b> Download the source code.
```console
    $ git clone https://github.com/gogo9th/yp-q2-cathedral
```

<b><u>Step 3.</u></b> Compile the source code by using one of the following 2 options:

* <u>*Option A:*</u> Use Makefile to compile.
```console
    $ cd yp-q2-cathedral
    $ make
```
* <u>*Option B:*</u> Use CMake to compile.
```console
    $ cd yp-q2-cathedral
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
```

<b><u>Step 3.</u></b> Run the created `q2cathedral`.
```console
    $ ./q2cathedral --help 
    Options:
      -h [ --help ]         Display the help message.
      -i [ --input ] arg    Input file(s)/directory.
                            - [Default: the current directory]
      -o [ --output ] arg   If the input is a directory, then output should be a 
                            directory.
                            If the input is a file, then the output should be a filename.
                            If the inputs are multiple files, then this option is ignored.
                            - [Default: './FINAL' directory]
      -t [ --threads ] arg  The number of CPU threads to run.
                            - [Default: the processor's available total cores]
      -f [ --filter ] arg   Filter(s) to be applied: 
                              CH[,roomSize[,gain]].
                            - [Default: CH[10,9]] (where 'CH' is Cathedral)
      -k [ --keepFormat ]   Keep each output file's format to each the same as its source file's.
                            - [Default: the output format is .flac]
```


## Windows Installation

<b><u>Step 1.</u></b> Go and install CMake: [https://cmake.org/install/](https://cmake.org/install/)

<b><u>Step 2.</u></b> Go and install Visual Studio (C++ Development): [https://visualstudio.microsoft.com/ko/downloads/](https://visualstudio.microsoft.com/ko/downloads/)

<b><u>Step 3.</u></b> Install vcpkg.

```console
    $ cd ~
    $ git clone https://github.com/Microsoft/vcpkg.git .
    $ cd vcpkg
    $ ./bootstrap-vcpkg.bat
```

<b><u>Step 4.</u></b> Install the boost and ffmpeg libraries

```console
    $ ./vcpkg.exe install boost-program-options boost-algorithm ffmpeg[avformat] ffmpeg[avcodec] ffmpeg[swresample] ffmpeg[zlib] --triplet x64-windows
```
(For x32 builds, run `"vcpkg install boost-program-options default"` instead)


<b><u>Step 5.</u></b> Compile the source code.
```console
    $ cd yp-q2-cathedral
    $ mkdir build
    $ cd build
      # Replace <Windows user name> to your home directory's user name
      # Must have double quotes around the path
      # If compilation fails, you must run "rm * .* -rf" to delete all previous CMake caches
    $ cmake .. -DCMAKE_TOOLCHAIN_FILE="C:\Users\<windows user name>\vcpkg\scripts\buildsystems\vcpkg.cmake"
    $ cmake --build . --config Release
```

<b><u>Step 6.</u></b> Check if `q2cathedral.exe` is created in the Release folder.

<b><u>Step 7.</u></b> Pack `.exe` and `.dll` files into a single `.exe` file

- Download and install Enigma Virtual Box at [https://enigmaprotector.com/en/downloads.html](https://enigmaprotector.com/en/downloads.html).

- Run Enigma Virtual Box.

- Choose the input file as "qe2ffect.exe" in the Release folder.

- Add all 6 .dll files in the Release folder: "avcodec-\*.dll", "avformat-\*.dll", "avutil-\*.dll", "boost_program_options-\*.dll", "swresample-\*.dll, zlib*.dll".

- Click "Process" to create `q2cathedral_boxed.exe`.


<b><u>Step 7.</u></b> Run the created `q2cathedral_boxed.exe`.

* <u>Option 1:</u> Use a terminal. 
```console
    $ cd Release
    $ ./q2cathedral_boxed.exe --help 
```
* <u>Option 2:</u> Double-click the generated `q2cathedral_boxed.exe`, then all music files in the same directory will be converted and stored in the `"FINAL"` folder.
