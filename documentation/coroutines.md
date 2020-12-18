# Coroutines

Sometimes it's necessary to run code in a background task while allowing game frames to continue to run. Although it's possible to use threading for this purpose, implementing the necessary locking becomes difficult and error prone.

Simulant instead uses Coroutines for executing long-running tasks.

# Starting a Coroutine

You can run a function as a coroutine by using the `start_coroutine` method.

```

start_coroutine([]() { 
    for(int i = 0; i < 1000; ++i) {
    	yield_coroutine();
    }
});

```

You must yield coroutines regularly to give up control to allow a game frame to run.
