/** @brief Utilities for handling JSON data. */
#ifndef JsonUtil_H
#define JsonUtil_H

class RageFileBasic;
#include "json/value.h"

namespace JsonUtil {
bool
LoadFromString(Json::Value& root, const RString& sData, RString& sErrorOut);
bool
LoadFromStringShowErrors(Json::Value& root, const RString& sData);
bool
LoadFromFileShowErrors(Json::Value& root, const RString& sFile);
bool
LoadFromFileShowErrors(Json::Value& root, RageFileBasic& f);

bool
WriteFile(const Json::Value& root, const RString& sFile, bool bMinified);

template<class T>
static void
SerializeVectorObjects(const std::vector<T>& v,
					   void fn(const T&, Json::Value&),
					   Json::Value& root)
{
	root = Json::Value(Json::arrayValue);
	root.resize(v.size());
	for (unsigned i = 0; i < v.size(); i++)
		fn(v[i], root[i]);
}

template<class T>
static void
SerializeVectorPointers(const std::vector<const T*>& v,
						void fn(const T&, Json::Value&),
						Json::Value& root)
{
	root = Json::Value(Json::arrayValue);
	root.resize(v.size());
	for (unsigned i = 0; i < v.size(); i++)
		fn(*v[i], root[i]);
}

template<class T>
static void
SerializeVectorPointers(const std::vector<T*>& v,
						void fn(const T&, Json::Value&),
						Json::Value& root)
{
	root = Json::Value(Json::arrayValue);
	root.resize(v.size());
	for (unsigned i = 0; i < v.size(); i++)
		fn(*v[i], root[i]);
}

template<class T>
static void
SerializeVectorPointers(const std::vector<const T*>& v,
						void fn(const T*, Json::Value&),
						Json::Value& root)
{
	root = Json::Value(Json::arrayValue);
	root.resize(v.size());
	for (unsigned i = 0; i < v.size(); i++)
		fn(*v[i], root[i]);
}

template<typename V, typename T>
static void
SerializeArray(const V& v, void fn(const T*, Json::Value&), Json::Value& root)
{
	root = Json::Value(Json::arrayValue);
	root.resize(v.size());
	int i = 0;
	for (typename V::const_iterator iter = v.begin(); iter != v.end(); iter++)
		fn(*iter, root[i++]);
}

template<typename V>
static void
SerializeArrayValues(const V& v, Json::Value& root)
{
	root = Json::Value(Json::arrayValue);
	root.resize(v.size());
	int i = 0;
	for (typename V::const_iterator iter = v.begin(); iter != v.end(); iter++)
		root[i++] = *iter;
}

template<typename V>
static void
SerializeArrayObjects(const V& v, Json::Value& root)
{
	root = Json::Value(Json::arrayValue);
	root.resize(v.size());
	int i = 0;
	for (typename V::const_iterator iter = v.begin(); iter != v.end(); iter++)
		iter->Serialize(root[i++]);
}

template<typename M, typename E, typename F>
static void
SerializeStringToObjectMap(const M& m, F fnEnumToString, Json::Value& root)
{
	for (typename M::const_iterator iter = m.begin(); iter != m.end(); iter++)
		iter->second.Serialize(root[fnEnumToString(iter->first)]);
}

template<typename M, typename E, typename F>
static void
SerializeStringToValueMap(const M& m, F fnToString, Json::Value& root)
{
	for (typename M::const_iterator iter = m.begin(); iter != m.end(); iter++)
		root[fnToString(iter->first)] = iter->second;
}

template<typename M>
static void
SerializeValueToValueMap(const M& m, Json::Value& root)
{
	for (typename M::const_iterator iter = m.begin(); iter != m.end(); iter++)
		root[(iter->first)] = iter->second;
}

// Serialize a map that has a non-string key type
template<typename V>
static void
SerializeObjectToObjectMapAsArray(const V& v,
								  const RString& sKeyName,
								  const RString& sValueName,
								  Json::Value& root)
{
	root = Json::Value(Json::arrayValue);
	root.resize(v.size());
	int i = 0;
	for (typename V::const_iterator iter = v.begin(); iter != v.end(); iter++) {
		Json::Value& vv = root[i++];
		iter->first.Serialize(vv[sKeyName]);
		iter->second.Serialize(vv[sValueName]);
	}
}

template<typename V>
static void
SerializeObjectToValueMapAsArray(const V& v,
								 const RString& sKeyName,
								 const RString& sValueName,
								 Json::Value& root)
{
	root = Json::Value(Json::arrayValue);
	root.resize(v.size());
	int i = 0;
	for (typename V::const_iterator iter = v.begin(); iter != v.end(); iter++) {
		Json::Value& vv = root[i++];
		iter->first.Serialize(vv[sKeyName]);
		vv[sValueName] = iter->second;
	}
}

template<class T>
static void
SerializeVectorValues(const std::vector<T>& v, Json::Value& root)
{
	root = Json::Value(Json::arrayValue);
	root.resize(v.size());
	for (unsigned i = 0; i < v.size(); i++)
		root[i] = v[i];
}

template<class T>
static void
DeserializeVectorObjects(std::vector<T>& v,
						 void fn(T&, const Json::Value&),
						 const Json::Value& root)
{
	v.resize(root.size());
	for (unsigned i = 0; i < v.size(); i++)
		fn(v[i], root[i]);
}

template<typename V>
static void
DeserializeArrayObjects(V& v, const Json::Value& root)
{
	v.resize(root.size());
	for (unsigned i = 0; i < v.size(); i++)
		v[i].Deserialize(root[i]);
}

template<class T>
static void
DeserializeVectorPointers(std::vector<T*>& v,
						  void fn(T&, const Json::Value&),
						  const Json::Value& root)
{
	for (unsigned i = 0; i < v.size(); i++)
		SAFE_DELETE(v[i]);
	v.resize(root.size());
	for (unsigned i = 0; i < v.size(); i++) {
		v[i] = new T;
		fn(*v[i], root[i]);
	}
}

template<class T>
static void
DeserializeVectorPointers(std::vector<T*>& v,
						  void fn(T*, const Json::Value&),
						  const Json::Value& root)
{
	for (unsigned i = 0; i < v.size(); i++)
		SAFE_DELETE(v[i]);
	v.resize(root.size());
	for (unsigned i = 0; i < v.size(); i++) {
		v[i] = new T;
		fn(*v[i], root[i]);
	}
}

/* For classes with one-parameter constructors, such as Steps */
template<class T, class P>
static void
DeserializeVectorPointersParam(std::vector<T*>& v,
							   void fn(T&, const Json::Value&),
							   const Json::Value& root,
							   const P param)
{
	for (unsigned i = 0; i < v.size(); i++)
		SAFE_DELETE(v[i]);
	v.resize(root.size());
	for (unsigned i = 0; i < v.size(); i++) {
		v[i] = new T(param);
		fn(*v[i], root[i]);
	}
}

template<class T>
static void
DeserializeArrayValues(std::vector<T>& v, const Json::Value& root)
{
	v.clear();
	for (unsigned i = 0; i < root.size(); i++) {
		T t;
		if (root[i].TryGet(t))
			v.push_back(t);
	}
}

// don't pull in the set header here
template<typename S, typename T>
static void
DeserializeArrayValuesIntoSet(S& s, const Json::Value& root)
{
	s.clear();
	for (unsigned i = 0; i < root.size(); i++) {
		T t;
		if (root[i].TryGet(t))
			s.insert(t);
	}
}

template<typename T>
static void
DeserializeArrayValuesIntoVector(std::vector<T>& v, const Json::Value& root)
{
	v.clear();
	for (unsigned i = 0; i < root.size(); i++) {
		T t;
		if (root[i].TryGet(t))
			v.push_back(t);
	}
}

template<typename M>
static void
DeserializeValueToValueMap(M& m, const Json::Value& root)
{
	for (Json::Value::const_iterator iter = root.begin(); iter != root.end();
		 iter++)
		(*iter).TryGet(m[iter.memberName()]);
}

template<typename M, typename E, typename F>
static void
DeserializeStringToValueMap(M& m, F fnToValue, const Json::Value& root)
{
	for (Json::Value::const_iterator iter = root.begin(); iter != root.end();
		 iter++)
		(*iter).TryGet(m[fnToValue(iter.memberName())]);
}

template<typename M, typename E, typename F>
static void
DeserializeStringToObjectMap(M& m, F fnToValue, const Json::Value& root)
{
	for (Json::Value::const_iterator iter = root.begin(); iter != root.end();
		 iter++)
		m[fnToValue(iter.memberName())].Deserialize(*iter);
}

// Serialize a map that has a non-string key type
template<typename K, typename V>
static void
DeserializeObjectToObjectMapAsArray(map<K, V>& m,
									const RString& sKeyName,
									const RString& sValueName,
									const Json::Value& root)
{
	m.clear();
	ASSERT(root.type() == Json::arrayValue);
	for (Json::Value::const_iterator iter = root.begin(); iter != root.end();
		 iter++) {
		ASSERT((*iter).type() == Json::objectValue);
		K k;
		if (!k.Deserialize((*iter)[sKeyName]))
			continue;
		V v;
		if (!v.Deserialize((*iter)[sValueName]))
			continue;
		m[k] = v;
	}
}

template<typename K, typename V>
static void
DeserializeObjectToValueMapAsArray(map<K, V>& m,
								   const RString& sKeyName,
								   const RString& sValueName,
								   const Json::Value& root)
{
	for (unsigned i = 0; i < root.size(); i++) {
		const Json::Value& root2 = root[i];
		K k;
		if (!k.Deserialize(root2[sKeyName]))
			continue;
		V v;
		if (!root2[sValueName].TryGet(v))
			continue;
		m[k] = v;
	}
}

template<class T>
static void
DeserializeVectorValues(std::vector<T>& v, const Json::Value& root)
{
	v.resize(root.size());
	for (unsigned i = 0; i < v.size(); i++)
		v[i] = root[i].asString();
}
}

#endif

/*
 * (c) 2010 Chris Danford
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
