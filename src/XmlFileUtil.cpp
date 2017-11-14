#include "global.h"
#include "XmlFileUtil.h"
#include "XmlFile.h"
#include "RageFile.h"
#include "RageFileDriverMemory.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "arch/Dialog/Dialog.h"
#include "Foreach.h"
#include "LuaManager.h"

bool XmlFileUtil::LoadFromFileShowErrors( XNode &xml, RageFileBasic &f )
{
	RString sError;
	RString s;
	if( f.Read( s ) == -1 )
		sError = f.GetError();
	else
		Load( &xml, s, sError );
	if( sError.empty() )
		return true;

	RString sWarning = ssprintf( "XML: LoadFromFile failed: %s", sError.c_str() );
	LuaHelpers::ReportScriptError(sWarning, "XML_PARSE_ERROR");
	return false;
}

bool XmlFileUtil::LoadFromFileShowErrors( XNode &xml, const RString &sFile )
{
	RageFile f;
	if( !f.Open(sFile, RageFile::READ) )
	{
		LuaHelpers::ReportScriptErrorFmt("Couldn't open %s for reading: %s", sFile.c_str(), f.GetError().c_str() );
		return false;
	}

	bool bSuccess = LoadFromFileShowErrors( xml, f );
	if( !bSuccess )
	{
		RString sWarning = ssprintf( "XML: LoadFromFile failed for file: %s", sFile.c_str() );
		LuaHelpers::ReportScriptError(sWarning, "XML_PARSE_ERROR");
	}
	return bSuccess;
}

static const char chXMLTagOpen		= '<';
static const char chXMLTagClose		= '>';
static const char chXMLQuestion		= '?';	// used in checking for meta tags: "<?TAG ... ?/>"
static const char chXMLTagPre			= '/';
static const char chXMLExclamation	= '!';
static const char chXMLDash			= '-';

static map<RString,RString> g_mapEntitiesToChars;
static map<char,RString> g_mapCharsToEntities;

static void InitEntities()
{
	if( !g_mapEntitiesToChars.empty() )
		return;

	static struct Entity
	{
		char c;
		const char *pEntity;
	}
	const EntityTable[] =
	{
		{ '&',  "amp", },
		{ '\"', "quot", },
		{ '\'', "apos", },
		{ '<',  "lt", },
		{ '>',  "gt", } 
	};

	for(auto ent : EntityTable)
	{
			g_mapEntitiesToChars[ent.pEntity] = RString(1, ent.c);
		g_mapCharsToEntities[ent.c] = ent.pEntity;
	}
}

// skip spaces
static void tcsskip( const RString &s, RString::size_type &i )
{
	i = s.find_first_not_of( " \t\r\n", i );
}

// put string of (psz~end) on ps string
static void SetString( const RString &s, int iStart, int iEnd, RString* ps, bool trim = false )
{
	if( trim )
	{
		while( iStart < iEnd && s[iStart] > 0 && isspace(s[iStart]) )
			iStart++;
		while( iEnd-1 >= iStart && s[iEnd-1] > 0 && isspace(s[iEnd-1]) )
			iEnd--;
	}

	int len = iEnd - iStart;
	if( len <= 0 )
		return;

	ps->assign( s, iStart, len );
}

