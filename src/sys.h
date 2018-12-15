#ifndef	SYS_H
#define	SYS_H

extern short iconLibraryV44Enabled;

short SysInit(int argc, char **argv);
void SysCleanup(void);
void SysSetVerboseEnabled(short enabled);

// Logging
void Information(const char *fmt, ...);
void Verbose(const char *fmt, ...);
void SysSetConsoleDelay(long delay);

#endif	/* SYS_H */
