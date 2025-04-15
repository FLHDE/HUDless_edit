/*
  HUDless.c - Remove the HUD.

  Jason Hood, 30 & 31 October, 2017.

  v1.02, 17 May, 2024:
  + support a secondary Open Log key, to allow it to work on bases.

  Create a secondary "MINIMIZE HUD" and/or "OPEN LOG" key, using a modifier
  (one or more of Shift, Ctrl or Alt), to remove the display of the HUD, then
  the cursor, then restore both.

  Install:
    add an entry for HUDless.dll to [Libraries] in EXE\dacom.ini.

  TODO:
  * replace TEST EAX,EAX at:
      4EC48A  targetting (brackets *and* guns)
      51DACA  another targetting
      58A815  target, weapons & bats/bots
    with INC AX to preserve targetting.
    Possibly use Target (contact list) and/or Status (weapon list) keys.
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define NAKED	__declspec( naked )
#define STDCALL __stdcall


#define ADDR_LOG    0x45caee	// USER_STORY_STAR
#define ADDR_MINHUD 0x4d5383
#define ADDR_TEST_TARGET 0x4E3CE0


DWORD dummy;
#define ProtectX( addr, size ) \
  VirtualProtect( (PVOID)(addr), size, PAGE_EXECUTE_READWRITE, &dummy )

#define RELOFS( from, to ) \
  *(PDWORD)(from) = (DWORD)(to) - (DWORD)(from) - 4;

#define JMP( from, to ) \
  *(PBYTE)(from) = 0xE9; \
  RELOFS( (DWORD)from+1, to )


BOOL targetDllLoaded = FALSE;

void __stdcall ToggleTargetWireframe(BOOL show)
{
	if (targetDllLoaded)
	{
		*(WORD*)(ADDR_TEST_TARGET + 2) = (show ? 0x0 : 0x028F);
	}
	else
	{
		*(WORD*)(ADDR_TEST_TARGET) = (show ? 0x850F : 0xE990);
	}
}


NAKED
void toggle( void )
{
  __asm {
	xor	eax, eax
	cmp	ds:[0x67c280], al	// any modifier keys pressed?
	jz	done			// no, normal routine
	cmp	ds:[0x679c0c], al
	jnz	HUD
	cmp	ds:[0x679c20], al
	sete	al
	mov	ds:[0x679c20], al	// cursor
  HUD:
	mov	ds:[0x679c0c], al	// the icons
	mov	ds:[0x679c10], al	// everything else
	mov	ds:[0x679c40], al	// scroll bars on weapon list
	push eax
	call ToggleTargetWireframe
	stc
  done:
	ret
  }
}


NAKED
void MinHUD_Hook( void )
{
  __asm {
	call	toggle
	jc	done
	mov	eax, 0x4d538a
	jmp	eax
  done:
	mov	al, 1
	ret	4
  }
}


NAKED
void Log_Hook( void )
{
  __asm {
	call	toggle
	jc	done
	mov	ecx, 0x45caf3
	mov	al, ds:[0x67dcc8]
	jmp	ecx
  done:
	pop	edi
	pop	esi
	mov	al, 1
	pop	ebp
	ret	4
  }
}

void Patch( void )
{
  ProtectX( ADDR_LOG, 5 );
  ProtectX( ADDR_MINHUD, 7 );
  ProtectX( ADDR_TEST_TARGET, 6 );

  *(WORD*)ADDR_MINHUD = 0xF475;
  JMP( ADDR_MINHUD + 2, MinHUD_Hook );
  JMP( ADDR_LOG, Log_Hook );

  if (GetModuleHandle( "HudTarget.dll" ))
  {
	  // Restore jne instruction.
	  *(WORD*)ADDR_TEST_TARGET = 0x850F;
	  *(DWORD*)(ADDR_TEST_TARGET + 2) = 0x0;
	  targetDllLoaded = TRUE;
  }
}


BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
  if (fdwReason == DLL_PROCESS_ATTACH)
    Patch();

  return TRUE;
}
