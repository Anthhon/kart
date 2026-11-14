# kart

`kart` is a lightweight project manager designed to simplify the process of creating, configuring, building and running C projects.

Instead of introducing a complex build system or requiring extensive configuration, kart provides a straightforward workflow **built around a directory structure and a single TOML configuration file**.

## Getting Started

### Building kart

Clone the repo and bootstrap the project for the first time:

```sh
git clone https://github.com/Anthhon/kart.git
cd kart
mkdir -p target
./first_compile.sh
```

The script compiles kart and its bundled TOML parser into `target/kart`.

You can also specify a different compiler through the `CC` environment variable:

```sh
CC=clang ./first_compile.sh
```

## How to use

You can check all the available commands using the commands `./kart` or `./kart -h`

```
USAGE:
    ./target/kart [OPTIONS]

OPTIONS:
    -n <name>          Create a new project with a given name
    -r                 Run project
    -b                 Build project
    -i                 Instal project system-wide
    -t                 Run testing modules
    -c                 Clear project artifacts
    --hot-reloading    Enables hot-reloading
    -h                 Show help

```

## Command Reference

| Command | Description |
|---|---|
| `kart -n <name>` | Create a new project. |
| `kart -b` | Build the current project. |
| `kart -r [args...]` | Run the compiled executable. |
| `kart -c` | Remove generated build artifacts. |
| `kart -h` | Display command-line help. |

## Project Structure

Kart encourages a simple and conventional organization for C projects.

| Directory | Purpose |
|---|---|
| `src/` | Application source files (`.c`). |
| `include/` | Public headers and interfaces. |
| `target/` | Generated executables and build artifacts. |
| `tests/` | Project testing modules. |

The directory structure is intentionally minimal and can be adapted to the needs of individual projects. *(Though, the project may implement a custom implementation of a TOML parser in the future)*

## Dependencies

Kart is written in C and keeps its dependencies minimal.

- **GCC:** Used to compile managed projects.
- **tomlc17:** Used to parse the TOML project configuration.

The TOML parser is bundled with the project, so there is no need to install it separately.

## License

Kart is distributed under the [MIT License](LICENSE).
