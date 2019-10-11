/* \brief Class for registering periodic calls, calling them periodically, allowing to merge them if emitted at similar time to optimise communication
*
* The template argument is whatever object that remains stored in the structure to provide information about the periodic call. RAII constructor accepts
* to arguments, first is a function that accepts a vector of pointers to the registered objects as its only argument, the second is the maximal difference
* of deadlines of actions merged.
*
* Method add() registers an object with a call period given by second argument and returns a key that can be used to unregister it. The object itself does
* nothing, it has to be done by the function given to the constructor.
*
* \note Internal operations are mutex-controlled. Destruction will clean up everything, including its worker thread.
*/
 
#include <vector>
#include <map>
#include <memory>
#include "looping_thread/looping_thread.hpp"
 
constexpr int STREAMLINER_WAIT_IF_EMPTY = 500000000; // in ns
 
template<typename T>
class RoutineStreamliner {
	struct Subscription {
		T data;
		std::chrono::steady_clock::duration period;
		unsigned int identifier;
	};
	bool _exit = false;
	bool _throttling = false;
	std::mutex _lock;
	std::chrono::steady_clock::duration _imprecisionPermitted;
	std::function<void(const std::vector<T*>&)> _merger;
	unsigned int identifier = 0;
	std::multimap<std::chrono::steady_clock::time_point, std::unique_ptr<Subscription>> _entries;
	std::unique_ptr<LoopingThread> _worker;

public:

	/*!
	* \brief Constructor
	*
	* \param Callback function that receives a vector of pointers to objects that are called
	* \param Time difference that can be joined into one callback function call
	* \param Whether throttling (discarding of identical routines if they would be run in the same tick) should be enabled
	*/
	inline RoutineStreamliner(std::chrono::steady_clock::duration imprecisionPermitted, std::function<void(const std::vector<T*>&)> merger, bool throttling = false) :
	_throttling(throttling), _imprecisionPermitted(imprecisionPermitted), _merger(merger)
	{
		_worker = std::unique_ptr<LoopingThread>(new LoopingThread(std::chrono::steady_clock::duration(STREAMLINER_WAIT_IF_EMPTY), [this] {
			std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
			std::chrono::steady_clock::time_point until = start + _imprecisionPermitted;

			std::vector<T*> merged;
			{
				std::lock_guard<std::mutex> lock(_lock);
				for(auto it = _entries.begin(); it != _entries.end();) {
					if(it->first > until) break;

					std::unique_ptr<Subscription> contents = std::move(it->second);
					merged.push_back(&contents->data);

					std::chrono::steady_clock::time_point next = it->first + contents->period;
					if (_throttling) while (next < until) {
						next += contents->period;
					}
					_entries.insert(std::make_pair(next, std::move(contents)));
					it = _entries.erase(it);
				}
			}
			if(!merged.empty())
				_merger(merged);

			if(!_entries.empty()) {
				std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
				_worker->setPeriod(_entries.begin()->first - start);
			}
		}));
	}

	/*!
	* \brief Destructor
	*/
	inline ~RoutineStreamliner()
	{
	}

	/*!
	* \brief Adds a periodic action
	*
	* \param The object that the function given in constructor will receive pointer at
	* \param The period of giving pointers to this object to the function given in constructor
	* \return An identifier of the object that can be used to remove this periodic action
	*/
	inline unsigned int add(const T& data, std::chrono::steady_clock::duration period)
	{
		std::lock_guard<std::mutex> lock(_lock);
		_entries.insert(std::make_pair(std::chrono::steady_clock::now(), std::unique_ptr<Subscription>(new Subscription{ data, period, identifier })));
		return identifier++;
	}

	/*!
	* \brief Adds a periodic action
	*
	* \param The object that the function given in constructor will receive pointer at
	* \param The period of giving pointers to this object to the function given in constructor
	* \return An identifier of the object that can be used to remove this periodic action
	*/
	inline unsigned int add(T&& data, std::chrono::steady_clock::duration period)
	{
		std::lock_guard<std::mutex> lock(_lock);
		_entries.insert(std::make_pair(std::chrono::steady_clock::now(), std::unique_ptr<Subscription>(new Subscription{ data, period, identifier })));
		return identifier++;
	}

	/*!
	* \brief Removes a periodic action
	*
	* \param An identifier of the action that was returned by the add() method
	*/
	inline void remove(unsigned int identifier)
	{
		std::lock_guard<std::mutex> lock(_lock);
		for(auto it = _entries.begin(); it != _entries.end(); ++it) {
			if(it->second->identifier == identifier) {
				_entries.erase(it);
				return;
			}
		}
		throw(std::logic_error("Unregistering streamlined action that isn't registered"));
	}

	/*!
	* \brief Sets throttling (discarding of identical routines if they would be run in the same tick) on and off
	*
	* \param Whether it should be throttling or not
	*/
	inline void setThrottling(bool throttling) {
		_throttling = throttling;
	}
};
