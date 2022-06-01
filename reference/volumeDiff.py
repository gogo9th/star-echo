import sys
from pathlib import Path
from pydub import AudioSegment


def db_to_float(db, using_amplitude=True):
    """
    Converts the input db to a float, which represents the equivalent
    ratio in power.
    """
    db = float(db)
    if using_amplitude:
        return 10 ** (db / 20)
    else:  # using power
        return 10 ** (db / 10)


def dbDiff(file1: str, file2: str):
    sound1 = AudioSegment.from_mp3(file1)
    sound2 = AudioSegment.from_mp3(file2)

    diff = db_to_float(sound1.dBFS - sound2.dBFS)
    print(f"[{Path(file1).name}] dBFS: {sound1.dBFS}, {sound2.dBFS}  gain: {diff}")

    return diff


if __name__ == "__main__":
    dir1 = sys.argv[1]
    dir2 = sys.argv[2]

    if dir1 == '' or dir2 == '':
        print('Provide 2 directories with audio files')
        sys.exit()

    differences = []
    for file in Path(dir1).glob('*'):
        differences.append(dbDiff(file, Path(dir2) / file.name))

    print(f"Average gain: {sum(differences) / len(differences)}")

