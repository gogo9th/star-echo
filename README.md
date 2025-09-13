
# Star Echo - Applying Filters to Music Files & Windows Audio Streams

## Software Description

- When the software is executed, all music extension files (`.flac, .mp3, .wav, .wma`) in the current directory are applied the filter's sound effect and their converted new music files are saved in the `"FINAL"` folder created in the same directory.

- All sub-directory structures of original music files are preserved and saved in the `"FINAL"` folder. 

- Already converted music files in the past (whose converted version already exists in the `"FINAL"` folder) are skipped. 

- A pre-built Windows software is available:  [`star_echo.exe`](https://github.com/gogo9th/star-echo/blob/main/star_echo.exe) (but might get blocked by Windows Defender, so you need to turn it off).

- Music File Conversion Web Server: [http://winterstar.org:3000](http://winterstar.org:3000)

## Ubuntu Installation (minimum 20.04 required)

<b><u>Step 1.</u></b> Install the boost and ffmpeg libraries.
```console
    $ sudo apt install ffmpeg libboost-program-options-dev libavformat-dev libavcodec-dev libavutil-dev libswresample-dev
```

<b><u>Step 2.</u></b> Download the source code.
```console
    $ git clone https://github.com/gogo9th/star-echo
```

<b><u>Step 3.</u></b> Compile the source code by using one of the following 2 options:

* <u>*Option A:*</u> Use Makefile to compile.
```console
    $ cd star-echo
    $ make
```
* <u>*Option B:*</u> Use CMake to compile.
```console
    $ cd star-echo
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
```

<b><u>Step 4.</u></b> Run the created `star-echo`.
```console
$ ./q2cathedral --help 
Options:
  -h [ --help ]         Display the help message.
  -i [ --input ] arg    Input file(s)/directory.
                        - [Default: the current directory]
  -o [ --output ] arg   If the input is a directory, then output should be a
                        directory.
                        If the input is a file, then the output should be a
                        filename.
                        If the inputs are multiple files, then this option is
                        ignored.
                        - [Default: './FINAL' directory]
  -t [ --threads ] arg  The number of CPU threads to run.
                        - [Default: the processor's available total cores]
  -k [ --keep-format ]   Keep each output file's format the same as its source
                        file's.
                        - [Default: the output format is .flac]
  -w [ --overwrite ]    overwrite output file if it exists [Default: false]
  -n [ --normalize ]    normalize the sound to avoid rips [Default: false]
  -s [ --silence ]      Append silence in seconds [Default: 0]
  -f [ --filter ] arg   Filter(s) to be applied:
                         CH[,roomSize[,gain]] - Cathedral,
                           Default is 'CH,10,9' if parameters omitted
                         EQ,b1,b2,b3,b4,b5,b6,b7 - Equalizer,
                           0<=b<=24, b=12 is '0 gain'
                         3D,strength,reverb,delay - 3D effect
                           0 <= parameters <= 9
                         BE,level,cutoff - Bass enhancement
                           1 <= parameters <= 15
                        Predefined filters:
                         studio,
                         rock,
                         classical,
                         jazz,
                         dance,
                         ballad,
                         club,
                         RnB,
                         cafe,
                         livecafe,
                         concert,
                         cathedral,
                         upscaling


# Recursive folder-wide cathedral .flac conversion into the folder FINAL
$ ./star-echo -i musicFolder    
```


## Windows Installation

<b><u>Step 1.</u></b> Go and install [CMake](https://cmake.org/install/)

<b><u>Step 2.</u></b> Go and install [Visual Studio C++ Development](https://visualstudio.microsoft.com/ko/downloads/)

<b><u>Step 3. Go and install [Git](https://git-scm.com/downloads/win)

<b><u>Step 4.</u></b> Install vcpkg. Open the PowerShell.

```console
    $ cd ~
    $ git clone https://github.com/Microsoft/vcpkg.git 
    $ cd vcpkg
    $ ./bootstrap-vcpkg.bat
```

<b><u>Step 5.</u></b> Install the boost and ffmpeg libraries

```console
    $ ./vcpkg.exe install boost-program-options boost-circular-buffer boost-algorithm ffmpeg[avformat] ffmpeg[avcodec] ffmpeg[swresample] ffmpeg[zlib] --triplet x64-windows
    $ ./vcpkg.exe integrate install
```
(For x32 builds, run `"vcpkg install boost-program-options default"` instead)


<b><u>Step 6.</u></b> Compile the source code.
```console
    $ cd star-echo
    $ mkdir build
    $ cd build
      # Replace <Windows user name> to your home directory's user name
      # Must have double quotes around the path
      # If compilation fails, you must run "rm * .* -rf" to delete all previous CMake caches
    $ cmake .. -DCMAKE_TOOLCHAIN_FILE="C:\Users\<windows user name>\vcpkg\scripts\buildsystems\vcpkg.cmake"
    $ cmake --build . --config Release
```

<b><u>Step 7.</u></b> Check if `star_echo.exe` is created in the Release folder.

<b><u>Step 8.</u></b> Pack `.exe` and `.dll` files into a single `.exe` file.

- Download and install [Enigma Virtual Box](https://enigmaprotector.com/en/downloads.html).

- Run Enigma Virtual Box.

- Choose the input file to be `star_echo.exe` in the Release folder.

- Add all 6 .dll files in the Release folder and press OK: `avcodec-\*.dll`, `avformat-\*.dll`, `avutil-\*.dll`, `boost_program_options-\*.dll`, `swresample-\*.dll`, `zlib*.dll`.

- Click "Process" to create `star_echo_boxed.exe`.


<b><u>Step 9.</u></b> Run the created `star_echo_boxed.exe`.

* <u>Option 1:</u> Use a terminal. 
```console
    $ cd Release
    $ ./star_echo_boxed.exe --help 
```
* <u>Option 2:</u> Double-click the generated `star_echo_boxed.exe`, then all music files in the same directory will be converted and stored in the `"FINAL"` folder.




## "StarEcho" Dynamic Audio Engine Installation (Windows)

<b><u>Step 1.</u></b> Go and install the [Wix toolset V3 or higher](https://wixtoolset.org/releases/) (both `wix*.exe` and Visual Studio Extension, separately).

<b><u>Step 2.</u></b> Go and install [Visual Studio C++ Development](https://visualstudio.microsoft.com/ko/downloads/).

<b><u>Step 2.</u></b> Go and install [PowerShell Core](https://learn.microsoft.com/en-us/powershell/scripting/install/installing-powershell-on-windows?view=powershell-7.3#install-powershell-using-winget-recommended).

<b><u>Step 4.</u></b> Build the installer at .

```console
    $ cd star-echo/StarEcho
    $ & "C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe" .\StarEcho.sln
    $ & "C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe" .\StarEcho.sln /p:Configuration="Release"
    $ & "C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe" .\StarEcho.sln /p:Configuration="Debug"
```

<b><u>Step 5.</u></b> The installer is created at `star-echo-main\star-echo-main\StarEcho\Setup\bin\x64\Debug\Setup.msi` and `star-echo-main\star-echo-main\StarEcho\Setup\bin\x64\Release\Setup.msi`.



## Music Converter Web Server Setup

<b><u>Step 1.</u></b> 
```console
	$ cd web-server
	$ npm install
```

<b><u>Step 2.</u></b> Install libssl 1.1 and MongoDB ([https://blog.stackademic.com/mongodb-cluster-setup-on-ubuntu-23-04-x64-223193fcdb5e](https://blog.stackademic.com/mongodb-cluster-setup-on-ubuntu-23-04-x64-223193fcdb5e))
```console
	$ wget http://archive.ubuntu.com/ubuntu/pool/main/o/openssl/libssl1.1_1.1.1f-1ubuntu2_amd64.deb
	$ dpkg -i libssl1.1_1.1.1f-1ubuntu2_amd64.deb
	$ apt update
	$ apt install libssl1.1

	$ wget -qO - https://www.mongodb.org/static/pgp/server-6.0.asc | sudo apt-key add -
	$ echo "deb [ arch=amd64,arm64 ] https://repo.mongodb.org/apt/ubuntu focal/mongodb-org/6.0 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-6.0.list
	$ apt update
	$ apt install -y mongodb-org
	$ sudo systemctl enable mongod
	$ sudo systemctl start mongod
	$ sudo systemctl status mongod
```
<b><u>Step 3.</u></b> Run the web server (at port 3000).
```console
	$ node app.js --http
        ### or use pm2 ###
        $ pm2 kill
        $ pm2 resurrect
        $ pm2 start app.js --http
```
