/* XmlFile - Simple XML reading and writing. */

#ifndef XML_FILE_H
#define XML_FILE_H

#include <map>

struct DateTime;
class RageFileBasic;
struct lua_State;

class XNodeValue
{
  public:
	virtual ~XNodeValue() = default;
	[[nodiscard]] virtual auto Copy() const -> XNodeValue* = 0;

	virtual void GetValue(std::string& out) const = 0;
	virtual void GetValue(int& out) const = 0;
	virtual void GetValue(float& out) const = 0;
	virtual void GetValue(bool& out) const = 0;
	virtual void GetValue(unsigned& out) const = 0;
	virtual void PushValue(lua_State* L) const = 0;

	template<typename T>
	auto GetValue() const -> T
	{
		T val;
		GetValue(val);
		return val;
	}

	virtual void SetValue(const std::string& v) = 0;
	virtual void SetValue(int v) = 0;
	virtual void SetValue(float v) = 0;
	virtual void SetValue(unsigned v) = 0;
	virtual void SetValueFromStack(lua_State* L) = 0;
};

class XNodeStringValue : public XNodeValue
{
  public:
	std::string m_sValue;

	[[nodiscard]] auto Copy() const -> XNodeValue* override
	{
		return new XNodeStringValue(*this);
	}

	void GetValue(std::string& out) const override;
	void GetValue(int& out) const override;
	void GetValue(float& out) const override;
	void GetValue(bool& out) const override;
	void GetValue(unsigned& out) const override;
	void PushValue(lua_State* L) const override;

	void SetValue(const std::string& v) override;
	void SetValue(int v) override;
	void SetValue(float v) override;
	void SetValue(unsigned v) override;
	void SetValueFromStack(lua_State* L) override;
};

using XAttrs = std::map<std::string, XNodeValue*>;
class XNode;
using XNodes = std::vector<XNode*>;
/** @brief Loop through each node. */
#define FOREACH_Attr(pNode, Var)                                               \
	for (XAttrs::iterator Var = (pNode)->m_attrs.begin();                      \
		 (Var) != (pNode)->m_attrs.end();                                      \
		 ++(Var))
/** @brief Loop through each node, using a constant iterator. */
#define FOREACH_CONST_Attr(pNode, Var)                                         \
	for (XAttrs::const_iterator Var = (pNode)->m_attrs.begin();                \
		 (Var) != (pNode)->m_attrs.end();                                      \
		 ++(Var))
/** @brief Loop through each child. */
#define FOREACH_Child(pNode, Var)                                              \
	XNode* Var = NULL;                                                         \
	for (XNodes::iterator Var##Iter = (pNode)->GetChildrenBegin();             \
		 (Var) = (Var##Iter != (pNode)->GetChildrenEnd()) ? *Var##Iter : NULL, \
						  Var##Iter != (pNode)->GetChildrenEnd();              \
		 ++Var##Iter)
/** @brief Loop through each child, using a constant iterator. */
#define FOREACH_CONST_Child(pNode, Var)                                        \
	const XNode* Var = NULL;                                                   \
	for (XNodes::const_iterator Var##Iter = (pNode)->GetChildrenBegin();       \
		 (Var) = (Var##Iter != (pNode)->GetChildrenEnd()) ? *Var##Iter : NULL, \
								Var##Iter != (pNode)->GetChildrenEnd();        \
		 ++Var##Iter)

class XNode
{
  private:
	XNodes m_childs; // child nodes
	std::multimap<std::string, XNode*> m_children_by_name;

  public:
	std::string m_sName;
	XAttrs m_attrs; // attributes

	void SetName(const std::string& sName) { m_sName = sName; }
	[[nodiscard]] auto GetName() const -> const std::string& { return m_sName; }

	static const std::string TEXT_ATTRIBUTE;
	template<typename T>
	void GetTextValue(T& out) const
	{
		GetAttrValue(TEXT_ATTRIBUTE, out);
	}

	// in own attribute list
	[[nodiscard]] auto GetAttr(const std::string& sAttrName) const
	  -> const XNodeValue*;
	auto GetAttr(const std::string& sAttrName) -> XNodeValue*;
	template<typename T>
	auto GetAttrValue(const std::string& sName, T& out) const -> bool
	{
		const XNodeValue* pAttr = GetAttr(sName);
		if (pAttr == nullptr) {
			return false;
		}
		pAttr->GetValue(out);
		return true;
	}
	auto PushAttrValue(lua_State* L, const std::string& sName) const -> bool;

	auto GetChildrenBegin() -> XNodes::iterator { return m_childs.begin(); }
	[[nodiscard]] auto GetChildrenBegin() const -> XNodes::const_iterator
	{
		return m_childs.begin();
	}
	auto GetChildrenEnd() -> XNodes::iterator { return m_childs.end(); }
	[[nodiscard]] auto GetChildrenEnd() const -> XNodes::const_iterator
	{
		return m_childs.end();
	}
	[[nodiscard]] auto ChildrenEmpty() const -> bool
	{
		return m_childs.empty();
	}

	// in one level child nodes
	[[nodiscard]] auto GetChild(const std::string& sName) const -> const XNode*;
	auto GetChild(const std::string& sName) -> XNode*;
	template<typename T>
	auto GetChildValue(const std::string& sName, T& out) const -> bool
	{
		const XNode* pChild = GetChild(sName);
		if (pChild == nullptr) {
			return false;
		}
		pChild->GetTextValue(out);
		return true;
	}
	auto PushChildValue(lua_State* L, const std::string& sName) const -> bool;

	// modify DOM
	template<typename T>
	auto AppendChild(const std::string& sName, T value) -> XNode*
	{
		XNode* p = AppendChild(sName);
		p->AppendAttr(XNode::TEXT_ATTRIBUTE, value);
		return p;
	}
	auto AppendChild(const std::string& sName) -> XNode*
	{
		auto* p = new XNode(sName);
		return AppendChild(p);
	}
	auto AppendChild(XNode* node) -> XNode*;
	auto RemoveChild(XNode* node, bool bDelete = true) -> bool;
	void RemoveChildFromByName(XNode* node);
	void RenameChildInByName(XNode* node);

	auto AppendAttrFrom(const std::string& sName,
						XNodeValue* pValue,
						bool bOverwrite = true) -> XNodeValue*;
	auto AppendAttr(const std::string& sName) -> XNodeValue*;
	template<typename T>
	auto AppendAttr(const std::string& sName, T value) -> XNodeValue*
	{
		XNodeValue* pVal = AppendAttr(sName);
		pVal->SetValue(value);
		return pVal;
	}
	auto RemoveAttr(const std::string& sName) -> bool;

	XNode();
	explicit XNode(const std::string& sName);
	XNode(const XNode& cpy);
	~XNode() { Free(); }

	void Clear();

  private:
	void Free();
	auto operator=(const XNode& cpy) -> XNode& = delete; // don't use
};

#endif
