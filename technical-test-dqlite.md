# Technical Test – dqlite
Design and develop a simple key-value database. Running `make` in your project's root
folder should produce a binary `kvdb` that at least implements the following commands.

`kvdb set {key}` {value}` : Associates `key` with `value`.
`kvdb get {key}`          : Fetches the value associated with `key`.
`kvdb del {key}`          : Removes `key` from the database.
`kvdb ts {key}`           : Returns the timestamp when `key` was first set and when it was last set.Expected timestamp format is "YYYY-MM-DD HH:MM:SS.SSS".



# Rules:
- Your program should be written in C.
- Assume that your database can be accessed by multiple processes concurrently.
- Keep performance and storage efficiency in mind.
- You are expected to not spend much more than 4 hours on this project.
- You are allowed to use libraries, but don't just write a wrapper around an existing database.
- We will not install extra dependencies to compile your program.
- Your program will be compiled with gcc 11.2.0 on a x86_64 machine running Ubuntu Jammy Jellyfish.

Please provide a short write-up of the limitations of your program. (20 lines max).
Hand in a tarball containing your `kvdb` root folder and the write-up.
