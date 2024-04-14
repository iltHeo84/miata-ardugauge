#define SERIAL_UPDATE_MS  100

//#define TEST // De-comment to enter test mode

/* ----- Engine status ----- */
#define ENGINE_OFF		0 
#define ENGINE_WARMUP	1
#define ENGINE_RUN		2

#define IDLE_RPM_ENTER  1000
#define IDLE_RPM_EXIT   1100
#define IDLE_DELAY_MS   2000
/* ----- Engine status end ----- */

/* ----- OLED display pages ----- */
#define PAGE_AUTO       0 // Must be the first page
#define PAGE_RPM_MAP    1
#define PAGE_AE         2
#define PAGE_AFRGRAPH   3 // Must be the last page
/* ----- OLED pages end     ----- */
