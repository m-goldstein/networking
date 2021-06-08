## http_server
### The server uses the stat libary and structures to gather information about the files to serve to clients. For the purposes of the MP, it is used to check the file-size and check the permission bits. The server should not fall for exploits facilitating arbitrary filesystem accesses over HTTP. (e.g. if a client requests http://localhost//etc/passwd, they should not get the contents of /etc/passwd or other files upstream. Further, every request is sandboxed to the current directory which http_server was executed.
### The man pages in terminal are sufficient but I've linked the man page of man.org below for reference.
### https://man7.org/linux/man-pages/man2/stat.2.html

