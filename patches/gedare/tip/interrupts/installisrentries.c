/*
 *  Install trap handlers for handling software interrupts.
 *  This file is deprecated, as the trap handlers are needed before this 
 *  function is called. We still use this as for debugging purposes.
 *
 *  Copyright 2010 Gedare Bloom.
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 *  $Id: installisrentries.c,v 1.1 2010/06/17 16:16:25 joel Exp $
 */

#include <rtems.h>

#include "hwpq_exceptions.h"

void sparc64_install_isr_entries( void )
{
  sparc64_hwpq_initialize();
}
