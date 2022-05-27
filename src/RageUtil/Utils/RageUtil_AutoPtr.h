/* AutoPtrCopyOnWrite - Simple smart pointer template. */

#ifndef RAGE_UTIL_AUTO_PTR_H
#define RAGE_UTIL_AUTO_PTR_H


/*
 * This smart pointer template is used to safely hide implementations from
 * headers, to reduce dependencies.  This is the same as declaring a pointer
 * to a class, and allocating/deallocating it in the implementation: only
 * the implementation needs to include that class.  This makes copying
 * and deletion automatic, so you don't need to include a copy ctor or
 * remember to delete it.
 *
 * There's one subtlety: in order to copy or delete an object, we need its
 * definition.  This is intended to avoid pulling in the definition.  So,
 * we use a traits class to hide it.  Use REGISTER_CLASS_TRAITS for each
 * class used with this template.
 *
 * Concepts from http://www.gotw.ca/gotw/062.htm.
 */
template<class T>
struct HiddenPtrTraits
{
	static auto Copy(const T* pCopy) -> T*;
	static void Delete(T* p);
};
#define REGISTER_CLASS_TRAITS(T, CopyExpr)                                     \
	template<>                                                                 \
	T* HiddenPtrTraits<T>::Copy(const T* pCopy)                                \
	{                                                                          \
		return CopyExpr;                                                       \
	}                                                                          \
	template<>                                                                 \
	void HiddenPtrTraits<T>::Delete(T* p)                                      \
	{                                                                          \
		delete p;                                                              \
	}

template<class T>
class HiddenPtr
{
  public:
	auto operator*() const -> const T& { return *m_pPtr; }
	auto operator->() const -> const T* { return m_pPtr; }
	auto operator*() -> T& { return *m_pPtr; }
	auto operator->() -> T* { return m_pPtr; }

	explicit HiddenPtr(T* p = NULL)
	  : m_pPtr(p)
	{
	}

	HiddenPtr(const HiddenPtr<T>& cpy)
	  : m_pPtr(nullptr)
	{
		if (cpy.m_pPtr != nullptr) {
			m_pPtr = HiddenPtrTraits<T>::Copy(cpy.m_pPtr);
		}
	}

#if 0 // broken VC6
	template<class U>
	HiddenPtr( const HiddenPtr<U> &cpy )
	{
		if( cpy.m_pPtr == NULL )
			m_pPtr = NULL;
		else
			m_pPtr = HiddenPtrTraits<U>::Copy( cpy.m_pPtr );
	}
#endif

	~HiddenPtr() { HiddenPtrTraits<T>::Delete(m_pPtr); }
	void Swap(HiddenPtr<T>& rhs) { std::swap(m_pPtr, rhs.m_pPtr); }

	auto operator=(T* p) -> HiddenPtr<T>&
	{
		HiddenPtr<T> t(p);
		Swap(t);
		return *this;
	}

	auto operator=(const HiddenPtr& cpy) -> HiddenPtr<T>&
	{
		HiddenPtr<T> t(cpy);
		Swap(t);
		return *this;
	}

#if 0 // broken VC6
	template<class U>
	HiddenPtr<T> &operator=( const HiddenPtr<U> &cpy )
	{
		HiddenPtr<T> t( cpy );
		Swap( t );
		return *this;
	}
#endif

  private:
	T* m_pPtr;

#if 0 // broken VC6
	template<class U>
	friend class HiddenPtr;
#endif
};

template<class T>
inline void
swap(HiddenPtr<T>& a, HiddenPtr<T>& b)
{
	a.Swap(b);
}

#endif
