#include "global.h"
#include "RageLog.h"
#include "InputHandler_MacOSX_HID.h"
#include "Foreach.h"
#include "PrefsManager.h"
#include "InputFilter.h"
#include "archutils/Darwin/DarwinThreadHelpers.h"
#include "archutils/Darwin/KeyboardDevice.h"
#include "archutils/Darwin/MouseDevice.h"
#include "archutils/Darwin/JoystickDevice.h"
#include "archutils/Darwin/PumpDevice.h"

#include <IOKit/IOMessage.h>
#include <Carbon/Carbon.h>

REGISTER_INPUT_HANDLER_CLASS2( HID, MacOSX_HID );

void InputHandler_MacOSX_HID::QueueCallback( void *target, int result, void *refcon, void *sender )
{
	// The result seems useless as you can't actually return anything...
	// refcon is the Device number

	auto now = std::chrono::steady_clock::now();
	InputHandler_MacOSX_HID *This = (InputHandler_MacOSX_HID *)target;
	IOHIDQueueInterface **queue = (IOHIDQueueInterface **)sender;
	IOHIDEventStruct event;
	AbsoluteTime zeroTime = { 0, 0 };
	HIDDevice *dev = This->m_vDevices[size_t( refcon )];
	vector<DeviceInput> vPresses;

	while( (result = CALL(queue, getNextEvent, &event, zeroTime, 0)) == kIOReturnSuccess )
	{
		if( event.longValueSize != 0 && event.longValue != NULL )
		{
			free( event.longValue );
			continue;
		}
		//LOG->Trace( "Got event with cookie %p, value %d", event.elementCookie, int(event.value) );
		dev->GetButtonPresses( vPresses, event.elementCookie, event.value, now);
	}
	FOREACH_CONST( DeviceInput, vPresses, i )
		INPUTFILTER->ButtonPressed( *i );
}

static void RunLoopStarted( CFRunLoopObserverRef o, CFRunLoopActivity a, void *sem )
{
	CFRunLoopObserverInvalidate( o );
	CFRelease( o ); // we don't need this any longer
	((RageSemaphore *)sem)->Post();
}

int InputHandler_MacOSX_HID::Run( void *data )
{
	InputHandler_MacOSX_HID *This = (InputHandler_MacOSX_HID *)data;

	This->m_LoopRef = CFRunLoopGetCurrent();
	CFRetain( This->m_LoopRef );

	This->StartDevices();
	{
		const RString sError = SetThreadPrecedence( 1.0f );
		if( !sError.empty() )
			LOG->Warn( "Could not set precedence of the input thread: %s", sError.c_str() );
	}
	// Add an observer for the start of the run loop
	{
		/* The function copies the information out of the structure, so the memory
		 * pointed to by context does not need to persist beyond the function call. */
		CFRunLoopObserverContext context = { 0, &This->m_Sem, NULL, NULL, NULL };
		CFRunLoopObserverRef o = CFRunLoopObserverCreate( kCFAllocatorDefault, kCFRunLoopEntry,
								  false, 0, RunLoopStarted, &context);
		CFRunLoopAddObserver( This->m_LoopRef, o, kCFRunLoopDefaultMode );
	}

	/* Add a source for ending the run loop. This serves two purposes:
	 * 1. it provides a way to terminate the run loop when IH_MacOSX_HID exists,
	 * and 2. it ensures that CFRunLoopRun() doesn't return immediately if
	 * there are no other sources. */
	{
		/* Being a little tricky here, the perform callback takes a void* and
		 * returns nothing. CFRunLoopStop takes a CFRunLoopRef (a pointer) so
		 * cast the function and pass the loop ref. */
		void *info = This->m_LoopRef;
		void (*perform)(void *) = (void (*)(void *))CFRunLoopStop;
		// { version, info, retain, release, copyDescription, equal, hash, schedule, cancel, perform }
		CFRunLoopSourceContext context = { 0, info, NULL, NULL, NULL, NULL, NULL, NULL, NULL, perform };

		// Pass 1 so that it is called after all inputs have been handled (they will have order = 0)
		This->m_SourceRef = CFRunLoopSourceCreate( kCFAllocatorDefault, 1, &context );

		CFRunLoopAddSource( This->m_LoopRef, This->m_SourceRef, kCFRunLoopDefaultMode );
	}
	CFRunLoopRun();
	LOG->Trace( "Shutting down input handler thread..." );
	return 0;
}

