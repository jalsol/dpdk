# Simple Character Device Driver

A minimal Linux character device driver that demonstrates basic kernel module development.

## What It Does

This driver creates a character device (`/dev/simple_chardev`) that:
- Accepts write operations (stores data in a kernel buffer)
- Supports read operations (retrieves data from the kernel buffer)
- Demonstrates kernel/user space data transfer

## Building

```bash
make
```

Requirements:
- Linux kernel headers: `sudo apt-get install linux-headers-$(uname -r)`
- Build tools: `sudo apt-get install build-essential`

## Loading the Module

```bash
make load
```

This will:
1. Insert the module into the kernel
2. Create `/dev/simple_chardev` automatically
3. Set appropriate permissions

## Testing

```bash
# Write data to the device
echo "Hello from userspace!" > /dev/simple_chardev

# Read data back
cat /dev/simple_chardev

# Or use the built-in test
make test

# View kernel messages
sudo dmesg | tail -20
```

## Unloading the Module

```bash
make unload
```

## Code Structure

- **device_open()**: Called when process opens the device
- **device_read()**: Transfers data from kernel to user space
- **device_write()**: Transfers data from user to kernel space
- **device_release()**: Called when process closes the device

## Key Concepts Demonstrated

1. **Module Init/Exit**: `module_init()` and `module_exit()` macros
2. **Character Device Registration**: `register_chrdev()`
3. **File Operations**: `struct file_operations`
4. **User/Kernel Data Transfer**: `copy_to_user()` and `copy_from_user()`
5. **Device Class Creation**: Automatic `/dev` node creation

## Cleaning Up

```bash
make clean
```

This removes all build artifacts.

