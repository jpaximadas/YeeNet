/**
 * Conditionally compile the platform.c file for the specified target
 */

#if defined(YEENET_BOARD_BLUEPILL_F103)
#include "platform_bluepill_f103.c"
#elif defined(YEENET_BOARD_BLACKPILL_F411)
#include "platform_blackpill_f411.c"
#elif defined(YEENET_BOARD_ROUTER_REV_A)
#include "platform_router_rev_A.c"
#else
#error "Unknown target!"
#endif
