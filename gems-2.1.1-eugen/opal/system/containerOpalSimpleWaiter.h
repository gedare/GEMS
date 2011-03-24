#ifndef _CONTAINEROPALSIMPLEWAITERH_
#define _CONTAINEROPALSIMPLEWAITERH_

#include "containerOpal.h"

class simple_waiter_t: public waiter_t{
	public:
	waiter_t *parent;
	virtual void Wakeup( void );
	simple_waiter_t(){};
	simple_waiter_t(waiter_t * p){ parent = p; }
	~simple_waiter_t(){};

};
#endif  /* _CONTAINEROPALSIMPLEWAITERH_ */
