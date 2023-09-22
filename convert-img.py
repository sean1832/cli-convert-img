from PIL import Image
import click
import os


def convert(input_file, output_file):
    try:
        with Image.open(input_file) as img:
            img.save(output_file)
    except Exception as e:
        print(f"Error converting {input_file} to {output_file}: {e}")


@click.group()
def command_group():
    """Converts images to other formats. You can convert a single image or a batch of images."""
    pass


@click.command('single')
@click.argument('input_file', type=click.Path(exists=True), required=True)
@click.argument('output_file', type=click.Path(exists=False), required=False)
@click.option('-of', '--output_format', '-of', help='Output format (e.g. png)', required=False)
def single_convert(input_file, output_file, output_format):
    """Converts a single image file to another format.
    if output_file is not specified, the output file will be the same as the input file with the new extension."""
    if not output_format and not output_file:
        raise click.BadParameter("Either output format or output file must be specified.")
    if not output_file:
        output_file = f"{os.path.splitext(input_file)[0]}.{output_format}"

    path = os.path.dirname(output_file)
    if not os.path.exists(path):
        os.makedirs(path)
        print(f"Path: {path} not found. Created new directory.")
    convert(input_file, output_file)
    print(f"Converted {input_file} to {output_file}")


@click.command('batch')
@click.argument('input_directory', type=click.Path(exists=True), required=True)
@click.argument('output_directory', type=click.Path(exists=False), required=True)
@click.option('-if', '--input_format', help='Filter formats (e.g. png, jpg)', required=False)
@click.option('-of', '--output_format', help='Output format (e.g. png)', required=True)
def batch_convert(input_directory, output_directory, output_format, input_format=None):
    """Converts all images in a directory to another format.
    If input_format is specified, only images with the specified format will be converted."""
    if input_format:
        filters = [x.strip() for x in input_format.split(',')]
    else:
        filters = None

    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

    for root, dirs, files in os.walk(input_directory):
        for file in files:
            file_extension = file.lower().split('.')[-1]
            if filters and file_extension not in filters:
                continue
            if file_extension in ['png', 'jpg', 'jpeg', 'gif', 'bmp', 'tiff', 'webp', 'heif', 'pdf', 'eps', 'raw']:
                input_file = os.path.join(root, file)
                filename_without_extension = os.path.splitext(file)[0]
                output_file = os.path.join(output_directory, f"{filename_without_extension}.{output_format}")
                convert(input_file, output_file)
                print(f"Converted {input_file} to {output_file}")


if __name__ == "__main__":
    command_group.add_command(single_convert)
    command_group.add_command(batch_convert)
    command_group()