// attr1="value1" attr2='value2' attr3=value3 />
//                                            ^- return pointer
// Desc   : loading attribute plain xml text
// Param  : pszAttrs - xml of attributes
//          pi = parser information
// Return : advanced string pointer. (error return npos)
namespace
{
RString::size_type LoadAttributes( XNode *pNode, const RString &xml, RString &sErrorOut, RString::size_type iOffset )
{
	while( iOffset < xml.size() )
	{
		tcsskip( xml, iOffset );
		if( iOffset >= xml.size()  )
			continue;

		// close tag
		if( iOffset < xml.size() &&
			(xml[iOffset] == chXMLTagClose || xml[iOffset] == chXMLTagPre || xml[iOffset] == chXMLQuestion || xml[iOffset] == chXMLDash) )
			return iOffset; // well-formed tag

		// XML Attr Name
		RString::size_type iEnd = xml.find_first_of( " =", iOffset );
		if( iEnd == xml.npos ) 
		{
			// error
			if( sErrorOut.empty() ) 
				sErrorOut = ssprintf( "<%s> attribute has error ", pNode->GetName().c_str() );
			return string::npos;
		}

		// XML Attr Name
		RString sName;
		SetString( xml, iOffset, iEnd, &sName );

		// add new attribute
		DEBUG_ASSERT( sName.size() );
		XNodeValue *pAttr = pNode->AppendAttr( sName );
		iOffset = iEnd;

		// XML Attr Value
		tcsskip( xml, iOffset );

		if( iOffset < xml.size() && xml[iOffset] == '=' )
		{
			++iOffset;

			tcsskip( xml, iOffset );
			if( iOffset >= xml.size()  )
				continue;

			// if " or '
			// or none quote
			char quote = xml[iOffset];
			if( quote == '"' || quote == '\'' )
			{
				++iOffset;
				iEnd = xml.find( quote, iOffset );
			}
			else
			{
				// XXX: This is invalid XML, should we be accepting it? -- Steve
				//attr= value>
				// none quote mode
				iEnd = xml.find_first_of( " >", iOffset );
			}

			if( iEnd == xml.npos ) 
			{
				// error
				if( sErrorOut.empty() ) 
					sErrorOut = ssprintf( "<%s> attribute text: couldn't find matching quote", sName.c_str() );
				return string::npos;
			}

			RString sValue;
			SetString( xml, iOffset, iEnd, &sValue, true );
			ReplaceEntityText( sValue, g_mapEntitiesToChars );
			pAttr->SetValue( sValue );
			iOffset = iEnd;

			if( quote == '"' || quote == '\'' )
				++iOffset;
		}
	}

	// not well-formed tag
	return string::npos;
}

// <TAG attr1="value1" attr2='value2' attr3=value3 >
// </TAG>
// or
// <TAG />
//        ^- return pointer
// Desc   : load xml plain text
// Param  : pszXml - plain xml text
//          pi = parser information
// Return : advanced string pointer  (error return npos)
RString::size_type LoadInternal( XNode *pNode, const RString &xml, RString &sErrorOut, RString::size_type iOffset )
{
	pNode->Clear();

	// <
	iOffset = xml.find( chXMLTagOpen, iOffset );
	if( iOffset == string::npos )
		return string::npos;

	// </
	if( xml[iOffset+1] == chXMLTagPre )
		return iOffset;

	// <!--
	if( !xml.compare(iOffset+1, 3, "!--") )
	{
		iOffset += 4;

		// Find the close tag.
		RString::size_type iEnd = xml.find( "-->", iOffset );
		if( iEnd == string::npos )
		{
			if( sErrorOut.empty() ) 
				sErrorOut = "Unterminated comment";

			return string::npos;
		}

		// Skip -->.
		iOffset = iEnd + 3;

		return LoadInternal( pNode, xml, sErrorOut, iOffset );
	}

	// XML Node Tag Name Open
	iOffset++;
	RString::size_type iTagEnd = xml.find_first_of( " \t\r\n/>", iOffset );
	RString sName;
	SetString( xml, iOffset, iTagEnd, &sName );
	pNode->SetName( sName );
	iOffset = iTagEnd;

	// Generate XML Attribute List
	iOffset = LoadAttributes( pNode, xml, sErrorOut, iOffset );
	if( iOffset == string::npos )
		return string::npos;

	// alone tag <TAG ... /> or <?TAG ... ?> or <!-- ... --> 
	// current pointer:   ^               ^              ^

	if( iOffset < xml.size() && (xml[iOffset] == chXMLTagPre || xml[iOffset] == chXMLQuestion || xml[iOffset] == chXMLDash) )
	{
		iOffset++;

		// skip over 2nd dash
		if( iOffset < xml.size() && xml[iOffset] == chXMLDash )
			iOffset++;

		if( iOffset == xml.size() || xml[iOffset] != chXMLTagClose )
		{
			// error: <TAG ... / >
			if( sErrorOut.empty() ) 
				sErrorOut = "Element must be closed.";

			// ill-formed tag
			return string::npos;
		}

		// well-formed tag
		++iOffset;

		// UGLY: We want to ignore all XML meta tags.  So, since the Node we 
		// just loaded is a meta tag, then Load ourself again using the rest 
		// of the file until we reach a non-meta tag.
		if( !pNode->GetName().empty() && (pNode->GetName()[0] == chXMLQuestion || pNode->GetName()[0] == chXMLExclamation) )
			iOffset = LoadInternal( pNode, xml, sErrorOut, iOffset );

		return iOffset;
	}

	// open/close tag <TAG ..> ... </TAG>
	//                        ^- current pointer
	if( pNode->GetAttr(XNode::TEXT_ATTRIBUTE) == NULL )
	{
		// Text Value 
		++iOffset;
		RString::size_type iEnd = xml.find( chXMLTagOpen, iOffset );
		if( iEnd == string::npos )
		{
			if( sErrorOut.empty() ) 
				sErrorOut = ssprintf( "%s must be closed with </%s>", pNode->GetName().c_str(), pNode->GetName().c_str() );
			// error cos not exist CloseTag </TAG>
			return string::npos;
		}

		RString sValue;
		SetString( xml, iOffset, iEnd, &sValue, true );

		iOffset = iEnd;
		ReplaceEntityText( sValue, g_mapEntitiesToChars );

		pNode->AppendAttr(XNode::TEXT_ATTRIBUTE, sValue );
	}

	// generate child nodes
	while( iOffset < xml.size() )
	{
		XNode *node = new XNode;

		iOffset = LoadInternal( node, xml, sErrorOut, iOffset );
		if( iOffset == string::npos )
		{
			delete node;
			return iOffset;
		}

		if( !node->GetName().empty() )
		{
			pNode->AppendChild(node);
		}
		else
		{
			delete node;
		}

		// open/close tag <TAG ..> ... </TAG>
		//                             ^- current pointer
		// CloseTag case
		if( iOffset+1 < xml.size() && xml[iOffset] == chXMLTagOpen && xml[iOffset+1] == chXMLTagPre )
		{
			// </Close>
			iOffset += 2; // C

			tcsskip( xml, iOffset );
			if( iOffset >= xml.size()  )
				continue;

			RString::size_type iEnd = xml.find_first_of( " >", iOffset );
			if( iEnd == string::npos )
			{
				if( sErrorOut.empty() ) 
					sErrorOut = ssprintf( "it must be closed with </%s>", pNode->GetName().c_str() );
				// error
				return string::npos;
			}

			RString closename;
			SetString( xml, iOffset, iEnd, &closename );
			iOffset = iEnd+1;
			if( closename == pNode->GetName() )
			{
				// wel-formed open/close
				// return '>' or ' ' after pointer
				return iOffset;
			}
			else
			{
				// not welformed open/close
				if( sErrorOut.empty() ) 
					sErrorOut = ssprintf( "'<%s> ... </%s>' is not well-formed.", pNode->GetName().c_str(), closename.c_str() );
				return string::npos;
			}
		}
		else	// Alone child Tag Loaded
		{
			if( pNode->GetAttr(XNode::TEXT_ATTRIBUTE) == NULL && iOffset < xml.size() && xml[iOffset] != chXMLTagOpen )
			{
				// Text Value 
				RString::size_type iEnd = xml.find( chXMLTagOpen, iOffset );
				if( iEnd == string::npos ) 
				{
					// error cos not exist CloseTag </TAG>
					if( sErrorOut.empty() )  
						sErrorOut = ssprintf( "it must be closed with </%s>", pNode->GetName().c_str() );
					return string::npos;
				}

				RString sValue;
				SetString( xml, iOffset, iEnd, &sValue, true );

				iOffset = iEnd;
				ReplaceEntityText( sValue, g_mapEntitiesToChars );
				pNode->AppendAttr(XNode::TEXT_ATTRIBUTE, sValue );
			}
		}
	}

	return iOffset;
}

bool GetXMLInternal( const XNode *pNode, RageFileBasic &f, bool bWriteTabs, int &iTabBase )
{
#define WRITE(x) if( f.Write(x) == -1 ) return false
	// tab
	WRITE( "\r\n" );
	if( bWriteTabs )
		for( int i = 0 ; i < iTabBase ; i++)
			WRITE( "\t" );

	// <TAG
	WRITE( "<" );
	WRITE( pNode->GetName() );

	// <TAG Attr1="Val1" 
	FOREACH_CONST_Attr( pNode, p )
	{
		if( p->first == XNode::TEXT_ATTRIBUTE )
			continue;
		RString attr( p->second->GetValue<RString>() );
		ReplaceEntityText( attr, g_mapCharsToEntities );
		WRITE( " " );
		WRITE( p->first );
		WRITE( "='" );
		WRITE( attr );
		WRITE( "'" );
	}

	if( pNode->ChildrenEmpty() && pNode->GetAttr(XNode::TEXT_ATTRIBUTE) == NULL )
	{
		// <TAG Attr1="Val1"/> alone tag 
		WRITE( "/>" );
	}
	else
	{
		// <TAG Attr1="Val1"> and get child
		WRITE( ">" );

		if( !pNode->ChildrenEmpty() )
			iTabBase++;

		FOREACH_CONST_Child( pNode, p )
			if( !GetXMLInternal( p, f, bWriteTabs, iTabBase ) )
				return false;

		// Text Value
		const XNodeValue *pText = pNode->GetAttr( XNode::TEXT_ATTRIBUTE );
		if( pText != NULL )
		{
			if( !pNode->ChildrenEmpty() )
			{
				WRITE( "\r\n" );
				if( bWriteTabs )
					for( int i = 0 ; i < iTabBase ; i++)
						WRITE( "\t" );
			}
			RString s;
			pText->GetValue( s );
			ReplaceEntityText( s, g_mapCharsToEntities );
			WRITE( s );
		}

		// </TAG> CloseTag
		if( !pNode->ChildrenEmpty() )
		{
			WRITE( "\r\n" );
			if( bWriteTabs )
				for( int i = 0 ; i < iTabBase-1 ; i++)
					WRITE( "\t" );
		}
		WRITE( "</" );
		WRITE( pNode->GetName() );
		WRITE( ">" );

		if( !pNode->ChildrenEmpty() )
			iTabBase--;
	}
	return true;
#undef WRITE
}
}

