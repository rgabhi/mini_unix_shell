# AP_SHELL - Mini UNIX Shell

**AP_SHELL** is a custom implementation of a UNIX command-line interpreter (shell) written in C. It serves as a functional REPL (Read-Eval-Print Loop) capable of executing system commands, managing processes, handling I/O redirection, and supporting advanced features like pipelines and logical operators.


### Core Functionality
- **External Command Execution:** Runs standard UNIX programs (e.g., `ls`, `grep`, `sleep`) using `PATH` lookup.
- **Input Parsing:** Custom tokenizer handles multiple arguments and **quoted strings** (e.g., `echo "Hello World"`).
- **Redirection:**
  - Input (`<`): Read from files.
  - Output (`>`): Write output to files.
- **Pipelines (`|`):** Connects the output of one process to the input of another (e.g., `ls | grep .c`).
- **Logical Operator (`&&`):** Conditional execution; runs the second command only if the first succeeds.
- **Background Execution (`&`):** Runs processes in the background without blocking the shell.

### Built-in Commands
- `cd <dir>`: Change the current working directory.
- `exit`: Terminate the shell session.
- `export KEY=VALUE`: Set environment variables.
- `history`: Display the last 20 commands used (implemented via an **LRU Cache**).

### Signal Handling
- **Ctrl-C (SIGINT):** Does not kill the shell; instead, it interrupts the currently running foreground process and reprompts.
- **Zombie Cleanup (SIGCHLD):** Automatically reaps background processes when they finish to prevent zombie entries in the process table.

## 🛠️ Installation & Compilation

The project includes a `Makefile` for easy compilation.

**Prerequisites:**
- GCC Compiler
- Make (optional, but recommended)
- Linux/UNIX environment

**Build the Shell:**
```bash
make

