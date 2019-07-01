# RoutineStreamliner

A header-only library providing a class to fuse a lot of periodically called operations to reduce the number of CPU wakeups or I/O operations.

A typical use case is that the program needs to send a lot of values through some I/O operation that can send more values at once, but each of these values needs to be sent at a different period. This class handles the timing and joining of I/O operations from similar times into larger operations.

The class' constructor takes the amount of time that can be sacrificed to reduce the number of operations and a lambda that handles the operation. It is templated to hold whatever classes the lambda needs to identify or read the objects whose period had passed. It can be pointers, lambdas that return the a value that is to be processed, etc. It can be also used to avoid having too many threads and CPU wakeups for short, periodically called routines by having the lambda simply call lambdas it is supplied.

Invividual actions are added using the `add()` method that contain the object that will be given to the lambda when the period supplied as the second argument passes. It returns a handle that can be used as an argument to its `remove()` method to unregister a routine. All routines are unregistered when the destructor ends.

It uses `looping_thread` as submodule to manage the thread and exit quickly when the destructor is called.

## Example

Because I/O operations usually need a lot of code, this example uses the flush of `std::cout` called by `std::endl` as a model I/O operation. All those routines are just different strings printed at different periods. Actions with time difference up to 300 milliseconds are merged in this example.

```C++
RoutineStreamliner<std::string> repeater(std::chrono::milliseconds(300), [] (const std::vector<std::string*>& lines) {
		for (std::string* line : lines)
		std::cout << *line << " ";
		std::cout << std::endl;
	});
repeater.add("1.1s", std::chrono::milliseconds(1100));
repeater.add("0.4s", std::chrono::milliseconds(400));
repeater.add("0.5s", std::chrono::milliseconds(500));
repeater.add("0.7s", std::chrono::milliseconds(700));
repeater.add("1.3s", std::chrono::milliseconds(1300));

```

## Preventing too many calls

If you don't want functions to be called too many times, you can use `setThrottling()` that will limit the number of calls of a function to 1 per cycle. It can be set as also as an additional argument to the constructor.

## Troubleshooting

Error message `undefined reference to 'pthread_create'` means that you need to add `-lpthread` to the compiler options.
