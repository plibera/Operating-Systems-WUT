# File system

This project creates a virtual file system in a file of a given size. The structure of the file system is based on the structure of the Unix file system.

### Usage

The project can be compiled with `cc main.c -o vfs`

After that it can be used to create and manage a simple file system inside a binary file of a given size.


Usage: ./vfs [diskName] [command] [parameters]  
Commands and parameters:  
&nbsp;&nbsp;&nbsp;&nbsp;c [diskSize] - create new disk named [diskName] in a file of size [diskSize] kB  
&nbsp;&nbsp;&nbsp;&nbsp;u [fileName] - upload file named [fileName] to the disk  
&nbsp;&nbsp;&nbsp;&nbsp;d [fileName] - download file named [fileName] from the disk  
&nbsp;&nbsp;&nbsp;&nbsp;l            - list files on the disk  
&nbsp;&nbsp;&nbsp;&nbsp;r [fileName] - remove file named [fileName] from the disk  
&nbsp;&nbsp;&nbsp;&nbsp;R            - remove the whole disk named [diskName]  
&nbsp;&nbsp;&nbsp;&nbsp;m            - show current disk map  

### Testing

The project contains testing files of different sizes (1, 1.5, 2, 10 kB) used by the testing script `test.sh`

The testing script tests the project and demonstrates some of the problems caused by the structure of the file system.