void XmlFileUtil::Load( XNode *pNode, const RString &sXml, RString &sErrorOut )
{
	InitEntities();
	LoadInternal( pNode, sXml, sErrorOut, 0 );
}

bool XmlFileUtil::GetXML( const XNode *pNode, RageFileBasic &f, bool bWriteTabs )
{
	int iTabBase = 0;
	InitEntities();
	return GetXMLInternal( pNode, f, bWriteTabs, iTabBase );
}

RString XmlFileUtil::GetXML( const XNode *pNode )
{
	RageFileObjMem f;
	int iTabBase = 0;
	InitEntities();
	GetXMLInternal( pNode, f, true, iTabBase );
	return f.GetString();
}

bool XmlFileUtil::SaveToFile( const XNode *pNode, RageFileBasic &f, const RString &sStylesheet, bool bWriteTabs )
{
	f.PutLine( "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" );
	if( !sStylesheet.empty() )
		f.PutLine( "<?xml-stylesheet type=\"text/xsl\" href=\"" + sStylesheet + "\"?>" );
	int iTabBase = 0;
	InitEntities();
	if( !GetXMLInternal(pNode, f, bWriteTabs, iTabBase) )
		return false;
	f.PutLine( "" );
	if( f.Flush() == -1 )
		return false;
	return true;
}