void InputHandler_MacOSX_HID::DeviceAdded( void *refCon, io_iterator_t )
{
	InputHandler_MacOSX_HID *This = (InputHandler_MacOSX_HID *)refCon;

	LockMut( This->m_ChangeLock );
	This->m_bChanged = true;
}

void InputHandler_MacOSX_HID::DeviceChanged( void *refCon, io_service_t service, natural_t messageType, void *arg )
{
	if( messageType == kIOMessageServiceIsTerminated )
	{
		InputHandler_MacOSX_HID *This = (InputHandler_MacOSX_HID *)refCon;

		LockMut( This->m_ChangeLock );
		This->m_bChanged = true;
	}
}

// m_LoopRef needs to be set before this is called
void InputHandler_MacOSX_HID::StartDevices()
{
	int n = 0;

	ASSERT( m_LoopRef );
	FOREACH( HIDDevice *, m_vDevices, i )
		(*i)->StartQueue( m_LoopRef, InputHandler_MacOSX_HID::QueueCallback, this, n++ );

	CFRunLoopSourceRef runLoopSource = IONotificationPortGetRunLoopSource( m_NotifyPort );

	CFRunLoopAddSource( m_LoopRef, runLoopSource, kCFRunLoopDefaultMode );
}

InputHandler_MacOSX_HID::~InputHandler_MacOSX_HID()
{
	FOREACH( HIDDevice *, m_vDevices, i )
		delete *i;
	if( PREFSMAN->m_bThreadedInput )
	{
		CFRunLoopSourceSignal( m_SourceRef );
		CFRunLoopWakeUp( m_LoopRef );
		m_InputThread.Wait();
		CFRelease( m_SourceRef );
		CFRelease( m_LoopRef );
		LOG->Trace( "Input handler thread shut down." );
	}

	FOREACH( io_iterator_t, m_vIters, i )
		IOObjectRelease( *i );
	IONotificationPortDestroy( m_NotifyPort );
}

static CFDictionaryRef GetMatchingDictionary( int usagePage, int usage )
{
	// Build the matching dictionary.
	CFMutableDictionaryRef dict;

	if( (dict = IOServiceMatching(kIOHIDDeviceKey)) == NULL )
		FAIL_M( "Couldn't create a matching dictionary." );
	// Refine the search by only looking for joysticks
	CFNumberRef usagePageRef = CFInt( usagePage );
	CFNumberRef usageRef = CFInt( usage );

	CFDictionarySetValue( dict, CFSTR(kIOHIDPrimaryUsagePageKey), usagePageRef );
	CFDictionarySetValue( dict, CFSTR(kIOHIDPrimaryUsageKey), usageRef );

	// Cleanup after ourselves
	CFRelease( usagePageRef );
	CFRelease( usageRef );

	return dict;
}

// Factor this out because nothing else in IH_C::AddDevices() needs to know the type of the device.
static HIDDevice *MakeDevice( InputDevice id )
{
	if( id == DEVICE_KEYBOARD )
		return new KeyboardDevice;
	if( id == DEVICE_MOUSE )
		return new MouseDevice;
	if( IsJoystick(id) )
		return new JoystickDevice;
	if( IsPump(id) )
		return new PumpDevice;
	return NULL;
}

void InputHandler_MacOSX_HID::AddDevices( int usagePage, int usage, InputDevice &id )
{
	io_iterator_t iter;
	CFDictionaryRef dict = GetMatchingDictionary( usagePage, usage );
	kern_return_t ret = IOServiceAddMatchingNotification( m_NotifyPort, kIOFirstMatchNotification, dict,
						InputHandler_MacOSX_HID::DeviceAdded, this, &iter );
	io_object_t device;

	if( ret != KERN_SUCCESS )
		return;

	m_vIters.push_back( iter );

	// Iterate over the devices and add them
	while( (device = IOIteratorNext(iter)) )
	{
		LOG->Trace( "\tFound device %d", id );
		HIDDevice *dev = MakeDevice( id );
		int num;

		if( !dev )
		{
			LOG->Trace( "\t\tInvalid id, deleting device" );
			IOObjectRelease( device );
			continue;
		}

		if( !dev->Open(device) || (num = dev->AssignIDs(id)) == -1 )
		{
			LOG->Trace( "\tFailed top open or assign id, deleting device" );
			delete dev;
			IOObjectRelease( device );
			continue;
		}
		io_iterator_t i;

		enum_add( id, num );
		m_vDevices.push_back( dev );

		ret = IOServiceAddInterestNotification(
			m_NotifyPort, device, kIOGeneralInterest,
			InputHandler_MacOSX_HID::DeviceChanged,
			this, &i
		);

		if( ret == KERN_SUCCESS )
			m_vIters.push_back( i );
		else
			LOG->Trace( "\t\tFailed to add device changed notification, deleting device" );
		IOObjectRelease( device );
	}
}

