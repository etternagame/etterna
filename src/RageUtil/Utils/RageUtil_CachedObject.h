#ifndef RAGE_UTIL_CACHED_OBJECT_H
#define RAGE_UTIL_CACHED_OBJECT_H

#include <set>

template<typename T>
class CachedObjectPointer;

/** @brief Utilities for working with the
 <a class="el" href="class_cachedobject.html">CachedObjects</a>. */
namespace CachedObjectHelpers {
void
Lock();
void
Unlock();
}

/** @brief Cached object pointers with automatic invalidation. */
template<typename T>
class CachedObject
{
  public:
	CachedObject()
	  : m_pObject(nullptr)
	{
		/* A new object is being constructed, so invalidate negative caching. */
		ClearCacheNegative();
	}

	CachedObject(const CachedObject& cpy)
	  : m_pObject(nullptr)
	{
		ClearCacheNegative();
	}

	~CachedObject()
	{
		if (m_pObject != nullptr)
			ClearCacheSpecific(m_pObject);
	}

	auto operator=(const CachedObject& rhs) -> CachedObject& { return *this; }

	/* Clear all cached entries for this type. */
	static void ClearCacheAll()
	{
		CachedObjectHelpers::Lock();
		for (typename std::set<ObjectPointer*>::iterator p =
			   m_spObjectPointers.begin();
			 p != m_spObjectPointers.end();
			 ++p) {
			(*p)->m_pCache = nullptr;
			(*p)->m_bCacheIsSet = false;
		}
		CachedObjectHelpers::Unlock();
	}

	/* Clear all cached entries pointing to a specific object. */
	static void ClearCacheSpecific(const T* pObject)
	{
		CachedObjectHelpers::Lock();
		for (typename std::set<ObjectPointer*>::iterator p =
			   m_spObjectPointers.begin();
			 p != m_spObjectPointers.end();
			 ++p) {
			if ((*p)->m_pCache == pObject) {
				(*p)->m_pCache = nullptr;
				(*p)->m_bCacheIsSet = false;
			}
		}
		CachedObjectHelpers::Unlock();
	}

	/* Clear all negative cached entries of this type. */
	static void ClearCacheNegative()
	{
		CachedObjectHelpers::Lock();
		for (typename std::set<ObjectPointer*>::iterator p =
			   m_spObjectPointers.begin();
			 p != m_spObjectPointers.end();
			 ++p) {
			if ((*p)->m_pCache == nullptr)
				(*p)->m_bCacheIsSet = false;
		}
		CachedObjectHelpers::Unlock();
	}

  private:
	using ObjectPointer = CachedObjectPointer<T>;
	friend class CachedObjectPointer<T>;

	static void Register(ObjectPointer* p) { m_spObjectPointers.insert(p); }

	static void Unregister(ObjectPointer* p)
	{
		typename std::set<ObjectPointer*>::iterator it =
		  m_spObjectPointers.find(p);
		ASSERT(it != m_spObjectPointers.end());
		m_spObjectPointers.erase(it);
	}

	/* This points to the actual T this object is contained in.  This is set
	 * the first time CachedObjectPointer::Set() is called for this object.
	 * That's more convenient than setting it ourselves; we don't need to
	 * do anything special in T's copy ctor.  This works because there's no
	 * need to clear cache for an object before any CachedObjectPointers have
	 * ever been set for it. */
	const T* m_pObject;
	static std::set<ObjectPointer*> m_spObjectPointers;
};
template<typename T>
std::set<CachedObjectPointer<T>*> CachedObject<T>::m_spObjectPointers =
  std::set<CachedObjectPointer<T>*>();

template<typename T>
class CachedObjectPointer
{
  public:
	using Object = CachedObject<T>;

	CachedObjectPointer()
	  : m_pCache(nullptr)
	{
		Object::Register(this);
	}

	CachedObjectPointer(const CachedObjectPointer& cpy)
	  : m_pCache(cpy.m_pCache)
	  , m_bCacheIsSet(cpy.m_bCacheIsSet)
	{
		CachedObjectHelpers::Lock();
		Object::Register(this);
		CachedObjectHelpers::Unlock();
	}

	~CachedObjectPointer() { Object::Unregister(this); }

	auto Get(T** pRet) const -> bool
	{
		CachedObjectHelpers::Lock();
		if (!m_bCacheIsSet) {
			CachedObjectHelpers::Unlock();
			return false;
		}
		*pRet = m_pCache;
		CachedObjectHelpers::Unlock();
		return true;
	}

	void Set(T* p)
	{
		CachedObjectHelpers::Lock();
		m_pCache = p;
		m_bCacheIsSet = true;
		if (p != nullptr)
			p->m_CachedObject.m_pObject = p;
		CachedObjectHelpers::Unlock();
	}

	void Unset()
	{
		CachedObjectHelpers::Lock();
		m_pCache = nullptr;
		m_bCacheIsSet = false;
		CachedObjectHelpers::Unlock();
	}

  private:
	friend class CachedObject<T>;

	T* m_pCache;
	bool m_bCacheIsSet{ false };
};

#endif
