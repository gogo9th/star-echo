# !/usr/bin/env python3

# System Libraries
import os, sys, glob, shutil, errno
from os import walk
import numpy as np
import shutil
import pathlib
import multiprocessing

# Audio Libraries
from scipy.io import wavfile
from scipy.signal import fftconvolve, resample
from pydub import AudioSegment
import soundfile as sf
import subprocess

# Media File Tag Libraries
import mimetypes
from mutagen.flac import FLAC, Picture
from mutagen.mp3 import MP3
from mutagen.id3 import ID3, APIC, error
import time
import ffmpeg


def is_mp3(path):
    try:
        mp3file = MP3(path, ID3=ID3)
        mp3file.save()
        return 1
    except:
        return 0


def is_flac(path):
    #	with open(path, "rb") as file:
    #		info = fleep.get(file.read(128))
    #	file.close()
    #	return ("audio/flac" in info.mime)
    try:
        flacfile = FLAC(path)
        flacfile.save()
        return 1
    except:
        return 0


def print_mime(path):
    with open(path, "rb") as file:
        info = fleep.get(file.read(128))
    file.close()
    print("Mime Type Unknown: " + path + " : " + str(info.mime))


def copy_cover(src, dst):
    if is_flac(dst):
        flacfile = FLAC(dst)
        for p in flacfile.pictures:
            if p.type == 3:  # front cover already exists
                flacfile.save()
                return
    elif is_mp3(dst):
        mp3file = MP3(dst, ID3=ID3)  # mutagen can automatically detect format and type of tags
        if not mp3file:
            return
        if 'APIC:' in mp3file.tags:
            apicTag = mp3file.tags['APIC:']  # access APIC frame and grab the image
            mp3file.save()
            if apicTag.mime != None and apicTag.data != None and apicTag.type == 3:
                return
    else:
        return

    imgFilename = ""
    picture = None
    if is_flac(src):
        found = False
        flacfile = FLAC(src)
        for p in flacfile.pictures:
            if p.type == 3:  # front cover
                picture = p
                imgFilename = os.path.basename(dst) + "." + os.path.basename(p.mime)
                with open(imgFilename, "wb") as f:
                    f.write(p.data)
                    f.close()
                    found = True
                    break
        flacfile.save()
        if found == False:
            return
    elif is_mp3(src):
        mp3file = MP3(src, ID3=ID3)  # mutagen can automatically detect format and type of tags
        if not mp3file:
            return
        if 'APIC:' not in mp3file.tags:
            return
        apicTag = mp3file.tags['APIC:']  # access APIC frame and grab the image
        if apicTag.mime == None:
            return
        if apicTag.data == None:
            return
        if apicTag.type != 3:
            return
        imgFilename = os.path.basename(dst) + '.' + os.path.basename(apicTag.mime)
        with open(imgFilename, 'wb') as img:
            img.write(apicTag.data)  # write artwork to new image
            img.close()
        mp3file.save()
    else:
        # print_mime(src)
        return

    if is_flac(dst):
        flacfile = FLAC(dst)
        image = None
        if (picture != None):
            image = picture
        else:
            image = Picture()
            image.type = 3
            image.mime = "audio/flac"
        # image.desc = picture.desc
        with open(imgFilename, 'rb') as f:
            image.data = f.read()
        flacfile.clear_pictures()
        flacfile.add_picture(image)
        flacfile.save()
    elif is_mp3(dst):
        mp3file.tags.clear()
        with open(imgFilename, 'rb') as img:
            mp3file.tags.add(
                APIC(
                    encoding=apicTag.encoding,  # 3 is for utf-8
                    mime=apicTag.mime,  # image/jpeg or image/png
                    type=3,  # 3 is for the cover image
                    desc=apicTag.desc,
                    data=img.read()
                )
            )
        img.close()
        mp3file.save()
    else:
        # print_mime(src)
        return

    os.remove(imgFilename)


def copy_flac_cover(src, dst):
    flacfile = FLAC(src)

    picture = None
    found = False
    for p in flacfile.pictures:
        if p.type == 3:  # front cover
            picture = p
            with open("temp." + os.path.basename(p.mime), "wb") as f:
                f.write(p.data)
                f.close()
                found = True
                break
    flacfile.save()

    if found == False:
        return

    flacfile = FLAC(dst)
    image = picture
    #	image.type = 3
    #	image.mime = picture.mime
    #	image.desc = picture.desc
    with open("temp." + os.path.basename(picture.mime), 'rb') as f:
        image.data = f.read()
    flacfile.clear_pictures()
    flacfile.add_picture(image)
    flacfile.save()
    os.remove("temp." + os.path.basename(picture.mime))