InputHandler_MacOSX_HID::InputHandler_MacOSX_HID() : m_Sem( "Input thread started" ), m_ChangeLock( "Input handler change lock" )
{
	InputDevice id = DEVICE_KEYBOARD;

	// Set up the notify ports.
	m_NotifyPort = IONotificationPortCreate( kIOMasterPortDefault );

	// Add devices.
	LOG->Trace( "Finding keyboards" );
	AddDevices( kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard, id );
	
	LOG->Trace( "Finding mice" );
	AddDevices( kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse, id );
	
	LOG->Trace( "Finding joysticks" );
	id = DEVICE_JOY1;
	AddDevices( kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick, id );
	AddDevices( kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad, id );
	LOG->Trace( "Finding pump" );
	id = DEVICE_PUMP1;
	AddDevices( kHIDPage_VendorDefinedStart, 0x0001, id ); // Pump pads use the first vendor specific usage page.
	m_bChanged = false;

	if( PREFSMAN->m_bThreadedInput )
	{
		m_InputThread.SetName( "Input thread" );
		m_InputThread.Create( InputHandler_MacOSX_HID::Run, this );
		// Wait for the run loop to start before returning.
		m_Sem.Wait();
	}
	else
	{
		m_LoopRef = CFRunLoopRef( GetCFRunLoopFromEventLoop(GetMainEventLoop()) );
		CFRetain( m_LoopRef );
		StartDevices();
	}
}

void InputHandler_MacOSX_HID::GetDevicesAndDescriptions( vector<InputDeviceInfo>& vDevices )
{
	FOREACH_CONST( HIDDevice *, m_vDevices, i )
		(*i)->GetDevicesAndDescriptions( vDevices );
}

RString InputHandler_MacOSX_HID::GetDeviceSpecificInputString( const DeviceInput &di )
{
	if( di.device == DEVICE_KEYBOARD )
	{
#define OTHER(n) (KEY_OTHER_0 + (n))
		switch( di.button )
		{
		case KEY_DEL: return "del";
		case KEY_BACK: return "delete";
		case KEY_ENTER: return "return";
		case KEY_LALT: return "left option";
		case KEY_RALT: return "right option";
		case KEY_LMETA: return "left cmd";
		case KEY_RMETA: return "right cmd";
		case KEY_INSERT: return "help";
		case OTHER(0): return "F17";
		case OTHER(1): return "F18";
		case OTHER(2): return "F19";
		case OTHER(3): return "F20";
		case OTHER(4): return "F21";
		case OTHER(5): return "F22";
		case OTHER(6): return "F23";
		case OTHER(7): return "F25";
		case OTHER(8): return "execute";
		case OTHER(9): return "select";
		case OTHER(10): return "stop";
		case OTHER(11): return "again";
		case OTHER(12): return "undo";
		case OTHER(13): return "cut";
		case OTHER(14): return "copy";
		case OTHER(15): return "paste";
		case OTHER(16): return "find";
		case OTHER(17): return "mute";
		case OTHER(18): return "volume up";
		case OTHER(19): return "volume down";
		case OTHER(20): return "AS/400 equal";
		case OTHER(21): return "international 1";
		case OTHER(22): return "international 2";
		case OTHER(23): return "international 3";
		case OTHER(24): return "international 4";
		case OTHER(25): return "international 5";
		case OTHER(26): return "international 6";
		case OTHER(27): return "international 7";
		case OTHER(28): return "international 8";
		case OTHER(29): return "international 9";
		case OTHER(30): return "lang 1";
		case OTHER(31): return "lang 2";
		case OTHER(32): return "lang 3";
		case OTHER(33): return "lang 4";
		case OTHER(34): return "lang 5";
		case OTHER(35): return "lang 6";
		case OTHER(36): return "lang 7";
		case OTHER(37): return "lang 8";
		case OTHER(38): return "lang 9";
		case OTHER(39): return "alt erase";
		case OTHER(40): return "sys req";
		case OTHER(41): return "cancel";
		case OTHER(42): return "separator";
		case OTHER(43): return "out";
		case OTHER(44): return "oper";
		case OTHER(45): return "clear/again"; // XXX huh?
		case OTHER(46): return "cr sel/props"; // XXX
		case OTHER(47): return "ex sel";
		case OTHER(48): return "non US backslash";
		case OTHER(49): return "application";
		case OTHER(50): return "prior";
		default: break;
		}
#undef OTHER
	}
	if( di.device == DEVICE_PUMP1 || di.device == DEVICE_PUMP2 )
	{
		switch( di.button )
		{
		case JOY_BUTTON_1:  return "UL";
		case JOY_BUTTON_2:  return "UR";
		case JOY_BUTTON_3:  return "MID";
		case JOY_BUTTON_4:  return "DL";
		case JOY_BUTTON_5:  return "DR";
		case JOY_BUTTON_6:  return "Esc";
		case JOY_BUTTON_7:  return "P2 UL";
		case JOY_BUTTON_8:  return "P2 UR";
		case JOY_BUTTON_9:  return "P2 MID";
		case JOY_BUTTON_10: return "P2 DL";
		case JOY_BUTTON_11: return "P2 DR";
		default: break;
		}
	}

	return InputHandler::GetDeviceSpecificInputString( di );
}

