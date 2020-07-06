/* SubscriptionManager - Object that accepts subscriptions. */

#ifndef SubscriptionManager_H
#define SubscriptionManager_H

#include <set>
#include <cassert>

// Since this class has only POD types and no constructor, there's no
// initialize order problem.
template<class T>
class SubscriptionManager
{
  public:
	// TRICKY: If we make this a global instead of a global pointer,
	// then we'd have to be careful that the static constructors of all
	// subscribers are called before the collection constructor.  It's
	// impossible to enfore that in C++.  Instead, we'll allocate the
	// collection ourself on first use.  SubscriptionHandler itself is
	// a POD type, so a static SubscriptionHandler will always have
	// m_pSubscribers == NULL (before any static constructors are called).
	std::set<T*>* m_pSubscribers;

	// Use this to access m_pSubscribers, so you don't have to worry about
	// it being NULL.
	std::set<T*>& Get()
	{
		if (m_pSubscribers == nullptr)
			m_pSubscribers = new std::set<T*>;
		return *m_pSubscribers;
	}

	void Subscribe(T* p)
	{
		if (m_pSubscribers == nullptr)
			m_pSubscribers = new std::set<T*>;
#ifdef DEBUG
		typename set<T*>::iterator iter = m_pSubscribers->find(p);
		ASSERT_M(iter == m_pSubscribers->end(), "already subscribed");
#endif
		m_pSubscribers->insert(p);
	}

	void Unsubscribe(T* p)
	{
		auto iter = m_pSubscribers->find(p);
		assert(iter != m_pSubscribers->end());
		m_pSubscribers->erase(iter);
	}
};

#endif