def copy_mp3_cover(src, dst):
    mp3file = MP3(src, ID3=ID3)  # mutagen can automatically detect format and type of tags
    if not mp3file:
        return
    if 'APIC:' not in mp3file.tags:
        return
    apicTag = mp3file.tags['APIC:']  # access APIC frame and grab the image
    if apicTag.mime == None:
        return
    if apicTag.data == None:
        return
    if apicTag.type != 3:
        return

    imgFilename = 'temp.' + os.path.basename(apicTag.mime)
    with open(imgFilename, 'wb') as img:
        img.write(apicTag.data)  # write artwork to new image
        img.close()

    mp3file = MP3(dst, ID3=ID3)
    mp3file.tags.clear()
    with open(imgFilename, 'rb') as img:
        mp3file.tags.add(
            APIC(
                encoding=apicTag.encoding,  # 3 is for utf-8
                mime=apicTag.mime,  # image/jpeg or image/png
                type=3,  # 3 is for the cover image
                desc=apicTag.desc,
                data=img.read()
            )
        )
    img.close()
    mp3file.save()
    os.remove(imgFilename)


def file_to_wavs(filepath, isFilter=False):
    _fname, ext = os.path.splitext(filepath)
    ext = ext[1:]
    
    if ext == "wav":
        sound = AudioSegment.from_wav(filepath)
        _arr = np.array(sound.get_array_of_samples(), dtype=dtype)
    elif ext == 'mp3':
	    # Add a 2-sec silence
        sound = AudioSegment.from_mp3(filepath)
    if ext == "flac":
	    # Add a 2-sec silence
        sound = AudioSegment.from_file(filepath, "flac")

    _arr = np.array(sound.get_array_of_samples(), dtype=dtype)
    nparr = np.zeros((int(len(_arr) / 2), 2), dtype=dtype)
    nparr[:, 0] = _arr[0::2]
    nparr[:, 1] = _arr[1::2]
    print("Old dBFS 3: " + str(sound.dBFS))
    return sound.frame_rate, nparr, sound.dBFS


def wavs_to_file(filepath, fs, data):
    _fname, ext = os.path.splitext(filepath)
    ext = ext[1:]
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    pathlib.Path(filepath + "-INCOMPLETE").touch(exist_ok=True)
    #open(, 'a').close()
    wavfile.write(filepath, fs, data)
    song = AudioSegment.from_wav(filepath)
    one_sec_segment = AudioSegment.silent(duration=1000)
    final_song = song.fade_out(2000) + one_sec_segment
    final_song.export(filepath.replace('.wav','.flac'), format="flac")
	

    os.remove(filepath + "-INCOMPLETE")




if __name__ == "__main__":


    dtype = 'int16'

    if len(sys.argv) != 3:
        print('Usage: ' + sys.argv[0] + ' <reference volume music file> <target music file to match>')
        sys.exit(0)
    
    inpath = sys.argv[1]
    outpath = sys.argv[2]
    gained_outpath = outpath + " - GAINED.flac"

    fs_src, wav_src, dBFS_src = file_to_wavs(sys.argv[1])
    fs_dst, wav_dst, dBFS_dst = file_to_wavs(sys.argv[2])

    # type casting and max calibration
    #dtype_dict = {'float32': 1.0, 'int32': 2147483648, 'int16': 32768, 'uint8': 128}

    max_src = np.amax(np.absolute(wav_src))
    max_dst = np.amax(np.absolute(wav_src))

    #wavs_to_file(outpath, fs_src, conved)

    _fname, ext = os.path.splitext(outpath)
    ext = ext[1:]
    
    if ext == "wav":
        audio_file = AudioSegment.from_wav(outpath)

    elif ext == "flac":
        audio_file = AudioSegment.from_file(outpath, "flac")

    else:
        audio_file = AudioSegment.from_mp3(outpath)

    print("dBFS: " + str(dBFS_dst) + " -> " + str(dBFS_src))
    audio_file = audio_file.apply_gain(dBFS_src - dBFS_dst)

    audio_file.export(gained_outpath, format="flac")

    copy_cover(outpath, gained_outpath)
       
