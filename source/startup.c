// startup.c — glue between libctru and stock newlib.
//
// libctru was written against devkitPro's patched newlib.  These stubs
// provide the pieces that are missing from stock newlib:
//   - __libc_init_array (we have no static constructors)
//   - TLS segment symbols (we use no TLS)
//   - newlib syscall stubs (_write, _sbrk, ...) backed by the_ctru heap
#include <3ds.h>
#include <errno.h>
#include <sys/types.h>
#include <stdint.h>
#include <netinet/in.h>

// ---- environment globals that devkitPro's crt0 usually provides ----------
void* __service_ptr = NULL;          // homebrew stolen-service-handle list
u32 __apt_appid = 0;                 // APT app id (0 => use APT defaults)
u32 __system_runflags = 0;           // environment flags
const char* __system_arglist = NULL; // homebrew arg list (none for CIA)
struct in_addr __3dslink_host = { 0 };

// newlib heap bounds, filled by libctru's __system_allocateHeaps()
char* fake_heap_start = 0;
char* fake_heap_end = 0;

// Empty init-array routine (called by libctru's initSystem).
void __libc_init_array(void) {
}

// ---- TLS segment symbols (unused, kept so libctru links) ----
const unsigned char __tdata_lma[] = { 0 };
const unsigned char __tdata_lma_end[] = { 0 };
const size_t __tdata_align = 8;
unsigned char __tls_start[] = { 0 };
unsigned char __tls_end[] = { 0 };

// ---- newlib heap ----------------------------------------------------------
// libctru's __system_allocateHeaps() maps the application heap and points
// fake_heap_start/fake_heap_end at it (after reserving room for the stack).

void* _sbrk(ptrdiff_t incr) {
	char* old = fake_heap_start;
	char* next = old + incr;
	if (incr > 0 && (next <= old || next > fake_heap_end)) {
		errno = ENOMEM;
		return (void*)-1;
	}
	if (incr < 0 && next < fake_heap_start) {
		errno = ENOMEM;
		return (void*)-1;
	}
	fake_heap_start = next;
	return old;
}

// ---- newlib syscalls (minimal) -------------------------------------------
int _write(int fd, const void* ptr, size_t len) {
	(void)fd;
	svcOutputDebugString((const char*)ptr, len);
	return (int)len;
}

int _read(int fd, void* ptr, size_t len) {
	(void)fd; (void)ptr; (void)len;
	return 0;
}

int _open(const char* path, int flags, int mode) {
	(void)path; (void)flags; (void)mode;
	return -1;
}

int _close(int fd) {
	(void)fd;
	return -1;
}

long _lseek(int fd, long offset, int whence) {
	(void)fd; (void)offset; (void)whence;
	return -1;
}

int _fstat(int fd, void* st) {
	(void)fd; (void)st;
	return -1;
}

int _isatty(int fd) {
	(void)fd;
	return 0;
}

int _kill(int pid, int sig) {
	(void)pid; (void)sig;
	return -1;
}

int _getpid(void) {
	return 1;
}

void _exit(int rc) {
	__ctru_exit(rc);
	for (;;);
}