bool XmlFileUtil::SaveToFile( const XNode *pNode, const RString &sFile, const RString &sStylesheet, bool bWriteTabs )
{
	RageFile f;
	if( !f.Open(sFile, RageFile::WRITE) )
	{
		LuaHelpers::ReportScriptErrorFmt( "Couldn't open %s for writing: %s", sFile.c_str(), f.GetError().c_str() );
		return false;
	}

	return SaveToFile( pNode, f, sStylesheet, bWriteTabs );
}

#include "LuaReference.h"
class XNodeLuaValue: public XNodeValue
{
public:
	LuaReference m_Value;
	XNodeValue *Copy() const override { return new XNodeLuaValue( *this ); }

	template<typename T>
	T GetValue() const { T val; GetValue(val); return val; }

	void GetValue( RString &out ) const override;
	void GetValue( int &out ) const override;
	void GetValue( float &out ) const override;
	void GetValue( bool &out ) const override;
	void GetValue( unsigned &out ) const override;
	void PushValue( lua_State *L ) const override;

	void SetValue( const RString &v ) override;
	void SetValue( int v ) override;
	void SetValue( float v ) override;
	void SetValue( unsigned v ) override;
	void SetValueFromStack( lua_State *L ) override;
};

void XNodeLuaValue::PushValue( lua_State *L ) const
{
	m_Value.PushSelf( L );
}

