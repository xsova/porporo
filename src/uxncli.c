#include <stdio.h>
#include <stdlib.h>

#include "uxn.h"
#include "devices/system.h"
#include "devices/console.h"
#include "devices/file.h"
#include "devices/datetime.h"

/*
Copyright (c) 2021-2023 Devine Lu Linvega, Andrew Alderwick

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

static Varvara v;

Uint8
emu_dei(Uxn *u, Uint8 addr)
{
	switch(addr & 0xf0) {
	case 0x00: return system_dei(u, addr);
	case 0xc0: return datetime_dei(u, addr);
	}
	return u->dev[addr];
}

void
emu_deo(Uxn *u, Uint8 addr, Uint8 value)
{
	Uint8 d = addr & 0xf0;
	u->dev[addr] = value;
	switch(d) {
	case 0x00: system_deo(u, &u->dev[d], addr & 0x0f); break;
	case 0x10: console_deo(&u->dev[d], addr & 0x0f); break;
	case 0xa0: file_deo(0, u->ram, &u->dev[d], addr & 0x0f); break;
	case 0xb0: file_deo(1, u->ram, &u->dev[d], addr & 0x0f); break;
	}
}

static void
emu_run(Uxn *u)
{
	while(!u->dev[0x0f]) {
		int c = fgetc(stdin);
		if(c == EOF) {
			console_input(u, 0x00, CONSOLE_END);
			break;
		}
		console_input(u, (Uint8)c, CONSOLE_STD);
	}
}

static int
emu_end(Uxn *u)
{
	free(u->ram);
	return u->dev[0x0f] & 0x7f;
}

int
main(int argc, char **argv)
{
	int i = 1;
	if(i == argc)
		return system_error("usage", "uxncli [-v] file.rom [args..]");
	/* Read flags */
	if(argv[i][0] == '-' && argv[i][1] == 'v')
		return !fprintf(stdout, "Uxncli - Varvara Emulator(Porporo ver.), 17 Dec 2023.\n");
	/* Boot */
	v.u.ram = (Uint8 *)calloc(0x10000 * RAM_PAGES, 1);
	if(!system_boot_rom(&v, &v.u, argv[i++], 0))
		return system_error("Init", "Failed to initialize uxn.");
	/* Game Loop */
	v.u.dev[0x17] = argc - i;
	if(uxn_eval(&v.u, PAGE_PROGRAM)) {
		console_listen(&v.u, i, argc, argv);
		emu_run(&v.u);
	}
	return emu_end(&v.u);
}
