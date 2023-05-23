from setuptools import setup, Extension
import os
import platform

version = {}
with open("sqlite_vss/version.py") as fp:
    exec(fp.read(), version)

VERSION = version['__version__']

system = platform.system()
machine = platform.machine()

print(system, machine)

if system == 'Darwin':
  if machine not in ['x86_64', 'arm64']:
    raise Exception("unsupported platform")  
elif system == 'Linux':
  if machine not in ['x86_64']:
    raise Exception("unsupported platform")
#elif system == 'Windows':
else: 
  raise Exception("unsupported platform")

setup(
    name="sqlite-vss",
    description="",
    long_description="",
    long_description_content_type="text/markdown",
    author="Alex Garcia",
    url="https://github.com/asg017/sqlite-vss",
    project_urls={
        "Issues": "https://github.com/asg017/sqlite-vss/issues",
        "CI": "https://github.com/asg017/sqlite-vss/actions",
        "Changelog": "https://github.com/asg017/sqlite-vss/releases",
    },
    license="MIT License, Apache License, Version 2.0",
    version=VERSION,
    packages=["sqlite_vss"],
    package_data={"sqlite_vss": ['*.so', '*.dylib', '*.dll']},
    install_requires=[],
    # Adding an Extension makes `pip wheel` believe that this isn't a 
    # pure-python package. The noop.c was added since the windows build
    # didn't seem to respect optional=True
    ext_modules=[Extension("noop", ["noop.c"], optional=True)],
    extras_require={"test": ["pytest"]},
    python_requires=">=3.7",
)