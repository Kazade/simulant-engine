

# C++ Guidelines

For the purposes of portability, there are some restrictions on C++ usage. Here are the 
things not to do, and the justification for why.

## Don't use language features after C++11

While it would be great to use the latest and greatest language features, unfortunately
embedded target platforms like the Dreamcast, PSP, etc. do not have compiler chains that
support them well.

## Don't use std::thread and friends

Amazing as the C++11 threading libraries are, they are not well supported in the Dreamcast
and PSP homebrew toolchains. Instead use the threading subsystems in simulant/thread.


