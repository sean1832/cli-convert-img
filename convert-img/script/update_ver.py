import json
import os

# variables
project_name = 'convert-img'

# Get the path of the parent directory
working_dir = f"{os.getcwd()}"

# Construct the path to the config.json file in the parent directory
manifest_path = os.path.join(working_dir, 'manifest.json')

# Load the config file
with open(manifest_path, 'r') as f:
    config = json.load(f)

# Replace the version info in the resource file
with open(f'{working_dir}/{project_name}.rc.temp', 'r', encoding='utf-16') as f:
    resource_content = f.read()

version_comma = config['version'].replace('.', ',')

resource_content = resource_content.replace('<Product name>', config['name'])
resource_content = resource_content.replace('<Author>', config['author'])
resource_content = resource_content.replace('<Description>', config['description'])
resource_content = resource_content.replace('<Version comma>', version_comma)
resource_content = resource_content.replace('<Version>', config['version'])
resource_content = resource_content.replace('<Copyright>', config['copyright'])

print(f"Product name: {config['name']}")
print(f"Author: {config['author']}")
print(f"Description: {config['description']}")
print(f"Version: {config['version']}")
print(f"Copyright: {config['copyright']}")


# write the template file to the resource file
with open(f'{working_dir}/{project_name}.rc', 'w', encoding="utf-16") as f:
    f.write(resource_content)

print('Resource file updated')