// Modified from NESControllerInterface of Macifom project,
// used under MIT license from http://macifom.googlecode.com/svn-history/r89/Macifom/trunk/NESControllerInterface.m
// Used under MIT license from http://inquisitivecocoa.com/2009/04/05/key-code-translator/
static wchar_t KeyCodeToChar(CGKeyCode keyCode, unsigned int modifierFlags)
{
	TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource();
	CFDataRef uchr = (CFDataRef)TISGetInputSourceProperty(currentKeyboard, kTISPropertyUnicodeKeyLayoutData);
	const UCKeyboardLayout *keyboardLayout = uchr ? (const UCKeyboardLayout*)CFDataGetBytePtr(uchr) : NULL;
	
	if( keyboardLayout )
	{
		UInt32 deadKeyState = 0;
		UniCharCount maxStringLength = 255;
		UniCharCount actualStringLength = 0;
		UniChar unicodeString[maxStringLength];
		
		OSStatus status = UCKeyTranslate(keyboardLayout,
						 keyCode, kUCKeyActionDown, modifierFlags,
						 LMGetKbdType(), 0,
						 &deadKeyState,
						 maxStringLength,
						 &actualStringLength, unicodeString);
		
		if( status != noErr )
		{
			fprintf(stderr, "There was an %s error translating from the '%d' key code to a human readable string: %s\n",
				GetMacOSStatusErrorString(status), (int)status, GetMacOSStatusCommentString(status));
		}
		else if( actualStringLength == 0 )
		{
			fprintf(stderr, "Couldn't find a translation for the '%d' key code\n", keyCode);
		}
		else
		{
			return unicodeString[0];
		}
	}
	else
	{
		fprintf(stderr, "Couldn't find a translation for the '%d' key code\n", keyCode);
	}
	return 0;
}

wchar_t InputHandler_MacOSX_HID::DeviceButtonToChar( DeviceButton button, bool bUseCurrentKeyModifiers )
{
	// KeyTranslate maps these keys to a character.  They shouldn't be mapped to any character.
	switch( button )
	{
		default:
			if( (button >= KEY_F1 && button <= KEY_F16) )
				return L'\0';
			break;
		case KEY_UP:
		case KEY_DOWN:
		case KEY_LEFT:
		case KEY_RIGHT:
		case KEY_ESC:
		case KEY_TAB:
		case KEY_ENTER:
		case KEY_PRTSC:
		case KEY_SCRLLOCK:
		case KEY_PAUSE:
		case KEY_DEL:
		case KEY_HOME:
		case KEY_END:
		case KEY_PGUP:
		case KEY_PGDN:
		case KEY_NUMLOCK:
		case KEY_KP_ENTER:
			return L'\0';
	}

	// Use Text Input Source services to convert the key code to character code.
	UInt8 iMacVirtualKey;
	if( KeyboardDevice::DeviceButtonToMacVirtualKey( button, iMacVirtualKey ) )
	{
		UInt32 nModifiers = bUseCurrentKeyModifiers ? GetCurrentKeyModifiers() : 0;
		wchar_t sCharCode = KeyCodeToChar( iMacVirtualKey, nModifiers );
		if( sCharCode != 0 )
		{
			return sCharCode;
		}
	}

	return InputHandler::DeviceButtonToChar( button, bUseCurrentKeyModifiers );
}

/*
 * (c) 2005, 2006 Steve Checkoway
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
