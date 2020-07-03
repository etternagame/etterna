#ifndef RAGE_THREADS_H
#define RAGE_THREADS_H

#include "Etterna/Globals/global.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <condition_variable>
#include "Etterna/Singletons/PrefsManager.h"

class ThreadData
{
  public:
	void waitForUpdate()
	{
		std::unique_lock<std::mutex> lk(_updatedMutex);
		_updatedCV.wait_for(
		  lk, chrono::milliseconds(100), [this] { return this->getUpdated(); });
	}
	void setUpdated(bool b)
	{
		{
			std::lock_guard<std::mutex> lk(_updatedMutex);
			_updated = b;
		}
		_updatedCV.notify_all();
	}
	bool getUpdated() { return _updated; }
	std::atomic<int> _threadsFinished{ 0 };
	std::atomic<int> _progress{ 0 };
	std::mutex _updatedMutex;
	std::condition_variable _updatedCV;
	void* data{ nullptr };

  private:
	std::atomic<bool> _updated{ false };
};

template<class T>
using vectorIt = typename vector<T>::iterator;
template<class T>
using vectorRange = std::pair<vectorIt<T>, vectorIt<T>>;

template<typename T>
std::vector<vectorRange<T>>
splitWorkLoad(vector<T>& v, size_t elementsPerThread)
{
	std::vector<vectorRange<T>> ranges;
	if (elementsPerThread <= 0 || elementsPerThread >= v.size()) {
		ranges.push_back(std::make_pair(v.begin(), v.end()));
		return ranges;
	}

	size_t range_count = (v.size() + 1) / elementsPerThread + 1;
	size_t ePT = v.size() / range_count;
	if (ePT == 0) {
		ranges.push_back(std::make_pair(v.begin(), v.end()));
		return ranges;
	}
	size_t i;

	vectorIt<T> b = v.begin();

	for (i = 0; i < v.size() - ePT; i += ePT)
		ranges.push_back(std::make_pair(b + i, b + i + ePT));

	ranges.push_back(std::make_pair(b + i, v.end()));
	return ranges;
}

template<typename T>
void
parallelExecution(vector<T> vec,
				  function<void(int)> update,
				  function<void(vectorRange<T>, ThreadData*)> exec,
				  void* stuff)
{
	const int THREADS = PREFSMAN->ThreadsToUse <= 0
						  ? std::thread::hardware_concurrency()
						  : min((int)PREFSMAN->ThreadsToUse,
								(int)std::thread::hardware_concurrency());
	std::vector<vectorRange<T>> workloads =
	  splitWorkLoad(vec, static_cast<size_t>(vec.size() / THREADS));
	ThreadData data;
	data.data = stuff;
	auto threadCallback = [&data, &exec](vectorRange<T> workload) {
		exec(workload, &data);
		data._threadsFinished++;
		data.setUpdated(true);
	};
	vector<thread> threadpool;
	for (auto& workload : workloads)
		threadpool.emplace_back(thread(threadCallback, workload));
	while (data._threadsFinished < (int)workloads.size()) {
		data.waitForUpdate();
		update(data._progress);
		data.setUpdated(false);
	}
	for (auto& thread : threadpool)
		thread.join();
}
template<typename T>
void
parallelExecution(vector<T> vec,
				  function<void(int)> update,
				  function<void(vectorRange<T>, ThreadData)> exec)
{
	parallelExecution(vec, update, exec, nullptr);
}

template<typename T>
void
parallelExecution(vector<T> vec,
				  function<void(vectorRange<T>, ThreadData*)> exec)
{
	const int THREADS = PREFSMAN->ThreadsToUse <= 0
						  ? std::thread::hardware_concurrency()
						  : min((int)PREFSMAN->ThreadsToUse,
								(int)std::thread::hardware_concurrency());
	std::vector<vectorRange<T>> workloads =
	  splitWorkLoad(vec, static_cast<size_t>(vec.size() / THREADS));
	ThreadData data;
	auto threadCallback = [&data, &exec](vectorRange<T> workload) {
		exec(workload, &data);
		data._threadsFinished++;
		data.setUpdated(true);
	};
	vector<thread> threadpool;
	for (auto& workload : workloads)
		threadpool.emplace_back(thread(threadCallback, workload));
	while (data._threadsFinished < (int)workloads.size()) {
		data.waitForUpdate();
		data.setUpdated(false);
	}
	for (auto& thread : threadpool)
		thread.join();
}

struct ThreadSlot;
class RageTimer;

/** @brief Thread, mutex, semaphore, and event classes. */
class RageThread
{
  public:
	RageThread();
	RageThread(const RageThread& cpy);
	~RageThread();

	void SetName(const std::string& n) { m_sName = n; }
	std::string GetName() const { return m_sName; }
	void Create(int (*fn)(void*), void* data);

	void Halt(bool Kill = false);
	void Resume();

	/* For crash handlers: kill or suspend all threads (except for
	 * the running one) immediately. */
	static void HaltAllThreads(bool Kill = false);

	/* If HaltAllThreads was called (with Kill==false), resume. */
	static void ResumeAllThreads();

	static uint64_t GetCurrentThreadID();

	static const char* GetCurrentThreadName();
	static const char* GetThreadNameByID(uint64_t iID);
	static bool EnumThreadIDs(int n, uint64_t& iID);
	int Wait();
	bool IsCreated() const { return m_pSlot != nullptr; }

