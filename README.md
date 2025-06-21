# ldf2root

`ldf2root` is a tool for converting LDF (List Data Format) files into ROOT files for further analysis in high-energy or nuclear physics experiments.

## Features

- Converts LDF files to ROOT format
- Supports batch processing of multiple files
- Customizable output options

## Requirements

- C++ compiler (e.g., g++)
- [ROOT](https://root.cern/) framework installed

## Installation

```bash
git clone https://github.com/yourusername/ldf2root.git
cd ldf2root
make
```

## Usage
To see all available options, run:

```bash
./ldf2root --help
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
./ldf2root -i data.ldf -c settings.conf
```
Custom usage
```bash
./ldf2root -i data.ldf -o custom-out.root --tree-name <tree-name> -c settings.conf
```

## Contributing

Pull requests are welcome. For major changes, please open an issue first.

## License

This project is licensed under the MIT License.