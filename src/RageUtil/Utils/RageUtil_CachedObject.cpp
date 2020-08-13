#include "RageUtil/Misc/RageThreads.h"
#include "RageUtil_CachedObject.h"

#if 0
/* Example use: */
struct Object
{
	CachedObject<Object> m_CachedObject;

	Object() { }
};

struct User
{
	CachedObjectPointer<Object> cache;

	Object *foobar()
	{
		Object *p;
		if( !cache.Get(&p) )
		{
			p = NULL;
			cache.Set(p);
		}
		return p;
	}
};

void test()
{
	Object p;
	User gar;
	gar.foobar();
}
#endif

static RageMutex m_Mutex("CachedObjects");
void
CachedObjectHelpers::Lock()
{
	m_Mutex.Lock();
}

void
CachedObjectHelpers::Unlock()
{
	m_Mutex.Unlock();
}
