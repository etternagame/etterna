#ifndef DARWIN_THREAD_HELPERS_H
#define DARWIN_THREAD_HELPERS_H

/**
 * @brief Attempt to suspend the specified thread.
 * @param threadHandle the thread to suspend.
 * @return true if the thread is suspended, false otherwise. */
bool
SuspendThread(uint64_t threadHandle);
/**
 * @brief Attempt to resume the specified thread.
 * @param threadHandle the thread to resume.
 * @return true if the thread is resumed, false otherwise. */
bool
ResumeThread(uint64_t threadHandle);
/**
 * @brief Retrieve the current thread ID.
 * @return the current thread ID. */
uint64_t
GetCurrentThreadId();
/**
 * @brief Set the precedence for the thread.
 *
 * Valid values for the thread are from 0.0f to 1.0f.
 * 0.5f is the default.
 * @param prec the precedence to set. */
std::string
SetThreadPrecedence(float prec);

#endif
