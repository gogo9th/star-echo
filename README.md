
#Samsung Q2-YP MP3 Player's Cathedral Sound Effect Generator


This software generates almost the same sound effect as [Samsung YP-Q2 MP3 player firmware](https://github.com/LemonBoy/Q2-Tools)'s Cathedral effect.


## Software Description

- When the software is executed, all music extension files (`.flac, .mp3, .wav, .wma`) in the current directory are applied the Cathedral sound effect and their converted music files are saved in the `"FINAL"` folder created in the same directory.

- All sub-directory structures of original music files are preserved within the `"FINAL"` folder. 

- Already converted files in the past (whose converted version already exists in the `"FINAL"` folder) are skipped. 

- The pre-built software for Windows is [`q2effect_windows.exe`]().


## Ubuntu Installation

<b><u>Step 1.</u></b> Install the boost and ffmpeg libraries.
```console
    $ sudo apt install libswresample-dev libboost-program-options-dev libavformat-dev libavcodec-dev libavutil-dev libswresample-dev
```

<b><u>Step 2.</u></b> Compile the source code by using one of the following 2 options:

* <u>Option 1:</u> Use Makefile to compile.
```
    $ cd yp-q2-cathedral
    $ make
```
* <u>Option 2:</u> Use CMake to compile.
```console
    $ yp-q2-cathedral
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
```

<b><u>Step 3.</u></b> Run the generated `q2effect.exe`.
```console
    $ ./q2effect --help 
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
      # Replace <windows user name> to your home directory's user name
      # Must have double quotes around the path
      # If compilation fails, you must run "rm * .* -rf" to delete all previous CMake caches
    $ cmake .. -DCMAKE_TOOLCHAIN_FILE="C:\Users\<windows user name>\vcpkg\scripts\buildsystems\vcpkg.cmake"
    $ cmake --build . --config Release
```

<b><u>Step 6.</u></b> Pack `.exe` and `.dll` files into a single `.exe` file

- Download and install Enigma Virtual Box at [https://enigmaprotector.com/en/downloads.html](https://enigmaprotector.com/en/downloads.html).

- Run Enigma Virtual Box.

- Set the input file to "qe2ffect.exe".

- Add all 6 .dll files: "avcodec-\*.dll", "avformat-\*.dll", "avutil-\*.dll", "boost_program_options-\*.dll", "swresample-\*.dll, zlib*.dll".

- Click "Process" to generate `q2effect_boxed.exe`.


<b><u>Step 7.</u></b> Run `q2effect_boxed.exe`.

* <u>Option 1:</u> Use a terminal. 
```console
    $ cd Release
    $ ./q2effect --help 
```
* <u>Option 2:</u> Double-click the generated `q2effect.exe`, then all music files in the same directory will be converted.
