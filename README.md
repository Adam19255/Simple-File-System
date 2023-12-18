# Simple File System
Authored by Adam Shay

## Description
This is a simple file system implemented in C++ that provides basic file operations like creating, opening, reading, writing, copying, renaming, and deleting files on a simulated disk.

## Features
The file system provides the following features:

**File Descriptors:** File descriptors are used to keep track of open files, allowing multiple files to be opened simultaneously.

**Bit Vector:** The file system uses a `bit vector` to manage disk block allocation. Blocks are marked as used or free in the bit vector.

1. **Disk Formatting:** You can format the virtual disk with a specified `block size`. 
	Formatting clears all existing data on the disk and prepares it for file storage. 
	Use the format command to initiate disk formatting, like this: `fsFormat <block_size>`.
2. **File Creation:** You can create new files on the disk using the create command: `CreateFile <file_name>`.
3. **Opening File:** You can open an existing file for reading and writing.
	Use the open command to open a file: `OpenFile <file_name>`.
4. **File Closure:** It's important to close files after reading or writing to them. 
	Use the close command to close a file: `CloseFile <fd>`.
5. **File Writing:** You can write data to a file using the write command: `WriteToFile <fd> <data>`.
6. **File Reading:** To read the contents of a file, you can use the read command: `ReadFromFile <fd> <length_to_read>`.
7. **File Copy:** The file system allows you to copy the contents of one file to another, creating a new file with the copied data. 
	Use the copy command for this purpose: `CopyFile <source_file_name> <destination_file_name>`.
8. **File Rename:** You can rename a file using the rename command: `RenameFile <old_file_name> <new_file_name>`.
9. **File Deletion:** To delete files from the file system, use the delete command: `DelFile <file_name>`.
10. **File Listing:** You can list all the files in the file system using the list all command: `listAll()`.

## How to Use
1. **Formatting the Disk:** Before you can use the file system, you must format the disk. Use the format command to specify the block size and prepare the disk.
2. **Creating Files:** Create new files using the create command. Provide a unique name for each file.
3. **Reading and Writing:** Use the read command to view the contents of a file and the write command to add or modify data in a file.
4. **Listing Files:** The list all command displays a list of all files and data currently in the file system.
5. **Closing Files:** Close files when needed using the close command.
6. **Copying Files:** You can copy the contents of one file to another using the copy command. Ensure that both the source and destination files are closed before copying.
7. **Deleting Files:** To delete a file, use the delete command followed by the name of the file you want to remove.
8. **Exit:** Exit the file system.
