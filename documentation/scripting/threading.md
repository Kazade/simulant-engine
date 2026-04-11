# Threading

Simulant ships its own set of threading primitives for portability. All of these primitives
exist in the smlt::thread namespace.

## Thread

`smlt::thread::Thread` exists in the `threads/thread.h` header file and let's you spawn a thread.

## Atomic<T>

`smlt::thread::Atomic<T>` exists in the `threads/atomic.h` header file and lets you perform
mutex protected operations on a wrapped type.

## async and Future<T>

`smlt::thread::async` allows you to run a function in a background thread, and then access its
return value which is stored in a `Future<T>`.

Example usage:

```
Future<int> future = async(calculate_some_int, arg1, arg2); // Trigger the task

while(!future.is_ready()) {}  // Wait while it runs

int result = future.get(); // Get the calculated result
```

# Mutex, SharedMutex, and RecursiveMutex

These mutexes are available in the smlt::thread namespace and in the `threads/mutex.h` and 
`threads/shared_mutex.h` headers.

