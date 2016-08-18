#ifndef _MODULESYSTEM_H_
#define _MODULESYSTEM_H_
		//TODO put the argument names for the functions in the macro so they are clear
#define AS_MODULE_INITS(x) x##_init();
#define AS_MODULE_TASKS(x) x##_task();
#define AS_PROX_HANDLER(x) x##_prox_handler(Data);
#define AS_ACT_HANDLER(x) x##_act_handler(Data);
#define AS_FEATURE_HANDLER(x) x##_featrep_handler(Data);

#define MODULE_INIT(name) void name##_init(void)
#define MODULE_TASK(name) void name##_task(void)
#define PROX_HANDLER(name) void name##_prox_handler(uint8_t* Data)
#define ACT_HANDLER(name) void name##_act_handler(uint8_t* Data)
		
#define INPUT_REQUESTEE(name) bool name##_input_requestee(uint8_t* Data)
//TODO define functions for handling input and providing output, so that 4 functions will suffice

#endif /*_MODULESYSTEM_H_*/
		