void XNodeLuaValue::GetValue( RString &out ) const { Lua *L = LUA->Get(); PushValue( L ); LuaHelpers::Pop( L, out ); LUA->Release( L ); }
void XNodeLuaValue::GetValue( int &out ) const { Lua *L = LUA->Get(); PushValue( L ); LuaHelpers::Pop( L, out ); LUA->Release( L ); }
void XNodeLuaValue::GetValue( float &out ) const { Lua *L = LUA->Get(); PushValue( L ); LuaHelpers::Pop( L, out ); LUA->Release( L ); }
void XNodeLuaValue::GetValue( bool &out ) const { Lua *L = LUA->Get(); PushValue( L ); LuaHelpers::Pop( L, out ); LUA->Release( L ); }
void XNodeLuaValue::GetValue( unsigned &out ) const { Lua *L = LUA->Get(); PushValue( L ); float fVal; LuaHelpers::Pop( L, fVal ); out = unsigned(fVal); LUA->Release( L ); }

void XNodeLuaValue::SetValueFromStack( lua_State *L )
{
	m_Value.SetFromStack( L );
}

void XNodeLuaValue::SetValue( const RString &v ) { Lua *L = LUA->Get(); LuaHelpers::Push( L, v ); SetValueFromStack( L ); LUA->Release( L ); }
void XNodeLuaValue::SetValue( int v ) { Lua *L = LUA->Get(); LuaHelpers::Push( L, v ); SetValueFromStack( L ); LUA->Release( L ); }
void XNodeLuaValue::SetValue( float v ) { Lua *L = LUA->Get(); LuaHelpers::Push( L, v ); SetValueFromStack( L ); LUA->Release( L ); }
void XNodeLuaValue::SetValue( unsigned v ) { Lua *L = LUA->Get(); LuaHelpers::Push( L, (float) v ); SetValueFromStack( L ); LUA->Release( L ); }

