# ldf2root

`ldf2root` is a tool for converting LDF files used by the ORNL-HRIBF nuclear physics group's Poll data acquisition system

## Credit

GenScan by @truland and @tking53 here: https://github.com/tking53/genScan
- This project relies on the LDFPixieTranslator for parsing the UTK PAASS Pixie16 LDF file format.
- It also uses the Translator and DataParser classes for managing and reading in files.

FRIBDAQ/DDASFormat by @aschester and @rfoxkendo: https://github.com/FRIBDAQ/DDASFormat
- This project uses the DDASHitUnpacker to convert raw data streams to DDASHit objects
- Also uses the DDASRootHit and DDASRootEvent classes from NSCLDAQ/main/ddas
/ddasdumper for saving the objects to .root files for later analysis using the ROOT Data Analysis software here: https://github.com/root-project/root

Both of these projects are the result of decades of work and experience on the data acquisition systems in nuclear physics using the Pixie16 modules by XIA, LLC.

## Features

- Converts LDF files to ROOT format
- Supports batch processing of multiple files
- Customizable output options

## Requirements

- C++ compiler (e.g., g++)
- [ROOT](https://root.cern/) installed
- spdlog https://github.com/gabime/spdlog

## Installation

1. Clone the project
    ```bash
    git clone https://github.com/yourusername/ldf2root.git
    cd ldf2root
    ```
2. Create a build directory, configure and compile:
   ```bash
   mkdir build
   cd build
   ccmake ..
   make
   make install
   ```
4. Load project modulefile:
    You will need to load the modulefile located in install/modulefile/ldf2root
    You can either copy this to $HOME/modulefiles or another location you use modulefiles and run
   ```bash
   module use $HOME/modulefiles
   module load ldf2root
   ```


## Usage
To see all available options, run:

```bash
ldf2root --help
```

### Command-line Arguments

- `-i, --input <file>`: Specify input LDF file (required)
- `-c, --config <file>`: Use a configuration file for custom settings (required)
- `-o, --output <file>`: Specify output ROOT file (optional; defaults to `<input>.root`)
- `-h, --help`: Show help message and exit
- `--generate-config`: generate an example config file
- `--tree-name`: Specify output root file a tree name other than the default
- `--branch-name`: Specify the output root file a branch name 
- `--log-file`: Save log files
- `--silent`: Surpress all command line output

**Example:**

Minimal usage
```bash
ldf2root -i data.ldf -c settings.conf
```
Custom usage
```bash
ldf2root -i data.ldf -o custom-out.root --tree-name <tree-name> -c settings.conf
```

## Contributing

Pull requests are welcome. For major changes, please open an issue first.

## License

This project is licensed under the MIT License.