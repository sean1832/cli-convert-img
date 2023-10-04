using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ImageMagick;
using CommandLine;
using System.Diagnostics;
using System.Runtime.InteropServices;

class Options
{
    [Value(0, MetaName = "input", Required = true, HelpText = "Input file path")]
    public string Input { get; set; }

    [Value(1, MetaName = "output", Required = true, HelpText = "Output file path")]
    public string Output { get; set; }

    [Option('s', "scale", Required = false, HelpText = "Scale of output image (0.1, 2.0)")]
    [Range(0.1, 2.0)]
    public double Scale { get; set; } = 1;

    [Option('q', "quality", Required = false, HelpText = "Quality of output image (1, 100)")]
    [Range(1, 100)]
    public int Quality { get; set; } = 80;

    [Option('c', "compression", Required = false, HelpText = "Compression algorithm (none, lzw, zip, jpeg, webp)")]
    public string Compression { get; set; } = null;

    [Option('f', "force", Required = false, HelpText = "Force override output file")]
    public bool OverrideMode { get; set; } = false;

    [Option('i', "input-ext", Required = false, HelpText = "Input file extension")]
    public string InputExt { get; set; }

    [Option('o', "output-ext", Required = false, HelpText = "Output file extension")]
    public string OutputExt { get; set; }

    [Option('t', "thread", Required = false, HelpText = "Manually specify how many threads to use")]
    public int? Thread { get; set; }
}

namespace convert_worker
{
    internal class Program
    {
        static int Main(string[] args)
        {
            try
            {
                Parser.Default.ParseArguments<Options>(args).WithParsed<Options>(o =>
                {
                    // check if input is directory
                    bool isInputDir = Directory.Exists(o.Input);
                    bool isInputFile = File.Exists(o.Input);
                    if (!isInputDir && !isInputFile)
                    {
                        throw new FileNotFoundException($"Input path \"{o.Input}\" is either a file path nor directory.");
                    }
                    CreateDirNotExist(o.Output);
                    Stopwatch stopwatch = new Stopwatch();
                    stopwatch.Start();
                    if (isInputFile)
                    {
                        string ext = Path.GetExtension(o.Input);
                        WarnTiff(ext, o.Quality);
                        if (!File.Exists(o.Input))
                        {
                            throw new FileNotFoundException($"Input path \"{o.Input}\" not exist");
                        }

                        Convert(o.Input, o.Output, o.Scale, o.Quality, o.OverrideMode, o.Compression);
                    }
                    else
                    {
                        WarnTiff(o.InputExt, o.Quality);
                        ConvertDir(o.Input, o.Output, o.InputExt, o.OutputExt, o.Scale, o.Quality, o.OverrideMode, o.Thread, o.Compression);
                    }
                    stopwatch.Stop();
                    Console.WriteLine($"Time elapsed: {stopwatch.ElapsedMilliseconds / (double)1000} Second");

                });
                Console.WriteLine("Done.");
                return 0;
            }
            catch (Exception e)
            {
                Console.WriteLine($"Error: {e.ToString()}");
                return 1;
            }

            void WarnTiff(string ext, int quality)
            {
                if (ext != ".tiff" && ext != ".tif") return;
                if (quality != 80)
                {
                    Console.WriteLine($"Warning: tiff file detected, quality parameter will be ignore. " +
                                      $"Please use '-c' or '-compression' to specify different compression methods for .tiff format in order to change image quality.");
                }
            }

            string GetNewFileName(string path)
            {
                string newPath = path;

                int count = 0;
                while (File.Exists(newPath))
                {
                    int lastDotIndex = path.LastIndexOf('.');
                    string nameWithoutExtension = path.Substring(0, lastDotIndex);
                    string extension = path.Substring(lastDotIndex);

                    newPath = $"{nameWithoutExtension}_{count}{extension}";

                    count++;
                }
                return newPath;
            }

            void CreateDirNotExist(string path)
            {
                string dir = Path.GetDirectoryName(path);
                if (!Directory.Exists(dir))
                {
                    Directory.CreateDirectory(dir);
                }
            }

            void SetCompression(MagickImage image, string compressionMethod)
            {
                switch (compressionMethod?.ToLower())
                {
                    case "none":
                        image.SetCompression(CompressionMethod.NoCompression);
                        break;
                    case "lzw":
                        image.SetCompression(CompressionMethod.LZW);
                        break;
                    case "zip":
                        image.SetCompression(CompressionMethod.Zip);
                        break;
                    case "jpeg":
                        image.SetCompression(CompressionMethod.JPEG);
                        break;
                    case "webp":
                        image.SetCompression(CompressionMethod.WebP);
                        break;
                    default:
                        throw new ArgumentException($"Unsupported TIFF compression method: {compressionMethod}");
                }
            }

            void Convert(string input, string output, double scale, int quality, bool overrideMode, string compression)
            {
                Console.WriteLine($"Converting '{input}' to '{output}'");
                using (MagickImage image = new MagickImage(input))
                {
                    image.Resize((int)(image.Width * scale), (int)(image.Height * scale));
                    image.Quality = quality;
                    string outputPath = output;
                    if (compression != null) SetCompression(image, compression);
                    if (!overrideMode)
                    {
                        outputPath = GetNewFileName(outputPath);
                    }
                    image.Write(outputPath);
                }
            }

            void ConvertDir(string inputDir, string outputDir, string inputExt, string outputExt, double scale, int quality, bool overrideMode, int? threadNum, string compression)
            {
                if (!Directory.Exists(inputDir))
                {
                    throw new DirectoryNotFoundException($"Input directory \"{inputDir}\" not exist");
                }

                string[] files = Directory.GetFiles(inputDir);
                string[] filteredFiles = files;
                foreach (string file in files)
                {
                    if (inputExt != null && Path.GetExtension(file) != inputExt)
                    {
                        filteredFiles.ToList().Remove(file);
                    }
                }

                int maxThreadNum = threadNum ?? Environment.ProcessorCount;
                var parallelOptions = new ParallelOptions { MaxDegreeOfParallelism = maxThreadNum };
                Parallel.ForEach(filteredFiles, parallelOptions, file =>
                {
                    string fileName = Path.GetFileNameWithoutExtension(file) + (outputExt ?? Path.GetExtension(file));
                    string outputPath = Path.Combine(outputDir, fileName);
                    Convert(file, outputPath, scale, quality, overrideMode, compression);
                });
            }
        }
    }
}
