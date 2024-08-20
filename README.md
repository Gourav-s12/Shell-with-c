
# Custom Shell with Embedded `vi` Editor

This project is a custom shell implemented in C, which includes basic shell functionalities along with a simple `vi`-like text editor. The shell supports common shell commands, vector operations, and command chaining with pipes.

## Features

- **Basic Shell Commands**: The shell supports commands like `pwd`, `cd`, `mkdir`, `ls`, and `exit`. You can also execute any external executable.
- **Vector Operations**: Supports vector addition (`addvec`), subtraction (`subvec`), and dot product (`dotprod`). These operations are multithreaded and can be executed using a specified number of threads.
- **Command Pipelining**: Supports command chaining with pipes (`|`).
- **File Redirection**: Supports input (`<`) and output (`>`) redirection.
- **Parallel Execution**: Commands can be run in parallel by appending `&` at the end.
- **Embedded `vi` Editor**: Includes a simple text editor that mimics the basic functionality of the `vi` editor.
- **Support for Other Commands**: The shell supports the execution of any other standard Unix commands or programs.

## Compilation

To compile the project, use the following command:

```sh
gcc -o my_shell_vi shell_vi.c -lreadline -lcurses
```

## Usage

After compiling, you can start the shell by running:

```sh
./my_shell_vi
```

### Available Commands

- `pwd`: Displays the current working directory.
- `cd <directory_name>`: Changes the current directory.
- `mkdir <directory_name>`: Creates a new directory.
- `ls <flags>`: Lists the contents of the current directory with optional flags.
- `addvec <file1> <file2> [-num_threads]`: Adds vectors from two files using a specified number of threads.
- `subvec <file1> <file2> [-num_threads]`: Subtracts vectors from two files using a specified number of threads.
- `dotprod <file1> <file2> [-num_threads]`: Calculates the dot product of vectors from two files using a specified number of threads.
- `vi <filename>`: Opens the specified file in the embedded `vi`-like editor.
- `exit`: Exits the shell.
- All other standard Unix commands can also be executed directly.

### Using the `vi` Editor

The embedded `vi` editor supports basic navigation and editing:

- **Arrow Keys**: Move the cursor up, down, left, and right.
- **Backspace**: Deletes the character to the left of the cursor.
- **Insert Mode**: Start typing to insert characters.
- **Save and Exit**: The file is saved when you exit the editor.

## License

This project is open-source and free to use under the MIT License.