namespace
{
	XNodeValue *CompileXMLNodeValue( Lua *L, const RString &sName, const XNodeValue *pValue, const RString &sFile )
	{
		RString sExpression;
		pValue->GetValue( sExpression );

		if( EndsWith(sName, "Command") )
		{
			// Use legacy parsing
			LuaHelpers::ParseCommandList( L, sExpression, sFile, true );
		}
		else if( sExpression.size() > 0 && sExpression[0] == '@' )
		{
			// Lua expression
			sExpression.erase( 0, 1 );
			LuaHelpers::RunExpression( L, sExpression, sFile );
		}
		else
		{
			// This is a raw string.
			LuaHelpers::Push( L, sExpression );
		}

		auto *pRet = new XNodeLuaValue;
		pRet->SetValueFromStack( L );
		return pRet;
	}
}

void XmlFileUtil::AnnotateXNodeTree( XNode *pNode, const RString &sFile )
{
	RString sDir = Dirname( sFile );

	vector<XNode *> queue;
	queue.push_back( pNode );
	while( !queue.empty() )
	{
		pNode = queue.back();
		queue.pop_back();
		FOREACH_Child( pNode, pChild )
			queue.push_back( pChild );

		/* Source file, for error messages: */
		pNode->AppendAttr( "_Source", sFile );

		/* Directory of caller, for relative paths: */
		pNode->AppendAttr( "_Dir", sDir );

		/* Note that this node came from a legacy XML file */
		pNode->AppendAttr( "_LegacyXml", true );
	}
}

void XmlFileUtil::CompileXNodeTree( XNode *pNode, const RString &sFile )
{
	vector<XNode *> aToCompile;
	aToCompile.push_back( pNode );

	Lua *L = LUA->Get();
	while( aToCompile.size() )
	{
		pNode = aToCompile.back();
		aToCompile.pop_back();
		FOREACH_Child( pNode, pChild )
			aToCompile.push_back( pChild );

		FOREACH_Attr( pNode, pAttr )
		{
			XNodeValue *pValue = CompileXMLNodeValue( L, pAttr->first, pAttr->second, sFile );
			delete pAttr->second;
			pAttr->second = pValue;
		}
	}

	LUA->Release( L );
}

namespace
{
	XNode *XNodeFromTableRecursive( lua_State *L, const RString &sName, LuaReference &ProcessedTables )
	{
		auto *pNode = new XNode( sName );

		// Set the value of the node to the table.
		{
			auto *pValue = new XNodeLuaValue;
			lua_pushvalue( L, -1 );
			pValue->SetValueFromStack( L );
			pNode->AppendAttrFrom( XNode::TEXT_ATTRIBUTE, pValue );
		}

		// Iterate over the table, pulling out attributes and tables to process.
		vector<RString> NodeNamesToAdd;
		vector<LuaReference> NodesToAdd;

		/* Add array elements first, in array order, so iterating over the XNode
		 * keeps the array in order. */
		FOREACH_LUATABLEI( L, -1, i )
		{
			if( !lua_istable(L, -1) )
				continue;
			NodeNamesToAdd.push_back( ssprintf("%i", i) );
			NodesToAdd.push_back( LuaReference() );
			NodesToAdd.back().SetFromStack( L );
		}

		int iLen = lua_objlen(L, -1);
		FOREACH_LUATABLE( L, -1 )
		{
			// If this entry is a table, add it recursively.
			if( lua_istable(L, -2) )
			{
				if( lua_isnumber(L, -1) )
				{
					/* If this number is an integer, and between [1,iLen], then
					 * we added this one already above. */
					lua_Number f = lua_tonumber( L, -1 );
					int i;
					lua_number2int(i, f);
					if( i >= 1 && i <= iLen && float(i) == f )
						continue;
				}

				RString nName;
				LuaHelpers::Pop( L, nName );
				NodeNamesToAdd.push_back( nName );
				NodesToAdd.push_back( LuaReference() );
				NodesToAdd.back().SetFromStack( L );
				continue;
			}

			RString nName;
			LuaHelpers::Pop( L, nName );

			// Otherwise, add an attribute.
			auto *pValue = new XNodeLuaValue;
			pValue->SetValueFromStack( L );
			pNode->AppendAttrFrom( nName, pValue );
		}
		lua_pop( L, 1 );

		// Recursively process tables.
		for( size_t i = 0; i < NodesToAdd.size(); ++i )
		{
			const RString &sNodeName = NodeNamesToAdd[i];
			LuaReference &NodeToAdd = NodesToAdd[i];

			// Check if the table is on the stack.
			ProcessedTables.PushSelf( L );
			NodeToAdd.PushSelf( L ); // push table
			lua_gettable( L, -2 );

			bool bSawThisTableAlready = !lua_isnil(L, -1);
			lua_pop( L, 2 ); // pop lua_gettable result, ProcessedTables
			if( bSawThisTableAlready )
				continue;

			// Add the table to the stack.
			ProcessedTables.PushSelf( L );
			NodeToAdd.PushSelf( L );
			lua_pushboolean( L, true );
			lua_settable( L, -3 );
			lua_pop( L, 1 ); // pop ProcessedTables

			NodeToAdd.PushSelf( L );
			XNode *pNewNode = XNodeFromTableRecursive( L, sNodeName, ProcessedTables );
			if( pNewNode )
				pNode->AppendChild( pNewNode );

			// Remove the table from the stack.
			ProcessedTables.PushSelf( L );
			NodeToAdd.PushSelf( L );
			lua_pushnil( L );
			lua_settable( L, -3 );
			lua_pop( L, 1 ); // pop ProcessedTables
		}

		return pNode;
	}

}

