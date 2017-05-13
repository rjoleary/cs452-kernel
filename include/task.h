// Task creation.

#ifndef TASK_H__INCLUDED
#define TASK_H__INCLUDED

#include "types.h"

Tid kCreate(Priority priority, void (*code)());
Tid kMyTid(void);
Tid kMyParentTid(void);
void kPass(void);
void kExeunt(void);
void kDestroy(void);

#endif // TASK_H__INCLUDED
