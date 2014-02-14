/**
*  @file SynchronizedQueue.h
*
*  @brief Header for the Template class SynchronizedQueue.
*
*  Provides a thread-safe blocking queue wrapper based on the std::queue
*
*  @date 03/28/2011
*
*  @author Alejandro Villamarin
*/


#include <queue>
#include <sstream>
#include <boost/thread/mutex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/any.hpp>
#include <boost/thread/condition_variable.hpp>


#ifndef SYNCHRONIZEDQUEUE_DEF
#define SYNCHRONIZEDQUEUE_DEF

/**
*  @class SynchronizedQueue
*
*  @brief Provides a thread-safe blocking queue wrapper based on the std::queue
*
*  @date 20/10/2010
*
*  @author Alejandro Villamarin
*/
	template<class T>
	class SynchronizedQueue
	{

	private:
		///The queue itself
		std::queue<T> sQueue;
		///A mutex object to control access to the std::queue
		boost::mutex io_mutex_;
		///A variable condition to make threads wait on specified condition values
		boost::condition_variable waitCondition;
	public:
		//default Constructor
		SynchronizedQueue();
		//destructor
		virtual ~SynchronizedQueue();
		void push(T& element);
		bool empty();
		bool pop(T&);
		bool flush();
		unsigned int sizeOfQueue();
		void waitAndPop(T&);
		std::string toString();
	void push_pre();
	void push_post();

	void pop_pre();
	bool pop_fast(T& element);
	void pop_post();

};

	/**
	*  Constructor
    *
	*  @return void
	*
	 */
	template<class T>
		//default constructor
		SynchronizedQueue<T>::SynchronizedQueue(){}


	/**
	*  Destructor
    *
	*  @return void
	*
	 */
	template<class T>
		//default destructor
		SynchronizedQueue<T>::~SynchronizedQueue(){}




	/**
	*  Adds an element to the queue in a blocking mode (thread-safe)
    *
	*  @return void
	*
	 */

	template<class T>
	void SynchronizedQueue<T>::push_pre()
	{
		//try to lock the mutex
		io_mutex_.lock();
	}


	template<class T>
	void SynchronizedQueue<T>::push(T& element)
	{
		//insert the element in the FIFO queue
		SynchronizedQueue::sQueue.push(element);
	}

	template<class T>
	void SynchronizedQueue<T>::push_post()
	{
		//Now we need to unlock the mutex otherwise waiting threads will not be able to wake and lock the
		//mutex by time before push is locking again
	    io_mutex_.unlock();
		//notifiy waiting thread they can pop/push now
	    waitCondition.notify_one();
	}


	/**
	*  Checks if the queue is empty or not
    *
	*  @return TRUE if the queue is not empty
	*
	 */
	template<class T>
	bool SynchronizedQueue<T>::empty()
	{
		//try to lock the mutex
		boost::mutex::scoped_lock lock(io_mutex_);
	    return SynchronizedQueue::sQueue.empty();
	}
	
	template<class T>
	bool SynchronizedQueue<T>::flush()
	{
	   T element;

       while(sizeOfQueue())
       {
         pop(element);
	}
       return true;
       
	}

	/**
	*  It tries to pop an element from the queue but does not keep waiting if the queue
	*  is empty
    *
    *  @param A reference to object T to hold the popped element
	*  @return void
	*
	 */
	template<class T>
	bool SynchronizedQueue<T>::pop(T& element)
	{

		//try to lock the mutex
		boost::mutex::scoped_lock lock(io_mutex_);

		//ask if the queue is empty
		if (SynchronizedQueue::sQueue.empty())
		{
			return false;
		}

		//get the last inserted element
	    element = SynchronizedQueue::sQueue.front();
	    //remove the last inserted element, since we have a copy in element
	    SynchronizedQueue::sQueue.pop();

	    //no need to unlock the mutex, it will get unlocked when the function ends
	    return true;

	};

	template<class T>
	void SynchronizedQueue<T>::pop_pre()
	{
		io_mutex_.lock();
	};

	template<class T>
	bool SynchronizedQueue<T>::pop_fast(T& element)
	{
	//get the last inserted element
		if (SynchronizedQueue::sQueue.empty())
		{
			return false;
		}

	    element = SynchronizedQueue::sQueue.front();
	    //remove the last inserted element, since we have a copy in element
	    SynchronizedQueue::sQueue.pop();
	    return true;

	};

	template<class T>
	void SynchronizedQueue<T>::pop_post()
	{
		//Now we need to unlock the mutex otherwise waiting threads will not be able to wake and lock the
		//mutex by time before push is locking again
	    io_mutex_.unlock();
		//notifiy waiting thread they can pop/push now
	    waitCondition.notify_one();
	};



	/**
	*  Return the size of the queue
    *
	*  @return number of elements in the queue
	*
	 */
	template<class T>
	unsigned int SynchronizedQueue<T>::sizeOfQueue()
	{
		//try to lock the mutex
		boost::mutex::scoped_lock lock(io_mutex_);
		return SynchronizedQueue::sQueue.size();

	};




	/**
	*  Keeps any thread waiting if the queue is empty and when is not
	*  it returns the element at the front of the queue
	*
	*  @param A reference to object T to hold the popped element
	*  @return A reference to an object of type T in the queue
	*
	 */
	template<class T>
	void SynchronizedQueue<T>::waitAndPop(T& element)
	{

boost::system_time const timeout=boost::get_system_time()+ boost::posix_time::milliseconds(50);


		//try to lock the mutex
		boost::mutex::scoped_lock lock(io_mutex_);
		//while the queue is empty, make the thread that runs this wait
	    while(SynchronizedQueue::sQueue.empty())
	    {
//			waitCondition.wait(lock);
if(waitCondition.timed_wait(lock,timeout))
{

}
else
{
    return;
}

	    }

	    //when the condition variable is unlocked, popped the element
	    element = SynchronizedQueue::sQueue.front();

	    //pop the element
	    sQueue.pop();

	};


	/**
	*  Iterates throught the queue elements and returns a string representing them
	*
	*  @return string the textual representation of the queue
	*
	 */
	template<class T>
	std::string SynchronizedQueue<T>::toString()
	{
		std::stringstream os;

		//make a copy of the class queue, so we dont care about mangling with threads
		std::queue<T> copy = SynchronizedQueue::sQueue;

		//check the queue is not empty
	    if (!copy.empty())
	    {
	    	int counter = 0;

	    	os << "Elements in the Synchronized queue are as follows:" << std::endl;
	    	os << "**************************************************" << std::endl;

	    	while (!copy.empty())
			{
	    		//get the first element in the queue
				boost::any object = copy.front();
				std::string b = "Element at position " + boost::lexical_cast<std::string>(counter) + " is: ";
				b.append("[");
				b.append(object.type().name());
				b.append("]\n");
		    	os << b;
		    	//remove the element in the queue
		    	copy.pop();
			}
	    	return os.str();

	    }
	    else
	    {
	    	os << "Queue is empty";
	    	return os.str();
	    }


	}




#endif


