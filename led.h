struct led {
	void (*on)(void);
	void (*off)(void);
	
	/* flashing, all centralized */
	//waveform of a flash /^^^^^^^\___
	int currently_flashing;
	int flashes_done; // incremented at end of off portion
	ms_time_t cur_flash_start;
	int cur_flash_mode;
	unsigned int flash_on_dur;
	unsigned int flash_off_dur;
	int times_to_flash;
};

void
do_flashing(struct led *l, ms_time_t now);

void
start_flashing(struct led *l, int times, int on_dur, int off_dur);

void
stop_flashing(struct led *l);
