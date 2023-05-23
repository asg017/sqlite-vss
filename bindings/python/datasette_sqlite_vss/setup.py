from setuptools import setup

version = {}
with open("datasette_sqlite_vss/version.py") as fp:
    exec(fp.read(), version)

VERSION = version['__version__']

setup(
    name="datasette-sqlite-vss",
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
    packages=["datasette_sqlite_vss"],
    entry_points={"datasette": ["sqlite_vss = datasette_sqlite_vss"]},
    install_requires=["datasette", "sqlite-vss"],
    extras_require={"test": ["pytest"]},
    python_requires=">=3.7",
)