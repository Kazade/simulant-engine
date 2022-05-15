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

Coroutines resume after `late_update()`. If you want to schedule a coroutine to resume after time has passed then use `cr_yield_for(seconds)` and the coroutine will not be resumed until `seconds` seconds have passed.

On very rare occasions you my be required to run code on the main thread, coroutines provide `cr_run_main(func)` to do that. The coroutine will be yielded and `func` will be called immediately and then the coroutine will
resume.

## Promises

`cr_async` will return a `Promise<T>` which allows you to check when the coroutine has run, and what its value was when it finished. If you want to block and await the resulting
value you can use `cr_await(promise)`. If `cr_await()` is called in the main thread then *only* idle tasks and other coroutines will run while waiting, rendering will not continue.

When waiting for a promise in the main thread, you're more likely to want to use `Promise<T>::then(callback)` which will trigger a callback when the promise completes. `then()` will also
return a `Promise<T>` and so you can change `then()` statements to run a sequence of tasks.
