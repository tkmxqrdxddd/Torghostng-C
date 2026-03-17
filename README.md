# Torghostng-C

Remake of torghostng in C.

This script belongs to [SusmithKrishnan](https://github.com/SusmithKrishnan/torghost).
I just remade it into actually usable language.
I may and may not maintain this repo, if there is something you truly need or if it doesn't work, the surest way to obtain it is by forking the repo and doing it yourself.

## Dependencies

- `gcc`
- `libcurl-dev` (or `libcurl-devel` on RHEL-based systems)

## Building

### Using Make

```bash
make
```

### Manual compilation

```bash
gcc -Wall -Wextra -O2 -o torghostng torghostng.c -lcurl
```

## Installation

### Using install script

```bash
git clone https://github.com/tkmxqrdxddd/Torghostng-C
cd Torghostng-C
sudo chmod +x install.sh
sudo ./install.sh
```

### Using Make

```bash
sudo make install
```

## Usage

| Command           | Description |
| ----------------- | ----------- |
| `torghostng --help` | Prints usage |
| `torghostng --version` | Display version |
| `torghostng -s` | Start the Tor proxy |
| `torghostng -s DE` | Start with German exit node |
| `torghostng -x` | Stop the Tor proxy |
| `torghostng -r` | Renew Tor circuit |
| `torghostng -c` | Check Tor connection and IP |

## Examples

```bash
# Start with default exit node
sudo torghostng --start

# Start with specific exit node (Germany)
sudo torghostng --start DE

# Renew Tor circuit
sudo torghostng --renew

# Check connection
torghostng --check

# Stop Tor proxy
sudo torghostng --stop
```

## Note

This program must be run as root for network configuration changes.
