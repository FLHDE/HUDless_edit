# Makefile (VC6) for HUDless.
# Jason Hood, 30 October, 2017.

CFLAGS = /nologo /W3 /O2 /MD

HUDless.dll: HUDless.c HUDless.res
	cl $(CFLAGS) /LD $** /link /base:0x61B0000 /filealign:512
