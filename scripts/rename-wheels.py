# This file is a small utility that rename all .whl files in a given directory
# and "generalizes" them. The wheels made by python/sqlite_ulid contain the 
# pre-compiled sqlite extension, but those aren't bound by a specfic Python 
# runtime or version, that other wheels might be. So, this file will rename
# those wheels to be "generalized", like replacing "c37-cp37" to "py3-none".
import sys
import os
from pathlib import Path

wheel_dir = sys.argv[1]

is_macos_arm_build = '--is-macos-arm' in sys.argv 

for filename in os.listdir(wheel_dir):
  filename = Path(wheel_dir, filename)
  if not filename.suffix == '.whl':
    continue
  new_filename = (filename.name
    .replace('cp37-cp37', 'py3-none')
    .replace('cp38-cp38', 'py3-none')
    .replace('cp39-cp39', 'py3-none')
    .replace('cp310-cp310', 'py3-none')
    .replace('cp311-cp311', 'py3-none')
    .replace('linux_x86_64', 'manylinux_2_17_x86_64.manylinux2014_x86_64.manylinux1_x86_64')
    
    
  )
  if is_macos_arm_build:
    new_filename = (new_filename
    .replace('macosx_12_0_universal2', 'macosx_11_0_arm64')
    .replace('macosx_13_0_universal2', 'macosx_11_0_arm64')
  )
  else:
    new_filename = (new_filename
    .replace('macosx_12_0_universal2', 'macosx_10_6_x86_64')
    .replace('macosx_12_0_x86_64', 'macosx_10_6_x86_64')
  )
  
  os.rename(filename, Path(wheel_dir, new_filename))