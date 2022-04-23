
Samsung Q2-YP MP3 Player's Cathedral Sound Effect



This software is implemented based on reverse-engineering Q2-YP's firmware at https://github.com/LemonBoy/Q2-Tools.

The release version (for Windows) is q2effect_windows.exe. 


Software Description

All music files (.flac, .mp3, .wav, .wma) in the same directory are filtered through YP-Q2's Cathedral sound effect and the converted music files are saved in the "FINAL" folder created in the same directory.

All sub-directory structures of original music files are preserved within the FINAL folder. 

Already converted files in the past (whose converted version already exists in the FINAL folder) are skipped. 



Ubuntu Installation

1. Install required libraries

$ sudo apt install ...


2. Compile the source code.

* Option 1
bash```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

* Option 2
bash```
$ make
```

3. Run q2effect.exe.

bash```
$ cd Release
$ ./q2effect --help 
```	


Windows Installation

1. Go and install CMake: https://cmake.org/install/

2. Go and install Visual Studio (C++ Development): https://visualstudio.microsoft.com/ko/downloads/

3. msvc/minimal c++ installation, nmake, windows sdk, cmake, git

* vcpkg installation (if missing)

Open 'x64 native developer command prompt' for Visual Studio (or Power Shell)

Install to some dedicated %vcpkg% directory:

bash```
$ cd ~
$ git clone https://github.com/Microsoft/vcpkg.git .
$ cd vcpkg
$ ./bootstrap-vcpkg.bat
```

4. Install boost libs

bash```
$ ./vcpkg.exe install boost-program-options boost-algorithm --triplet x64-windows
# For x32 builds, run "vcpkg install boost-program-options default"
```

5. Install ffmpeg libs

bash```
$ ./vcpkg.exe install ffmpeg[avformat] ffmpeg[avcodec] ffmpeg[swresample] ffmpeg[zlib] --triplet x64-windows
```

6. Configure in separate build directory

```
$ cd <the Q2 source file directory>
$ mkdir build
$ cd build
$
$ # Replace 'gogo9th' to your home directory's user ID
$ # Must have double quotes around the path
$ # If compilation fails, you must run "rm * .* -rf" to delete all previous CMake caches
$ cmake .. -DCMAKE_TOOLCHAIN_FILE="C:\Users\gogo9th\vcpkg\scripts\buildsystems\vcpkg.cmake"
$ cmake --build . --config Release
```

7. Pack .exe and .dll files into s a single .exe file

- Download and install Enigma Virtual Box (https://enigmaprotector.com/en/downloads.html).

- Run Enigma Virtual Box.

- Set the input file to 'qe2ffect.exe'.

- Add all .dll files: avcodec-*.dll, avformat-*.dll, avutil-*.dll, boost_program_options-*.dll, swresample-*.dll, zlib*.dll.

- Click "Process" to generate q2effect_boxed.exe.


8. Run q2effect_boxed.exe.

* Option 1
    bash```
    $ cd Release
    $ ./q2effect --help 
    ```

* Option 2: Double-click it and all music files in the same directory will be converted
# samsung-yp-q2