	/* A system can define HAVE_TLS, indicating that it can compile thread_local
	 * code, but an individual environment may not actually have functional TLS.
	 * If this returns false, thread_local variables are considered undefined.
	 */
	static bool GetSupportsTLS() { return s_bSystemSupportsTLS; }
	static void SetSupportsTLS(bool b) { s_bSystemSupportsTLS = b; }

	static bool GetIsShowingDialog() { return s_bIsShowingDialog; }
	static void SetIsShowingDialog(bool b) { s_bIsShowingDialog = b; }
	static uint64_t GetInvalidThreadID();

  private:
	ThreadSlot* m_pSlot;
	std::string m_sName;

	static bool s_bSystemSupportsTLS;
	static bool s_bIsShowingDialog;

	// Swallow up warnings. If they must be used, define them.
	RageThread& operator=(const RageThread& rhs);
};

/**
 * @brief Register a thread created outside of RageThread.
 *
 * This gives it a name for RageThread::GetCurrentThreadName,
 * and allocates a slot for checkpoints. */
class RageThreadRegister
{
  public:
	RageThreadRegister(const std::string& sName);
	~RageThreadRegister();

  private:
	ThreadSlot* m_pSlot;
	// Swallow up warnings. If they must be used, define them.
	RageThreadRegister& operator=(const RageThreadRegister& rhs) = delete;
	RageThreadRegister(const RageThreadRegister& rhs) = delete;
};

namespace Checkpoints {
void
LogCheckpoints(bool yes = true);
void
SetCheckpoint(const char* file, int line, const char* message);
void
GetLogs(char* pBuf, int iSize, const char* delim);
};

#define CHECKPOINT_M(m) (Checkpoints::SetCheckpoint(__FILE__, __LINE__, m))

/* Mutex class that follows the behavior of Windows mutexes: if the same
 * thread locks the same mutex twice, we just increase a refcount; a mutex
 * is considered unlocked when the refcount reaches zero.  This is more
 * convenient, though much slower on some archs.  (We don't have any tightly-
 * coupled threads, so that's OK.) */
class MutexImpl;
class RageMutex
{
  public:
	std::string GetName() const { return m_sName; }
	void SetName(const std::string& s) { m_sName = s; }
	virtual void Lock();
	virtual bool TryLock();
	virtual void Unlock();
	virtual bool IsLockedByThisThread() const;

	RageMutex(const std::string& name);
	virtual ~RageMutex();

  protected:
	MutexImpl* m_pMutex;
	std::string m_sName;

	uint64_t m_LockedBy;
	int m_LockCnt;

	void MarkLockedMutex();

  private:
	// Swallow up warnings. If they must be used, define them.
	RageMutex& operator=(const RageMutex& rhs);
	RageMutex(const RageMutex& rhs);
};

/**
 * @brief Lock a mutex on construction, unlock it on destruction.
 *
 * Helps for functions with more than one return path. */
class LockMutex
{
	RageMutex& mutex;

	const char* file;
	int line;
	float locked_at;
	bool locked;

  public:
	LockMutex(RageMutex& mut, const char* file, int line);
	LockMutex(RageMutex& mut)
	  : mutex(mut)
	  , file(nullptr)
	  , line(-1)
	  , locked_at(-1)
	  , locked(true)
	{
		mutex.Lock();
	}
	~LockMutex();
	LockMutex(LockMutex& cpy)
	  : mutex(cpy.mutex)
	  , file(nullptr)
	  , line(-1)
	  , locked_at(cpy.locked_at)
	  , locked(true)
	{
		mutex.Lock();
	}

	/**
	 * @brief Unlock the mutex (before this would normally go out of scope).
	 *
	 * This can only be called once. */
	void Unlock();

  private:
	// Swallow up warnings. If they must be used, define them.
	LockMutex& operator=(const LockMutex& rhs) = delete;
};

#define LockMut(m) LockMutex SM_UNIQUE_NAME(LocalLock)(m, __FILE__, __LINE__)

class EventImpl;
class RageEvent : public RageMutex
{
  public:
	RageEvent(const std::string& name);
	~RageEvent() override;

	/*
	 * If pTimeout is non-NULL, the event will be automatically signalled at the
	 * given time.  Note that implementing this timeout is optional; not all
	 * archs support it. If false is returned, the wait timed out (and the mutex
	 * is locked, as if the event had been signalled).
	 */
	bool Wait(RageTimer* pTimeout = nullptr);
	void Signal();
	void Broadcast();
	bool WaitTimeoutSupported() const;
	// Swallow up warnings. If they must be used, define them.
	RageEvent& operator=(const RageEvent& rhs);
	RageEvent(const RageEvent& rhs);

  private:
	EventImpl* m_pEvent;
};

class SemaImpl;
class RageSemaphore
{
  public:
	RageSemaphore(const std::string& sName, int iInitialValue = 0);
	~RageSemaphore();

	std::string GetName() const { return m_sName; }
	int GetValue() const;
	void Post();
	void Wait(bool bFailOnTimeout = true);
	bool TryWait();

  private:
	SemaImpl* m_pSema;
	std::string m_sName;

	// Swallow up warnings. If they must be used, define them.
	RageSemaphore& operator=(const RageSemaphore& rhs) = delete;
	RageSemaphore(const RageSemaphore& rhs) = delete;
};

#endif
