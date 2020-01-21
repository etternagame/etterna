#ifndef APP_INSTANCE_H
#define APP_INSTANCE_H

#include "windows.h"

/** @brief get an HINSTANCE for starting dialog boxes. */
class AppInstance
{
  public:
	AppInstance();
	~AppInstance();
	HINSTANCE Get() const { return h; }
	operator HINSTANCE() const { return h; }

  private:
	HINSTANCE h;
};

#endif
