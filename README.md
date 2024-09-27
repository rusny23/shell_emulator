# shell_emulator - Jumboshell

This project implements a custom shell called `jumboshell`. This shell allows users to execute commands with or without arguments, similar to traditional shells like Bash. It supports piping ('|'), which allows users to chain commands by passing the output of one command as the input to the next.

## Features

- **Command Execution**: Execute standard Unix commands (e.g., `ls`, `grep`).
- **Piping**: Support for piping between multiple commands.
- **Custom Prompt**: A simple shell prompt (`jsh$`) to indicate to the user that it is ready for input.
- **Graceful Exit**: Type `exit` to close the shell.


## How to Use

1. **Compile**: Use command `gcc` to compile the source code:
   ```bash
   gcc -o jumboshell shell.c
