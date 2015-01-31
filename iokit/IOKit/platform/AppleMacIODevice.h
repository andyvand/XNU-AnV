/*
 * Copyright (c) 1998-2000 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1999 Apple Computer, Inc.  All rights reserved. 
 *
 * HISTORY
 *
 */


#ifndef _IOKIT_APPLEMACIODEVICE_H
#define _IOKIT_APPLEMACIODEVICE_H

#include <IOKit/IOService.h>

class AppleMacIODevice : public IOService
{
  OSDeclareDefaultStructors(AppleMacIODevice);

private:  
  struct ExpansionData { };
  ExpansionData *reserved;

public:
  virtual bool compareName( OSString * name, OSString ** matched = 0 ) const;
  virtual IOService *matchLocation(IOService *client);
  virtual IOReturn getResources( void );
    
  OSMetaClassDeclareReservedUnused(AppleMacIODevice,  0);
  OSMetaClassDeclareReservedUnused(AppleMacIODevice,  1);
  OSMetaClassDeclareReservedUnused(AppleMacIODevice,  2);
  OSMetaClassDeclareReservedUnused(AppleMacIODevice,  3);
};

#endif /* ! _IOKIT_APPLEMACIODEVICE_H */
