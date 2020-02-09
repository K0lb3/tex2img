from setuptools import setup, Extension, find_packages
import os

with open("README.md", "r") as fh:
	long_description = fh.read()

setup(
      name = "tex2img",
	description="a texture decompression C++-extension for Python",
	author="K0lb3",
      version = "0.8.0",
	keywords=['astc', 'atc', 'pvrtc', "etc", "crunch"],
	classifiers=[
		"License :: OSI Approved :: MIT License",
		"Operating System :: OS Independent",
		"Intended Audience :: Developers",
		"Programming Language :: Python",
		"Programming Language :: Python :: 3",
		"Programming Language :: Python :: 3.6",
		"Programming Language :: Python :: 3.7",
            "Programming Language :: Python :: 3.8",
		"Topic :: Multimedia :: Graphics",
	],
      url="https://github.com/K0lb3/tex2img",
	download_url="https://github.com/K0lb3/tex2img/tarball/master",
	long_description=long_description,
	long_description_content_type="text/markdown",
      ext_modules = [
            Extension(
                  "tex2img", 
                  [
                        os.path.join(root, f)
                        for root, dirs, files in os.walk("src")
                        for f in files
                        if f[-3:] in ["cpp", "cxx"] and not f in ["basisu_tool.cpp"]
                  ],
                  language = "c++",
                  extra_compile_args=["-std=c++11"]
      )]
)