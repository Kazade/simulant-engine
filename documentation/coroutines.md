# Coroutines

Sometimes it's necessary to run code in a background task while allowing game frames to continue to run. Although it's possible to use threading for this purpose, implementing the necessary locking becomes difficult and error prone.

Simulant instead uses Coroutines for executing long-running tasks.

# Starting a Coroutine

You can run a function as a coroutine by using the `cr_async` method.

```

cr_async([]() { 
    for(int i = 0; i < 1000; ++i) {
    	cr_yield();
    }
});

```

You must yield coroutines regularly to give up control to allow a game frame to run.

## Promises

`cr_async` will return a `CRPromise<T>` which allows you to check when the coroutine has run, and what its value was when it finished. If you want to block and await the resulting
value you can use `cr_await(promise)`. If `cr_await()` is called in the main thread then *only* idle tasks and other coroutines will run while waiting, rendering will not continue.

When waiting for a promise in the main thread, you're more likely to want to use `CRPromise<T>::then(callback)` which will trigger a callback when the promise completes. `then()` will also
return a `CRPromise<T>` and so you can change `then()` statements to run a sequence of tasks.
