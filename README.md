# Convert-img
A high performance multi-threaded cli image conversion tool based on [ImageMagick](https://github.com/ImageMagick/ImageMagick).

> This project is for my personal use, but I hope it can help you. If you have any questions, please submit an issue.

*This project is still in the experimental stage, and the API may change at any time.*

## Features
- [x] Support for converting images in batches
- [x] Support for multi-threaded conversion
- [x] Support conversion of scale, resize, quality, format.


## Installation
Download the latest version of the binary file from the [release](https://github.com/sean1832/cli-convert-img/releases/latest) page. You can choose between `cpp` and `cs` versions. 

### `cpp` vs `cs`
The `cpp` version uses `GraphicsMagick` as the underlying library, and the `cs` version uses `ImageMagick` as the underlying library. 

>Performance wise I found `ImageMagick` is faster wise than `GraphicsMagick` so I recommend using the `cs` version. 
> `cpp` version is still experimental but still available for use.


## Usage
```
convert-img [input] [output] [options]
```
- `-i` or `input-ext`: Set the input extension to filter
- `-o` or `output-ext`: Set the output extension to export
- `-s` or `--scale` : Scale the image by the given percentage (`0.1`, `2.0`).  
- `-q` or `--quality` : Set the image quality. (`1`-`100`)
- `-c` or `--compression` : Set the compression algorithm. (`none`, `lzw`, `zip`, `jpeg`, `webp`)
- `-t` or `--threads` : Set the number of threads to use. (`1`-`system max`)  

- `--version` : Print the version number.  
- `--help` : Print the help message.