/* Pop a table off of the stack, and return an XNode tree referring recursively
 * to entries in the table.
 *
 * The table may not contain table cycles; if a cycle is detected, only the first
 * table seen will have a corresponding XNode.
 *
 * Users of the resulting XNode may access the original table via PushValue. */
XNode *XmlFileUtil::XNodeFromTable( lua_State *L )
{
	/* Maintain a set of references that we've created. Tables may loop; XNode
	 * trees may not. If we encounter a cycle, skip creating an XNode for
	 * that node. */
	LuaReference ProcessedTables;
	lua_newtable( L );
	ProcessedTables.SetFromStack( L );

	return XNodeFromTableRecursive( L, "Layer", ProcessedTables );
}

/* Move nodes from pFrom into pTo which don't already exist in pTo. For
 * efficiency, nodes will be moved, not copied, so pFrom will be modified.
 * On return, the contents of pFrom will be undefined and should be deleted. */
void XmlFileUtil::MergeIniUnder( XNode *pFrom, XNode *pTo )
{
	/* Batch up nodes to move, and do them all at once, to deal sanely
	 * with the possibility of duplicate child names. */
	vector<XNodes::iterator> aToMove;

	// Iterate over each section in pFrom.
	XNodes::iterator it = pFrom->GetChildrenBegin();
	while( it != pFrom->GetChildrenEnd() )
	{
		XNodes::iterator next = it;
		++next;

		// If this node doesn't exist in pTo, just move the whole node.
		XNode *pSectionNode = *it;
		XNode *pChildNode = pTo->GetChild( pSectionNode->GetName() );
		if( pChildNode == NULL )
		{
			aToMove.push_back( it );
		}
		else
		{
			FOREACH_Attr( pSectionNode, it2 )
			{
				// Don't overwrite existing nodes.
				pChildNode->AppendAttrFrom( it2->first, it2->second->Copy(), false );
			}
		}

		it = next;
	}

	/* Iterate in reverse, since erasing iterators will invalidate the
	 * iterators after it. */
	for( int i = aToMove.size()-1; i >= 0; --i )
	{
		XNode *pNode = *aToMove[i];
		pFrom->RemoveChild(pNode, false);
		pTo->AppendChild( pNode );
	}
}

/*
 * (c) 2001-2006 Chris Danford, Glenn Maynard
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
